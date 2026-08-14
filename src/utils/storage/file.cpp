/*
	Armazenamento Local com CSV, também tem log por terminal
*/

#include "file.h"

struct State {
    const char* PATH = "/telemetry.csv";
	const char* PATH_OLD = "/telemetry.csv.1";

    File file;
	bool isOpen = false;
	size_t cachedSize = 0;
};
static State state;


// Utilitários
static bool headerChecked = false;

size_t getCachedFileSize() {
    return state.cachedSize;
}

size_t fileSize() {
    File f = LittleFS.open(state.PATH, "r"); 
    if (!f) 
        return 0;

    size_t size = f.size(); 
    f.close();
    return size;
}

void clear() {
    if (LittleFS.exists(state.PATH))     
        LittleFS.remove(state.PATH);
    
    if (LittleFS.exists(state.PATH_OLD)) 
        LittleFS.remove(state.PATH_OLD);

	state.cachedSize = 0;
	headerChecked = false;
}

void createCSVHeader() {
	if (LittleFS.exists(state.PATH) && fileSize() > 0) return;

	File f = LittleFS.open(state.PATH, "a");
	if (!f) {
		Serial.println("[CSV] Erro ao criar header");
		return;
	}
	
	f.println(F(
		"ts_iso,"
		"ms,"
		"volts,"
		"pct,"
		"temp,"
		"humi,"
		"rpm,"
		"speed_kmh,"
		"current_bat_a,"
		"current_mot_a,"
		"voltage_min,"
		"voltage_max,"
		"wheel_cm,"
		"ppr,"
		"max_pct,"
		"rssi,"
		"log_enabled,"
		"log_iv_ms,"
		"pwm_hz,"
		"start_min_pct,"
		"rapid_ms,"
		"rapid_up,"
		"slew_up,"
		"slew_dn,"
		"zero_timeout_ms,"
		"last_ack"
	));	
	
	f.close();
}


// Controle de arquivo
void openLogFile() {
	if (state.isOpen) return;

	if (!headerChecked) {
		createCSVHeader();
		headerChecked = true;
	}

	state.file = LittleFS.open(state.PATH, "a");
	if (state.file) {
		state.isOpen = true;
		state.cachedSize = state.file.size();
	} else {
		Serial.println("[CSV] Falha ao abrir arquivo");
	}
}

void closeLogFile() {
	if (state.isOpen) {
		state.file.flush();
		state.file.close();
		state.isOpen = false;
	}
}

// Rotação de arquivo
void rotateIfNeeded() {
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck < 10000) return;
    lastCheck = millis();

    static uint32_t lastSync = 0;

    if (millis() - lastSync > 60000) { 
		state.cachedSize = fileSize();
		lastSync = millis();
    }

    const size_t MAX_BYTES = 1024UL * 1024UL;
    size_t size = state.cachedSize;

    if (size >= MAX_BYTES) {
		closeLogFile();

		if (LittleFS.exists(state.PATH_OLD) && !LittleFS.remove(state.PATH_OLD)) {
			Serial.println("[CSV] Falha ao remover arquivo antigo"); 
			return;
		}

		if (!LittleFS.rename(state.PATH, state.PATH_OLD)) {
			Serial.println("[CSV] Falha ao rotacionar arquivo");
			return;
		}

		headerChecked = false;
		state.cachedSize = 0;
    }
} 

// Formatadores
void formatTimestamp(char* buf, size_t size) {
	time_t now = time(nullptr);
	if (now > 1600000000) { //2020
		static bool tzSet = false;
        if (!tzSet) {
            setenv("TZ", "UTC+3", 1);  // Horário de Brasília
            tzset();
            tzSet = true;
        }

		struct tm t;
		localtime_r(&now, &t);
		
		if (strftime(buf, size, "%Y-%m-%d %H:%M:%S", &t) == 0) {
            snprintf(buf, size, "%" PRIu32, millis());
        }
    } else {
       snprintf(buf, size, "%" PRIu32, millis());
    }
}

static void formatMaybeNan(char* out, size_t outSize, float value, uint8_t digits = 3) {
	if (!out || outSize == 0) return;

	if (!isfinite(value)) {
		out[0] = '\0';
		return;
	}

	int len = snprintf(out, outSize, "%.*f", digits, value);
	if (len < 0 || len >= outSize) {
		out[outSize - 1] = '\0';
	}
}

int wifiRSSI() {
    return (WiFi.status()==WL_CONNECTED) ? WiFi.RSSI() : 0;
}

// CSV Append (talvez fragmentar mais)
void appendCsvRow() {
	if (!LOGGER::isEnabled()) 
        return;

	rotateIfNeeded();

	openLogFile();
	if (!state.isOpen) 
        return;

	char timeStamp[32];
	char tempBuffer[16];
	char humiBuffer[16];

	formatTimestamp(timeStamp, sizeof(timeStamp));
	uint32_t now = millis();

	formatMaybeNan(tempBuffer, sizeof(tempBuffer), Data::data.temp, 1);
	formatMaybeNan(humiBuffer, sizeof(humiBuffer), Data::data.humi, 1);

	// Buffer para a linha CSV
	char lineBuffer[512];  
	int written = snprintf(lineBuffer, sizeof(lineBuffer),
		"%s,""%" PRIu32 ",%.3f,%.2f,%s,%s,%.1f,%.2f,%.2f,%.2f,"
		"%.3f,%.3f,%.1f,%" PRIu8 ",%" PRIu8 ",%d,%d,"
		"%" PRIu32 ",%" PRIu16 ",%" PRIu8 ",%" PRIu16 ","
		"%.1f,%.1f,%.1f,%" PRIu32 ",%s\n",
		
		timeStamp, // mudar
		now,

		Data::data.volts,
		Data::data.pct,

		tempBuffer,
		humiBuffer,

		Data::data.rpm,
		Data::data.speed_kmh,

		Data::data.currentBat,
	    Data::data.currentMot,

		Data::config.voltageMin,
		Data::config.voltageMax,

		Data::config.wheel_cm,
		Data::config.ppr,
		Data::config.maxPct,

		wifiRSSI(),

		LOGGER::isEnabled() ? 1 : 0,
		LOGGER::getInterval(),

		Data::config.pwm_hz,
		Data::config.startMin,
		Data::config.rapid_ms,
		Data::config.rapidUp,
		Data::config.slewUp,
		Data::config.slewDown,
		Data::config.zeroTimeout,

		ACK::getLast()
	);

	if (written > 0 && written < (int)sizeof(lineBuffer)) {
		// Escreve no Serial Monitor
		Serial.print(lineBuffer);
		
		// Escreve no arquivo
		size_t bytes = state.file.print(lineBuffer);

		if (bytes != (size_t)written) {
			Serial.println("[CSV] Writing Error");
		}

		state.cachedSize += bytes;
	}

	//Flush
	static uint32_t lastFlush = 0;
	static uint16_t lines = 0;
	lines++;

	if (lines >= 10 || millis() - lastFlush > 5000) {
		state.file.flush();
		lastFlush = millis();
		lines = 0;
	}

	closeLogFile();	
}
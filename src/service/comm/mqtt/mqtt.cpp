#include "mqtt.h"

// Lapidar Melhor
// Receber e enviar todos os comandos presentes no Mega

namespace {
    // WIFI
    WiFiClient espClient;
	PubSubClient client(espClient);

    // CONFIG
    // Retirado commandThrottle
    struct Topics {
        const char* telemetryJson;
        const char* commandMotor;
        //const char* commandThrottle;
        const char* commandConfig;
        const char* status;
    };

    struct Config {
        const char* host;
        uint16_t port;
        const char* clientId;

        Topics topics;
    };

    constexpr Config CONFIG = {
        .host      =  "broker.emqx.io",
        .port      =  1883,
        .clientId  =  "EWolfTelemetryS3",
        .topics {
            .telemetryJson    =  "pb/telemetry/json",
            .commandMotor     =  "pb/cmd/motor",
            //.commandThrottle  =  "pb/cmd/throttle",
            .commandConfig    =  "pb/cmd/config",
            .status           =  "pb/status",
        }
    };

	uint32_t lastMqtt = 0;
	constexpr uint32_t MQTT_IV_MS = 1000;

	// Auxiliares
	// Envia o comando ao Arduino mega
	void sendCmd(const char* s){
		UART::SerialArduino.println(s);
		ACK::setLast(s);
	}

	// Handlers
	// Atualizar função para que receba comandos mais diretos, como set voltagemax now
	void handleMotor(const char* msg) {
		// Uso: 
    	// - Start: Uso padrão, motor funcionando de forma comum
    	// - Hold: Trava o motor em uma velocidade fixa, ignorando o pedal.
    	// - Stop: Motor desacelera suavemente até ficar totalmente parado. Bom para testes envolvendo diodos e mofestes

		if (!strcasecmp(msg, "STOP") || !strcasecmp(msg, "OFF")  || !strcmp(msg, "0")) {
			sendCmd("STOP");
		}

		else if (!strcasecmp(msg, "START") || !strcasecmp(msg, "ON")    || !strcmp(msg, "1")) {
			sendCmd("START");
		}

        // Deve seguir a estrutura: "HOLD 50"
		else if (strncasecmp(msg, "HOLD", 4) == 0) {
			char command[15];

			if (msg[4] == ' ') {
        		int pct = atoi(msg + 5);
				pct = constrain(pct, 0, 100);

				sprintf(command,"HOLD %d", pct);

				sendCmd(command);
			}
		}

		else {
			Serial.printf("[MQTT] Unknown motor cmd: %s\n", msg);
		}
	}

	// Modificar função para que siga a ideia:
	// ConfigSite envia valor modificado -> Esp recebe e atualiza dados -> Esp envia dados por meio de sendCmd para o Mega
	void handleConfig(JsonDocument& doc) {
		// -------- LOG --------
        if (doc["log_enabled"].is<bool>())
			LOGGER::setEnabled(doc["log_enabled"].as<bool>());

		if (doc["log_iv_ms"].is<int>()) {
			LOGGER::setInterval(doc["log_iv_ms"].as<int>());
		}

		if (doc["log_clear"].is<bool>() && doc["log_clear"].as<bool>()) {
			LOGGER::clearFile(); 
		}

		// -------- CONFIG AVANÇADA --------
        // Config responsável pela Rampagem, PWM, RPM. Config que mais afeta o motor
		if (doc["max_pct"].is<float>()) {
            float max_pct = doc["max_pct"].as<float>();
            Data::config.maxPct = constrain(max_pct, 0.0f, 100.0f);

            char command[32];
            snprintf(command, sizeof(command), "SET_MAXPCT %.0f", max_pct);
            sendCmd(command);
		}
		
		if (doc["min_v"].is<float>()) {
            float minV = doc["min_v"].as<float>();
            Data::config.voltageMin = minV;

            char command[32];
            snprintf(command, sizeof(command), "SET_MINV %.3f", minV);
            sendCmd(command);
		}
		
		if (doc["max_v"].is<float>()) {
            float max_v = doc["max_v"].as<float>();
            Data::config.voltageMax = max_v;

            char command[32];
            snprintf(command, sizeof(command), "SET_MAXV %.3f", max_v);
            sendCmd(command);
		}
		
		if (doc["wheel_cm"].is<float>()) {
            float wheel_cm = doc["wheel_cm"].as<float>();
            Data::config.wheel_cm = wheel_cm;

            char command[32];
            snprintf(command, sizeof(command), "SET_WHEEL %.1f", wheel_cm);
            sendCmd(command);
		}
		
		if (doc["ppr"].is<int>()) {
           int ppr = doc["ppr"].as<int>();
           Data::config.ppr = (uint8_t)ppr;

           char command[32];
           snprintf(command, sizeof(command), "SET_PPR %d", ppr);
           sendCmd(command);
		}

        // Mudar
		if (doc["poll_ms"].is<int>())
			Data::data.now = (uint32_t)doc["poll_ms"].as<int>();

		if (doc["pwm_hz"].is<float>()) {
           float pwm_hz = doc["pwm_hz"].as<float>();
           Data::config.pwm_hz = constrain(pwm_hz, 100.0f, 8000.0f);

           char command[32];
           snprintf(command, sizeof(command), "SET_PWMF %.0f", pwm_hz);
           sendCmd(command);
		}

		if (doc["start_min_pct"].is<float>()) {
            float start_min_pct = doc["start_min_pct"].as<float>();
            Data::config.startMin = constrain(start_min_pct, 0.0f, 40.0f);

            char command[32];
            snprintf(command, sizeof(command), "SET_STARTMIN %.0f", start_min_pct);
            sendCmd(command);
		}

		if (doc["rapid_ms"].is<float>()) {
            float rapid_ms = doc["rapid_ms"].as<float>();
            Data::config.rapid_ms = constrain(rapid_ms, 50.0f, 1500.0f);

            char command[32];
            snprintf(command, sizeof(command), "SET_RAPIDMS %.0f", rapid_ms);
            sendCmd(command);
		}

		if (doc["rapid_up"].is<float>()) {
            float rapid_up = doc["rapid_up"].as<float>();
            Data::config.rapidUp = constrain(rapid_up, 10.0f, 400.0f);

            char command[32];
            snprintf(command, sizeof(command), "SET_RAPIDUP %.1f", rapid_up);
            sendCmd(command);
		}

		if (doc["slew_up"].is<float>() || doc["slew_dn"].is<float>()) {

            float slew_up = Data::config.slewUp;
            float slew_dn = Data::config.slewDown;

            if (doc["slew_up"].is<float>()) {
              slew_up = doc["slew_up"].as<float>();
              Data::config.slewUp = constrain(slew_up, 5.0f, 200.0f);
           }

            if (doc["slew_dn"].is<float>()) {
              slew_dn = doc["slew_dn"].as<float>();
              Data::config.slewDown = constrain(slew_dn, 5.0f, 300.0f);
    }

            char command[64];
            snprintf(command, sizeof(command), "SET_SLEW %.1f %.1f",slew_up, slew_dn);

            sendCmd(command);
		}

		if (doc["zero_timeout_ms"].is<float>()) {
            float zero_timeout_ms = doc["zero_timeout_ms"].as<float>();
            Data::config.zeroTimeout = constrain(zero_timeout_ms, 50.0f, 2000.0f);

            char command[32];
            snprintf(command, sizeof(command), "SET_ZEROTO %.0f", zero_timeout_ms);
            sendCmd(command);
		}

		ACK::setLast("CONFIG_UPDATED");
	}


	// Callback | Site -> Esp -> Motor
	void mqttCallback(char* topic, byte* payload, unsigned int length) {
		char msg[256];

		// Verificar 
		if (length >= sizeof(msg)) {
			Serial.printf("[MQTT] Payload too large (%u bytes)\n", length);
			return;
		}

		unsigned int len = min(length, sizeof(msg)-1);

		memcpy(msg, payload, len);
		msg[len] = '\0';

		Serial.printf("[MQTT RX] %s | %s\n", topic, msg);

		// MOTOR (texto puro, não é JSON)
		if (strcmp(topic, CONFIG.topics.commandMotor) == 0) {
			handleMotor(msg);
			return;
		}

		// CONFIG (JSON)
		if (strcmp(topic, CONFIG.topics.commandConfig) == 0) {
			JsonDocument doc;
			if (deserializeJson(doc, msg)) {
				Serial.println("[MQTT] JSON error");
				return;
			}
			handleConfig(doc);
			return;
		}
	}

	// Conexão
	void ensureMqtt() {
		if (client.connected()) return;

		static uint32_t lastTry = 0;
		static int lastErr = -999;
		static bool firstTry = true;
		static uint32_t lastErrLog = 0;

		if (millis() - lastTry < 1500) return;
		lastTry = millis();

		char clientId[64];
		snprintf(clientId, sizeof(clientId), "%s-%llX", CONFIG.clientId, ESP.getEfuseMac());

		Serial.println("======================================");
		if (firstTry) {
			Serial.println("[MQTT] Connecting...");
			firstTry = false;
		}

		if (client.connect(clientId, CONFIG.topics.status, 0, true, "offline")) {
			Serial.println("[MQTT] Connected");

			client.publish  (CONFIG.topics.status, "online", true);
			client.subscribe(CONFIG.topics.commandMotor);
			client.subscribe(CONFIG.topics.commandConfig);

			lastErr = -999;
			firstTry = true;
			lastErrLog = 0;
		} else {
			int err = client.state();

			if (err != lastErr || millis() - lastErrLog > 60000) {
				Serial.printf("[MQTT] Connection failed, state=%d\n", err);
				lastErr = err;
				lastErrLog = millis();
			}
		}
		Serial.println("======================================");
	}

	// Publicar | ESP -> Dashboard
	void mqttPublishTelemetry() {
		if (!client.connected()) return;

		JsonDocument doc;

		// -------- DADOS --------
		doc["volts"]     = Data::data.volts;
		doc["pct"]       = Data::data.pct;
		doc["rpm"]       = Data::data.rpm;
		doc["speed_kmh"] = Data::data.speed_kmh;

		if (isnan(Data::data.temp)) {
			doc["temp"] = nullptr;
		} else {
			doc["temp"] = Data::data.temp;
		}
		if (isnan(Data::data.humi)) {
			doc["humi"] = nullptr;
		} else {
			doc["humi"] = Data::data.humi;
		}

		doc["current_bat_a"] = Data::data.currentBat;
		doc["current_mot_a"] = Data::data.currentMot;

		doc["poll_ms"] = Data::data.now;

		// -------- LOGGER --------
		doc["log_enabled"] = LOGGER::isEnabled();
		doc["log_iv_ms"]   = LOGGER::getInterval();
		doc["log_size"]    = LOGGER::getCachedSize();

		// -------- CONFIG AVANÇADA --------
		doc["min"]      = Data::config.voltageMin;
		doc["max"]      = Data::config.voltageMax;

		doc["wheel_cm"] = Data::config.wheel_cm;
		doc["ppr"]      = Data::config.ppr;
		doc["max_pct"]  = Data::config.maxPct;

		doc["pwm_hz"]          = Data::config.pwm_hz;

		doc["start_min_pct"]   = Data::config.startMin;
		doc["rapid_ms"]        = Data::config.rapid_ms;
		doc["rapid_up"]        = Data::config.rapidUp;
		doc["slew_up"]         = Data::config.slewUp;
		doc["slew_dn"]         = Data::config.slewDown;

		doc["zero_timeout_ms"] = Data::config.zeroTimeout;

		// -------- ACK --------
		doc["ack"] = (ACK::getLast()[0] != '\0') ? ACK::getLast() : nullptr;

		// -------- SERIALIZAÇÃO --------
		char buffer[768];
		size_t len = serializeJson(doc, buffer, sizeof(buffer));

		if (len == 0 || len >= sizeof(buffer)) {
		    Serial.println("[MQTT] JSON serialize error");
		    return;
		}

		// -------- PUBLISH --------
		bool ok = client.publish(CONFIG.topics.telemetryJson, buffer, len);

		if (!ok) {
		    Serial.printf("[MQTT] publish FAIL, len = %u\n", len);
		}
	}
}

namespace MQTT {
    void setup() {
		client.setServer(CONFIG.host, CONFIG.port);
		client.setCallback(mqttCallback);
		client.setBufferSize(2048);

		Serial.println("======================================");
		Serial.println("[MQTT] Configuration:");
		Serial.printf("  Broker : %s:%d\n", CONFIG.host, CONFIG.port);
		Serial.printf("  Client : %s\n", CONFIG.clientId);

		Serial.println("  Topics:");
		Serial.printf("    PUB  : %s\n", CONFIG.topics.telemetryJson);
		Serial.printf("    SUB  : %s\n", CONFIG.topics.commandMotor);
		Serial.printf("    SUB  : %s\n", CONFIG.topics.commandConfig);
		Serial.printf("    STAT : %s\n", CONFIG.topics.status);
		Serial.println("======================================");
	}

    void loop() {
		static bool wasConnected = false;
		bool wifiOk = (WiFi.status() == WL_CONNECTED);

		if (!wifiOk && wasConnected) {
			client.disconnect();
		}

		wasConnected = wifiOk;

		ensureMqtt();
		client.loop();

		uint32_t now = millis();
		if (now - lastMqtt < MQTT_IV_MS) return;
		lastMqtt = now;

		mqttPublishTelemetry();

		if (ACK::getLast()[0] != '\0' && (now - ACK::getTimestamp()) > 2000) {
			ACK::setLast(""); // Adicionar mensagem?
		}
	}

	bool isClientConnected() { return client.connected(); }
}

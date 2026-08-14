#include "logger.h"

namespace LOGGER {
    struct State {
		bool enabled = false;
		uint32_t interval_ms = 1000;
		uint32_t lastLog = 0;
	};
	static State state;

	void setup() {
        state.enabled = true;

		if (LittleFS.begin(true)) {
		    Serial.println("[LOG] LittleFS Initialized");
		} else {
		    Serial.println("[LOG] LittleFS initialization failed");
		}
    }

	void loop() {
        if (!state.enabled || state.interval_ms == 0) 
            return;

		uint32_t now = millis();

		if (now - state.lastLog < state.interval_ms) 
            return;

		state.lastLog = now;
        appendCsvRow();
    }

    bool isEnabled() { return state.enabled; }
    uint32_t getInterval() { return state.interval_ms; }
    size_t getCachedSize() { return getCachedFileSize(); } // Pega o CachedSize do file.cpp

	void setEnabled(bool value) { state.enabled = value; }
	void setInterval(uint32_t value) { state.interval_ms = constrain(value, 100UL, 60000UL); }

	void clearFile() { clear(); };
}
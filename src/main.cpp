#include <Arduino.h>

#include "drivers/uart.h"
#include "drivers/parser.h"

#include "utils/ota.h"
#include "utils/wifi.h"
#include "utils/storage/logger.h"

#include "service/comm/ble/ble.h"
#include "service/comm/lora/lora.h"
#include "service/comm/mqtt/mqtt.h"

void setupTime() {
    configTime(0, 0, "pool.ntp.org", "time.google.com");
}

void setup() {
	Serial.begin(115200);
    Serial.setTimeout(50);
    Serial.println("\n[BOOT] Initializing system...");

	UART::setup();
	LOGGER::setup();

	WIFI::setup();

	OTA::setup();
	setupTime();
	BLE::setup();
	MQTT::setup();

	Serial.println("[BOOT] System Ready!\n");
}

void loop() {
	// Leitura de dados vindos do Arduino
	while (UART::SerialArduino.available()) {
        Parser::loop(UART::SerialArduino.read());
    }

	// Funções dependentes de WiFi só são executadas se tiver WiFi
    if (WIFI::loop()) {
		OTA::loop();
		MQTT::loop();
    }

	BLE::loop();
	LORA::loop();
	LOGGER::loop();	
}
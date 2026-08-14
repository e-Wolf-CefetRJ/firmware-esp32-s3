#include "wifi.h"

namespace WIFI {

    static WiFiManager manager;

    void setup() {
        WiFi.mode(WIFI_AP_STA);

        manager.setConfigPortalBlocking(false);
        manager.setTimeout(120);
        manager.setDebugOutput(true);

        Serial.println("======================================");
        Serial.println("[WiFi] Initializing Connection...");

        if (!manager.autoConnect("Telemetry-Setup")) {
            Serial.println("[WiFi] Initial connection failed.");
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Connected | IP: %s\n",
                          WiFi.localIP().toString().c_str());
        }

        Serial.println("UART Arduino in Serial1 (GPIO9 RX, GPIO10 TX)");
        Serial.println("======================================");
    }

    bool loop() {
        static uint32_t disconnectTime = 0;
        static uint32_t lastReconnect = 0;
        static bool portalOpen = false;

        manager.process();

        if (WiFi.status() == WL_CONNECTED) {

            if (portalOpen) {
                Serial.println("[WiFi] Connected. Closing config portal.");
                manager.stopConfigPortal();
                portalOpen = false;
            }

            disconnectTime = 0;
            return true;
        }

        if (disconnectTime == 0) {
            disconnectTime = millis();
            Serial.println("[WiFi] Connection lost.");
        }

        if (millis() - lastReconnect >= 5000) {
            Serial.println("[WiFi] Reconnecting...");
            WiFi.reconnect();
            lastReconnect = millis();
        }

        if (!portalOpen &&
            millis() - disconnectTime >= 10000) {

            Serial.println("[WiFi] Opening configuration portal...");
            manager.startConfigPortal("Telemetry-Setup");
            portalOpen = true;
        }

        return false;
    }

}

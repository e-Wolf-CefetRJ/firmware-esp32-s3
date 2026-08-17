#include "wifi.h"

namespace WIFI {

    static WiFiManager manager;

    static uint32_t lastReconnect = 0;
    static uint32_t lastDebug = 0;

    void setup() {

        // Permite:
        // STA -> conexão com o Wi-Fi configurado
        // AP  -> Telemetry-Setup
        WiFi.mode(WIFI_AP_STA);

        manager.setConfigPortalBlocking(false);
        manager.setDebugOutput(true);

        /*
         * Quando uma nova rede for salva pelo portal,
         * permite que o portal continue ativo.
         */
        manager.setDisableConfigPortal(false);

        /*
         * Não queremos que o portal desapareça por timeout.
         *
         * 0 = sem timeout.
         */
        manager.setConfigPortalTimeout(0);

        Serial.println("======================================");
        Serial.println("[WiFi] Initializing Connection...");


        /*
         * --------------------------------------------------
         * 1. Tenta conectar usando as credenciais salvas
         * --------------------------------------------------
         */

        Serial.println("[WiFi] Connecting to saved network...");

        WiFi.begin();

        uint32_t start = millis();

        while (WiFi.status() != WL_CONNECTED &&
               millis() - start < 10000) {

            delay(100);

            Serial.print(".");
        }

        Serial.println();


        if (WiFi.status() == WL_CONNECTED) {

            Serial.printf(
                "[WiFi] Connected | SSID: %s | IP: %s\n",
                WiFi.SSID().c_str(),
                WiFi.localIP().toString().c_str()
            );

        } else {

            Serial.println("[WiFi] No saved WiFi connection.");

        }


        /*
         * --------------------------------------------------
         * 2. Abre o portal de configuração
         * --------------------------------------------------
         *
         * O AP Telemetry-Setup fica disponível
         * independentemente de existir uma conexão STA.
         */

        Serial.println("[WiFi] Starting configuration portal...");

        manager.startConfigPortal("Telemetry-Setup");


        Serial.println("[WiFi] Configuration portal active.");

        Serial.print("[WiFi] AP SSID: ");
        Serial.println(WiFi.softAPSSID());

        Serial.print("[WiFi] AP IP: ");
        Serial.println(WiFi.softAPIP());


        Serial.println("UART Arduino in Serial1 (GPIO16 RX, GPIO17 TX)");
        Serial.println("======================================");
    }


    bool loop() {

        /*
         * IMPORTANTE:
         *
         * Como o portal é non-blocking,
         * process() precisa ser chamado continuamente.
         */
        manager.process();


        /*
         * --------------------------------------------------
         * DEBUG
         * --------------------------------------------------
         */

        if (millis() - lastDebug >= 5000) {

            lastDebug = millis();

            Serial.println("--------------------------------------");

            Serial.print("[WiFi] STA SSID: ");
            Serial.println(WiFi.SSID());

            Serial.print("[WiFi] STA IP: ");
            Serial.println(WiFi.localIP());

            Serial.print("[WiFi] STA Status: ");
            Serial.println(WiFi.status());

            Serial.print("[WiFi] AP SSID: ");
            Serial.println(WiFi.softAPSSID());

            Serial.print("[WiFi] AP IP: ");
            Serial.println(WiFi.softAPIP());

            Serial.println("--------------------------------------");
        }


        /*
         * --------------------------------------------------
         * WI-FI CONECTADO
         * --------------------------------------------------
         */

        if (WiFi.status() == WL_CONNECTED) {

            return true;
        }


        /*
         * --------------------------------------------------
         * WI-FI DESCONECTADO
         * --------------------------------------------------
         */

        if (millis() - lastReconnect >= 5000) {

            lastReconnect = millis();

            Serial.println("[WiFi] Connection lost.");
            Serial.println("[WiFi] Reconnecting...");

            WiFi.reconnect();
        }


        return false;
    }

}
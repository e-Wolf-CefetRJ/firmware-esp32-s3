#include "ble.h"

namespace {
    struct State {
        NimBLECharacteristic* txChar = nullptr;

        bool clientConnected = false;
        uint16_t mtu = 23; // Maximum Transmission Unit, Valor padrão 23 bytes (20 bytes úteis)
    };
    State state;

    // BLE apenas transmite
    class MyServerCallbacks : public NimBLEServerCallbacks {
        // Conexão
        public: 
            void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
                state.clientConnected = true;
                state.mtu = connInfo.getMTU();
                Serial.printf("[BLE] Client connected (MTU=%d)\n", state.mtu);
            }

            void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
                state.clientConnected = false;
                state.mtu = 23;

                Serial.printf("[BLE] Client desconnected (reason=%d)\n", reason);
                NimBLEDevice::startAdvertising();
                }
            };

    // Prepara JSON do BLE
    bool sendSpeedo() {
        if (!state.clientConnected || !state.txChar) return false;
        
        JsonDocument doc;   
        
        doc["speed_kmh"] = Data::data.speed_kmh;
        doc["rpm"] = Data::data.rpm;
        doc["pct"] = Data::data.pct;
        
        doc["temp"] = isnan(Data::data.temp) ? 0.0f : Data::data.temp;
        doc["current"] = isnan(Data::data.currentBat) ? 0.0f : Data::data.currentBat;
    

        char buffer[160];
        size_t len = serializeJson(doc, buffer, sizeof(buffer));

        if (len <= 0 || len >= sizeof(buffer)) {
            Serial.println("[BLE] Serializing Json Error");
            return false;
        }

        state.txChar->setValue((uint8_t*)buffer, len);
        state.txChar->notify();
        return true;
    }
}

namespace BLE {
    void setup() {
        Serial.println("=============================================================");
        Serial.println("[BLE] Initializing...");

        // 1. Inicializa e define o nome base
        NimBLEDevice::init("EWolf-Telemetry");
        NimBLEDevice::setDeviceName("EWolf-Telemetry");
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);
        NimBLEDevice::setMTU(128);

        // 2. Cria o Servidor e Serviço
        NimBLEServer* pServer = NimBLEDevice::createServer();
        pServer->setCallbacks(new MyServerCallbacks()); 

        static NimBLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
        static NimBLEUUID charTxUUID ("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

        NimBLEService* pService = pServer->createService(serviceUUID);

        // TX: telemetria para app
        state.txChar = pService->createCharacteristic(
            charTxUUID,
            NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
        );

        state.txChar->createDescriptor(
            NimBLEUUID((uint16_t)0x2902),
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
        );

        // 3. Configuração do Advertising (CORRIGIDO - sem setCompleteName)
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

        // Dados do pacote principal (onde o nome realmente aparece na varredura)
        NimBLEAdvertisementData advData;
        advData.setName("EWolf-Telemetry");  // <-- Única linha necessária para o nome
        advData.addServiceUUID(serviceUUID);

        // Dados do scan response (resposta quando o celular pede mais info)
        NimBLEAdvertisementData scanRespData;
        scanRespData.setName("EWolf-Telemetry");

        // Aplica os dados configurados
        pAdvertising->setAdvertisementData(advData);
        pAdvertising->setScanResponseData(scanRespData);
        pAdvertising->enableScanResponse(true);
        pAdvertising->setMinInterval(32);
        pAdvertising->setMaxInterval(48);

        // 4. INICIA o serviço e o advertising
        pService->start();

        if (pAdvertising->start()) {
            Serial.println("[BLE] Advertising inicialized as 'EWolf-Telemetry'");
        } else {
            Serial.println("[BLE] ERROR: Error of inicializing advertising!");
        }
        Serial.println("=============================================================");
    }

    void loop() {
        static uint32_t last = 0;
        uint32_t now = millis();

        // Envio de 10 pacotes por segundo
        if (now - last < 100) return;
        last = now;

        bool sent = sendSpeedo();

        // Debug a cada 5 segundos
        static uint32_t lastDebug = 0;
        if (sent && now - lastDebug > 5000) {
            Serial.printf("[BLE TX] Packed Sent\n");
            lastDebug = now;
        }
    }

    bool isClientConnected() { return state.clientConnected; }
}
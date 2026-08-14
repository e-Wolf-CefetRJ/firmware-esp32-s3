/*
    Transmissão de Pacote binário
*/

#include "lora.h"

namespace {
    // Precisa ser: __attribute__((packed))
    struct __attribute__((packed)) LoraTelemetryFrame {
        uint8_t  sync;       // 0xAA
        uint8_t  seq;

        uint16_t rpm;        
        uint16_t voltage_mv;   // milivolts
        int16_t  currentBat_ma;   // miliamperes

        uint8_t  flags;      // estados reais
        uint8_t  crc;
    };

    // Detecta erros
    uint8_t lora_crc8(const uint8_t* data, uint8_t len) {
        uint8_t crc = 0x00;

        for (uint8_t i = 0; i < len; i++) {
            crc ^= data[i];

            for (uint8_t j = 0; j < 8; j++) {

                if (crc & 0x80) 
                    crc = (crc << 1) ^ 0x07;
                else 
                    crc <<= 1;

            }
        }
        return crc;
    }

    enum FrameFlags : uint8_t {
        BLE_CONNECTED_LORA  = 1 << 0,
        MQTT_CONNECTED_LORA = 1 << 1,
        LOGGER_ENABLED_LORA = 1 << 2,
    };

    static uint8_t loraSeq = 0;

    void txTelemetry() {
        LoraTelemetryFrame frame;

        frame.sync = 0xAA;
        frame.seq  = loraSeq++;

        frame.rpm = constrain((uint16_t) Data::data.rpm, 0, 65535);

        frame.voltage_mv = constrain(
            (uint16_t) roundf(Data::data.volts * 1000.0f),
            0,
            65535
        );        
        frame.currentBat_ma  = constrain(
            (int16_t) roundf(Data::data.currentBat * 1000.0f), 
            -32768, 
            32767
        );

        frame.flags = 0;

        if (BLE::isClientConnected())      
            frame.flags |= BLE_CONNECTED_LORA;
        
        if (MQTT::isClientConnected())       
            frame.flags |= MQTT_CONNECTED_LORA;

        if (LOGGER::isEnabled())         
            frame.flags |= LOGGER_ENABLED_LORA;

        frame.crc = lora_crc8((uint8_t*)&frame, sizeof(frame) - 1);

        UART::SerialLora.write((uint8_t*)&frame, sizeof(frame));
    }
}

namespace LORA {
    void loop() {
		static uint32_t last = 0;
		uint32_t now = millis();

		if (now - last < 100) return;

		last = now;

		txTelemetry();
	}
}
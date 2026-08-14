#include "uart.h"

namespace UART {
    HardwareSerial SerialArduino(1);
    HardwareSerial SerialLora(2);

    void setup() {
        SerialArduino.begin(115200,SERIAL_8N1,Pins::ARDUINO_RX,Pins::ARDUINO_TX);
        SerialLora.begin(9600,SERIAL_8N1,Pins::LORA_RX,Pins::LORA_TX);
        Serial.println("[LoRa] SerialLoRa inicializada");
    }
}
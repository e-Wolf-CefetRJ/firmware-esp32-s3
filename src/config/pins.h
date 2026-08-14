#pragma once

#include <Arduino.h>

namespace Pins {
    // ========= UART COM ARDUINO (Serial1) =========
    static constexpr int ARDUINO_RX = 16;   // ESP32-S3 RX(2)  <- TX do MEGA
    static constexpr int ARDUINO_TX = 17;  // ESP32-S3 TX(2)  -> RX do MEGA

    // ========= UART LORA (Serial2) =========
    static constexpr int LORA_RX = 4; // ESP32 RX  <- TX do LoRa
    static constexpr int LORA_TX = 5; // ESP32 TX  -> RX do LoRa
    static constexpr int LORA_BAUD = 9600;
}
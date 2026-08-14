#pragma once

#include <Arduino.h>

namespace Data {
    // ESP -> MEGA
    struct ArduinoConfig {
        // Acelerador
        float voltageMin; 
        float voltageMax;
        
        // PWM
        uint16_t pwm_hz;
        
        // Rampa
        uint16_t rapid_ms;
        float rapidUp;
        float slewUp;
        float slewDown;
        uint8_t startMin;
        uint8_t maxPct;

        // RPM
        float wheel_cm;
        uint8_t ppr;
        uint32_t zeroTimeout; 
    };

    // ESP -> MQTT
    struct TelemetryData {
        uint32_t now;   

        // Sensors
        float volts;
        float pct;

        float temp;
        float humi;

        float rpm;
        float speed_kmh;

        float currentBat;
        float currentMot;

        bool overrideEnabled;
        float overridePct;
    };

    extern ArduinoConfig config;
    extern TelemetryData data;
}
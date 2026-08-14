#pragma once

#include <Arduino.h>

#include "config/pins.h"

namespace UART {
    extern HardwareSerial SerialArduino;
    extern HardwareSerial SerialLora; // não utilizado atualmente

    void setup();
}
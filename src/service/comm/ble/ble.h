#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ArduinoJSON.h>

#include "core/data.h"

namespace BLE {
    void setup();
	void loop();

    bool isClientConnected();
}
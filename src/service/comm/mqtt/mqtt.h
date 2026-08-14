#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "core/data.h"
#include "core/ack.h"

#include "drivers/uart.h"

#include "utils/storage/logger.h"

namespace MQTT {
    void setup();
    void loop();

    bool isClientConnected();
}
#pragma once

#include <Arduino.h>

#include "drivers/uart.h"

#include "utils/storage/logger.h"

#include "core/data.h"

#include "service/comm/ble/ble.h"
#include "service/comm/mqtt/mqtt.h"

namespace LORA {
    void loop();
}
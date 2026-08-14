#pragma once

#include <Arduino.h>

namespace ACK {
    void setLast(const char* ack);
    const char* getLast();

    uint32_t getTimestamp();
}
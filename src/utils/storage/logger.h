#pragma once

#include <Arduino.h>
#include <LittleFS.h>

#include "file.h"

namespace LOGGER {
    void setup();
	void loop();

	bool isEnabled();
    uint32_t getInterval();
    size_t getCachedSize();

    void setEnabled(bool value);
    void setInterval(uint32_t value);

    void clearFile();
}
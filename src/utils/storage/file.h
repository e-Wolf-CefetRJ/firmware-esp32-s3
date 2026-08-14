#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>

#include "core/data.h"
#include "core/ack.h"
#include "logger.h"

void appendCsvRow();
size_t getCachedFileSize();
void clear();
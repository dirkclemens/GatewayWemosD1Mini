#pragma once

#include <Arduino.h>

String buildTelemetryJson(uint32_t heap, uint8_t frag, uint32_t maxBlock, int32_t rssi,
                          const String &wifiStatus, int lastIndicatorCode,
                          unsigned long gwRx, unsigned long gwTx,
                          unsigned long sensorRx, unsigned long sensorTx,
                          unsigned long indicatorErrors);


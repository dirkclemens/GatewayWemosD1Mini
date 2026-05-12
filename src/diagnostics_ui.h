#pragma once

#include <stdint.h>
#include <stddef.h>

void buildTelemetryJson(char *buf, size_t buflen,
                        uint32_t heap, uint8_t frag, uint32_t maxBlock, int32_t rssi,
                        const char *wifiStatus, int lastIndicatorCode,
                        unsigned long gwRx, unsigned long gwTx,
                        unsigned long sensorRx, unsigned long sensorTx,
                        unsigned long indicatorErrors,
                        bool controllerUp,
                        const char *controllerType);

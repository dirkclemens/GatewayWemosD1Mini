#include "diagnostics_ui.h"
#include <stdio.h>

void buildTelemetryJson(char *buf, size_t buflen,
                        uint32_t heap, uint8_t frag, uint32_t maxBlock, int32_t rssi,
                        const char *wifiStatus, int lastIndicatorCode,
                        unsigned long gwRx, unsigned long gwTx,
                        unsigned long sensorRx, unsigned long sensorTx,
                        unsigned long indicatorErrors,
                        bool controllerUp,
                        const char *controllerType)
{
	if (snprintf(buf, buflen,
	             "{\"heap\":%lu,\"frag\":%u,\"maxblk\":%lu,\"rssi\":%ld,\"wifi\":\"%s\","
	             "\"lastInd\":%d,\"gwRx\":%lu,\"gwTx\":%lu,\"sRx\":%lu,\"sTx\":%lu,\"err\":%lu,"
	             "\"ctrlUp\":%s,\"ctrlType\":\"%s\"}",
	             (unsigned long)heap, (unsigned)frag, (unsigned long)maxBlock, (long)rssi,
	             wifiStatus, lastIndicatorCode,
	             gwRx, gwTx, sensorRx, sensorTx, indicatorErrors,
	             controllerUp ? "true" : "false",
	             controllerType ? controllerType : "none") < 0) {
		buf[0] = '{'; buf[1] = '}'; buf[2] = '\0';
	}
}

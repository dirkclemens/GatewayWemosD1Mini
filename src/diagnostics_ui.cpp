#include "diagnostics_ui.h"

String buildTelemetryJson(uint32_t heap, uint8_t frag, uint32_t maxBlock, int32_t rssi,
                          const String &wifiStatus, int lastIndicatorCode,
                          unsigned long gwRx, unsigned long gwTx,
                          unsigned long sensorRx, unsigned long sensorTx,
                          unsigned long indicatorErrors)
{
	char buf[256];
	if (snprintf(buf, sizeof(buf),
	             "{\"heap\":%lu,\"frag\":%u,\"maxblk\":%lu,\"rssi\":%ld,\"wifi\":\"%s\","
	             "\"lastInd\":%d,\"gwRx\":%lu,\"gwTx\":%lu,\"sRx\":%lu,\"sTx\":%lu,\"err\":%lu}",
	             static_cast<unsigned long>(heap),
	             static_cast<unsigned>(frag),
	             static_cast<unsigned long>(maxBlock),
	             static_cast<long>(rssi),
	             wifiStatus.c_str(),
	             lastIndicatorCode,
	             gwRx, gwTx, sensorRx, sensorTx, indicatorErrors) < 0) {
		return String("{}");
	}
	return String(buf);
}


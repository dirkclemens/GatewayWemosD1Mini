#include "config.h"

void setup_WebServer();
void loop_WebServer();
void send_Event(const char *content, const char *section);
String getNodeNameById(uint8_t nodeId);
void updateSensorStateCache(uint8_t nodeId,
							uint8_t sensorId,
							const char *msgType,
							const char *payload,
							const char *timestamp,
							bool isSetMessage);
void triggerNtpSync();
void updateHealthSnapshot(uint32_t bootCount,
						  uint32_t lastBootEpoch,
						  uint8_t resetReasonCode,
						  bool ntpSynced);

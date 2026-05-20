#include "config.h"

void setup_WebServer();
void loop_WebServer();
void send_Event(const char *content, const char *section);
bool isWebDebugCompiled();
bool isWebDebugEnabled();
void setWebDebugEnabled(bool enabled);
bool isWebLiveUiEnabled();
void setWebLiveUiEnabled(bool enabled);
bool isTelnetRuntimeEnabled();
void setTelnetRuntimeEnabled(bool enabled);
bool isOtaRuntimeEnabled();
void setOtaRuntimeEnabled(bool enabled);
uint32_t getOtaWindowRemainingSec();
uint32_t getUiUpdateIntervalMs();
bool setUiUpdateIntervalMs(uint32_t intervalMs);
const char *getNodeNameById(uint8_t nodeId);
void registerPresentedSensor(uint8_t nodeId, uint8_t sensorId);
bool updateSensorStateCache(uint8_t nodeId,
							uint8_t sensorId,
							const char *msgType,
							const char *payload,
							const char *timestamp,
							bool isSetMessage);
void triggerNtpSync();
void updateHealthSnapshot(uint32_t bootCount,
						  uint32_t lastBootEpoch,
						  uint8_t resetReasonCode,
						  const char *resetReasonText,
						  const char *resetReasonRaw,
						  const char *lastRestartMarker,
						  bool ntpSynced);

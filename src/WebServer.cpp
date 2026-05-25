/*
 * GatewayESP32 WebServer
 */

#include "WebServer.h"
#include "config.h"
#include "common.h"
#include "uptime.h"
#include "platform_compat.h"

#include "index.h"
#include "style_css.h"
#include "reboot.h"
// #include "svg_gz.h"

#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <cstring>
#include <cstdlib>
#include <cmath>

AsyncWebServer server(80);
AsyncEventSource events("/events");
static const char *NODE_NAMES_FILE = "/node_names.txt";
static const uint8_t NODE_NAME_SLOTS = 32;
struct NodeNameEntry {
	uint8_t id;
	String name;
	bool used;
};
static NodeNameEntry nodeNames[NODE_NAME_SLOTS];
static uint32_t gBootCount = 0;
static uint32_t gLastBootEpoch = 0;
static uint8_t gResetReasonCode = 0;
static char gResetReasonText[64] = {'\0'};
static char gResetReasonRaw[128] = {'\0'};
static char gLastRestartMarker[160] = {'\0'};
static bool gNtpSynced = false;
static bool gWebDebugEnabled = false;
static bool gWebLiveUiEnabled = true;
static const int UI_RSSI_MIN_DBM = -85;

static const uint16_t SENSOR_STATE_SLOTS = 256;
struct SensorStateEntry {
	uint8_t nodeId;
	uint8_t sensorId;
	char value[48];
	char time[24];
	char type[24];
	bool used;
};
static SensorStateEntry sensorStates[SENSOR_STATE_SLOTS];
struct PresentedSensorEntry {
	uint8_t nodeId;
	uint8_t sensorId;
	bool used;
};
static PresentedSensorEntry presentedSensors[SENSOR_STATE_SLOTS];

static bool typeEquals(const char *msgType, const char *expected)
{
	if (!msgType || !expected) {
		return false;
	}
	const size_t len = strlen(expected);
	return strncmp(msgType, expected, len) == 0 &&
		   (msgType[len] == '\0' || msgType[len] == ' ');
}

static bool isUnknownTypePlaceholder(const char *msgType)
{
	return msgType && strncmp(msgType, "type:", 5) == 0;
}

static bool parseLeadingNumber(const char *payload, double &value)
{
	if (!payload) {
		return false;
	}
	char *end = nullptr;
	value = strtod(payload, &end);
	return end != payload && std::isfinite(value);
}

static bool isBinaryLikeValue(double value)
{
	const double rounded = round(value);
	return fabs(value - rounded) < 0.001 && (rounded == 0.0 || rounded == 1.0);
}

static int findPresentedSensorIndex(uint8_t nodeId, uint8_t sensorId)
{
	for (uint16_t i = 0; i < SENSOR_STATE_SLOTS; i++) {
		if (presentedSensors[i].used &&
			presentedSensors[i].nodeId == nodeId &&
			presentedSensors[i].sensorId == sensorId) {
			return i;
		}
	}
	return -1;
}

static int findFreePresentedSensorSlot()
{
	for (uint16_t i = 0; i < SENSOR_STATE_SLOTS; i++) {
		if (!presentedSensors[i].used) {
			return i;
		}
	}
	return -1;
}

static bool nodeHasPresentedChildren(uint8_t nodeId)
{
	for (uint16_t i = 0; i < SENSOR_STATE_SLOTS; i++) {
		if (presentedSensors[i].used && presentedSensors[i].nodeId == nodeId) {
			return true;
		}
	}
	return false;
}

static bool shouldAcceptSensorValue(uint8_t nodeId,
									uint8_t sensorId,
									const char *msgType,
									const char *payload)
{
	if (!msgType || !msgType[0] || !payload || !payload[0]) {
		return false;
	}

	// Child 255 is reserved for internal/system messages, not regular sensors.
	if (sensorId == 255) {
		return false;
	}
	if (isUnknownTypePlaceholder(msgType)) {
		dbgprintf(ico_warning, "drop unknown SET type: node=%u child=%u %s", nodeId, sensorId, msgType);
		return false;
	}
	if (nodeHasPresentedChildren(nodeId) && findPresentedSensorIndex(nodeId, sensorId) < 0) {
		dbgprintf(ico_warning, "accept unpresented sensor id: node=%u child=%u (learning)", nodeId, sensorId);
	}

	double numericValue = 0.0;
	const bool isNumeric = parseLeadingNumber(payload, numericValue);
	if (isNumeric && fabs(numericValue) > 1000000.0) {
		dbgprintf(ico_warning, "drop implausible sensor value: %s=%s", msgType, payload);
		return false;
	}

	if (typeEquals(msgType, "V_TEMP")) {
		return isNumeric && numericValue >= -60.0 && numericValue <= 125.0;
	}
	if (typeEquals(msgType, "V_HUM")) {
		return isNumeric && numericValue >= 0.0 && numericValue <= 100.0;
	}
	if (typeEquals(msgType, "V_VOLTAGE")) {
		return isNumeric && numericValue >= 0.0 && numericValue <= 500.0;
	}
	if (typeEquals(msgType, "V_PERCENTAGE") || typeEquals(msgType, "V_LEVEL")) {
		return isNumeric && numericValue >= 0.0 && numericValue <= 100.0;
	}
	if (typeEquals(msgType, "V_STATUS") ||
		typeEquals(msgType, "V_TRIPPED") ||
		typeEquals(msgType, "V_ARMED") ||
		typeEquals(msgType, "V_LOCK_STATUS")) {
		return isNumeric && isBinaryLikeValue(numericValue);
	}
	if (typeEquals(msgType, "V_HVAC_FLOW_MODE")) {
		return isNumeric && fabs(numericValue - round(numericValue)) < 0.001 &&
			   numericValue >= 0.0 && numericValue <= 6.0;
	}
	if (typeEquals(msgType, "V_UNIT_PREFIX")) {
		// Unit prefixes are expected to be textual ("V", "m", "cm", ...).
		return !isNumeric;
	}

	// Keep known free-form values (e.g. V_TEXT, V_CUSTOM) after basic checks above.
	return true;
}

//flag to use from web update to reboot the ESP
bool shouldReboot 							= false;

static String trimCopy(const String &in)
{
	String s = in;
	s.trim();
	return s;
}

static bool isSafeFsPath(const String &path)
{
	if (!path.length() || path.length() > 63) {
		return false;
	}
	if (path[0] != '/') {
		return false;
	}
	if (path.indexOf("..") >= 0 || path.indexOf('\\') >= 0) {
		return false;
	}
	return true;
}

static const char *guessContentType(const String &path)
{
	if (path.endsWith(".txt") || path.endsWith(".log")) return "text/plain";
	if (path.endsWith(".json")) return "application/json";
	if (path.endsWith(".html")) return "text/html";
	if (path.endsWith(".css")) return "text/css";
	if (path.endsWith(".js")) return "application/javascript";
	if (path.endsWith(".svg")) return "image/svg+xml";
	if (path.endsWith(".ico")) return "image/x-icon";
	return "application/octet-stream";
}

static int findNodeNameIndexById(int id)
{
	for (uint8_t i = 0; i < NODE_NAME_SLOTS; i++) {
		if (nodeNames[i].used && nodeNames[i].id == static_cast<uint8_t>(id)) {
			return i;
		}
	}
	return -1;
}

static int findFreeNodeNameSlot()
{
	for (uint8_t i = 0; i < NODE_NAME_SLOTS; i++) {
		if (!nodeNames[i].used) {
			return i;
		}
	}
	return -1;
}

static bool isDebugEventSection(const char *section)
{
	return section &&
		   (strcmp(section, "debug") == 0 ||
		    strcmp(section, "indicator") == 0 ||
		    strcmp(section, "led") == 0 ||
		    strcmp(section, "telemetry") == 0);
}

static bool isLiveUiEventSection(const char *section)
{
	return section &&
		   (strcmp(section, "info") == 0 ||
		    strcmp(section, "clients") == 0 ||
		    isDebugEventSection(section));
}

static bool writeRestartMarker(const char *reason)
{
	if (!reason || !gatewayFsBegin()) {
		return false;
	}
	File file = GATEWAY_FS.open("/last_restart_marker.txt", "w");
	if (!file || file.isDirectory()) {
		return false;
	}
	char marker[160] = {'\0'};
	if (snprintf(marker, sizeof(marker),
				 "cause=%s|uptime_s=%lu|heap=%lu|frag=%u|wifi=%d|rssi=%d",
				 reason,
				 millis() / 1000UL,
				 static_cast<unsigned long>(ESP.getFreeHeap()),
				 static_cast<unsigned>(getHeapFragmentationPct()),
				 static_cast<int>(WiFi.status()),
				 WiFi.RSSI()) < 0) {
		file.close();
		return false;
	}
	file.print(marker);
	file.print('\n');
	file.close();
	return true;
}

static int findSensorStateIndex(uint8_t nodeId, uint8_t sensorId)
{
	for (uint16_t i = 0; i < SENSOR_STATE_SLOTS; i++) {
		if (sensorStates[i].used && sensorStates[i].nodeId == nodeId && sensorStates[i].sensorId == sensorId) {
			return i;
		}
	}
	return -1;
}

static int findFreeSensorStateSlot()
{
	for (uint16_t i = 0; i < SENSOR_STATE_SLOTS; i++) {
		if (!sensorStates[i].used) {
			return i;
		}
	}
	return -1;
}

static void appendJsonEscapedToStream(AsyncResponseStream *resp, const char *text)
{
	if (!resp || !text) {
		return;
	}
	const char *p = text;
	while (*p) {
		if (*p == '\\' || *p == '"') {
			resp->write(static_cast<uint8_t>('\\'));
		}
		resp->write(static_cast<uint8_t>(*p));
		p++;
	}
}

static void sendSensorStateJson(AsyncWebServerRequest *request)
{
	if (!request) {
		return;
	}

	AsyncResponseStream *resp = request->beginResponseStream("application/json");
	bool hasNode[256] = {false};
	for (uint16_t i = 0; i < SENSOR_STATE_SLOTS; i++) {
		if (sensorStates[i].used) {
			hasNode[sensorStates[i].nodeId] = true;
		}
	}

	resp->print("{");
	bool firstNode = true;
	for (uint16_t node = 0; node <= 255; node++) {
		if (!hasNode[node]) {
			continue;
		}
		if (!firstNode) {
			resp->print(",");
		}
		firstNode = false;
		resp->printf("\"%u\":{", node);

		bool firstSensor = true;
		for (uint16_t i = 0; i < SENSOR_STATE_SLOTS; i++) {
			if (!sensorStates[i].used || sensorStates[i].nodeId != node) {
				continue;
			}
			if (!firstSensor) {
				resp->print(",");
			}
			firstSensor = false;
			resp->printf("\"%u\":{\"value\":\"", sensorStates[i].sensorId);
			appendJsonEscapedToStream(resp, sensorStates[i].value);
			resp->print("\",\"time\":\"");
			appendJsonEscapedToStream(resp, sensorStates[i].time);
			resp->print("\",\"type\":\"");
			appendJsonEscapedToStream(resp, sensorStates[i].type);
			resp->print("\"}");
		}
		resp->print("}");
	}
	resp->print("}");
	request->send(resp);
}

static void appendFsListing(AsyncResponseStream *resp, bool asJson, size_t maxEntries = 0)
{
	File root = GATEWAY_FS.open("/");
	if (!root || !root.isDirectory()) {
		return;
	}
	File file = root.openNextFile();
	bool first = true;
	size_t count = 0;
	while (file) {
		if (maxEntries > 0 && count >= maxEntries) {
			if (asJson) {
				if (!first) {
					resp->print(",");
				}
				resp->print("{\"path\":\"...\",\"size\":0}");
			} else {
				resp->print("... file listing truncated\n");
			}
			break;
		}
		if (asJson) {
			if (!first) {
				resp->print(",");
			}
			resp->printf("{\"path\":\"%s\",\"size\":%lu}",
						 file.name(),
						 static_cast<unsigned long>(file.size()));
		} else {
			char szBuf[12];
			formatBytes(file.size(), szBuf, sizeof(szBuf));
			resp->printf("FS File: %s, size: %s\n", file.name(), szBuf);
		}
		first = false;
		count++;
		file = root.openNextFile();
	}
}

static void loadNodeNames()
{
	for (uint16_t i = 0; i < NODE_NAME_SLOTS; i++) {
		nodeNames[i].id = 0;
		nodeNames[i].name = "";
		nodeNames[i].used = false;
	}
	if (!gatewayFsBegin()) {
		dbgprintln(ico_error, "could not open filesystem for node name loading");
		return;
	}
	if (!GATEWAY_FS.exists(NODE_NAMES_FILE)) {
		return;
	}
	File file = GATEWAY_FS.open(NODE_NAMES_FILE, "r");
	if (!file) {
		dbgprintln(ico_error, "could not open node_names.txt");
		return;
	}
	while (file.available()) {
		String line = file.readStringUntil('\n');
		line.trim();
		if (!line.length()) {
			continue;
		}
		int sep = line.indexOf('=');
		if (sep <= 0) {
			continue;
		}
		String idPart = trimCopy(line.substring(0, sep));
		String namePart = trimCopy(line.substring(sep + 1));
		int id = idPart.toInt();
		if (id >= 0 && id <= 255 && namePart.length()) {
			if (namePart.length() > 31) {
				namePart = namePart.substring(0, 31);
			}
			int slot = findNodeNameIndexById(id);
			if (slot < 0) {
				slot = findFreeNodeNameSlot();
			}
			if (slot >= 0) {
				nodeNames[slot].id = static_cast<uint8_t>(id);
				nodeNames[slot].name = namePart;
				nodeNames[slot].used = true;
			}
		}
	}
	file.close();
}

static bool saveNodeNames()
{
	if (!gatewayFsBegin()) {
		dbgprintln(ico_error, "could not open filesystem for node name saving");
		return false;
	}
	File file = GATEWAY_FS.open(NODE_NAMES_FILE, "w");
	if (!file) {
		dbgprintln(ico_error, "could not write node_names.txt");
		return false;
	}
	for (uint16_t i = 0; i < NODE_NAME_SLOTS; i++) {
		if (nodeNames[i].used && nodeNames[i].name.length()) {
			file.printf("%u=%s\n", nodeNames[i].id, nodeNames[i].name.c_str());
		}
	}
	file.close();
	return true;
}

static String nodeNamesAsJson()
{
	String out = "{";
	bool first = true;
	for (uint16_t i = 0; i < NODE_NAME_SLOTS; i++) {
		if (!nodeNames[i].used || !nodeNames[i].name.length()) {
			continue;
		}
		if (!first) {
			out += ",";
		}
		first = false;
		out += "\"";
		out += nodeNames[i].id;
		out += "\":\"";
		String name = nodeNames[i].name;
		name.replace("\\", "\\\\");
		name.replace("\"", "\\\"");
		out += name;
		out += "\"";
	}
	out += "}";
	return out;
}

/*
 *
 */
void send_Event(const char *content, const char *section)
{
	if (content == nullptr || section == nullptr || content[0] == '\0') {
		return;
	}
	if (!gWebLiveUiEnabled && isLiveUiEventSection(section)) {
		return;
	}
	if (!gWebDebugEnabled && isDebugEventSection(section)) {
		return;
	}
	if (events.count() == 0) {
		return;
	}
	// Drop low-priority bursts when client queues are already backed up.
	const size_t packetsWaiting = events.avgPacketsWaiting();
	if (packetsWaiting > 4 &&
		(strcmp(section, "debug") == 0 ||
		 strcmp(section, "telemetry") == 0 ||
		 strcmp(section, "info") == 0 ||
		 strcmp(section, "clients") == 0)) {
		return;
	}
	events.send(content, section);
}

bool isWebDebugCompiled()
{
#ifdef WITH_WEB_DEBUG
	return true;
#else
	return false;
#endif
}

bool isWebDebugEnabled()
{
	return gWebDebugEnabled;
}

void setWebDebugEnabled(bool enabled)
{
#ifdef WITH_WEB_DEBUG
	gWebDebugEnabled = enabled;
#else
	(void)enabled;
	gWebDebugEnabled = false;
#endif
}

bool isWebLiveUiEnabled()
{
	return gWebLiveUiEnabled;
}

void setWebLiveUiEnabled(bool enabled)
{
	gWebLiveUiEnabled = enabled;
}

bool updateSensorStateCache(uint8_t nodeId,
							uint8_t sensorId,
							const char *msgType,
							const char *payload,
							const char *timestamp,
							bool isSetMessage)
{
	if (!isSetMessage) {
		return false;
	}
	if (!shouldAcceptSensorValue(nodeId, sensorId, msgType, payload)) {
		return false;
	}

	int idx = findSensorStateIndex(nodeId, sensorId);
	if (idx < 0) {
		idx = findFreeSensorStateSlot();
		if (idx < 0) {
			return false;
		}
	}

	registerPresentedSensor(nodeId, sensorId);
	sensorStates[idx].used = true;
	sensorStates[idx].nodeId = nodeId;
	sensorStates[idx].sensorId = sensorId;
	strncpy(sensorStates[idx].value, payload ? payload : "", sizeof(sensorStates[idx].value) - 1);
	sensorStates[idx].value[sizeof(sensorStates[idx].value) - 1] = '\0';
	strncpy(sensorStates[idx].time, timestamp ? timestamp : "", sizeof(sensorStates[idx].time) - 1);
	sensorStates[idx].time[sizeof(sensorStates[idx].time) - 1] = '\0';
	strncpy(sensorStates[idx].type, msgType ? msgType : "", sizeof(sensorStates[idx].type) - 1);
	sensorStates[idx].type[sizeof(sensorStates[idx].type) - 1] = '\0';
	return true;
}

void registerPresentedSensor(uint8_t nodeId, uint8_t sensorId)
{
	if (sensorId == 255) {
		return;
	}

	int idx = findPresentedSensorIndex(nodeId, sensorId);
	if (idx < 0) {
		idx = findFreePresentedSensorSlot();
		if (idx < 0) {
			return;
		}
	}

	presentedSensors[idx].used = true;
	presentedSensors[idx].nodeId = nodeId;
	presentedSensors[idx].sensorId = sensorId;
}

void updateHealthSnapshot(uint32_t bootCount,
						  uint32_t lastBootEpoch,
						  uint8_t resetReasonCode,
						  const char *resetReasonText,
						  const char *resetReasonRaw,
						  const char *lastRestartMarker,
						  bool ntpSynced)
{
	gBootCount = bootCount;
	gLastBootEpoch = lastBootEpoch;
	gResetReasonCode = resetReasonCode;
	strncpy(gResetReasonText, resetReasonText ? resetReasonText : "unknown", sizeof(gResetReasonText) - 1);
	gResetReasonText[sizeof(gResetReasonText) - 1] = '\0';
	strncpy(gResetReasonRaw, resetReasonRaw ? resetReasonRaw : "n/a", sizeof(gResetReasonRaw) - 1);
	gResetReasonRaw[sizeof(gResetReasonRaw) - 1] = '\0';
	strncpy(gLastRestartMarker, lastRestartMarker ? lastRestartMarker : "", sizeof(gLastRestartMarker) - 1);
	gLastRestartMarker[sizeof(gLastRestartMarker) - 1] = '\0';
	gNtpSynced = ntpSynced;
}

const char *getNodeNameById(uint8_t nodeId)
{
	int idx = findNodeNameIndexById(nodeId);
	if (idx >= 0) {
		return nodeNames[idx].name.c_str();
	}
	return "";
}


void send_Status(AsyncWebServerRequest *request)
{
	static const size_t STATUS_MAX_BOOTLOG_LINES = 30;
	static const size_t STATUS_MAX_BOOTLOG_BYTES = 3072;
	static const size_t STATUS_MAX_FILE_ENTRIES = 80;
	static const size_t STATUS_LINE_BUF_SIZE = 192;
	const bool includeDetails =
		request && request->hasParam("full") &&
		trimCopy(request->getParam("full")->value()) == "1";

	uint32_t realSize = ESP.getFlashChipSize();
	uint32_t ideSize = ESP.getFlashChipSize();
	uint8_t heapFragmentation = getHeapFragmentationPct();
	uint32_t maxFreeBlocks = getMaxFreeBlockBytes();

	AsyncResponseStream *resp = request->beginResponseStream("text/plain");
	resp->print ("Status\n-----------------------------\n");
	resp->printf("             Chip model: ESP32\n");
	resp->println();
	resp->printf("               Heap: %d bytes\n", ESP.getFreeHeap());
	resp->printf(" Heap Fragmentation: %d %%\n", heapFragmentation);
	resp->printf("Max Free Block Size: %d bytes\n", maxFreeBlocks);
	resp->printf("    Flash real size: %d bytes\n", realSize);
	resp->printf("    Flash ide  size: %d bytes\n", ideSize);
	resp->printf("          CPU speed: %d MHz\n",  ESP.getCpuFreqMHz());
	resp->printf("    Flash ide speed: %d MHz\n",  ESP.getFlashChipSpeed()/1000/1000);
	resp->println();
	resp->printf("       reset reason: %d\n", static_cast<int>(esp_reset_reason()));
	resp->println();

	resp->printf("          boot time: %s\n", getBootTime());
	resp->printf("           hostname: %s\n", getWifiHostname().c_str());
	resp->printf("            runtime: %s\n", runtime());
	resp->printf("             uptime: %s\n", uptime());
	resp->printf("              build: %s %s\n", __DATE__, __TIME__);
	resp->printf("         sw version: %s\n", __SWVERSION__);
	resp->printf("           local ip: %s\n", WiFi.localIP().toString().c_str());
	resp->printf("               ssid: %s\n", WiFi.SSID().c_str());
	resp->printf("         gateway ip: %s\n", WiFi.gatewayIP().toString().c_str());
	resp->printf("        mac address: %s\n", WiFi.macAddress().c_str());
		resp->printf("               rssi: %d\n", WiFi.RSSI());
	resp->println();
	resp->printf("details: %s (use /stats?full=1 for FS+bootlog)\n", includeDetails ? "enabled" : "disabled");
	resp->println();

	if (includeDetails) {
		boolean fsOk = gatewayFsBegin();
		if (fsOk)
		{
			resp->print ("Filesystem\n-----------------------------\n");
			resp->printf("      FS totalBytes: %lu bytes\n", static_cast<unsigned long>(GATEWAY_FS.totalBytes()));
			resp->printf("      FS  usedBytes: %lu bytes\n", static_cast<unsigned long>(GATEWAY_FS.usedBytes()));
			resp->println();

			resp->print ("Files\n-----------------------------\n");
			appendFsListing(resp, false, STATUS_MAX_FILE_ENTRIES);
			resp->println();

			resp->print ("Bootlog\n-----------------------------\n");
			File file = GATEWAY_FS.open("/bootlog.txt", "r");
			if (file && !file.isDirectory()) {
				size_t lines = 0;
				size_t writtenBytes = 0;
				char lineBuf[STATUS_LINE_BUF_SIZE];
				while (file.available() && lines < STATUS_MAX_BOOTLOG_LINES && writtenBytes < STATUS_MAX_BOOTLOG_BYTES) {
					size_t n = file.readBytesUntil('\n', lineBuf, STATUS_LINE_BUF_SIZE - 1);
					lineBuf[n] = '\0';
					resp->printf("%s\n", lineBuf);
					writtenBytes += n + 1;
					lines++;
				}
				if (file.available()) {
					resp->printf("... bootlog truncated (%u lines / %u bytes limit)\n",
								 static_cast<unsigned>(STATUS_MAX_BOOTLOG_LINES),
								 static_cast<unsigned>(STATUS_MAX_BOOTLOG_BYTES));
				}
				file.close();
			} else {
				resp->print("bootlog not available\n");
			}
		}
		else
		{
			resp->printf("error opening filesystem!\n");
			dbgprintln(ico_error, "error: reading from filesystem.");
		}
	}

	request->send(resp);
}

static void send_StatusMini(AsyncWebServerRequest *request)
{
	if (!request) {
		return;
	}

	const bool wifiConnected = (WiFi.status() == WL_CONNECTED);
	const int rssi = WiFi.RSSI();
	const bool weakLink = (!wifiConnected || rssi < UI_RSSI_MIN_DBM);
	char nowStr[22] = {'\0'};
	getCurrentTimeString(nowStr, sizeof(nowStr), "%Y-%m-%d %H:%M:%S");

	AsyncResponseStream *resp = request->beginResponseStream("text/html");
	resp->print("<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Gateway Mini</title></head><body style=\"font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;padding:14px;line-height:1.35\">");
	resp->print("<h3 style=\"margin:0 0 10px 0\">Gateway Mini Status</h3>");
	if (weakLink) {
		resp->printf("<p style=\"margin:0 0 10px 0;color:#9a5a00;background:#fff6e5;border:1px solid #f0d9a8;padding:8px;border-radius:6px\">Schwache WLAN-Verbindung erkannt (RSSI %d dBm, Schwellwert %d dBm). Mini-Ansicht wird empfohlen.</p>",
					 rssi,
					 UI_RSSI_MIN_DBM);
	}
	resp->print("<p style=\"margin:0 0 12px 0\"><a href=\"/ui\">Volle UI laden</a> | <a href=\"/stats\">Stats</a> | <a href=\"/stats?full=1\">Stats full</a> | <a href=\"/healthz\">Health</a></p>");
	resp->print("<pre style=\"white-space:pre-wrap;background:#f7f7f7;border:1px solid #ddd;padding:10px;border-radius:6px;margin:0\">");
	resp->printf("host: %s\n", getWifiHostname().c_str());
	resp->printf("time: %s\n", nowStr);
	resp->printf("uptime: %s\n", uptime());
	resp->printf("runtime: %s\n", runtime());
	resp->printf("build: %s %s\n", __DATE__, __TIME__);
	resp->printf("sw version: %s\n\n", __SWVERSION__);
	resp->printf("wifi connected: %s\n", wifiConnected ? "yes" : "no");
	resp->printf("wifi status: %d\n", static_cast<int>(WiFi.status()));
	resp->printf("ip: %s\n", WiFi.localIP().toString().c_str());
	resp->printf("ssid: %s\n", WiFi.SSID().c_str());
	resp->printf("rssi: %d\n\n", rssi);
	resp->printf("heap: %lu bytes\n", static_cast<unsigned long>(ESP.getFreeHeap()));
	resp->printf("heap fragmentation: %u %%\n", static_cast<unsigned>(getHeapFragmentationPct()));
	resp->printf("max free block: %u bytes\n", static_cast<unsigned>(getMaxFreeBlockBytes()));
	resp->print("</pre></body></html>");

	request->send(resp);
}

/*
 *
 */
String processor(const String& var)
{
	(void)var;
	return String();
}

/*
 *
 */
void setup_WebServer()
{
	// boolean fsOK = 
	gatewayFsBegin();// Filesystem mounten
	loadNodeNames();

	events.onConnect([](AsyncEventSourceClient *client) {
		client->send("connected !", NULL, millis(), 1000);
	});
	server.addHandler(&events);

	server.on("/favicon.ico", [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon_ico_gz, sizeof(favicon_ico_gz));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server.on("/mask-icon.svg", [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "image/svg", mask_icon_svg_gz, sizeof(mask_icon_svg_gz));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	// server.on("/red.svg", [](AsyncWebServerRequest *request) {
	// 	AsyncWebServerResponse *response = request->beginResponse_P(200, "image/svg", red_svg_gz, sizeof(red_svg_gz));
	// 	response->addHeader("Content-Encoding", "gzip");
	// 	request->send(response);
	// });
	// server.on("/green.svg", [](AsyncWebServerRequest *request) {
	// 	AsyncWebServerResponse *response = request->beginResponse_P(200, "image/svg", green_svg_gz, sizeof(green_svg_gz));
	// 	response->addHeader("Content-Encoding", "gzip");
	// 	request->send(response);
	// });
	// server.on("/yellow.svg", [](AsyncWebServerRequest *request) {
	// 	AsyncWebServerResponse *response = request->beginResponse_P(200, "image/svg", yellow_svg_gz, sizeof(yellow_svg_gz));
	// 	response->addHeader("Content-Encoding", "gzip");
	// 	request->send(response);
	// });

	server.on("/style.css", [](AsyncWebServerRequest *request){
		request->send_P(200, "text/css", style_css, nullptr);
	});

	server.on("/api/node-names", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(200, "application/json", nodeNamesAsJson());
	});

	server.on("/api/sensor-state", HTTP_GET, [](AsyncWebServerRequest *request){
		sendSensorStateJson(request);
	});

	server.on("/api/node-name", HTTP_POST, [](AsyncWebServerRequest *request){
		if (!request->hasParam("id", true) || !request->hasParam("name", true)) {
			request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing id or name\"}");
			return;
		}

		int id = request->getParam("id", true)->value().toInt();
		String name = trimCopy(request->getParam("name", true)->value());

		if (id < 0 || id > 255) {
			request->send(400, "application/json", "{\"ok\":false,\"error\":\"id out of range (0-255)\"}");
			return;
		}
		if (name.length() > 31) {
			name = name.substring(0, 31);
		}

		const int existing = findNodeNameIndexById(id);
		if (!name.length()) {
			if (existing >= 0) {
				nodeNames[existing].used = false;
				nodeNames[existing].name = "";
				nodeNames[existing].id = 0;
			}
		} else if (existing >= 0) {
			nodeNames[existing].name = name;
		} else {
			const int freeSlot = findFreeNodeNameSlot();
			if (freeSlot < 0) {
				request->send(400, "application/json", "{\"ok\":false,\"error\":\"node name slots full (max 32)\"}");
				return;
			}
			nodeNames[freeSlot].id = static_cast<uint8_t>(id);
			nodeNames[freeSlot].name = name;
			nodeNames[freeSlot].used = true;
		}

		bool ok = saveNodeNames();
		request->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"save failed\"}");
	});

	server.on("/api/web-settings", HTTP_GET, [](AsyncWebServerRequest *request){
		const unsigned long intervalSec = static_cast<unsigned long>(getUiUpdateIntervalMs() / 1000UL);
		const unsigned long otaWindowSec = static_cast<unsigned long>(getOtaWindowRemainingSec());
		const unsigned long bootlogMax = static_cast<unsigned long>(getBootLogMaxEntries());
		const unsigned long authFailBackoffSec = static_cast<unsigned long>(getWifiAuthFailBackoffMs() / 1000UL);
		const unsigned long authFailBackoffThreshold = static_cast<unsigned long>(getWifiAuthFailBackoffThreshold());
		char json[576] = {'\0'};
		snprintf(json, sizeof(json),
				 "{\"debugCompiled\":%s,\"debugEnabled\":%s,\"liveUiEnabled\":%s,\"telnetEnabled\":%s,"
				 "\"otaEnabled\":%s,\"otaWindowSec\":%lu,\"intervalSec\":%lu,"
				 "\"wifiBssidLocked\":%s,\"wifiBssid\":\"%s\",\"bootlogMax\":%lu,\"wifiAuthFailBackoffSec\":%lu,\"wifiAuthFailBackoffThreshold\":%lu}",
				 isWebDebugCompiled() ? "true" : "false",
				 isWebDebugEnabled() ? "true" : "false",
				 isWebLiveUiEnabled() ? "true" : "false",
				 isTelnetRuntimeEnabled() ? "true" : "false",
				 isOtaRuntimeEnabled() ? "true" : "false",
				 otaWindowSec,
				 intervalSec,
				 isWifiRepeaterBssidLocked() ? "true" : "false",
				 getWifiRepeaterBssid(),
				 bootlogMax,
				 authFailBackoffSec,
				 authFailBackoffThreshold);
		request->send(200, "application/json", json);
	});

	server.on("/api/web-settings", HTTP_POST, [](AsyncWebServerRequest *request){
		if (request->hasParam("debug", true)) {
			const String v = trimCopy(request->getParam("debug", true)->value());
			const bool enabled = (v == "1" || v == "true" || v == "on");
			setWebDebugEnabled(enabled);
		}
		if (request->hasParam("live", true)) {
			const String v = trimCopy(request->getParam("live", true)->value());
			const bool enabled = (v == "1" || v == "true" || v == "on");
			setWebLiveUiEnabled(enabled);
		}
		if (request->hasParam("telnet", true)) {
			const String v = trimCopy(request->getParam("telnet", true)->value());
			const bool enabled = (v == "1" || v == "true" || v == "on");
			setTelnetRuntimeEnabled(enabled);
		}
		if (request->hasParam("ota", true)) {
			const String v = trimCopy(request->getParam("ota", true)->value());
			const bool enabled = (v == "1" || v == "true" || v == "on");
			setOtaRuntimeEnabled(enabled);
		}
		if (request->hasParam("interval", true)) {
			const String v = trimCopy(request->getParam("interval", true)->value());
			const long sec = v.toInt();
			if (sec > 0) {
				setUiUpdateIntervalMs(static_cast<uint32_t>(sec) * 1000UL);
			}
		}
		if (request->hasParam("wifiBssid", true)) {
			const String v = trimCopy(request->getParam("wifiBssid", true)->value());
			if (!setWifiRepeaterBssid(v)) {
				request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid BSSID format, expected XX:XX:XX:XX:XX:XX\"}");
				return;
			}
		}
		if (request->hasParam("bootlogMax", true)) {
			const String v = trimCopy(request->getParam("bootlogMax", true)->value());
			const long n = v.toInt();
			if (n <= 0 || !setBootLogMaxEntries(static_cast<uint32_t>(n))) {
				request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid bootlogMax, allowed range 50..500\"}");
				return;
			}
		}
		if (request->hasParam("wifiAuthFailBackoffSec", true)) {
			const String v = trimCopy(request->getParam("wifiAuthFailBackoffSec", true)->value());
			const long sec = v.toInt();
			if (sec <= 0 || !setWifiAuthFailBackoffMs(static_cast<uint32_t>(sec) * 1000UL)) {
				request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid wifiAuthFailBackoffSec, allowed range 5..300\"}");
				return;
			}
		}
		if (request->hasParam("wifiAuthFailBackoffThreshold", true)) {
			const String v = trimCopy(request->getParam("wifiAuthFailBackoffThreshold", true)->value());
			const long threshold = v.toInt();
			if (threshold <= 0 || !setWifiAuthFailBackoffThreshold(static_cast<uint32_t>(threshold))) {
				request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid wifiAuthFailBackoffThreshold, allowed range 2..20\"}");
				return;
			}
		}
		const unsigned long intervalSec = static_cast<unsigned long>(getUiUpdateIntervalMs() / 1000UL);
		const unsigned long otaWindowSec = static_cast<unsigned long>(getOtaWindowRemainingSec());
		const unsigned long bootlogMax = static_cast<unsigned long>(getBootLogMaxEntries());
		const unsigned long authFailBackoffSec = static_cast<unsigned long>(getWifiAuthFailBackoffMs() / 1000UL);
		const unsigned long authFailBackoffThreshold = static_cast<unsigned long>(getWifiAuthFailBackoffThreshold());
		char json[608] = {'\0'};
		snprintf(json, sizeof(json),
				 "{\"ok\":true,\"debugCompiled\":%s,\"debugEnabled\":%s,\"liveUiEnabled\":%s,\"telnetEnabled\":%s,"
				 "\"otaEnabled\":%s,\"otaWindowSec\":%lu,\"intervalSec\":%lu,"
				 "\"wifiBssidLocked\":%s,\"wifiBssid\":\"%s\",\"bootlogMax\":%lu,\"wifiAuthFailBackoffSec\":%lu,\"wifiAuthFailBackoffThreshold\":%lu}",
				 isWebDebugCompiled() ? "true" : "false",
				 isWebDebugEnabled() ? "true" : "false",
				 isWebLiveUiEnabled() ? "true" : "false",
				 isTelnetRuntimeEnabled() ? "true" : "false",
				 isOtaRuntimeEnabled() ? "true" : "false",
				 otaWindowSec,
				 intervalSec,
				 isWifiRepeaterBssidLocked() ? "true" : "false",
				 getWifiRepeaterBssid(),
				 bootlogMax,
				 authFailBackoffSec,
				 authFailBackoffThreshold);
		request->send(200, "application/json", json);
	});

	server.on("/bootlog.txt", [](AsyncWebServerRequest *request){
		AsyncResponseStream *resp = request->beginResponseStream("text/plain");
		dbgprintln(ico_info, "start reading from bootlog.txt");
		boolean fsOk = gatewayFsBegin();
		if (fsOk) 
		{	
			dbgprintln(ico_info, "filesystem is open, start reading file ...");
			File file = GATEWAY_FS.open("/bootlog.txt", "r");
			while (file.available()) {
				resp->printf("%s\n", file.readStringUntil('\n').c_str());
			}			
			dbgprintln(ico_info, "closing file");
			file.close();			
		}
		else 
		{
			dbgprintln(ico_error, "error: reading from filesystem.");	
		}
		dbgprintln(ico_info, "reading from bootlog.txt - done.");
		request->send(resp);
	});

	server.on("/fs", HTTP_GET, [](AsyncWebServerRequest *request){
		if (!gatewayFsBegin()) {
			request->send(500, "application/json", "{\"ok\":false,\"error\":\"filesystem unavailable\"}");
			return;
		}

		if (!request->hasParam("path")) {
			AsyncResponseStream *resp = request->beginResponseStream("application/json");
			resp->print("{\"ok\":true,\"files\":[");
			appendFsListing(resp, true, 300);
			resp->print("]}");
			request->send(resp);
			return;
		}

		String path = trimCopy(request->getParam("path")->value());
		if (!isSafeFsPath(path)) {
			request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid path\"}");
			return;
		}
		if (!GATEWAY_FS.exists(path)) {
			request->send(404, "application/json", "{\"ok\":false,\"error\":\"file not found\"}");
			return;
		}

		request->send(GATEWAY_FS, path, guessContentType(path), false);
	});

	server.on("/wipe", [](AsyncWebServerRequest *request){
		dbgprintln(ico_info, "removing bootlog.txt");
		boolean fsOk = gatewayFsBegin();
		if (fsOk) 
		{	
			GATEWAY_FS.remove("/bootlog.txt");
		}
		else 
		{
			dbgprintln(ico_error, "error: reading from filesystem.");	
		}
		dbgprintln(ico_info, "done.");
		request->send_P(200, "text/html", index_html, processor);
	});


	server.on("/ui", HTTP_GET, [](AsyncWebServerRequest *request){
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, nullptr);
		response->addHeader("Server", getWifiHostname().c_str());
		request->send(response);
	});

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		const bool wifiConnected = (WiFi.status() == WL_CONNECTED);
		const int rssi = WiFi.RSSI();
		if (!wifiConnected || rssi < UI_RSSI_MIN_DBM) {
			request->redirect("/mini");
			return;
		}
		request->redirect("/ui");
	});	

	server.on("/stats", [](AsyncWebServerRequest *request) {
		send_Status(request);
	});

	server.on("/mini", HTTP_GET, [](AsyncWebServerRequest *request) {
		send_StatusMini(request);
	});

	server.on("/healthz", HTTP_GET, [](AsyncWebServerRequest *request) {
		const unsigned long uptimeSec = millis() / 1000UL;
		const bool wifiConnected = (WiFi.status() == WL_CONNECTED);
		const time_t nowTs = time(nullptr);
		const unsigned long nowEpoch = (nowTs > 0) ? static_cast<unsigned long>(nowTs) : 0UL;

		char json[768] = {'\0'};
		snprintf(json, sizeof(json),
				 "{\"ok\":true,\"host\":\"%s\",\"boot_count\":%lu,\"last_boot_epoch\":%lu,\"ntp_synced\":%s,"
				 "\"reset_reason_code\":%u,\"reset_reason\":\"%s\",\"reset_reason_raw\":\"%s\","
				 "\"planned_restart_marker\":\"%s\",\"uptime_s\":%lu,"
				 "\"wifi_connected\":%s,\"wifi_status\":%d,\"heap\":%lu,\"heap_frag\":%u,"
				 "\"max_free_block\":%lu,\"rssi\":%d,\"sse_clients\":%u,\"now_epoch\":%lu}",
				 getWifiHostname().c_str(),
				 static_cast<unsigned long>(gBootCount),
				 static_cast<unsigned long>(gLastBootEpoch),
				 gNtpSynced ? "true" : "false",
				 static_cast<unsigned>(gResetReasonCode),
				 gResetReasonText,
				 gResetReasonRaw,
				 gLastRestartMarker[0] ? gLastRestartMarker : "none",
				 uptimeSec,
				 wifiConnected ? "true" : "false",
				 static_cast<int>(WiFi.status()),
				 static_cast<unsigned long>(ESP.getFreeHeap()),
				 static_cast<unsigned>(getHeapFragmentationPct()),
				 static_cast<unsigned long>(getMaxFreeBlockBytes()),
				 WiFi.RSSI(),
				 static_cast<unsigned>(events.count()),
				 nowEpoch);

		request->send(200, "application/json", json);
	});

	server.on("/reconnect", HTTP_GET, [](AsyncWebServerRequest *request){
		// server.send(304, "message/http");
		WiFi.reconnect();
		delay(2000);
		request->send_P(200, "text/html", index_html, processor);
	});

	server.on("/sync-time", HTTP_GET, [](AsyncWebServerRequest *request){
		triggerNtpSync();
		dbgprintln(ico_info, "[NTP] manual time sync requested");
		request->send_P(200, "text/html", index_html, processor);
	});

  	// server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
	// 	request->send_P(200, "text/html", reboot_html, processor);
	// });
	server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {
		shouldReboot = !Update.hasError();
		AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
    	response->addHeader("Connection", "close");
    	request->send(response);
	});

	// When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
	server.onNotFound([](AsyncWebServerRequest *request) {
		dbgprintln(ico_error, "404: NOT FOUND");
		request->send(404);
	});

	server.begin();
	dbgprintln(ico_info, "webserver started...");
}

/*
 *
 */
void loop_WebServer()
{
	if(shouldReboot){
		dbgprintln(ico_info, "rebooting...");
		if (!writeRestartMarker("web-ui-reboot")) {
			dbgprintln(ico_warning, "could not persist web reboot marker");
		}
		delay(100);
		ESP.restart();
	}
}

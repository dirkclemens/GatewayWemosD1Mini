/*
 *  https://tttapa.github.io/ESP8266/Chap09%20-%20Web%20Server.html
 * 
 * 
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
#ifdef USE_ESP32
  #include <AsyncTCP.h>
#else
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <cstring>

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
static bool gNtpSynced = false;

#ifdef USE_ESP32
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
#endif

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

#ifdef USE_ESP32
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

static void appendJsonEscaped(String &out, const char *text)
{
	if (!text) {
		return;
	}
	const char *p = text;
	while (*p) {
		if (*p == '\\') {
			out += "\\\\";
		} else if (*p == '"') {
			out += "\\\"";
		} else {
			out += *p;
		}
		p++;
	}
}

static String sensorStateAsJson()
{
	bool hasNode[256] = {false};
	for (uint16_t i = 0; i < SENSOR_STATE_SLOTS; i++) {
		if (sensorStates[i].used) {
			hasNode[sensorStates[i].nodeId] = true;
		}
	}

	String out = "{";
	bool firstNode = true;
	for (uint16_t node = 0; node <= 255; node++) {
		if (!hasNode[node]) {
			continue;
		}
		if (!firstNode) {
			out += ",";
		}
		firstNode = false;
		out += "\"";
		out += node;
		out += "\":{";

		bool firstSensor = true;
		for (uint16_t i = 0; i < SENSOR_STATE_SLOTS; i++) {
			if (!sensorStates[i].used || sensorStates[i].nodeId != node) {
				continue;
			}
			if (!firstSensor) {
				out += ",";
			}
			firstSensor = false;
			out += "\"";
			out += sensorStates[i].sensorId;
			out += "\":{\"value\":\"";
			appendJsonEscaped(out, sensorStates[i].value);
			out += "\",\"time\":\"";
			appendJsonEscaped(out, sensorStates[i].time);
			out += "\",\"type\":\"";
			appendJsonEscaped(out, sensorStates[i].type);
			out += "\"}";
		}
		out += "}";
	}
	out += "}";
	return out;
}
#endif

static void appendFsListing(AsyncResponseStream *resp, bool asJson)
{
#ifdef USE_ESP32
	File root = GATEWAY_FS.open("/");
	if (!root || !root.isDirectory()) {
		return;
	}
	File file = root.openNextFile();
	bool first = true;
	while (file) {
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
		file = root.openNextFile();
	}
#else
	Dir dir = GATEWAY_FS.openDir("/");
	bool first = true;
	while (dir.next()) {
		if (asJson) {
			if (!first) {
				resp->print(",");
			}
			resp->printf("{\"path\":\"%s\",\"size\":%lu}",
						 dir.fileName().c_str(),
						 static_cast<unsigned long>(dir.fileSize()));
		} else {
			char szBuf[12];
			formatBytes(dir.fileSize(), szBuf, sizeof(szBuf));
			resp->printf("FS File: %s, size: %s\n", dir.fileName().c_str(), szBuf);
		}
		first = false;
	}
#endif
}

static void loadNodeNames()
{
	for (uint16_t i = 0; i < NODE_NAME_SLOTS; i++) {
		nodeNames[i].id = 0;
		nodeNames[i].name = "";
		nodeNames[i].used = false;
	}
	if (!gatewayFsBegin()) {
		dbgprintln(ico_error, "could not open LittleFS for node name loading");
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
		dbgprintln(ico_error, "could not open LittleFS for node name saving");
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

void updateSensorStateCache(uint8_t nodeId,
							uint8_t sensorId,
							const char *msgType,
							const char *payload,
							const char *timestamp,
							bool isSetMessage)
{
#ifdef USE_ESP32
	if (!isSetMessage) {
		return;
	}

	int idx = findSensorStateIndex(nodeId, sensorId);
	if (idx < 0) {
		idx = findFreeSensorStateSlot();
		if (idx < 0) {
			return;
		}
	}

	sensorStates[idx].used = true;
	sensorStates[idx].nodeId = nodeId;
	sensorStates[idx].sensorId = sensorId;
	strncpy(sensorStates[idx].value, payload ? payload : "", sizeof(sensorStates[idx].value) - 1);
	sensorStates[idx].value[sizeof(sensorStates[idx].value) - 1] = '\0';
	strncpy(sensorStates[idx].time, timestamp ? timestamp : "", sizeof(sensorStates[idx].time) - 1);
	sensorStates[idx].time[sizeof(sensorStates[idx].time) - 1] = '\0';
	strncpy(sensorStates[idx].type, msgType ? msgType : "", sizeof(sensorStates[idx].type) - 1);
	sensorStates[idx].type[sizeof(sensorStates[idx].type) - 1] = '\0';
#else
	(void)nodeId;
	(void)sensorId;
	(void)msgType;
	(void)payload;
	(void)timestamp;
	(void)isSetMessage;
#endif
}

void updateHealthSnapshot(uint32_t bootCount,
						  uint32_t lastBootEpoch,
						  uint8_t resetReasonCode,
						  bool ntpSynced)
{
	gBootCount = bootCount;
	gLastBootEpoch = lastBootEpoch;
	gResetReasonCode = resetReasonCode;
	gNtpSynced = ntpSynced;
}

String getNodeNameById(uint8_t nodeId)
{
	int idx = findNodeNameIndexById(nodeId);
	if (idx >= 0) {
		return nodeNames[idx].name;
	}
	return String("");
}


/* 
 *	https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/CheckFlashConfig/CheckFlashConfig.ino
 *	https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/TestEspApi/TestEspApi.ino
 */ 
const char * const FLASH_SIZE_MAP_NAMES[] = {
    "FLASH_SIZE_4M_MAP_256_256",  /**<  Flash size : 4Mbits. Map : 256KBytes + 256KBytes */
    "FLASH_SIZE_2M",                  /**<  Flash size : 2Mbits. Map : 256KBytes */
    "FLASH_SIZE_8M_MAP_512_512",      /**<  Flash size : 8Mbits. Map : 512KBytes + 512KBytes */
    "FLASH_SIZE_16M_MAP_512_512",     /**<  Flash size : 16Mbits. Map : 512KBytes + 512KBytes */
    "FLASH_SIZE_32M_MAP_512_512",     /**<  Flash size : 32Mbits. Map : 512KBytes + 512KBytes */
    "FLASH_SIZE_16M_MAP_1024_1024",   /**<  Flash size : 16Mbits. Map : 1024KBytes + 1024KBytes */
    "FLASH_SIZE_32M_MAP_1024_1024",    /**<  Flash size : 32Mbits. Map : 1024KBytes + 1024KBytes */
    "FLASH_SIZE_32M_MAP_2048_2048",    /**<  attention: don't support now ,just compatible for nodemcu;
                                           Flash size : 32Mbits. Map : 2048KBytes + 2048KBytes */
    "FLASH_SIZE_64M_MAP_1024_1024",     /**<  Flash size : 64Mbits. Map : 1024KBytes + 1024KBytes */
    "FLASH_SIZE_128M_MAP_1024_1024"     /**<  Flash size : 128Mbits. Map : 1024KBytes + 1024KBytes */
};
const char * const RST_REASONS[] = {
  "REASON_DEFAULT_RST",
  "REASON_WDT_RST",
  "REASON_EXCEPTION_RST",
  "REASON_SOFT_WDT_RST",
  "REASON_SOFT_RESTART",
  "REASON_DEEP_SLEEP_AWAKE",
  "REASON_EXT_SYS_RST"
};
// https://arduino-esp8266.readthedocs.io/en/latest/libraries.html#esp-specific-apis
void send_Status(AsyncWebServerRequest *request)
{
	uint32_t realSize = ESP.getFlashChipSize();
	uint32_t ideSize = ESP.getFlashChipSize();
	uint8_t heapFragmentation = getHeapFragmentationPct();
	uint32_t maxFreeBlocks = getMaxFreeBlockBytes();

	AsyncResponseStream *resp = request->beginResponseStream("text/plain");
	resp->print ("Status\n-----------------------------\n");
#ifdef USE_ESP32
	resp->printf("             Chip model: ESP32\n");
#else
	resp->printf("        ESP Chip id: 0x%8x\n", ESP.getChipId());
	resp->printf("      Flash real id: 0x%8x\n", ESP.getFlashChipId());
#endif
	resp->println();
	resp->printf("               Heap: %d bytes\n", ESP.getFreeHeap());
	resp->printf(" Heap Fragmentation: %d %%\n", heapFragmentation);
	resp->printf("Max Free Block Size: %d bytes\n", maxFreeBlocks);
	resp->printf("    Flash real size: %d bytes\n", realSize);
	resp->printf("    Flash ide  size: %d bytes\n", ideSize);
	resp->printf("          CPU speed: %d MHz\n",  ESP.getCpuFreqMHz());
	resp->printf("    Flash ide speed: %d MHz\n",  ESP.getFlashChipSpeed()/1000/1000);
#ifndef USE_ESP32
	FlashMode_t ideMode = ESP.getFlashChipMode();
	resp->print ("    Flash ide  mode: " + String(ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN") + "\n");
	resp->printf("              Flash: %s\n", FLASH_SIZE_MAP_NAMES[system_get_flash_size_map()]);
#endif
	resp->println();
#ifdef USE_ESP32
	resp->printf("       reset reason: %d\n", static_cast<int>(esp_reset_reason()));
#else
	resp->printf("       reset reason: %s\n", ESP.getResetReason().c_str());
#endif
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

	// https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#file-object
	boolean fsOk = gatewayFsBegin();
	if (fsOk) 
	{	
		resp->print ("Filesystem\n-----------------------------\n");
#ifdef USE_ESP32
		resp->printf("      FS totalBytes: %lu bytes\n", static_cast<unsigned long>(GATEWAY_FS.totalBytes()));
		resp->printf("      FS  usedBytes: %lu bytes\n", static_cast<unsigned long>(GATEWAY_FS.usedBytes()));
#else
		FSInfo fs_info;
		if (GATEWAY_FS.info(fs_info))
		{
			resp->printf("      FS totalBytes: %d bytes\n", fs_info.totalBytes);
			resp->printf("      FS  usedBytes: %d bytes\n", fs_info.usedBytes);
			resp->printf("      FS  blockSize: %d bytes\n", fs_info.blockSize);
			resp->printf("      FS   pageSize: %d bytes\n", fs_info.pageSize);
			resp->printf("       maxOpenFiles: %d \n", fs_info.maxOpenFiles);
			resp->printf("      maxPathLength: %d \n", fs_info.maxPathLength);
		}
#endif
		resp->println();

		resp->print ("Files\n-----------------------------\n");
		appendFsListing(resp, false);
		resp->println();

		resp->print ("Bootlog\n-----------------------------\n");
		File file = GATEWAY_FS.open("/bootlog.txt", "r");
		while (file.available()) {
			resp->printf("%s\n", file.readStringUntil('\n').c_str());
			// resp->printf("%s\n", file.readString().c_str());
		}
		// bootloglines += String(file.size()) + " Bytes";
		file.close();
	} 
	else
	{
		resp->printf("error opening LittleFS!\n");
		dbgprintln(ico_error, "error: reading from LittleFS.");	
	}
	// LittleFS.end();

	// resp->print ("config.json\n-----------------------------\n");
	// File configFile = LittleFS.open("/config.json", "r");
	// String data = configFile.readString();
	// configFile.close();
	// resp->printf("%s\n", data.c_str());

	request->send(resp);
}

/*
 *
 */
String processor(const String& var)
{
	// dbgprintln(ico_null, var.c_str());
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
#ifdef USE_ESP32
		request->send(200, "application/json", sensorStateAsJson());
#else
		request->send(200, "application/json", "{}");
#endif
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

	server.on("/bootlog.txt", [](AsyncWebServerRequest *request){
		AsyncResponseStream *resp = request->beginResponseStream("text/plain");
		dbgprintln(ico_info, "start reading from bootlog.txt");
		boolean fsOk = gatewayFsBegin();
		if (fsOk) 
		{	
			dbgprintln(ico_info, "LittleFS is open, start reading file ...");
			File file = GATEWAY_FS.open("/bootlog.txt", "r");
			while (file.available()) {
				resp->printf("%s\n", file.readStringUntil('\n').c_str());
			}			
			dbgprintln(ico_info, "closing file");
			file.close();			
		}
		else 
		{
			dbgprintln(ico_error, "error: reading from LittleFS.");	
		}
		// LittleFS.end();
		dbgprintln(ico_info, "reading from bootlog.txt - done.");
		request->send(resp);
	});

	server.on("/fs", HTTP_GET, [](AsyncWebServerRequest *request){
		if (!gatewayFsBegin()) {
			request->send(500, "application/json", "{\"ok\":false,\"error\":\"LittleFS unavailable\"}");
			return;
		}

		if (!request->hasParam("path")) {
			AsyncResponseStream *resp = request->beginResponseStream("application/json");
			resp->print("{\"ok\":true,\"files\":[");
			appendFsListing(resp, true);
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
			dbgprintln(ico_error, "error: reading from LittleFS.");	
		}

		// LittleFS.end();
		dbgprintln(ico_info, "done.");
		request->send_P(200, "text/html", index_html, processor);
	});


	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, nullptr);
		response->addHeader("Server", getWifiHostname().c_str());
		request->send(response);
		// request->send_P(200, "text/html", index_html, processor);
	});	

	server.on("/stats", [](AsyncWebServerRequest *request) {
		send_Status(request);
	});

	server.on("/healthz", HTTP_GET, [](AsyncWebServerRequest *request) {
		const unsigned long uptimeSec = millis() / 1000UL;
		const bool wifiConnected = (WiFi.status() == WL_CONNECTED);
		const time_t nowTs = time(nullptr);
		const unsigned long nowEpoch = (nowTs > 0) ? static_cast<unsigned long>(nowTs) : 0UL;
		const char *resetTxt = (gResetReasonCode < 7) ? RST_REASONS[gResetReasonCode] : "UNKNOWN";

		char json[320] = {'\0'};
		snprintf(json, sizeof(json),
				 "{\"ok\":true,\"host\":\"%s\",\"boot_count\":%lu,\"last_boot_epoch\":%lu,\"ntp_synced\":%s,"
				 "\"reset_reason_code\":%u,\"reset_reason\":\"%s\",\"uptime_s\":%lu,"
				 "\"wifi_connected\":%s,\"wifi_status\":%d,\"heap\":%lu,\"heap_frag\":%u,"
				 "\"max_free_block\":%lu,\"rssi\":%d,\"sse_clients\":%u,\"now_epoch\":%lu}",
				 getWifiHostname().c_str(),
				 static_cast<unsigned long>(gBootCount),
				 static_cast<unsigned long>(gLastBootEpoch),
				 gNtpSynced ? "true" : "false",
				 static_cast<unsigned>(gResetReasonCode),
				 resetTxt,
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
		delay(100);
		ESP.restart();
	}
}

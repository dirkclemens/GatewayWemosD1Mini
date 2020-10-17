/*
 *  https://tttapa.github.io/ESP8266/Chap09%20-%20Web%20Server.html
 * 
 * 
 */

#include "WebServer.h"
#include "config.h"
#include "common.h"
#include "uptime.h"

#include "index.h"
#include "style_css.h"
#include "reboot.h"

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);
AsyncEventSource events("/events");

//flag to use from web update to reboot the ESP
bool shouldReboot 							= false;

/*
 *
 */
void send_Event(const char *content, const char *section)
{
	events.send(content, section);
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
void send_Status(AsyncWebServerRequest *request)
{
	uint32_t realSize = ESP.getFlashChipRealSize();
	uint32_t ideSize = ESP.getFlashChipSize();
	FlashMode_t ideMode = ESP.getFlashChipMode();
	uint8_t heapFragmentation = ESP.getHeapFragmentation();
	uint8_t maxFreeBlocks = ESP.getMaxFreeBlockSize();

	AsyncResponseStream *resp = request->beginResponseStream("text/plain");
	resp->print ("Status\n-----------------------------\n");
	resp->printf("        ESP Chip id: 0x%8x\n", ESP.getChipId());
	resp->printf("      Flash real id: 0x%8x\n", ESP.getFlashChipId());
	resp->println();
	resp->printf("               Heap: %d bytes\n", ESP.getFreeHeap());
	resp->printf(" Heap Fragmentation: %d %%\n", heapFragmentation);
	resp->printf("Max Free Block Size: %d bytes\n", maxFreeBlocks);
	resp->printf("    Flash real size: %d bytes\n", realSize);
	resp->printf("    Flash ide  size: %d bytes\n", ideSize);
	resp->printf("          CPU speed: %d MHz\n",  ESP.getCpuFreqMHz());
	resp->printf("    Flash ide speed: %d MHz\n",  ESP.getFlashChipSpeed()/1000/1000);
	resp->print ("    Flash ide  mode: " + String(ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN") + "\n");
	resp->println();
	resp->printf("              Flash: %s\n", FLASH_SIZE_MAP_NAMES[system_get_flash_size_map()]);
	resp->println();

	const rst_info * resetInfo = system_get_rst_info();
  	resp->printf("       reset reason: %s\n", RST_REASONS[resetInfo->reason]);
	resp->println();

	resp->printf("          boot time: %s\n", getBootTime().c_str());
	resp->printf("           hostname: %s\n", WiFi.hostname().c_str());
	resp->printf("            runtime: %s\n", runtime());
	resp->printf("             uptime: %s\n", uptime());
	resp->printf("              build: %s %s\n", __DATE__, __TIME__);
	resp->printf("         sw version: %s\n", __SWVERSION__);
	resp->printf("           local ip: %s\n", WiFi.localIP().toString().c_str());
	resp->printf("               ssid: %s\n", WiFi.SSID().c_str());
	resp->printf("         gateway ip: %s\n", WiFi.gatewayIP().toString().c_str());
	resp->printf("        mac address: %s\n", WiFi.macAddress().c_str());
	resp->printf("               rssi: %s\n", String(WiFi.RSSI()).c_str());
	resp->println();

	// FSInfo fs_info;
	// if (SPIFFS.info(fs_info))
	// {
	// 	resp->printf("  FS totalBytes: %d bytes\n", fs_info.totalBytes);
	// 	resp->printf("  FS  usedBytes: %d bytes\n", fs_info.usedBytes);
	// 	resp->printf("  FS  blockSize: %d bytes\n", fs_info.blockSize);
	// 	resp->printf("  FS   pageSize: %d bytes\n", fs_info.pageSize);
	// 	resp->printf("   maxOpenFiles: %d \n", fs_info.maxOpenFiles);
	// 	resp->printf("  maxPathLength: %d \n", fs_info.maxPathLength);
	// 	resp->println();
	// }

	// resp->print ("Files\n-----------------------------\n");
	// Dir dir = SPIFFS.openDir("/");
	// while (dir.next()) {    
	// 	resp->printf("FS File: %s, size: %s\n",  dir.fileName().c_str(), formatBytes(dir.fileSize()).c_str());
	// }
	// resp->println();

	// resp->print ("config.json\n-----------------------------\n");
	// File configFile = SPIFFS.open("/config.json", "r");
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

	server.on("/style.css", [](AsyncWebServerRequest *request){
		request->send_P(200, "text/css", style_css, nullptr);
	});

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, nullptr);
		response->addHeader("Server", WiFi.hostname().c_str());
		request->send(response);
		// request->send_P(200, "text/html", index_html, processor);
	});	

	server.on("/stats", [](AsyncWebServerRequest *request) {
		send_Status(request);
	});

	server.on("/reconnect", HTTP_GET, [](AsyncWebServerRequest *request){
		// server.send(304, "message/http");
		WiFi.reconnect();
		delay(2000);
		request->send_P(200, "text/html", index_html, processor);
	});

  	server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/html", reboot_html, processor);
	});
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



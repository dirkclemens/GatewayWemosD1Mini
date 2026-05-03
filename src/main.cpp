/*
  	Wemos D1 mini, 4MB Chip
	PLATFORM: Espressif 8266 (2.6.2) > WeMos D1 R2 and mini
	HARDWARE: ESP8266 80MHz, 80KB RAM, 4MB Flash

	PLATFORM: Espressif 8266 (2.6.2) > WeMos D1 mini Pro
	HARDWARE: ESP8266 80MHz, 80KB RAM, 16MB Flash


  	Firmware auslesen: 
  	/opt/homebrew/bin/esptool.py -p /dev/cu.usbserial-1220 -b 115200 read_flash 0 0x400000 flash_contents.bin

	OTA Upload
	// python3 -m pip install --user espota
	// /Users/dirk/.platformio/packages/framework-arduinoespressif8266/tools/espota.py
	// /Users/dirk/Library/Arduino15/packages/esp8266/hardware/esp8266/3.1.2/tools/espota.py
	python3 espota.py -i 192.168.2.211 -p 8266 -f .pio/build/d1_mini_ota/firmware.bin


	bauen unbedingt mit 'platform = espressif8266@2.6.2'	

	LittleFS: 
	https://docs.platformio.org/en/latest/platforms/espressif8266.html#using-filesystem
	https://randomnerdtutorials.com/esp8266-nodemcu-vs-code-platformio-littlefs/
 
 */

extern "C" {
#include <user_interface.h>
}

#include "config.h"
#include "common.h"
#include "uptime.h"
#include "mysensors_types.h"
#include "diagnostics_ui.h"

// MQTT Gateway
// https://github.com/mysensors/MySensors/blob/master/examples/GatewayESP8266MQTTClient/GatewayESP8266MQTTClient.ino
// #define WITH_MQTT

// else IP-Gateway
// https://github.com/mysensors/MySensors/blob/master/examples/GatewayESP8266/GatewayESP8266.ino

//#define MAX_MESSAGE_SIZE  (32u) 
// dic: extended debugging
#define MY_DEBUG_VERBOSE

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
// #define RF24_PA_LEVEL RF24_PA_MAX

#include "./../../credentials.h"

//=====================================================================
#pragma region configuration

#ifdef WITH_MQTT
	#define MY_GATEWAY_MQTT_CLIENT
#else
	#define MY_GATEWAY_ESP8266
#endif


#ifdef WITH_MQTT
	#define MY_GATEWAY_MQTT_CLIENT
	// Set this node's subscribe and publish topic prefix
	#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mygateway1-out"
	#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mygateway1-in"
	// Set MQTT client id
	#define MY_MQTT_CLIENT_ID "mysensors-1"
	// Enable these if your MQTT broker requires usenrame/password
	// #define MY_MQTT_USER "***"
	// #define MY_MQTT_PASSWORD "***"
	// MQTT broker ip address or url. Define one or the other.
	//#define MY_CONTROLLER_URL_ADDRESS "m20.cloudmqtt.com"
	#define MY_CONTROLLER_IP_ADDRESS 192, 168, 2, 222
	// The MQTT broker port to to open
	#define MY_PORT 1883
#endif

// Enable UDP communication
// #define MY_USE_UDP

// Set the hostname for the WiFi Client. This is the hostname
// it will pass to the DHCP server if not static.
#define MY_HOSTNAME "GatewayWemosD1Mini"

// #define MY_ESP8266_HOSTNAME "MYSD1MiniGatewayOTA"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
// DiC: nicht definieren, sonst läuft FHEM nicht damit
// #define MY_IP_ADDRESS 192, 168, 2, 221

// If using static ip you need to define Gateway and Subnet address as well
// #define MY_IP_GATEWAY_ADDRESS 192,168,2,23    /// IMMER DIE IP ADRESSE DES CONTROLLERS: smarthome !!!!
// DiC: nicht definieren, sonst läuft FHEM nicht damit
// #define MY_IP_GATEWAY_ADDRESS 192, 168, 2, 222 // (geändert 09.07.2020)  /// IMMER DIE IP ADRESSE DES CONTROLLERS: smarthome !!!!
// #define MY_IP_SUBNET_ADDRESS 255, 255, 255, 0

// The port to keep open on node server mode
#ifndef WITH_MQTT
	#define MY_PORT 5003
#endif

// How many clients should be able to connect to this gateway (default 1)
// https://forum.mysensors.org/topic/2712/my_gateway_max_clients
#define MY_GATEWAY_MAX_CLIENTS 4 // allow HA + fallback client as before

// Controller ip address. Enables client mode (default is "server" mode).
// Also enable this if MY_USE_UDP is used and you want sensor data sent somewhere.
// #define MY_CONTROLLER_IP_ADDRESS 192, 168, 2, 222

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN   4 // D1 to GND alt: 3 // (GIPO2 D4)

// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Flash leds on rx/tx/err
// Led pins used if blinking feature is enabled above
// Wemos D1 Mini Pins !!!
// D4	 IO, 10k Pull-up, BUILTIN_LED	 GPIO2
#define MY_DEFAULT_ERR_LED_PIN D1
#define MY_DEFAULT_RX_LED_PIN D0
#define MY_DEFAULT_TX_LED_PIN D3
#define MY_WITH_LEDS_BLINKING_INVERSE	// bei externen LEDs

#define MY_INDICATION_HANDLER
#define MY_SPLASH_SCREEN_DISABLED

unsigned long gatewayTxMessage = 0;
unsigned long gatewayRxMessage = 0;
unsigned long sensorTxMessage = 0;
unsigned long sensorRxMessage = 0;
unsigned long indicatorTxErrors = 0;


// because SPIFFS is deprecated
#include <LittleFS.h>
//#define SPIFFS LittleFS
// #include "FS.h" 

// #ifdef MY_USE_UDP
// #include <WiFiUdp.h>
// #endif

#ifdef WWW
#include "WebServer.h"
#endif // WWW

#ifdef OTA
#include <ArduinoOTA.h> // neu seit 2.2
#endif

#ifdef PUSHOVER
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#endif // PUSHOVER
boolean alertSent = false;

////////////////////////////////////////////////////////////////////////
// Child declarations
// wichtig für fhem https://fhem.de/commandref.html#MYSENSORS_DEVICE
// https://tecdox.adcore.de/edit/wiki/iot/mysensors-sensoren
#define CHILD_ID_GENERAL 0
#define CHILD_ID_UPTIME 197

////////////////////////////////////////////////////////////////////////
// includes
#include <MySensors.h>

/** serial or telnet output **/
#ifdef TELNET
WiFiServer telnetServer(TELNET_PORT);
WiFiClient telnetClient;
#endif

// 	interval for updating events / web ui
static unsigned long interval = 1000; // every second
static unsigned long prev_time;

// 	interval for sending messages to server
static unsigned long gw_send_interval = 1000 * 60 * 15; // every 15 min
static unsigned long gw_send_prev_time;

unsigned long cpuLastMicros = 0; // cpu utilisation
unsigned long avgCpuDelta = 0;	 // cpu utilisation

//---------------------------------------------------------------------
#pragma endregion

// /////// /////////////////////////////////////////////////////////////////////
// Initialize message types
// https://www.mysensors.org/download/serial_api_20#variable-types
// https://github.com/mysensors/MySensors/blob/development/core/MyMessage.h
MyMessage msgGeneral(CHILD_ID_GENERAL, V_VAR1);
MyMessage msgUptime(CHILD_ID_UPTIME, V_TEXT);

// ARC (Automatic Retries Count) message
#define SENSOR_ID_ARC		98
#define V_TYPE_ARC			V_VAR5
MyMessage arcMessage(SENSOR_ID_ARC, V_TYPE_ARC);

//=====================================================================
#pragma region telnet functions
/* 
 * 	https://github.com/dragondaud/SolarGuardn/blob/master/SolarGuardn.ino
 */
template <typename T>
void telnetOut(const T x)
{
	Serial.print(x);
#ifdef TELNET
	if (telnetClient && telnetClient.connected())
	{
		telnetClient.print(x);
	}
#endif
	yield();
} // telnetOut()

/* 
 *
 */
void telnetOutLN()
{
	telnetOut(FPSTR(EOL));
}

/* 
 *
 */
template <typename T>
void telnetOutLN(const T x)
{
	telnetOut(x);
	telnetOut(FPSTR(EOL));
} // telnetOutLN()

#ifdef TELNET
/* 
 *
 */
void loop_Telnet(void)
{
	if (telnetServer.hasClient())
	{
		if (!telnetClient || !telnetClient.connected())
		{
			if (telnetClient)
			{
				telnetClient.stop();
			}
			telnetClient = telnetServer.available();

			char c = telnetClient.read();
			if (c == 'c')
			{
				telnetClient.println("bye bye.\r\n");
				telnetClient.flush();
				telnetClient.stop();
			}
			else
			{
				telnetClient.println(c);
			}

			telnetClient.flush();
			yield();
			telnetOut(F("telnet connected from "));
			telnetOutLN(telnetClient.remoteIP());
			telnetOut(F("type 'c' to stop.\r\n\r\n"));
		}
		else
		{
			telnetServer.available().stop();
		}
	}
} // handleTelnet()
#endif // TELNET

//---------------------------------------------------------------------
#pragma endregion

//=====================================================================
#pragma region stats functions

/// for counting indication() status notifications
struct RxTxStats_t {
	unsigned nRx, nTx, nGwRx, nGwTx, nErr;
} rxtxStats;

/// nMessagesRx[i] counts messages received from node id `i`
unsigned nMessagesRx[256];
/// nMessagesTx[i] counts messages sent to node id `i`
unsigned nMessagesTx[256];
/// nRetries[i] counts mretries required for messages sent to node id `i`
unsigned nRetries[256];

struct ArcStats_t {
    unsigned packets;   ///< number of packets sent
    unsigned retries;   ///< number of retries required
    unsigned success;   ///< success rate in percent
} arcStats;

// https://github.com/esp8266/Arduino/blob/master/tools/sdk/include/user_interface.h
const char* reset_reasons_esp8266[] = {
	"0: power on",
	"1: hardware watch dog",
	"2: exception reset",
	"3: software watch dog",
	"4: software restart",
	"5: wake up from deep-sleep",
	"6: external system reset"
};

const char* reset_reasons_esp32[] = {
	"0: none",
	"1: Vbat power on reset",
	"2: unknown",
	"3: Software reset digital core",
	"4: Legacy watch dog reset digital core",
	"5: Deep Sleep reset digital core",
	"6: Reset by SLC module, reset digital core",
	"7: Timer Group0 Watch dog reset digital core",
	"8: Timer Group1 Watch dog reset digital core",
	"9: RTC Watch dog Reset digital core",
	"10: Instrusion tested to reset CPU",
	"11: Time Group reset CPU",
	"12: Software reset CPU",
	"13: RTC Watch dog Reset CPU",
	"14: for APP CPU, reseted by PRO CPU",
	"15: Reset when the vdd voltage is not stable",
	"16: RTC Watch dog reset digital core and rtc module"
};
//---------------------------------------------------------------------
#pragma endregion

//=====================================================================
#pragma region ARC statistics
/**
 * @brief Collect statistics re Automatic Retries Count (ARC) for RF24.
 * Call this function immediately after each `send()` call.
 * 
 * @return int  number of retries required for most recent send
 * 
 */
int collectArcStatistics()
{
	int rssi = transportHALGetSendingRSSI();	// boils down to (-29 - (8 * (RF24_getObserveTX() & 0xF)))
	int arc = (-(rssi+29))/8;
	arcStats.packets++;         // # of packets sent
	arcStats.retries += arc;    // # of retries required
    arcStats.success = 
        arcStats.packets ? 
            (100uL * arcStats.packets) / (arcStats.packets + arcStats.retries) 
            : 100;
    return arc;
}

time_t t_last_clear = 0;

time_t getTimeNow()
{
#ifdef USE_NTP
    return ntpClient.getEpochTime();
#else
    return time(nullptr);
#endif
}

/**
 * @brief Reset all statistics counters to zero. Do this every hour or so
 * 
 */
void initStats()
{
	memset( nMessagesRx, 0, sizeof(nMessagesRx));
	memset( nMessagesTx, 0, sizeof(nMessagesTx));
	memset( nRetries, 0, sizeof(nRetries));
	memset( &rxtxStats, 0, sizeof(rxtxStats) );
    memset( &arcStats, 0, sizeof arcStats );
    t_last_clear = getTimeNow();
}


/**
 * @brief Send JSON-esque message with error statistics, then reset counters.
 * Error statistics include # of packets sent, # of retries required, success rate
 * Call this once per hour or so
 * 
 * @return const char* pointer to string sent to MySensors, like "{P:100;R:10;S:90}"
 *
 * Success rate:
 * 5 packets 0 retries = 100%
 * 5 packets 5 retries = 50%
 * 5 packets 20 retries = 20%
 */
const char* reportArcStatistics()
{
	//              				    1...5...10...15...20...25 max payload
	//				                    |   |    |    |    |    |
	static char payload[26];	//      {P:65535;R:65535;S:100}
	snprintf(payload, sizeof payload, "{P:%u,R:%u,S:%u}",
        arcStats.packets, arcStats.retries, arcStats.success );

    //memset( &arcStats, 0, sizeof arcStats );
	arcMessage.setSensor(SENSOR_ID_ARC).setType(V_TYPE_ARC);
	delay(10);
    send(arcMessage.set(payload));
	return payload;
}

/**
 * @brief Called immdiately after a message has been sent, so we can do ARC statistics
 * 
 * @param nextRecipient     the immediate destination node id, may be final destination or repeater
 * @param message           reference to the message being sent
 */
void aftertransportSend(const uint8_t nextRecipient, const MyMessage &message) 
{
    int arc = collectArcStatistics();
    nMessagesTx[ nextRecipient ]++;
    nRetries[ nextRecipient ] += arc;
}
//---------------------------------------------------------------------
#pragma endregion


//=====================================================================
#pragma region logBootTime

/*
 *
 */
void logBootTime()
{
	dbgprintln(ico_info, "Updating bootlog");
	boolean fsOk = LittleFS.begin();
	if (fsOk)
	{
		File file = LittleFS.open("/bootlog.txt", "a");
		if (!file || file.isDirectory())
		{
			dbgprintln(ico_error, "Error: Unable to open boot log in LittleFS");
		}
		else
		{
			char resetReason[48];
			if (resetInfo.reason < 7)
				strncpy(resetReason, reset_reasons_esp8266[resetInfo.reason], sizeof(resetReason) - 1);
			else
				snprintf(resetReason, sizeof(resetReason), "Unknown Reset reason: %u", resetInfo.reason);
			resetReason[sizeof(resetReason) - 1] = '\0';

			char buffer[64] = {'\0'};
			if (snprintf(buffer, sizeof(buffer), "%s - %s", getBootTime(), resetReason) < 0)
			{
				buffer[0] = '\0';
			}
			dbgprintf(ico_info, "adding to bootlog.txt: %s", buffer);
			file.println(buffer); // add entry to log file
			buffer[0] = '\0';
			file.close();
		}
		// LittleFS.end();
	}
	else
	{
		dbgprintln(ico_error, "error opening LittleFS!");
	}
}
//---------------------------------------------------------------------
#pragma endregion

//=====================================================================
#pragma region setup_OTA
/*
 *
 */
void setup_OTA()
{
#ifdef OTA
	ArduinoOTA.setHostname(MY_HOSTNAME);

	ArduinoOTA.onStart([]() {
		// Clean LittleFS
		// https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#end
		LittleFS.end();
		dbgprintln(ico_info, "Start");
		send_Event("[OTA] Start", "debug");
	});
	ArduinoOTA.onEnd([]() {
		dbgprintln(ico_info, "\nEnd");
		send_Event("[OTA] End", "debug");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		dbgprintf(ico_info, "[OTA] Progress: %d%%", progress / (total / 100));
		char buf[32] = {'\0'};
		if (snprintf(buf, sizeof(buf), "[OTA] Progress: %d%%", abs(int(progress / int(total / 100)))) < 0) // Means append did not (entirely) fit
		{
			buf[0] = '\0';
		}
		send_Event(buf, "debug");
	});
	ArduinoOTA.onError([](ota_error_t error) {
		dbgprintf(ico_info, "Error: %s", String(error).c_str());
		if (error == OTA_AUTH_ERROR)
		{
			dbgprintln(ico_info, "Auth Failed");
			send_Event("[OTA] Auth Failed", "debug");
		}
		else if (error == OTA_BEGIN_ERROR)
		{
			dbgprintln(ico_info, "Begin Failed");
			send_Event("[OTA] Auth Failed", "debug");
		}
		else if (error == OTA_CONNECT_ERROR)
		{
			dbgprintln(ico_info, "Connect Failed");
			send_Event("[OTA] Connect Failed", "debug");
		}
		else if (error == OTA_RECEIVE_ERROR)
		{
			dbgprintln(ico_info, "Receive Failed");
			send_Event("[OTA] Receive Failed", "debug");
		}
		else if (error == OTA_END_ERROR)
		{
			dbgprintln(ico_info, "End Failed");
			send_Event("[OTA] End Failed", "debug");
		}
	});
	ArduinoOTA.begin();
	dbgprintf(ico_info, "FOTA Initialized using IP address: %s", WiFi.localIP().toString().c_str());
#endif // OTA
}
//---------------------------------------------------------------------
#pragma endregion

///////////////////////////////////////////////////////
/* 
 	P_STRING				= 0,	//!< Payload type is string
	P_BYTE					= 1,	//!< Payload type is byte
	P_INT16					= 2,	//!< Payload type is INT16
	P_UINT16				= 3,	//!< Payload type is UINT16
	P_LONG32				= 4,	//!< Payload type is INT32
	P_ULONG32				= 5,	//!< Payload type is UINT32
	P_CUSTOM				= 6,	//!< Payload type is binary
	P_FLOAT32				= 7		//!< Payload type is float32

	http://www.cplusplus.com/reference/cstdio/printf/
 */
void getMessagePayload(char *payload, size_t payloadSize, MyMessage message)
{
	char _payload[2 * MAX_MESSAGE_SIZE -1 ] = {'\0'};
	const uint8_t setReqUnitsCount = sizeof(mysSetReqUnits) / sizeof(mysSetReqUnits[0]);
	const char *unit = (message.type < setReqUnitsCount) ? mysSetReqUnits[message.type] : "";
	// mysensors_payload_t plt = ;
	// DEBUG_PRINTF("getMessagePayload: %d - %d\n", message.getPayloadType(), message.type);
	switch (message.getPayloadType())
	{
	case 0:
		snprintf(payload, payloadSize, "%s", message.getString() ? message.getString() : "");
		break;
	case 1:
		if (snprintf(_payload, sizeof(_payload), "%d %s", message.getByte(), unit) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		snprintf(payload, payloadSize, "%s", _payload);
		break;
	case 2:
		if (snprintf(_payload, sizeof(_payload), "%d %s", message.getInt(), unit) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		snprintf(payload, payloadSize, "%s", _payload);
		break;
	case 3:
		if (snprintf(_payload, sizeof(_payload), "%u %s", message.getUInt(), unit) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		snprintf(payload, payloadSize, "%s", _payload);
		break;
	case 4:
		if (snprintf(_payload, sizeof(_payload), "%ld %s", (long int)message.getLong(), unit) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		snprintf(payload, payloadSize, "%s", _payload);
		break;
	case 5:
		if (snprintf(_payload, sizeof(_payload), "%lu %s", (long unsigned int)message.getULong(), unit) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		snprintf(payload, payloadSize, "%s", _payload);
		break;
	case 6:
		// if (snprintf(_payload, sizeof(_payload), "%p %s", (void *)message.getCustom(), (char *)mysSetReqUnits[message.type]) < 0) // Means append did not (entirely) fit
		if (message.getCommand() == C_STREAM) {
			message.getStream(payload);
		}
		else 
		{
			if (snprintf(_payload, sizeof(_payload), "%p (%u)", (void *)message.getCustom(), message.getLength()) < 0) // Means append did not (entirely) fit
			{
				_payload[0] = '\0';
			}
			snprintf(payload, payloadSize, "%s", _payload);
		}
		break;
	case 7:
		if (snprintf(_payload, sizeof(_payload), "%0.2f %s", message.getFloat(), unit) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		snprintf(payload, payloadSize, "%s", _payload);
		break;
	default:
		if (snprintf(_payload, sizeof(_payload), "error: payload.type: %d - message.type: %d", message.getPayloadType(), message.type) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		snprintf(payload, payloadSize, "%s", _payload);
		break;
	}
	_payload[0] = {'\0'};
	dbgprintf(ico_info, "getMessagePayload %s\n", payload);
}

#include "gw_clients.h"

void updateWebStats()
{
	char timestamp[21];
	getCurrentTimeString(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S");

	char resetReason[48];
	if (resetInfo.reason < 7)
		strncpy(resetReason, reset_reasons_esp8266[resetInfo.reason], sizeof(resetReason) - 1);
	else
		snprintf(resetReason, sizeof(resetReason), "Unbekannter Reset-Grund: %u", resetInfo.reason);
	resetReason[sizeof(resetReason) - 1] = '\0';

	char heapBuf[12], flashBuf[12], sketchBuf[12], freeSketchBuf[12];
	formatBytes(ESP.getFreeHeap(),         heapBuf,       sizeof(heapBuf));
	formatBytes(ESP.getFlashChipRealSize(), flashBuf,     sizeof(flashBuf));
	formatBytes(ESP.getSketchSize(),        sketchBuf,    sizeof(sketchBuf));
	formatBytes(ESP.getFreeSketchSpace(),   freeSketchBuf, sizeof(freeSketchBuf));

	// https://arduino-esp8266.readthedocs.io/en/latest/libraries.html#esp-specific-apis
	static char page[1024];
	page[0] = '\0';
	char *p = page;
	size_t rem = sizeof(page);
	int n;
#define WS_APPEND(...) do { \
		if (rem > 1) { \
			n = snprintf(p, rem, __VA_ARGS__); \
			if (n < 0) { \
				p[0] = '\0'; \
				rem = 0; \
			} else if ((size_t)n >= rem) { \
				p += rem - 1; \
				rem = 1; \
			} else { \
				p += n; \
				rem -= n; \
			} \
		} \
	} while(0)

	WS_APPEND("<table><thead><tr><th>&nbsp;</th><th>&nbsp;</th></tr></thead>");
	WS_APPEND("<tr><td>gateway started at</td><td>%s</td></tr>", getBootTime());
	WS_APPEND("<tr><td>hostname</td><td>%s</td></tr>",           WiFi.hostname().c_str());
	WS_APPEND("<tr><td>current time</td><td>%s</td></tr>",       timestamp);
	WS_APPEND("<tr><td>runtime</td><td>%s</td></tr>",            runtime());
	WS_APPEND("<tr><td>build</td><td>%s %s</td></tr>",           __DATE__, __TIME__);
	WS_APPEND("<tr><td>sw version</td><td>%s</td></tr>",         __SWVERSION__);
	WS_APPEND("<tr><td>free heap</td><td>%s</td></tr>",          heapBuf);
	WS_APPEND("<tr><td>flash space</td><td>%s</td></tr>",        flashBuf);
	WS_APPEND("<tr><td>used sketch space</td><td>%s</td></tr>",  sketchBuf);
	WS_APPEND("<tr><td>free sketch space</td><td>%s</td></tr>",  freeSketchBuf);
	WS_APPEND("<tr><td>rssi</td><td>%ddB</td></tr>",             WiFi.RSSI());
	WS_APPEND("<tr><td>local ip</td><td>%s</td></tr>",           WiFi.localIP().toString().c_str());
	WS_APPEND("<tr><td>ssid</td><td>%s</td></tr>",               WiFi.SSID().c_str());
	WS_APPEND("<tr><td>mac address</td><td>%s</td></tr>",        WiFi.macAddress().c_str());
#ifdef MY_CORE_ONLY
	WS_APPEND("<tr><td>MY_CORE_ONLY</td><td>TRUE</td></tr>");
#endif
	WS_APPEND("<tr><td><a href=\"/bootlog.txt\">reset reason</a></td><td>%s</td></tr>", resetReason);
	WS_APPEND("</table>");
#undef WS_APPEND

	send_Event(page, "info");
}


//=====================================================================
#pragma region MySensors receive
/* 
 *
 */
void receive(const MyMessage &message)
{
	char logbuf[128] = {'\0'};
	char webjson[256] = {'\0'};
	char timestamp[21] = {'\0'};
	char payload[2 * MAX_MESSAGE_SIZE -1] = {'\0'}; // 2 * (32u) - 1 = 63 ?
	char msgtype[32] = {'\0'};
	char cmdtype[16] = {'\0'};
	char ack = '0';

	getCurrentTimeString(timestamp, sizeof(timestamp), "%H:%M:%S");

	if (message.isEcho())
	{
		ack = '1';
	}

	const uint8_t commandCodesCount = sizeof(mysCommandCodes) / sizeof(mysCommandCodes[0]);
	const uint8_t commandIndex = (message.getCommand() < commandCodesCount) ? message.getCommand() : (commandCodesCount - 1);
	const uint8_t presentationCodesCount = sizeof(mysPresenationCodes) / sizeof(mysPresenationCodes[0]);
	const uint8_t internalCodesCount = sizeof(mysInternalCodes) / sizeof(mysInternalCodes[0]);
	const uint8_t setReqCodesCount = sizeof(mysSetReqCodes) / sizeof(mysSetReqCodes[0]);
	const uint8_t streamCodesCount = sizeof(mysStreamCodes) / sizeof(mysStreamCodes[0]);
	snprintf(cmdtype, sizeof(cmdtype), "%s", mysCommandCodes[commandIndex]);

	switch (message.getCommand())
	{
	case C_PRESENTATION:
		if (message.type < presentationCodesCount)
		{
			snprintf(msgtype, sizeof(msgtype), "%s", mysPresenationCodes[message.type]);
		}
		else
		{
			snprintf(msgtype, sizeof(msgtype), "type: %u", message.type);
		}
		// if (message.sensor == 255)
		// {
		getMessagePayload(payload, sizeof(payload), message);
		// }
		break;

	case C_INTERNAL:
		if (message.type < internalCodesCount)
		{
			snprintf(msgtype, sizeof(msgtype), "%s", mysInternalCodes[message.type]);
		}
		else
		{
			snprintf(msgtype, sizeof(msgtype), "type: %u", message.type);
		}
		getMessagePayload(payload, sizeof(payload), message);
		break;

	case C_SET:
		if (message.type < setReqCodesCount)
		{
			snprintf(msgtype, sizeof(msgtype), "%s", mysSetReqCodes[message.type]);
		}
		else
		{
			snprintf(msgtype, sizeof(msgtype), "type: %u", message.type);
		}
		getMessagePayload(payload, sizeof(payload), message);
		break;

	case C_REQ:
		// not used normally
		snprintf(msgtype, sizeof(msgtype), "type: %u", message.type);
		getMessagePayload(payload, sizeof(payload), message);
		break;

	case C_STREAM:
		if (message.type < streamCodesCount)
		{
			snprintf(msgtype, sizeof(msgtype), "%s", mysStreamCodes[message.type]);
		}
		else
		{
			snprintf(msgtype, sizeof(msgtype), "type: %u", message.type);
		}
		getMessagePayload(payload, sizeof(payload), message);
		break;

	default:
		snprintf(msgtype, sizeof(msgtype), "type: %u", message.type);
		getMessagePayload(payload, sizeof(payload), message);
		break;
	}

#ifdef TELNET
	if (snprintf(logbuf, sizeof(logbuf), "%s - %-15s - node [%3d] child [%03d] [%c] %-26s %-32s",
				 timestamp,		 // timestamp
				 cmdtype,		 //
				 message.sender, // node id
				 message.sensor, // sensor id
				 ack,			 // char
				 msgtype,		 //
				 payload) < 0)	 // Means append did not (entirely) fit
	{
		logbuf[0] = '\0';
	}
	telnetOutLN(logbuf);
	logbuf[0] = {'\0'};
#endif // TELNET

#ifdef WWW
	if (snprintf(webjson, sizeof(webjson), "{\"time\":\"%s\",\"cmd\":\"%s\",\"sender\":\"%d\",\"sensor\":\"%d\",\"ack\":\"%c\",\"msgtype\":\"%s\",\"payload\":\"%s\"}",
				 timestamp,
				 cmdtype,
				 message.sender,
				 message.sensor,
				 ack,
				 msgtype,
				 payload) < 0)
	{
		webjson[0] = '\0';
	}
	send_Event(webjson, "messagesjson");
	webjson[0] = {'\0'};
#endif
}

//=====================================================================
#pragma endregion

/*
 *
 */
int counter = 0;
static unsigned long wifiReconnectFirstFailMs = 0;
static unsigned long wifiLastBeginMs = 0;
static wl_status_t wifiLastStatus = WL_IDLE_STATUS;
static unsigned long lastIndicatorMs = 0;
static int lastIndicatorCode = -1;

WiFiEventHandler wifiDisconnectHandler;
WiFiEventHandler wifiGotIpHandler;

static const unsigned long WIFI_BEGIN_INTERVAL_MS = 15000UL;          // retry WiFi.begin every 15s
static const unsigned long WIFI_RECONNECT_TIMEOUT_MS = 1000UL * 60UL * 3UL; // restart only after 3 minutes

const char *wifiStatusToString(wl_status_t status)
{
	switch (status)
	{
	case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
	case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
	case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
	case WL_CONNECTED: return "WL_CONNECTED";
	case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
	case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
	case WL_DISCONNECTED: return "WL_DISCONNECTED";
	default: return "WL_UNKNOWN";
	}
}

const char *wifiDisconnectReasonToString(uint8_t reason)
{
	switch (reason)
	{
	case 1: return "UNSPECIFIED";
	case 2: return "AUTH_EXPIRE";
	case 3: return "AUTH_LEAVE";
	case 4: return "ASSOC_EXPIRE";
	case 5: return "ASSOC_TOOMANY";
	case 6: return "NOT_AUTHED";
	case 7: return "NOT_ASSOCED";
	case 8: return "ASSOC_LEAVE";
	case 9: return "ASSOC_NOT_AUTHED";
	case 10: return "DISASSOC_PWRCAP_BAD";
	case 11: return "DISASSOC_SUPCHAN_BAD";
	case 13: return "IE_INVALID";
	case 14: return "MIC_FAILURE";
	case 15: return "4WAY_HANDSHAKE_TIMEOUT";
	case 16: return "GROUP_KEY_UPDATE_TIMEOUT";
	case 17: return "IE_IN_4WAY_DIFFERS";
	case 18: return "GROUP_CIPHER_INVALID";
	case 19: return "PAIRWISE_CIPHER_INVALID";
	case 20: return "AKMP_INVALID";
	case 21: return "UNSUPP_RSN_IE_VERSION";
	case 22: return "INVALID_RSN_IE_CAP";
	case 23: return "802_1X_AUTH_FAILED";
	case 24: return "CIPHER_SUITE_REJECTED";
	case 200: return "BEACON_TIMEOUT";
	case 201: return "NO_AP_FOUND";
	case 202: return "AUTH_FAIL";
	case 203: return "ASSOC_FAIL";
	case 204: return "HANDSHAKE_TIMEOUT";
	default: return "UNKNOWN";
	}
}

void logWifiStatus(const char *reason)
{
	wl_status_t status = WiFi.status();
	dbgprintf(ico_warning,
			  "[WiFi] %s | status=%s (%d) | RSSI=%d dBm | SSID=%s | IP=%s",
			  reason,
			  wifiStatusToString(status),
			  static_cast<int>(status),
			  WiFi.RSSI(),
			  WiFi.SSID().c_str(),
			  WiFi.localIP().toString().c_str());
}

void setupWifiEventLogging()
{
	wifiDisconnectHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &event) {
		dbgprintf(ico_error,
				  "[WiFiEvt] disconnected | reason=%u (%s) | ssid=%s | status=%s (%d) | RSSI=%d",
				  event.reason,
				  wifiDisconnectReasonToString(event.reason),
				  event.ssid.c_str(),
				  wifiStatusToString(WiFi.status()),
				  static_cast<int>(WiFi.status()),
				  WiFi.RSSI());
	});

	wifiGotIpHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &event) {
		dbgprintf(ico_ok,
				  "[WiFiEvt] got-ip | ip=%s | gw=%s | mask=%s | status=%s (%d) | RSSI=%d",
				  event.ip.toString().c_str(),
				  event.gw.toString().c_str(),
				  event.mask.toString().c_str(),
				  wifiStatusToString(WiFi.status()),
				  static_cast<int>(WiFi.status()),
				  WiFi.RSSI());
	});
}

void loop_Wifi()
{
	const unsigned long now = millis();
	const wl_status_t status = WiFi.status();

	// connected again: clear reconnect state
	if (status == WL_CONNECTED)
	{
		if (wifiReconnectFirstFailMs != 0)
		{
			logWifiStatus("reconnected");
		}
		wifiReconnectFirstFailMs = 0;
		wifiLastBeginMs = 0;
		counter = 0;
		wifiLastStatus = status;
		return;
	}

	if (wifiReconnectFirstFailMs == 0)
	{
		wifiReconnectFirstFailMs = now;
		logWifiStatus("connection lost - starting reconnect sequence");
	}

	// log status transitions for diagnostics
	if (status != wifiLastStatus)
	{
		logWifiStatus("status changed");
		wifiLastStatus = status;
	}

	// avoid WiFi.begin() spam; retry periodically
	if (wifiLastBeginMs == 0 || (now - wifiLastBeginMs) >= WIFI_BEGIN_INTERVAL_MS)
	{
		dbgprintln(ico_info, "[WiFi] calling WiFi.begin()");
#ifdef MY_CORE_ONLY
		WiFi.begin(MY_WIFI_SSID, MY_WIFI_PASSWORD);
#else
		WiFi.begin();
#endif
		wifiLastBeginMs = now;
		counter++;
	}

	yield();
	delay(10);

	// very last resort, with long timeout
	if ((now - wifiReconnectFirstFailMs) >= WIFI_RECONNECT_TIMEOUT_MS)
	{
		logWifiStatus("reconnect timeout reached - restarting");
		ESP.restart();
	}
}

/* 
 *
 */
unsigned long getCpuDelta()
{
	unsigned long thisMicros, delta;
	thisMicros = micros();
	delta = thisMicros - cpuLastMicros;
	cpuLastMicros = thisMicros;
	// dbgprintf(ico_info, "cpu load: %lu \u03BCs", delta);
	return delta;
}

#ifdef MY_CORE_ONLY
void setup_wifi()
{
	// scrollMessage("wifi setup");
	WiFi.setPhyMode(WIFI_PHY_MODE_11N); // Force 802.11N connection

	delay(10);
	// We start by connecting to a WiFi network
	dbgprintln();
	dbgprintf(ico_ok, "Connecting to %s", MY_WIFI_SSID);

	WiFi.mode(WIFI_STA);
	WiFi.begin(MY_WIFI_SSID, MY_WIFI_PASSWORD);

	int counter = 0;
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		if (++counter > 100)
			ESP.restart();
		Serial.print(".");
	}

	randomSeed(micros());

	dbgprintln();
	dbgprintf(ico_ok, "WiFi connected with IP address: %s", (WiFi.localIP().toString()).c_str());
}
#endif

//=====================================================================
#pragma region pushover function
/* 
 *	send alerts via Pushover.net API (registration necessary)
 */
void pushover(const char *message, const char *title = "MySensors Gateway")
{
	WiFiClientSecure client;
	HTTPClient http;
	client.setInsecure();
	Serial.println("Pushover: connecting ...");
	if (!client.connect("api.pushover.net", 443))
	{
		char err_buf[100];
		if (client.getLastSSLError(err_buf, 100) < 0)
		{
			Serial.printf_P(PSTR("Pushover: connection failed: %s\n"), err_buf);
		}
		else
		{
			Serial.printf_P(PSTR("Pushover: connection failed. Could not connect to api.pushover.net:443.\n"));
		}
	}

	// +"&device="+_device
	// +"&url="+_url+"&url_title="+_urltitle
	// +&retry="+_retry
	// +"&expire="+_expire
	// +"&sound="+_sound
	// +"&timestamp=")+_timestamp
	// +"&html=1"

	int _priority = 0;
	char pushmessage[256] = {'\0'};
	if (snprintf(pushmessage, sizeof(pushmessage),
				 "token=%s&user=%s&title=%s&message=%s&priority=%d",
				 _token,
				 _user,
				 title,
				 message,
				 _priority) < 0) // Means append did not (entirely) fit
	{
		pushmessage[0] = '\0';
	}
	else
	{
		http.begin(client, "https://api.pushover.net/1/messages.json");
		int httpCode = http.POST((uint8_t *)pushmessage, sizeof(pushmessage));
		dbgprintf(httpCode < 0 ? ico_error : ico_ok, "Pushover: posting result: ", httpCode);
	}
	pushmessage[0] = '\0';
}
//---------------------------------------------------------------------
#pragma endregion

//=====================================================================
#pragma region MySensors before
//////////////////////////////////////////////////////////////////////////////////////////////////
//
void before()
{
}
//---------------------------------------------------------------------
#pragma endregion

//=====================================================================
#pragma region MySensors presentation
//////////////////////////////////////////////////////////////////////////////////////////////////
//
void presentation()
{
	sendSketchInfo(MY_HOSTNAME, __DATE__ " " __TIME__);
	wait(200);
	present(CHILD_ID_UPTIME, S_INFO, "uptime");
	present(SENSOR_ID_ARC, S_CUSTOM, F("ARC stats (JSON)") );
}

void loop_NTP()
{
	struct tm tm;
	static time_t lastsec = 0;
	time_t now = time(&now);
	localtime_r(&now, &tm);

	if (tm.tm_sec != lastsec)
	{
		// einmal am Tag die Zeit vom NTP Server holen o. jede Stunde "% 3600" aller zwei "% 7200"
		if (!(time(&now) % 86400))
		{
			configTime(TZ_INFO, NTP_SERVER[0], NTP_SERVER[1], NTP_SERVER[2]); // check TZ.h, find your location
		}
	}
}
//---------------------------------------------------------------------
#pragma endregion

/**
 * @brief React to various events reported by MySensors
 * (Standard MySensors function to be implemented in application)
 * 
 * @param ind 
 */
//////////////////////////////////////////////////////////////////////////////////////////////////
//	#define MY_INDICATION_HANDLER is needed
//  - Grün: &#128994; (🟢)
//  - Rot: &#128308; (🔴)
//  - Gelb: &#128993; (🟡)

void indication(const indication_t indicator)
{
	lastIndicatorCode = static_cast<int>(indicator);
	lastIndicatorMs = millis();

	switch (indicator)
	{
	case INDICATION_GW_TX:
		gatewayTxMessage++;
		rxtxStats.nGwTx++;
		send_Event("&#128994;", "led"); // 🟢
		break;

	case INDICATION_GW_RX:
		gatewayRxMessage++;
		rxtxStats.nGwRx++; 
		send_Event("&#128308;", "led"); // 🔴
		break;

	case INDICATION_TX:
		sensorTxMessage++;
		rxtxStats.nTx++; 
		send_Event("&#128994;", "led"); // 🟢
		break;

	case INDICATION_RX:
		sensorRxMessage++;
		rxtxStats.nRx++; 
		send_Event("&#128308;", "led"); // 🔴
		break;

	case INDICATION_ERR_TX:
		rxtxStats.nErr++;
		send_Event("&#128308;", "led"); // 🔴
		break;

	default:
		send_Event("&#128993;", "led"); // 🟡
		break;
	};

	const char *mysIndication = "";
	if (indicator <= 18)
	{
		mysIndication = mysIndicationErrorCodes0[indicator];
	}
	else if (indicator >= 101 && indicator <= 116)
	{
		mysIndication = mysIndicationErrorCodes100[indicator - 101];
		indicatorTxErrors++;
	}
	else
	{
		mysIndication = "Unknown indication";
	}

	char msgbuf[192] = {'\0'};
	if (snprintf(msgbuf, sizeof(msgbuf),
				 "%s | gateway: rx: %lu - tx: %lu  | sensors: rx: %lu - tx: %lu  | err: %lu <br />",
				 mysIndication,
				 gatewayRxMessage,
				 gatewayTxMessage,
				 sensorRxMessage,
				 sensorTxMessage,
				 indicatorTxErrors) < 0)
	{
		strcpy(msgbuf, "<div class=\"error\">error</div>");
	}
	send_Event(msgbuf, "indicator");
	// dbgprintln(ico_info, msgbuf);

	dbgprintf(ico_info,
			  "[INDICATION] code=%d | text=%s | gw(rx=%lu tx=%lu) sensor(rx=%lu tx=%lu) errors=%lu | wifi=%s (%d)",
			  static_cast<int>(indicator),
			  mysIndication,
			  gatewayRxMessage,
			  gatewayTxMessage,
			  sensorRxMessage,
			  sensorTxMessage,
			  indicatorTxErrors,
			  wifiStatusToString(WiFi.status()),
			  static_cast<int>(WiFi.status()));
}


//=====================================================================
#pragma region MySensors setup
//////////////////////////////////////////////////////////////////////////////////////////////////
//
void setup()
{
	// esp8266 reset reasons
  	rst_info *resetInfo;
  	resetInfo = ESP.getResetInfoPtr();
	// Serial.println(resetInfo->reason);

	// esp32 reset reasons
	// int rtc_reset_reason = rtc_get_reset_reason(0);    

#ifdef MY_DEBUG
	Serial.begin(9600);
	while (!Serial)
	{
	} // Wait

	Serial.println("---------- begin setup()");
	Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
	Serial.printf("SW version: %s\n", __SWVERSION__);
	
	// esp8266
	Serial.printf("Reset reason code: %d\n", resetInfo->reason);
	Serial.printf("Reset reason text: %s\n", ESP.getResetReason().c_str());
	Serial.printf("Reset info: %s\n", ESP.getResetInfo().c_str());
#endif

	cpuLastMicros = micros();

#ifdef MY_CORE_ONLY
	setup_wifi();
#endif

#ifdef TELNET
	telnetServer.begin();
	//telnetServer.setNoDelay(true); // drops chars if set true
	telnetServer.printf("Reset reason %s\n", reset_reasons_esp8266[resetInfo->reason]);
#endif // TELNET

#ifdef NTP
	configTime(TZ_INFO, NTP_SERVER[0], NTP_SERVER[1], NTP_SERVER[2]); // check TZ.h, find your location
#endif																  // NTP

	setupWifiEventLogging();

#ifdef WWW
	setup_WebServer();
#endif // WWW

#ifdef OTA
	setup_OTA();
#endif // OTA

	if (WiFi.status() == WL_CONNECTED)
	{
		// pushover("Gateway successfully started");
	}
	
	// boolean fsOK = 
	LittleFS.begin();// Filesystem mounten

	// initialize statistics
    initStats();
	
	// call not before time was set to local time
	setBootTime();
	// and write it to LittleFS
	logBootTime();
}
//---------------------------------------------------------------------
#pragma endregion

/*
 *
 */
int day = -1;
boolean checkMidnight()
{
	struct tm tm;
	time_t now = time(&now);
	localtime_r(&now, &tm);
	char stoday[3] = "\0";
	strftime(stoday, sizeof(stoday), "%d", &tm); // http://www.cplusplus.com/reference/ctime/strftime/
	int today = atoi(stoday);
	boolean result = false;
	if (day == -1)
		day = today; // init values
	if (today != day)
	{
		day = today; // set day to new day
		result = true;
	}
	return result;
}

//=====================================================================
#pragma region MySensors loop
//////////////////////////////////////////////////////////////////////////////////////////////////
//
void loop()
{
	avgCpuDelta = getCpuDelta();

	delay(10); // https://github.com/espressif/esp-idf/issues/1021
	yield();

	if (checkMidnight())
	{
		// reset all counter on midnight
		gatewayTxMessage = 0;
		gatewayRxMessage = 0;
		sensorTxMessage = 0;
		sensorRxMessage = 0;
		indicatorTxErrors = 0;
	}

	if (WiFi.status() != WL_CONNECTED)
	{
		loop_Wifi();
	}

#ifdef WWW
	loop_WebServer();
#endif // WWW

#ifdef TELNET
	loop_Telnet(); // handle telnet server
#endif

#ifdef NTP
	loop_NTP();
#endif // NTP

#ifdef OTA
	ArduinoOTA.handle(); // neu seit 2.2
#endif					 // OTA

	// interval based jobs
	if (millis() - prev_time > interval)
	{
		prev_time = millis();
		yield();

		char timestamp[22];
		getCurrentTimeString(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S");
		uint32 heap = system_get_free_heap_size();
		if ((alertSent == false) && (heap < 10.0))
		{
			pushover("Heap size < 10Kb !!!");
			alertSent = true;
		}
		else
		{
			alertSent = false;
		}

		char heapFmt[12];
		formatBytes(heap, heapFmt, sizeof(heapFmt));
		char buf[128];
		if (snprintf(buf, sizeof(buf),
					 "%s | %s | cycle: %lu &mu;s | heap: %s | fragm: %u%% | blocks: %u<br />",
					 timestamp,
					 runtime(),
					 getCpuDelta(),
					 heapFmt,
					 ESP.getHeapFragmentation(),
					 ESP.getMaxFreeBlockSize()) < 0)
		{
			strcpy(buf, "<div class=\"error\">error</div>");
		}
		send_Event(buf, "debug");
		timestamp[0] = {'\0'};
		buf[0] = {'\0'};

		char telem[256];
		buildTelemetryJson(telem, sizeof(telem),
			heap,
			ESP.getHeapFragmentation(),
			ESP.getMaxFreeBlockSize(),
			WiFi.RSSI(),
			wifiStatusToString(WiFi.status()),
			lastIndicatorCode,
			gatewayRxMessage,
			gatewayTxMessage,
			sensorRxMessage,
			sensorTxMessage,
			indicatorTxErrors);
		send_Event(telem, "telemetry");

		static unsigned long lastControllerDiagMs = 0;
		if (millis() - lastControllerDiagMs > 30000UL)
		{
			lastControllerDiagMs = millis();
			dbgprintf(ico_info,
					  "[CTRL-DIAG] WiFi=%s(%d) RSSI=%d SSID=%s IP=%s heap=%lu frag=%u maxblk=%u lastInd=%d (%lums ago)",
					  wifiStatusToString(WiFi.status()),
					  static_cast<int>(WiFi.status()),
					  WiFi.RSSI(),
					  WiFi.SSID().c_str(),
					  WiFi.localIP().toString().c_str(),
					  system_get_free_heap_size(),
					  ESP.getHeapFragmentation(),
					  ESP.getMaxFreeBlockSize(),
					  lastIndicatorCode,
					  (lastIndicatorMs == 0 ? 0UL : millis() - lastIndicatorMs));
		}

		static unsigned long lastInfoSendMs = 0;
		if (millis() - lastInfoSendMs >= 5000UL) {
			lastInfoSendMs = millis();
			updateWebStats();
		}

		static unsigned long lastGwClientsSendMs = 0;
		if (millis() - lastGwClientsSendMs >= 5000UL) {
			lastGwClientsSendMs = millis();
			static char clientsBuf[640];
			clientsBuf[0] = '\0';
			buildGwClientsHtml(clientsBuf, sizeof(clientsBuf));
			send_Event(clientsBuf, "clients");
		}
	}

	// interval based jobs
	if (millis() - gw_send_prev_time > gw_send_interval)
	{
		gw_send_prev_time = millis();

		sendHeartbeat();
		send(msgUptime.set(uptime()));

		// every now and then, report ARC statistics ("pseudo-RSSI")
		const char* arc = reportArcStatistics();
		send_Event(arc, "debug");
	}
}
//---------------------------------------------------------------------
#pragma endregion

/*
 * 
 */

#include "config.h"
#include "common.h"
#include "uptime.h"
#include "mysensors_types.h"

// dic: extended debugging
//#define MY_DEBUG_VERBOSE

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
// #define RF24_PA_LEVEL RF24_PA_MAX

// dic: repeater, already included when using as controller
//#define MY_REPEATER_FEATURE

#define MY_GATEWAY_ESP8266

#include "./../../credentials.h"

// Enable UDP communication
// #define MY_USE_UDP

// Set the hostname for the WiFi Client. This is the hostname
// it will pass to the DHCP server if not static.
#define MY_HOSTNAME "GatewayWemosD1Mini"
#define SKETCH_NAME "GatewayWemosD1Mini"
// #define MY_ESP8266_HOSTNAME "MYSD1MiniGatewayOTA"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
// #define MY_IP_ADDRESS 192, 168, 2, 221

// If using static ip you need to define Gateway and Subnet address as well
//#define MY_IP_GATEWAY_ADDRESS 192,168,2,23    /// IMMER DIE IP ADRESSE DES CONTROLLERS: smarthome !!!!
// #define MY_IP_GATEWAY_ADDRESS 192, 168, 2, 222 // (geändert 09.07.2020)  /// IMMER DIE IP ADRESSE DES CONTROLLERS: smarthome !!!!
// #define MY_IP_SUBNET_ADDRESS 255, 255, 255, 0

// The port to keep open on node server mode
#define MY_PORT 5003

// How many clients should be able to connect to this gateway (default 1)
// https://forum.mysensors.org/topic/2712/my_gateway_max_clients
#define MY_GATEWAY_MAX_CLIENTS 2 // (geändert 2.5.2019, voher 3)

// Controller ip address. Enables client mode (default is "server" mode).
// Also enable this if MY_USE_UDP is used and you want sensor data sent somewhere.
// #define MY_CONTROLLER_IP_ADDRESS 192, 168, 2, 68

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
// #define MY_DEFAULT_ERR_LED_PIN 5
#define MY_DEFAULT_RX_LED_PIN D4
// #define MY_DEFAULT_TX_LED_PIN D0
#define MY_INDICATION_HANDLER

unsigned long gatewayTxMessage = 0;
unsigned long gatewayRxMessage = 0;
unsigned long sensorTxMessage = 0;
unsigned long sensorRxMessage = 0;
unsigned long indicatorTxErrors = 0;

// because SPIFFS is deprecated
// #include <LittleFS.h>
// #define SPIFFS LittleFS
#include "FS.h"

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

// /////// /////////////////////////////////////////////////////////////////////
// Initialize message types
// https://www.mysensors.org/download/serial_api_20#variable-types
// https://github.com/mysensors/MySensors/blob/development/core/MyMessage.h
MyMessage msgGeneral(CHILD_ID_GENERAL, V_VAR1);
MyMessage msgUptime(CHILD_ID_UPTIME, V_TEXT);

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

/*
 *
 */
void logBootTime()
{
	dbgprintln(ico_info, "Updating bootlog");
	boolean fsOk = SPIFFS.begin();
	if (fsOk)
	{
		File file = SPIFFS.open("/bootlog.txt", "a");
		if (!file || file.isDirectory())
		{
			dbgprintln(ico_error, "Error: Unable to open boot log in SPIFFS");
		}
		else
		{
			char buffer[64] = {'\0'};
			snprintf(buffer, "%s - %s", getBootTime().c_str(), ESP.getResetReason().c_str());
			dbgprintf(ico_info, "adding to bootlog.txt: %s", buffer);
			file.println(buffer); // add entry to log file
			buffer[0] = '\0';
			file.close();
		}
		SPIFFS.end();
	}
	else
	{
		dbgprintln(ico_error, "error opening SPIFFS!");
	}
}

/*
 *
 */
void setup_OTA()
{
#ifdef OTA
	ArduinoOTA.setHostname(MY_HOSTNAME);

	ArduinoOTA.onStart([]() {
		// Clean SPIFFS
		// https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#end
		SPIFFS.end();
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
		if (snprintf(buf, sizeof(buf), "[OTA] Progress: %d%%", abs(progress / (total / 100))) < 0) // Means append did not (entirely) fit
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
void getMessagePayload(char *payload, MyMessage message)
{
	char _payload[32] = {'\0'};
	// mysensors_payload_t plt = ;
	// DEBUG_PRINTF("getMessagePayload: %d - %d\n", message.getPayloadType(), message.type);
	switch (message.getPayloadType())
	{
	case 0:
		strcpy(payload, message.getString());
		break;
	case 1:
		if (snprintf(_payload, sizeof(_payload), "%d %s", message.getByte(), (char *)mysSetReqUnits[message.type]) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		strcpy(payload, _payload);
		break;
	case 2:
		if (snprintf(_payload, sizeof(_payload), "%d %s", message.getInt(), (char *)mysSetReqUnits[message.type]) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		strcpy(payload, _payload);
		break;
	case 3:
		if (snprintf(_payload, sizeof(_payload), "%u %s", message.getUInt(), (char *)mysSetReqUnits[message.type]) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		strcpy(payload, _payload);
		break;
	case 4:
		if (snprintf(_payload, sizeof(_payload), "%ld %s", (long int)message.getLong(), (char *)mysSetReqUnits[message.type]) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		strcpy(payload, _payload);
		break;
	case 5:
		if (snprintf(_payload, sizeof(_payload), "%lu %s", (long unsigned int)message.getULong(), (char *)mysSetReqUnits[message.type]) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		strcpy(payload, _payload);
		break;
	case 6:
		if (snprintf(_payload, sizeof(_payload), "%p %s", (void *)message.getCustom(), (char *)mysSetReqUnits[message.type]) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		strcpy(payload, _payload);
		break;
	case 7:
		if (snprintf(_payload, sizeof(_payload), "%0.2f %s", message.getFloat(), (char *)mysSetReqUnits[message.type]) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		strcpy(payload, _payload);
		break;
	default:
		if (snprintf(_payload, sizeof(_payload), "error: payload.type: %d - message.type: %d", message.getPayloadType(), message.type) < 0) // Means append did not (entirely) fit
		{
			_payload[0] = '\0';
		}
		strcpy(payload, _payload);
		break;
	}
	_payload[0] = {'\0'};
	// dbgprintf("getMessagePayload %s\n", payload);
}

/*
 *
 */
void updateWebStats()
{
	char timestamp[21] = {'\0'};

	getCurrentTimeString(timestamp, "%Y-%m-%d %H:%M:%S");
	// https://arduino-esp8266.readthedocs.io/en/latest/libraries.html#esp-specific-apis
	String page = "<table><thead><tr><th>&nbsp;</th><th>&nbsp;</th></thead><tr><td>gateway started at</td><td>" +
				  getBootTime() +
				  "</td></tr><tr><td>hostname</td><td>" + String(WiFi.hostname()) + "</td></tr>" +
				  "<tr><td>current time</td><td>" + String(timestamp) + "</td></tr>" +
				  "<tr><td>runtime</td><td>" + runtime() + "</td></tr>" +
				  //   "<tr><td>uptime</td><td>" + uptime() + "</td></tr>" +
				  "<tr><td>build</td><td>" + (String)__DATE__ + " " + (String)__TIME__ + "</td></tr>" +
				  "<tr><td>sw version</td><td>" + String(__SWVERSION__) + "</td></tr>" +
				  "<tr><td>free heap</td><td>" + formatBytes(ESP.getFreeHeap()) + "</td></tr>" +
				  "<tr><td>flash space</td><td>" + formatBytes(ESP.getFlashChipRealSize()) + "</td></tr>" +
				  "<tr><td>used sketch space</td><td>" + formatBytes(ESP.getSketchSize()) + "</td></tr>" +
				  "<tr><td>free sketch space</td><td>" + formatBytes(ESP.getFreeSketchSpace()) + "</td></tr>" +
				  //   "<tr><td>file system</td><td>" + String(fs_info.usedBytes*100.0/fs_info.totalBytes) + "%</td></tr>" +
				  "<tr><td>rssi</td><td>" + String(WiFi.RSSI()) + "dB</td></tr>" +
				  "<tr><td>local ip</td><td>" + WiFi.localIP().toString() + "</td></tr>" +
				  "<tr><td>ssid</td><td>" + WiFi.SSID() + "</td></tr>" +
				  "<tr><td>mac address</td><td>" + WiFi.macAddress() + "</td></tr>" +
#ifdef MY_CORE_ONLY
				  "<tr><td>MY_CORE_ONLY</td><td>TRUE</td></tr>" +
#endif
				  "<tr><td><a href=\"/bootlog.txt\">reset reason</a></td><td>" + ESP.getResetReason() + "</td></tr>" +
				  "</table>" +
				  "</div></body></html>";
	send_Event(page.c_str(), "info");
	page.clear();
	page = String();
}

/* 
 *
 */
void receive(const MyMessage &message)
{
	char logbuf[128] = {'\0'};
	char webjson[256] = {'\0'};
	char timestamp[21] = {'\0'};
	char payload[32] = {'\0'};
	char msgtype[26] = {'\0'};
	char cmdtype[4] = {'\0'};
	char ack = '0';

	// getCurrentTimeString(timestamp, "%Y-%m-%d %H:%M:%S");
	getCurrentTimeString(timestamp, "%H:%M:%S");

	if (message.isAck())
	{
		ack = '1';
	}

	switch (message.getCommand())
	{
	case C_PRESENTATION:
		// strcpy(cmdtype, "PRS");
		strcpy(cmdtype, mysCommandCodes[message.getCommand()]);
		strcpy(msgtype, mysPresenationCodes[message.type]);
		// if (message.sensor == 255)
		// {
		getMessagePayload(payload, message);
		// }
		break;

	case C_INTERNAL:
		// strcpy(cmdtype, "INT");
		strcpy(cmdtype, mysCommandCodes[message.getCommand()]);
		strcpy(msgtype, mysInternalCodes[message.type]);
		getMessagePayload(payload, message);
		break;

	case C_SET:
		// strcpy(cmdtype, "SET");
		strcpy(cmdtype, mysCommandCodes[message.getCommand()]);
		strcpy(msgtype, mysSetReqCodes[message.type]);
		getMessagePayload(payload, message);
		break;

	case C_REQ:
		// strcpy(cmdtype, "REQ");
		strcpy(cmdtype, mysCommandCodes[message.getCommand()]);
		// not used normally
		snprintf(msgtype, sizeof(msgtype), "type: %u", message.type);
		getMessagePayload(payload, message);
		break;

	case C_STREAM:
		// strcpy(cmdtype, "STR");
		strcpy(cmdtype, mysCommandCodes[message.getCommand()]);
		strcpy(msgtype, mysStreamCodes[message.type]);
		getMessagePayload(payload, message);
		break;

	default:
		strcpy(cmdtype, mysCommandCodes[message.getCommand()]);
		snprintf(msgtype, sizeof(msgtype), "type: %u", message.type);
		getMessagePayload(payload, message);
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
	if (snprintf(webjson, sizeof(webjson), "{\n \"time\" : \"%s\", \"cmd\" : \"%s\", \"sender\" : \"%d\", \"sensor\" : \"%d\", \"ack\" : \"%c\", \"msgtype\" : \"%s\", \"payload\" : \"%s\" \n}",
				 timestamp,		 // timestamp
				 cmdtype,		 //
				 message.sender, // node id
				 message.sensor, // sensor id
				 ack,			 // char
				 msgtype,		 //
				 payload) < 0)	 // Means append did not (entirely) fit
	{
		webjson[0] = '\0';
	}
	send_Event(webjson, "messagesjson");
	webjson[0] = {'\0'};
#endif
}

/*
 *
 */
int counter = 0;
void loop_Wifi()
{
	dbgprintln(ico_info, "loop_Wifi()");
#ifdef MY_CORE_ONLY
	WiFi.begin(MY_WIFI_SSID, MY_WIFI_PASSWORD);
#else
	WiFi.begin();
#endif
	delay(500);
	// WiFi.printDiag(Serial);
	if (++counter > 50)
	{
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

//////////////////////////////////////////////////////////////////////////////////////////////////
//
void before()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
void presentation()
{
	sendSketchInfo(SKETCH_NAME, __DATE__ " " __TIME__);
	wait(200);
	present(CHILD_ID_UPTIME, S_INFO, "uptime");
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

//////////////////////////////////////////////////////////////////////////////////////////////////
//	#define MY_INDICATION_HANDLER is needed
//
void indication(const indication_t indicator)
{
	switch (indicator)
	{
	case INDICATION_GW_TX:
		gatewayTxMessage++;
		// send_Event("<img src=\"green.svg\" />", "led");
		break;

	case INDICATION_GW_RX:
		gatewayRxMessage++;
		// send_Event("<img src=\"red.svg\" />", "led");
		break;

	case INDICATION_TX:
		sensorTxMessage++;
		// send_Event("<img src=\"green.svg\" />", "led");
		break;

	case INDICATION_RX:
		sensorRxMessage++;
		// send_Event("<img src=\"red.svg\" />", "led");
		break;

	default:
		// send_Event("<img src=\"yellow.svg\" />", "led");
		break;
	};

	static char mysIndication[32]; // = {'\0'};
	if (indicator > 3 && indicator < 19)
	{
		strcpy(mysIndication, mysIndicationErrorCodes0[indicator]);
	}
	if (indicator > 100)
	{
		strcpy(mysIndication, mysIndicationErrorCodes100[indicator - 101]);
		indicatorTxErrors++;
	}

	char msgbuf[128] = {'\0'};
	if (snprintf(msgbuf, sizeof(msgbuf),
				 "%s<br />gateway: rx: %lu - tx: %lu <br />sensors: rx: %lu - tx: %lu <br />errors: %lu",
				 mysIndication,
				 // indicator,
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
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
void setup()
{
#ifdef MY_DEBUG
	Serial.begin(9600);
	while (!Serial)
	{
	} // Wait
#endif

	cpuLastMicros = micros();

#ifdef MY_CORE_ONLY
	setup_wifi();
#endif

#ifdef TELNET
	telnetServer.begin();
	//telnetServer.setNoDelay(true); // drops chars if set true
#endif // TELNET

#ifdef NTP
	configTime(TZ_INFO, NTP_SERVER[0], NTP_SERVER[1], NTP_SERVER[2]); // check TZ.h, find your location
#endif																  // NTP

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

	// call not before time was set to local time
	setBootTime();
	// and write it to SPIFFS
	logBootTime();
}

/*
 *
 */
int day = -1;
boolean checkMidnight()
{
	struct tm tm;
	time_t now = time(&now);
	localtime_r(&now, &tm);
	char stoday[2] = "00";
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

//////////////////////////////////////////////////////////////////////////////////////////////////
//
void loop()
{
	avgCpuDelta = getCpuDelta();

	delay(10); // https://github.com/espressif/esp-idf/issues/1021

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
		getCurrentTimeString(timestamp, "%Y-%m-%d %H:%M:%S");
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

		char buf[128];
		if (snprintf(buf, sizeof(buf),
					 "%s<br />%s<br />cycle: %lu &mu;s<br />heap: %s / fragm: %u%% / blocks: %u",
					 timestamp,
					 runtime(),
					 getCpuDelta(),
					 formatBytes(heap).c_str(),
					 ESP.getHeapFragmentation(),
					 ESP.getMaxFreeBlockSize()) < 0)
		{
			strcpy(buf, "<div class=\"error\">error</div>");
		}
		send_Event(buf, "debug");
		timestamp[0] = {'\0'};
		buf[0] = {'\0'};

		updateWebStats();
	}

	// interval based jobs
	if (millis() - gw_send_prev_time > gw_send_interval)
	{
		gw_send_prev_time = millis();

		sendHeartbeat();
		send(msgUptime.set(uptime()));
	}
}

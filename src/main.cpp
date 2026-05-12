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

#include "config.h"
#include "common.h"
#include "uptime.h"
#include "mysensors_types.h"
#include "diagnostics_ui.h"
#include "platform_compat.h"

// MQTT Gateway
// https://github.com/mysensors/MySensors/blob/master/examples/GatewayESP8266MQTTClient/GatewayESP8266MQTTClient.ino
#define WITH_MQTT

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

#ifdef USE_ESP32
	#define MY_GATEWAY_ESP32
	// Set the hostname for the WiFi Client. This is the hostname
	// it will pass to the DHCP server if not static.
	#define MY_HOSTNAME "GatewayESP32Wroom"
#else
	#define MY_GATEWAY_ESP8266
	// Set the hostname for the WiFi Client. This is the hostname
	// it will pass to the DHCP server if not static.
	#define MY_HOSTNAME "GatewayWemosD1Mini"
#endif

// Enable UDP communication
// #define MY_USE_UDP


// #define MY_ESP8266_HOSTNAME "MYSD1MiniGatewayOTA"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
// DiC: nicht definieren, sonst läuft FHEM nicht damit
#ifndef WITH_MQTT
	#define MY_IP_ADDRESS 192, 168, 2, 211
#endif

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
#ifdef USE_ESP32
	// https://www.mysensors.org/build/connect_radio
	// ESP32 has 3 SPI interfaces, but only one is usable for RF24, 
	// so we have to use the default HSPI pins (GPIO 14-12-13) or 
	// remap them to other pins using SPI.begin(SCK, MISO, MOSI, SS);
	#define MY_RF24_CE_PIN 			17   // CE Pin
	#define MY_RF24_CS_PIN 			 5   // CSN Pin oder 13

	// The IRQ pin is only required to be connected if the MY_RX_MESSAGE_BUFFER_FEATURE 
	// is defined in the sketch. Using this feature is recommended for high traffic nodes or gateways. 
	// Enabling it will result in better throughput but will require some additional memory to keep the message in memory before processing.	
	//#define MY_RX_MESSAGE_BUFFER_FEATURE

	#define MY_DEFAULT_ERR_LED_PIN 	32	// red
	#define MY_DEFAULT_RX_LED_PIN 	33	// green
	#define MY_DEFAULT_TX_LED_PIN 	27	// yellow
	#define WITH_LEDS_BLINKING
#else
	#define MY_DEFAULT_ERR_LED_PIN 	D1
	#define MY_DEFAULT_RX_LED_PIN 	D0
	#define MY_DEFAULT_TX_LED_PIN 	D3
	#define MY_WITH_LEDS_BLINKING_INVERSE	// bei externen LEDs
#endif


#define MY_INDICATION_HANDLER
#define MY_SPLASH_SCREEN_DISABLED

unsigned long gatewayTxMessage = 0;
unsigned long gatewayRxMessage = 0;
unsigned long sensorTxMessage = 0;
unsigned long sensorRxMessage = 0;
unsigned long indicatorTxErrors = 0;
uint32_t bootCount = 0;
uint32_t lastBootEpoch = 0;
bool bootTimeSynced = false;
static uint8_t gResetReasonCode = 0;

static const char *BOOT_COUNT_FILE = "/boot_count.txt";
static const char *LAST_BOOT_EPOCH_FILE = "/last_boot_epoch.txt";
static bool bootLogSyncedWritten = false;


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

static uint8_t getResetReasonCode()
{
#ifdef USE_ESP32
	return static_cast<uint8_t>(esp_reset_reason());
#else
	const rst_info *ri = ESP.getResetInfoPtr();
	return ri ? static_cast<uint8_t>(ri->reason) : 0;
#endif
}

static const char *getResetReasonText(uint8_t reasonCode)
{
#ifdef USE_ESP32
	const uint8_t count = sizeof(reset_reasons_esp32) / sizeof(reset_reasons_esp32[0]);
	return (reasonCode < count) ? reset_reasons_esp32[reasonCode] : "unknown";
#else
	const uint8_t count = sizeof(reset_reasons_esp8266) / sizeof(reset_reasons_esp8266[0]);
	return (reasonCode < count) ? reset_reasons_esp8266[reasonCode] : "unknown";
#endif
}
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
	(void)nextRecipient;
	(void)message;
    int arc = collectArcStatistics();
	(void)arc;
}
//---------------------------------------------------------------------
#pragma endregion


//=====================================================================
#pragma region logBootTime

static bool isTimeSane()
{
	time_t now = time(nullptr);
	return now > 1700000000; // ~2023-11-14
}

static uint32_t readUint32File(const char *path)
{
	if (!path || !GATEWAY_FS.begin() || !GATEWAY_FS.exists(path)) {
		return 0;
	}
	File file = GATEWAY_FS.open(path, "r");
	if (!file || file.isDirectory()) {
		return 0;
	}
	char buf[16] = {'\0'};
	const size_t n = file.readBytesUntil('\n', buf, sizeof(buf) - 1);
	file.close();
	buf[n] = '\0';
	char *endp = nullptr;
	const unsigned long v = strtoul(buf, &endp, 10);
	return (endp == buf) ? 0 : static_cast<uint32_t>(v);
}

static bool writeUint32File(const char *path, uint32_t value)
{
	if (!path || !GATEWAY_FS.begin()) {
		return false;
	}
	File file = GATEWAY_FS.open(path, "w");
	if (!file || file.isDirectory()) {
		return false;
	}
	file.printf("%lu\n", static_cast<unsigned long>(value));
	file.close();
	return true;
}

static void initBootCounter()
{
	bootCount = readUint32File(BOOT_COUNT_FILE);
	bootCount++;
	if (!writeUint32File(BOOT_COUNT_FILE, bootCount)) {
		dbgprintln(ico_error, "boot counter could not be persisted");
	}

	lastBootEpoch = readUint32File(LAST_BOOT_EPOCH_FILE);
}

/*
 *
 */
void logBootTime(bool ntpOk)
{
	dbgprintln(ico_info, "Updating bootlog");
	boolean fsOk = GATEWAY_FS.begin();
	if (fsOk)
	{
		File file = GATEWAY_FS.open("/bootlog.txt", "a");
		if (!file || file.isDirectory())
		{
			dbgprintln(ico_error, "Error: Unable to open boot log in LittleFS");
		}
		else
		{
			char resetReason[48];
			snprintf(resetReason, sizeof(resetReason), "%s", getResetReasonText(gResetReasonCode));
			resetReason[sizeof(resetReason) - 1] = '\0';

			char timestamp[24] = {'\0'};
			if (ntpOk) {
				getCurrentTimeString(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S");
			} else {
				snprintf(timestamp, sizeof(timestamp), "unsynced@%lus", millis() / 1000UL);
			}

			char buffer[176] = {'\0'};
			if (snprintf(buffer, sizeof(buffer),
						 "%s | boot=%lu | rst=%s | heap=%lu | ip=%s",
						 timestamp,
						 static_cast<unsigned long>(bootCount),
						 resetReason,
						 static_cast<unsigned long>(ESP.getFreeHeap()),
						 WiFi.localIP().toString().c_str()) < 0)
			{
				buffer[0] = '\0';
			}
			dbgprintf(ico_info, "adding to bootlog.txt: %s", buffer);
			file.println(buffer); // add entry to log file
			buffer[0] = '\0';
			file.close();
		}
		// GATEWAY_FS.end();
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
		GATEWAY_FS.end();
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
		dbgprintf(ico_info, "Error: %d", error);
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
	const uint8_t cmd = message.getCommand();
	const bool isSetReq = (cmd == C_SET || cmd == C_REQ);
	const bool isInternalBattery = (cmd == C_INTERNAL && message.type == I_BATTERY_LEVEL);
	const char *unit = "";
	if (isSetReq && message.type < setReqUnitsCount) {
		unit = mysSetReqUnits[message.type];
	} else if (isInternalBattery) {
		unit = "%";
	}
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

#ifdef WITH_MQTT
static String mqttBrokerAddress()
{
#ifdef MY_CONTROLLER_URL_ADDRESS
	return String(MY_CONTROLLER_URL_ADDRESS);
#elif defined(MY_CONTROLLER_IP_ADDRESS)
	const IPAddress brokerIp(MY_CONTROLLER_IP_ADDRESS);
	return brokerIp.toString();
#else
	return String("n/a");
#endif
}

static void buildMqttServerHtml(char *buf, size_t buflen)
{
	if (!buf || buflen == 0) {
		return;
	}
	const bool up = isTransportReady();
	const String broker = mqttBrokerAddress();
	snprintf(buf, buflen,
			 "<b>MQTT Server</b><br />"
			 "<table><thead><tr><th>&nbsp;</th><th>&nbsp;</th></tr></thead><tbody>"
			 "<tr><td>broker</td><td>%s:%u</td></tr>"
			 "<tr><td>uplink</td><td>%s</td></tr>"
			 "<tr><td>wifi</td><td>%s (%d)</td></tr>"
			 "</tbody></table>",
			 broker.c_str(),
			 static_cast<unsigned>(MY_PORT),
			 up ? "connected" : "disconnected",
			 (WiFi.status() == WL_CONNECTED) ? "WL_CONNECTED" : "not connected",
			 static_cast<int>(WiFi.status()));
}
#endif

void updateWebStats()
{
	char timestamp[21];
	getCurrentTimeString(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S");

	char resetReason[48];
	snprintf(resetReason, sizeof(resetReason), "%s", getResetReasonText(gResetReasonCode));
	resetReason[sizeof(resetReason) - 1] = '\0';

	char heapBuf[12], flashBuf[12], sketchBuf[12], freeSketchBuf[12];
	formatBytes(ESP.getFreeHeap(),         heapBuf,       sizeof(heapBuf));
	formatBytes(ESP.getFlashChipSize(),     flashBuf,     sizeof(flashBuf));
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
	WS_APPEND("<tr><td>hostname</td><td>%s</td></tr>",           getWifiHostname().c_str());
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
#ifdef WITH_MQTT
	WS_APPEND("<tr><td>controller type</td><td>MQTT</td></tr>");
	WS_APPEND("<tr><td>mqtt broker</td><td>%s:%u</td></tr>", mqttBrokerAddress().c_str(), static_cast<unsigned>(MY_PORT));
	WS_APPEND("<tr><td>mqtt uplink</td><td>%s</td></tr>", isTransportReady() ? "connected" : "disconnected");
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
	updateSensorStateCache(static_cast<uint8_t>(message.sender),
						   static_cast<uint8_t>(message.sensor),
						   msgtype,
						   payload,
						   timestamp,
						   message.getCommand() == C_SET);
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

#ifndef USE_ESP32
WiFiEventHandler wifiDisconnectHandler;
WiFiEventHandler wifiGotIpHandler;
#endif

static const unsigned long WIFI_BEGIN_INTERVAL_MS = 15000UL;          // retry WiFi.begin every 15s
static const unsigned long WIFI_RECONNECT_TIMEOUT_MS = 1000UL * 60UL * 3UL; // restart only after 3 minutes
static const unsigned long NTP_RETRY_INTERVAL_MS = 30000UL;
static unsigned long lastNtpConfigMs = 0;
static uint8_t ntpServerSetIndex = 0;
static const char * const NTP_ALT_SERVERS[] = {
	"pool.ntp.org",
	"time.google.com",
	"time.cloudflare.com",
	"162.159.200.1",
	"216.239.35.0",
	"129.6.15.28"
};

static void applyTimeZone()
{
#ifdef USE_ESP32
	setenv("TZ", TZ_INFO, 1);
	tzset();
#endif
}

static void requestNtpSync(bool force = false)
{
#ifdef NTP
	if (WiFi.status() != WL_CONNECTED) {
		return;
	}
	const unsigned long nowMs = millis();
	if (!force && (nowMs - lastNtpConfigMs) < NTP_RETRY_INTERVAL_MS) {
		return;
	}
	lastNtpConfigMs = nowMs;

	const uint8_t altCount = sizeof(NTP_ALT_SERVERS) / sizeof(NTP_ALT_SERVERS[0]);
	const char *s1 = NTP_SERVER[0];
	const char *s2 = NTP_SERVER[1];
	const char *s3 = NTP_SERVER[2];

	if (ntpServerSetIndex > 0) {
		const uint8_t i = static_cast<uint8_t>((ntpServerSetIndex - 1) % altCount);
		const uint8_t j = static_cast<uint8_t>((i + 1) % altCount);
		const uint8_t k = static_cast<uint8_t>((j + 1) % altCount);
		s1 = NTP_ALT_SERVERS[i];
		s2 = NTP_ALT_SERVERS[j];
		s3 = NTP_ALT_SERVERS[k];
	}

#ifdef USE_ESP32
	configTzTime(TZ_INFO, s1, s2, s3);
#else
	configTime(TZ_INFO, s1, s2, s3);
#endif
	dbgprintf(ico_info, "[NTP] sync requested (%s, %s, %s)", s1, s2, s3);
	ntpServerSetIndex = static_cast<uint8_t>((ntpServerSetIndex + 1) % (altCount + 1));
#else
	(void)force;
#endif
}

void triggerNtpSync()
{
	requestNtpSync(true);
}

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
#ifdef USE_ESP32
	WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
		if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
			dbgprintf(ico_error,
					  "[WiFiEvt] disconnected | reason=%u (%s) | status=%s (%d) | RSSI=%d",
					  info.wifi_sta_disconnected.reason,
					  wifiDisconnectReasonToString(info.wifi_sta_disconnected.reason),
					  wifiStatusToString(WiFi.status()),
					  static_cast<int>(WiFi.status()),
					  WiFi.RSSI());
		} else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
			dbgprintf(ico_ok,
					  "[WiFiEvt] got-ip | ip=%s | status=%s (%d) | RSSI=%d",
					  WiFi.localIP().toString().c_str(),
					  wifiStatusToString(WiFi.status()),
					  static_cast<int>(WiFi.status()),
					  WiFi.RSSI());
			requestNtpSync(true);
		}
	});
#else
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
		requestNtpSync(true);
	});
#endif
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
#ifndef USE_ESP32
	client.setInsecure();
#endif
	Serial.println("Pushover: connecting ...");
	if (!client.connect("api.pushover.net", 443))
	{
		char err_buf[100];
#ifndef USE_ESP32
		if (client.getLastSSLError(err_buf, 100) < 0)
		{
			Serial.printf_P(PSTR("Pushover: connection failed: %s\n"), err_buf);
		}
		else
#endif
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

#ifdef EXTERNAL_HEARTBEAT_URL
static unsigned long lastExternalHeartbeatMs = 0;
#ifndef EXTERNAL_HEARTBEAT_INTERVAL_MS
#define EXTERNAL_HEARTBEAT_INTERVAL_MS 60000UL
#endif

static void sendExternalHeartbeat()
{
	if (WiFi.status() != WL_CONNECTED) {
		return;
	}

	const unsigned long nowMs = millis();
	if ((nowMs - lastExternalHeartbeatMs) < EXTERNAL_HEARTBEAT_INTERVAL_MS) {
		return;
	}
	lastExternalHeartbeatMs = nowMs;

	char body[256] = {'\0'};
	if (snprintf(body, sizeof(body),
				 "host=%s&boot=%lu&uptime=%lu&heap=%lu&rssi=%d&wifi=%d&rst=%u&ntp=%u&boot_epoch=%lu",
				 MY_HOSTNAME,
				 static_cast<unsigned long>(bootCount),
				 nowMs / 1000UL,
				 static_cast<unsigned long>(ESP.getFreeHeap()),
				 WiFi.RSSI(),
				 static_cast<int>(WiFi.status()),
				 static_cast<unsigned>(gResetReasonCode),
				 bootTimeSynced ? 1U : 0U,
				 static_cast<unsigned long>(lastBootEpoch)) < 0) {
		return;
	}

	WiFiClient client;
	HTTPClient http;
	http.setReuse(false);
	http.setTimeout(2500);
	if (!http.begin(client, EXTERNAL_HEARTBEAT_URL)) {
		dbgprintln(ico_error, "heartbeat begin failed");
		return;
	}
	http.addHeader("Content-Type", "application/x-www-form-urlencoded");
	const int code = http.POST(String(body));
	dbgprintf(code < 200 || code > 299 ? ico_warning : ico_info,
			  "heartbeat -> %s (%d)", EXTERNAL_HEARTBEAT_URL, code);
	http.end();
}
#endif

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
	time_t now = time(nullptr);
	localtime_r(&now, &tm);

	if (tm.tm_sec != lastsec)
	{
		lastsec = tm.tm_sec;
		// einmal am Tag die Zeit vom NTP Server holen o. jede Stunde "% 3600" aller zwei "% 7200"
		if ((now % 86400) == 0)
		{
			requestNtpSync(true);
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
	gResetReasonCode = getResetReasonCode();
	applyTimeZone();

#ifdef MY_DEBUG
	Serial.begin(9600);
	while (!Serial)
	{
	} // Wait

	Serial.println("---------- begin setup()");
	Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
	Serial.printf("SW version: %s\n", __SWVERSION__);
	
	Serial.printf("Reset reason code: %u\n", static_cast<unsigned>(gResetReasonCode));
	Serial.printf("Reset reason text: %s\n", getResetReasonText(gResetReasonCode));
#ifndef USE_ESP32
	Serial.printf("Reset info: %s\n", ESP.getResetInfo().c_str());
#endif
#endif

	cpuLastMicros = micros();

#ifdef MY_CORE_ONLY
	setup_wifi();
#endif

#ifdef TELNET
	telnetServer.begin();
	//telnetServer.setNoDelay(true); // drops chars if set true
	telnetServer.printf("Reset reason %s\n", getResetReasonText(gResetReasonCode));
#endif // TELNET

#ifdef NTP
	requestNtpSync(true);
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
	GATEWAY_FS.begin();// Filesystem mounten
	initBootCounter();

	// initialize statistics
    initStats();
	
	bootTimeSynced = isTimeSane();
	if (bootTimeSynced) {
		lastBootEpoch = static_cast<uint32_t>(time(nullptr));
		writeUint32File(LAST_BOOT_EPOCH_FILE, lastBootEpoch);
		setBootTime();
		logBootTime(true);
		bootLogSyncedWritten = true;
	} else {
		logBootTime(false);
	}
#ifdef WWW
	updateHealthSnapshot(bootCount, lastBootEpoch, gResetReasonCode, bootTimeSynced);
#endif
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
	if (!bootTimeSynced) {
		requestNtpSync();
	}
#endif // NTP

#ifdef OTA
	ArduinoOTA.handle(); // neu seit 2.2
#endif					 // OTA

	if (!bootTimeSynced && isTimeSane())
	{
		bootTimeSynced = true;
		lastBootEpoch = static_cast<uint32_t>(time(nullptr));
		writeUint32File(LAST_BOOT_EPOCH_FILE, lastBootEpoch);
		setBootTime();
		if (!bootLogSyncedWritten)
		{
			logBootTime(true);
			bootLogSyncedWritten = true;
		}
		dbgprintf(ico_ok, "time synchronized, boot epoch: %lu", static_cast<unsigned long>(lastBootEpoch));
	}

#ifdef EXTERNAL_HEARTBEAT_URL
	sendExternalHeartbeat();
#endif

	// interval based jobs
	if (millis() - prev_time > interval)
	{
		prev_time = millis();
		yield();
#ifdef WWW
		updateHealthSnapshot(bootCount, lastBootEpoch, gResetReasonCode, bootTimeSynced);
#endif

		char timestamp[22];
		getCurrentTimeString(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S");
		uint32_t heap = ESP.getFreeHeap();
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
					 getHeapFragmentationPct(),
					 getMaxFreeBlockBytes()) < 0)
		{
			strcpy(buf, "<div class=\"error\">error</div>");
		}
		send_Event(buf, "debug");
		timestamp[0] = {'\0'};
		buf[0] = {'\0'};

		const bool controllerUp = isTransportReady();
#ifdef WITH_MQTT
		const char *controllerType = "mqtt";
#else
		const char *controllerType = "gateway";
#endif

		char telem[320];
		buildTelemetryJson(telem, sizeof(telem),
			heap,
			getHeapFragmentationPct(),
			getMaxFreeBlockBytes(),
			WiFi.RSSI(),
			wifiStatusToString(WiFi.status()),
			lastIndicatorCode,
			gatewayRxMessage,
			gatewayTxMessage,
			sensorRxMessage,
			sensorTxMessage,
			indicatorTxErrors,
			controllerUp,
			controllerType);
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
					  ESP.getFreeHeap(),
					  getHeapFragmentationPct(),
					  getMaxFreeBlockBytes(),
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
#ifdef WITH_MQTT
			buildMqttServerHtml(clientsBuf, sizeof(clientsBuf));
#else
			buildGwClientsHtml(clientsBuf, sizeof(clientsBuf));
#endif
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

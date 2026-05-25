/*
  GatewayESP32
  Build/Upload ueber PlatformIO Environments in platformio.ini
*/

#include "config.h"
#include "common.h"
#include "uptime.h"
#include "mysensors_types.h"
#include "diagnostics_ui.h"
#include "platform_compat.h"

// MQTT Gateway
#define WITH_MQTT

// else IP-Gateway

//#define MAX_MESSAGE_SIZE  (32u) 
// dic: extended debugging
#define MY_DEBUG_VERBOSE

// Baudrate fuer serielle Debug-Ausgabe
#define MY_BAUD_RATE 115200

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
#define RF24_PA_LEVEL RF24_PA_MAX

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

#define MY_GATEWAY_ESP32
// Set the hostname for the WiFi Client. This is the hostname
// it will pass to the DHCP server if not static.
#define MY_HOSTNAME "GatewayESP32Wroom"

// Enable UDP communication
// #define MY_USE_UDP


// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
// DiC: nicht definieren, sonst läuft FHEM nicht damit
#ifndef WITH_MQTT
	#define MY_IP_ADDRESS 192, 168, 2, 211
	// The port to keep open on node server mode
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


#ifdef WITH_WEB_DEBUG
	#define MY_INDICATION_HANDLER
#endif
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
static char gResetReasonRaw[128] = {'\0'};
static char gLastRestartMarker[160] = {'\0'};

static const char *BOOT_COUNT_FILE = "/boot_count.txt";
static const char *LAST_BOOT_EPOCH_FILE = "/last_boot_epoch.txt";
static const char *LAST_RESTART_MARKER_FILE = "/last_restart_marker.txt";
static const char *BOOT_LOG_FILE = "/bootlog.txt";
static const char *BOOT_LOG_TMP_FILE = "/bootlog.tmp";
static const char *BOOT_LOG_LIMIT_FILE = "/bootlog_limit.txt";
static const uint32_t BOOTLOG_MIN_ENTRIES = 50U;
static const uint32_t BOOTLOG_MAX_ENTRIES_LIMIT = 500U;
static bool bootLogSyncedWritten = false;
static uint32_t gBootLogMaxEntries = BOOTLOG_MAX_ENTRIES;


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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <stdio.h>

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


/** serial or telnet output **/
#ifdef TELNET
WiFiServer telnetServer(TELNET_PORT);
WiFiClient telnetClient;
static bool gTelnetRuntimeEnabled = false;
#endif

// 	interval for updating events / web ui
static unsigned long interval = 1000; // every second
static unsigned long prev_time;
static const uint8_t UI_INTERVAL_STEPS_SEC[] = {1, 2, 5, 10, 15, 20, 30, 45, 60};
#ifdef OTA
static bool gOtaRuntimeEnabled = false;
static bool gOtaUpdateInProgress = false;
static unsigned long gOtaWindowUntilMs = 0;
static unsigned long gLastOtaHandleMs = 0;
static const unsigned long OTA_WINDOW_MS = 10UL * 60UL * 1000UL;
static const unsigned long OTA_HANDLE_INTERVAL_MS = 50UL;
#endif

static bool isValidUiIntervalMs(uint32_t intervalMs)
{
	if (intervalMs < 1000UL || intervalMs > 60000UL || (intervalMs % 1000UL) != 0UL) {
		return false;
	}
	const uint8_t sec = static_cast<uint8_t>(intervalMs / 1000UL);
	for (uint8_t i = 0; i < sizeof(UI_INTERVAL_STEPS_SEC); i++) {
		if (UI_INTERVAL_STEPS_SEC[i] == sec) {
			return true;
		}
	}
	return false;
}

uint32_t getUiUpdateIntervalMs()
{
	return interval;
}

bool setUiUpdateIntervalMs(uint32_t intervalMs)
{
	if (!isValidUiIntervalMs(intervalMs)) {
		return false;
	}
	interval = intervalMs;
	return true;
}

bool isOtaRuntimeEnabled()
{
#ifdef OTA
	return gOtaRuntimeEnabled;
#else
	return false;
#endif
}

void setOtaRuntimeEnabled(bool enabled)
{
#ifdef OTA
	if (enabled) {
		gOtaRuntimeEnabled = true;
		gOtaWindowUntilMs = millis() + OTA_WINDOW_MS;
	} else if (!gOtaUpdateInProgress) {
		gOtaRuntimeEnabled = false;
		gOtaWindowUntilMs = 0;
	}
#else
	(void)enabled;
#endif
}

uint32_t getOtaWindowRemainingSec()
{
#ifdef OTA
	if (!gOtaRuntimeEnabled || gOtaUpdateInProgress || gOtaWindowUntilMs == 0) {
		return 0U;
	}
	const unsigned long now = millis();
	if (static_cast<long>(now - gOtaWindowUntilMs) >= 0L) {
		return 0U;
	}
	return static_cast<uint32_t>((gOtaWindowUntilMs - now) / 1000UL);
#else
	return 0U;
#endif
}

// 	interval for sending messages to server
static unsigned long gw_send_interval = 1000 * 60 * 15; // every 15 min
static unsigned long gw_send_prev_time;

unsigned long cpuLastMicros = 0; // cpu utilisation
unsigned long avgCpuDelta = 0;	 // cpu utilisation
static unsigned long lastIndicatorMs = 0;
static int lastIndicatorCode = -1;

//---------------------------------------------------------------------
#pragma endregion


//=====================================================================
#pragma region FreeRTOS Task function

static SemaphoreHandle_t gStatsMutex = nullptr;
static TaskHandle_t gSystemTaskHandle = nullptr;
static volatile uint32_t gSystemHeartbeatMs = 0;
static volatile uint32_t gMySensorsHeartbeatMs = 0;
static volatile uint32_t gSystemLoopCount = 0;
static volatile uint32_t gMySensorsLoopCount = 0;
static const uint32_t TASK_HEALTH_STALL_MS = TASK_HEALTH_WATCHDOG_STALL_MS;
static const uint32_t TASK_HEALTH_DIAG_MS = TASK_HEALTH_DIAG_INTERVAL_MS;

struct SharedStatsSnapshot {
	unsigned long gatewayTxMessage;
	unsigned long gatewayRxMessage;
	unsigned long sensorTxMessage;
	unsigned long sensorRxMessage;
	unsigned long indicatorTxErrors;
	unsigned long lastIndicatorMs;
	int lastIndicatorCode;
};

static inline bool lockStats(uint32_t timeoutMs = 20)
{
	if (!gStatsMutex) {
		return false;
	}
	return xSemaphoreTake(gStatsMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

static inline void unlockStats()
{
	if (gStatsMutex) {
		xSemaphoreGive(gStatsMutex);
	}
}

static SharedStatsSnapshot snapshotStats()
{
	SharedStatsSnapshot s{};
	if (lockStats()) {
		s.gatewayTxMessage = gatewayTxMessage;
		s.gatewayRxMessage = gatewayRxMessage;
		s.sensorTxMessage = sensorTxMessage;
		s.sensorRxMessage = sensorRxMessage;
		s.indicatorTxErrors = indicatorTxErrors;
		s.lastIndicatorMs = lastIndicatorMs;
		s.lastIndicatorCode = lastIndicatorCode;
		unlockStats();
	} else {
		s.gatewayTxMessage = gatewayTxMessage;
		s.gatewayRxMessage = gatewayRxMessage;
		s.sensorTxMessage = sensorTxMessage;
		s.sensorRxMessage = sensorRxMessage;
		s.indicatorTxErrors = indicatorTxErrors;
		s.lastIndicatorMs = lastIndicatorMs;
		s.lastIndicatorCode = lastIndicatorCode;
	}
	return s;
}

static void resetDailyStats()
{
	if (lockStats()) {
		gatewayTxMessage = 0;
		gatewayRxMessage = 0;
		sensorTxMessage = 0;
		sensorRxMessage = 0;
		indicatorTxErrors = 0;
		unlockStats();
		return;
	}
	gatewayTxMessage = 0;
	gatewayRxMessage = 0;
	sensorTxMessage = 0;
	sensorRxMessage = 0;
	indicatorTxErrors = 0;
}

static void taskSystemCore(void *pvParameters);
static void loopSystemCore();
static void loopMySensorsCore();
static void persistPlannedRestartMarker(const char *cause);
static bool readTextFile(const char *path, char *out, size_t outSize);
static bool writeTextFile(const char *path, const char *text);
static bool applyBootLogMaxEntries(uint32_t maxEntries, bool persist);
static void loadBootLogMaxEntriesFromStorage();
static bool applyWifiAuthFailBackoffMs(uint32_t backoffMs, bool persist);
static void loadWifiAuthFailBackoffFromStorage();
static bool applyWifiAuthFailBackoffThreshold(uint32_t threshold, bool persist);
static void loadWifiAuthFailBackoffThresholdFromStorage();
static void beatSystemTask();
static void beatMySensorsTask();
static void checkTaskHealthWatchdog();

static void beatSystemTask()
{
	gSystemHeartbeatMs = millis();
	gSystemLoopCount++;
}

static void beatMySensorsTask()
{
	gMySensorsHeartbeatMs = millis();
	gMySensorsLoopCount++;
}

static uint32_t safeElapsedMs(uint32_t now, uint32_t then)
{
	// If 'then' is observed slightly in the future due to cross-core race,
	// clamp to 0 to avoid false watchdog trips (e.g. age=4294967295).
	const int32_t signedDelta = static_cast<int32_t>(now - then);
	return (signedDelta < 0) ? 0U : static_cast<uint32_t>(signedDelta);
}

static bool heartbeatInFuture(uint32_t now, uint32_t then)
{
	return static_cast<int32_t>(now - then) < 0;
}

static void checkTaskHealthWatchdog()
{
	static unsigned long lastCheckMs = 0;
	static unsigned long lastDiagMs = 0;
	static unsigned long lastRaceDiagMs = 0;
	static uint32_t prevSysLoops = 0;
	static uint32_t prevMyLoops = 0;
	static unsigned long prevDiagMs = 0;
	const unsigned long now = millis();
	if ((now - lastCheckMs) < 1000UL) {
		return;
	}
	lastCheckMs = now;

	const uint32_t sysBeat = gSystemHeartbeatMs;
	const uint32_t myBeat = gMySensorsHeartbeatMs;
	const uint32_t sysAge = (sysBeat == 0U) ? 0U : safeElapsedMs(static_cast<uint32_t>(now), sysBeat);
	const uint32_t myAge = (myBeat == 0U) ? 0U : safeElapsedMs(static_cast<uint32_t>(now), myBeat);
	const bool sysRace = (sysBeat != 0U) && heartbeatInFuture(static_cast<uint32_t>(now), sysBeat);
	const bool myRace = (myBeat != 0U) && heartbeatInFuture(static_cast<uint32_t>(now), myBeat);

	if ((sysRace || myRace) && (now - lastRaceDiagMs) >= 30000UL) {
		lastRaceDiagMs = now;
		dbgprintf(ico_warning,
				  "[TASK] heartbeat race detected (sysRace=%u myRace=%u now=%lu sysBeat=%lu myBeat=%lu)",
				  static_cast<unsigned>(sysRace ? 1U : 0U),
				  static_cast<unsigned>(myRace ? 1U : 0U),
				  static_cast<unsigned long>(now),
				  static_cast<unsigned long>(sysBeat),
				  static_cast<unsigned long>(myBeat));
	}

	if (myBeat != 0 && myAge > TASK_HEALTH_STALL_MS) {
		dbgprintf(ico_error,
				  "[TASK-WD] MySensors core stalled (age=%lu ms, loops sys=%lu my=%lu)",
				  static_cast<unsigned long>(myAge),
				  static_cast<unsigned long>(gSystemLoopCount),
				  static_cast<unsigned long>(gMySensorsLoopCount));
		persistPlannedRestartMarker("task-watchdog-mysensors-stall");
		delay(100);
		ESP.restart();
	}

	if ((now - lastDiagMs) >= TASK_HEALTH_DIAG_MS) {
		lastDiagMs = now;
		const uint32_t sysLoops = gSystemLoopCount;
		const uint32_t myLoops = gMySensorsLoopCount;
		const uint32_t sysDelta = (prevDiagMs == 0UL) ? 0U : (sysLoops - prevSysLoops);
		const uint32_t myDelta = (prevDiagMs == 0UL) ? 0U : (myLoops - prevMyLoops);
		const unsigned long diagElapsedMs = (prevDiagMs == 0UL) ? 0UL : (now - prevDiagMs);
		const unsigned long sysLoopsPerSecX100 =
			(diagElapsedMs > 0UL) ? static_cast<unsigned long>((100000UL * static_cast<unsigned long>(sysDelta)) / diagElapsedMs) : 0UL;
		const unsigned long myLoopsPerSecX100 =
			(diagElapsedMs > 0UL) ? static_cast<unsigned long>((100000UL * static_cast<unsigned long>(myDelta)) / diagElapsedMs) : 0UL;
		const unsigned long sysLoopsPerSecInt = sysLoopsPerSecX100 / 100UL;
		const unsigned long sysLoopsPerSecFrac = sysLoopsPerSecX100 % 100UL;
		const unsigned long myLoopsPerSecInt = myLoopsPerSecX100 / 100UL;
		const unsigned long myLoopsPerSecFrac = myLoopsPerSecX100 % 100UL;
		dbgprintf(ico_info,
				  "[TASK] sys(core0): loops=%lu delta=%lu rate=%lu.%02lu/s age=%lu ms | mys(core1): loops=%lu delta=%lu rate=%lu.%02lu/s age=%lu ms",
				  static_cast<unsigned long>(sysLoops),
				  static_cast<unsigned long>(sysDelta),
				  sysLoopsPerSecInt,
				  sysLoopsPerSecFrac,
				  static_cast<unsigned long>(sysAge),
				  static_cast<unsigned long>(myLoops),
				  static_cast<unsigned long>(myDelta),
				  myLoopsPerSecInt,
				  myLoopsPerSecFrac,
				  static_cast<unsigned long>(myAge));
		prevSysLoops = sysLoops;
		prevMyLoops = myLoops;
		prevDiagMs = now;
	}
}

//---------------------------------------------------------------------
#pragma endregion


//=====================================================================
#pragma region telnet functions
/* 
 * 	https://github.com/dragondaud/SolarGuardn/blob/master/SolarGuardn.ino
 */
template <typename T>
void telnetOut(const T x)
{
#ifdef MY_DEBUG	
	Serial.print(x);
#endif

#ifdef TELNET
	if (gTelnetRuntimeEnabled && telnetClient && telnetClient.connected())
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

bool isTelnetRuntimeEnabled()
{
#ifdef TELNET
	return gTelnetRuntimeEnabled;
#else
	return false;
#endif
}

void setTelnetRuntimeEnabled(bool enabled)
{
#ifdef TELNET
	gTelnetRuntimeEnabled = enabled;
	if (!enabled && telnetClient) {
		telnetClient.stop();
	}
#else
	(void)enabled;
#endif
}

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
	return static_cast<uint8_t>(esp_reset_reason());
}

static const char *getResetReasonText(uint8_t reasonCode)
{
	const uint8_t count = sizeof(reset_reasons_esp32) / sizeof(reset_reasons_esp32[0]);
	return (reasonCode < count) ? reset_reasons_esp32[reasonCode] : "unknown";
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
	if (!path || !gatewayFsBegin() || !GATEWAY_FS.exists(path)) {
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
	if (!path || !gatewayFsBegin()) {
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

static bool readTextFile(const char *path, char *out, size_t outSize)
{
	if (!path || !out || outSize == 0 || !gatewayFsBegin() || !GATEWAY_FS.exists(path)) {
		return false;
	}
	File file = GATEWAY_FS.open(path, "r");
	if (!file || file.isDirectory()) {
		return false;
	}
	const size_t n = file.readBytesUntil('\n', out, outSize - 1);
	out[n] = '\0';
	file.close();
	return n > 0;
}

static bool writeTextFile(const char *path, const char *text)
{
	if (!path || !text || !gatewayFsBegin()) {
		return false;
	}
	File file = GATEWAY_FS.open(path, "w");
	if (!file || file.isDirectory()) {
		return false;
	}
	file.print(text);
	file.print('\n');
	file.close();
	return true;
}

static void removeFileIfExists(const char *path)
{
	if (!path || !gatewayFsBegin() || !GATEWAY_FS.exists(path)) {
		return;
	}
	GATEWAY_FS.remove(path);
}

static bool appendBootLogBounded(const char *line)
{
	if (!line || !gatewayFsBegin()) {
		return false;
	}

	uint32_t lineCount = 0;
	if (GATEWAY_FS.exists(BOOT_LOG_FILE)) {
		File srcCount = GATEWAY_FS.open(BOOT_LOG_FILE, "r");
		if (srcCount && !srcCount.isDirectory()) {
			while (srcCount.available()) {
				(void)srcCount.readStringUntil('\n');
				lineCount++;
			}
		}
		srcCount.close();
	}

	const uint32_t maxEntries = (gBootLogMaxEntries > 0U) ? gBootLogMaxEntries : BOOTLOG_MAX_ENTRIES;
	const uint32_t keepExisting = (maxEntries > 0U) ? (maxEntries - 1U) : 0U;
	const uint32_t skipLines = (lineCount > keepExisting) ? (lineCount - keepExisting) : 0U;

	File dst = GATEWAY_FS.open(BOOT_LOG_TMP_FILE, "w");
	if (!dst || dst.isDirectory()) {
		dst.close();
		return false;
	}

	if (GATEWAY_FS.exists(BOOT_LOG_FILE)) {
		File src = GATEWAY_FS.open(BOOT_LOG_FILE, "r");
		if (src && !src.isDirectory()) {
			uint32_t idx = 0;
			while (src.available()) {
				String l = src.readStringUntil('\n');
				if (idx >= skipLines) {
					dst.println(l);
				}
				idx++;
			}
		}
		src.close();
	}

	dst.println(line);
	dst.close();

	removeFileIfExists(BOOT_LOG_FILE);
	if (!GATEWAY_FS.rename(BOOT_LOG_TMP_FILE, BOOT_LOG_FILE)) {
		removeFileIfExists(BOOT_LOG_TMP_FILE);
		return false;
	}
	return true;
}

static bool applyBootLogMaxEntries(uint32_t maxEntries, bool persist)
{
	if (maxEntries < BOOTLOG_MIN_ENTRIES || maxEntries > BOOTLOG_MAX_ENTRIES_LIMIT) {
		return false;
	}

	const bool changed = (gBootLogMaxEntries != maxEntries);
	gBootLogMaxEntries = maxEntries;

	if (persist && changed) {
		char buf[16] = {'\0'};
		snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(maxEntries));
		if (!writeTextFile(BOOT_LOG_LIMIT_FILE, buf)) {
			return false;
		}
	}

	if (changed) {
		dbgprintf(ico_info, "[Bootlog] max entries set to %lu", static_cast<unsigned long>(gBootLogMaxEntries));
	}

	return true;
}

static void loadBootLogMaxEntriesFromStorage()
{
	char buf[24] = {'\0'};
	if (!readTextFile(BOOT_LOG_LIMIT_FILE, buf, sizeof(buf))) {
		return;
	}
	char *endp = nullptr;
	const unsigned long v = strtoul(buf, &endp, 10);
	if (endp == buf || !applyBootLogMaxEntries(static_cast<uint32_t>(v), false)) {
		dbgprintf(ico_warning,
				  "[Bootlog] invalid persisted max entries ignored: %s (allowed %lu..%lu)",
				  buf,
				  static_cast<unsigned long>(BOOTLOG_MIN_ENTRIES),
				  static_cast<unsigned long>(BOOTLOG_MAX_ENTRIES_LIMIT));
	}
}

uint32_t getBootLogMaxEntries()
{
	return gBootLogMaxEntries;
}

bool setBootLogMaxEntries(uint32_t maxEntries)
{
	return applyBootLogMaxEntries(maxEntries, true);
}

static void captureResetReasonRaw()
{
	snprintf(gResetReasonRaw, sizeof(gResetReasonRaw), "esp_reset_reason=%u", static_cast<unsigned>(esp_reset_reason()));
	gResetReasonRaw[sizeof(gResetReasonRaw) - 1] = '\0';
}

static void persistPlannedRestartMarker(const char *cause)
{
	char marker[sizeof(gLastRestartMarker)] = {'\0'};
	if (snprintf(marker, sizeof(marker),
				 "cause=%s|uptime_s=%lu|heap=%lu|frag=%u|wifi=%d|rssi=%d",
				 cause ? cause : "unknown",
				 millis() / 1000UL,
				 static_cast<unsigned long>(ESP.getFreeHeap()),
				 static_cast<unsigned>(getHeapFragmentationPct()),
				 static_cast<int>(WiFi.status()),
				 WiFi.RSSI()) < 0) {
		return;
	}
	if (writeTextFile(LAST_RESTART_MARKER_FILE, marker)) {
		snprintf(gLastRestartMarker, sizeof(gLastRestartMarker), "%s", marker);
		gLastRestartMarker[sizeof(gLastRestartMarker) - 1] = '\0';
		dbgprintf(ico_warning, "planned restart marker persisted: %s", gLastRestartMarker);
	}
}

static void consumeLastRestartMarker()
{
	gLastRestartMarker[0] = '\0';
	if (readTextFile(LAST_RESTART_MARKER_FILE, gLastRestartMarker, sizeof(gLastRestartMarker))) {
		dbgprintf(ico_warning, "detected previous planned restart: %s", gLastRestartMarker);
	}
	removeFileIfExists(LAST_RESTART_MARKER_FILE);
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
	boolean fsOk = gatewayFsBegin();
	if (fsOk)
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

		char buffer[320] = {'\0'};
		if (snprintf(buffer, sizeof(buffer),
					 "%s | boot=%lu | rst=%s | rst_raw=%s | planned=%s | heap=%lu | ip=%s",
					 timestamp,
					 static_cast<unsigned long>(bootCount),
					 resetReason,
					 gResetReasonRaw,
					 gLastRestartMarker[0] ? gLastRestartMarker : "none",
					 static_cast<unsigned long>(ESP.getFreeHeap()),
					 WiFi.localIP().toString().c_str()) < 0)
		{
			buffer[0] = '\0';
		}
		dbgprintf(ico_info, "adding to bootlog.txt: %s", buffer);
		if (!appendBootLogBounded(buffer)) {
			dbgprintln(ico_error, "Error: Unable to write bounded boot log");
		}
		// GATEWAY_FS.end();
	}
	else
	{
		dbgprintln(ico_error, "error opening filesystem!");
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
		gOtaUpdateInProgress = true;
		// Unmount filesystem waehrend OTA
		GATEWAY_FS.end();
		dbgprintln(ico_info, "Start");
		send_Event("[OTA] Start", "debug");
	});
	ArduinoOTA.onEnd([]() {
		gOtaUpdateInProgress = false;
		gOtaRuntimeEnabled = false;
		gOtaWindowUntilMs = 0;
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
		gOtaUpdateInProgress = false;
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
#ifdef WWW
	if (isWebDebugEnabled()) {
		dbgprintf(ico_info, "getMessagePayload %s\n", payload);
	}
#else
	dbgprintf(ico_info, "getMessagePayload %s\n", payload);
#endif
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

	static char page[3072];
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

	WS_APPEND("<table><thead><tr><th>&nbsp;</th><th>&nbsp;</th></tr></thead><tbody>");
	WS_APPEND("<tr><td>gateway started at</td><td>%s</td></tr>", getBootTime());
	WS_APPEND("<tr><td>hostname</td><td>%s</td></tr>",           getWifiHostname().c_str());
	WS_APPEND("<tr><td>current time</td><td>%s</td></tr>",       timestamp);
	WS_APPEND("<tr><td>runtime</td><td>%s</td></tr>",            runtime());
	WS_APPEND("<tr><td>build</td><td>%s %s</td></tr>",           __DATE__, __TIME__);
	WS_APPEND("<tr><td>sw version</td><td>%s</td></tr>",         __SWVERSION__);
	WS_APPEND("<tr><td>boot count</td><td>%lu</td></tr>",        static_cast<unsigned long>(bootCount));
	WS_APPEND("<tr><td>bootlog limit</td><td>%lu Eintraege</td></tr>", static_cast<unsigned long>(getBootLogMaxEntries()));
	WS_APPEND("<tr><td>reset reason code</td><td>%u</td></tr>",  static_cast<unsigned>(gResetReasonCode));
	WS_APPEND("<tr><td>reset reason raw</td><td>%s</td></tr>",   gResetReasonRaw);
	WS_APPEND("<tr><td><a href=\"/bootlog.txt\">reset reason</a></td><td>%s</td></tr>", resetReason);
	WS_APPEND("<tr><td>planned restart marker</td><td>%s</td></tr>", gLastRestartMarker[0] ? gLastRestartMarker : "none");
	WS_APPEND("<tr><td>free heap</td><td>%s</td></tr>",          heapBuf);
	WS_APPEND("<tr><td>flash space</td><td>%s</td></tr>",        flashBuf);
	WS_APPEND("<tr><td>used sketch space</td><td>%s</td></tr>",  sketchBuf);
	WS_APPEND("<tr><td>free sketch space</td><td>%s</td></tr>",  freeSketchBuf);
	WS_APPEND("<tr><td>rssi</td><td>%ddB</td></tr>",             WiFi.RSSI());
	WS_APPEND("<tr><td>local ip</td><td>%s</td></tr>",           WiFi.localIP().toString().c_str());
	WS_APPEND("<tr><td>ssid</td><td>%s</td></tr>",               WiFi.SSID().c_str());
	WS_APPEND("<tr><td>mac address</td><td>%s</td></tr>",        WiFi.macAddress().c_str());
	WS_APPEND("<tr><td>wifi</td><td>%s</td></tr>",        		(WiFi.status() == WL_CONNECTED) ? "WL_CONNECTED" : "not connected");
#ifdef MY_CORE_ONLY
	WS_APPEND("<tr><td>MY_CORE_ONLY</td><td>TRUE</td></tr>");
#endif
#ifdef WITH_MQTT
	WS_APPEND("<tr><td>controller type</td><td>MQTT</td></tr>");
#endif

	static char clientsBuf[640];
	clientsBuf[0] = '\0';
#ifdef WITH_MQTT
	// buildMqttServerHtml(clientsBuf, sizeof(clientsBuf));
	const bool up = isTransportReady();
	const String broker = mqttBrokerAddress();
	WS_APPEND("<tr><td>broker</td><td>%s:%u</td></tr>", broker.c_str(), static_cast<unsigned>(MY_PORT));
	WS_APPEND("<tr><td>uplink</td><td>%s</td></tr>", up ? "connected" : "disconnected");
	WS_APPEND("</tbody></table>");
#else
	WS_APPEND("</tbody></table>");
	buildGwClientsHtml(clientsBuf, sizeof(clientsBuf));
#endif

	if (clientsBuf[0] != '\0') {
		WS_APPEND("<br />%s", clientsBuf);
	}
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
#ifdef WWW
		registerPresentedSensor(static_cast<uint8_t>(message.sender),
								static_cast<uint8_t>(message.sensor));
#endif
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
	const bool acceptedSet = updateSensorStateCache(static_cast<uint8_t>(message.sender),
													static_cast<uint8_t>(message.sensor),
													msgtype,
													payload,
													timestamp,
													message.getCommand() == C_SET);
	if (snprintf(webjson, sizeof(webjson), "{\"time\":\"%s\",\"cmd\":\"%s\",\"sender\":\"%d\",\"sensor\":\"%d\",\"ack\":\"%c\",\"msgtype\":\"%s\",\"payload\":\"%s\",\"accepted\":%u}",
				 timestamp,
				 cmdtype,
				 message.sender,
				 message.sensor,
				 ack,
				 msgtype,
				 payload,
				 acceptedSet ? 1U : 0U) < 0)
	{
		webjson[0] = '\0';
	}
	send_Event(webjson, "messagesjson");
	webjson[0] = {'\0'};
#endif
}

//=====================================================================
#pragma endregion


//=====================================================================
#pragma region NTP/Time function

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
	setenv("TZ", TZ_INFO, 1);
	tzset();
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

	configTzTime(TZ_INFO, s1, s2, s3);
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

//=====================================================================
#pragma region Wifi function
/*
 *
 */
int counter = 0;
static unsigned long wifiReconnectFirstFailMs = 0;
static unsigned long wifiLastBeginMs = 0;
static unsigned long wifiNextBeginAllowedMs = 0;
static wl_status_t wifiLastStatus = WL_IDLE_STATUS;
static uint8_t wifiReconnectTimeoutCycles = 0;
static uint8_t wifiReconnectAttempts = 0;
static bool wifiStackRecoveryDone = false;
static volatile uint8_t wifiAuthFailStreak = 0;
static volatile bool wifiAuthFailFallbackPending = false;
static unsigned long wifiAuthFailBackoffUntilMs = 0;
static volatile bool wifiDisconnectedEventPending = false;
static volatile uint8_t wifiLastDisconnectReason = 0U;
static volatile bool wifiGotIpEventPending = false;
static const char *WIFI_REPEATER_BSSID_FILE = "/wifi_repeater_bssid.txt";
static const char *WIFI_AUTHFAIL_BACKOFF_FILE = "/wifi_authfail_backoff_ms.txt";
static const char *WIFI_AUTHFAIL_BACKOFF_THRESHOLD_FILE = "/wifi_authfail_backoff_threshold.txt";
static const uint32_t WIFI_AUTHFAIL_BACKOFF_MIN_MS = 5000U;
static const uint32_t WIFI_AUTHFAIL_BACKOFF_MAX_MS = 300000U;
static const uint32_t WIFI_AUTHFAIL_BACKOFF_THRESHOLD_MIN = 2U;
static const uint32_t WIFI_AUTHFAIL_BACKOFF_THRESHOLD_MAX = 20U;
static bool gWifiRepeaterBssidLocked = false;
static uint8_t gWifiRepeaterBssid[6] = {0};
static char gWifiRepeaterBssidText[18] = {'\0'};
static bool gWifiForceAnyApFallback = false;
static uint32_t gWifiAuthFailBackoffMs = CFG_WIFI_AUTHFAIL_BACKOFF_MS;
static uint8_t gWifiAuthFailBackoffThreshold = static_cast<uint8_t>(CFG_WIFI_AUTHFAIL_STREAK_BACKOFF_THRESHOLD);

static const unsigned long WIFI_RECONNECT_MIN_FAIL_MS = CFG_WIFI_RECONNECT_MIN_FAIL_MS;
static const unsigned long WIFI_BEGIN_BASE_INTERVAL_MS = CFG_WIFI_RECONNECT_BASE_INTERVAL_MS;
static const unsigned long WIFI_BEGIN_MAX_INTERVAL_MS = CFG_WIFI_RECONNECT_MAX_INTERVAL_MS;
static const unsigned long WIFI_STACK_RECOVERY_MS = CFG_WIFI_STACK_RECOVERY_MS;
static const unsigned long WIFI_RECONNECT_TIMEOUT_MS = CFG_WIFI_RECONNECT_TIMEOUT_MS;
static const uint8_t WIFI_TIMEOUT_CYCLES_BEFORE_RESTART = static_cast<uint8_t>(CFG_WIFI_TIMEOUT_CYCLES_BEFORE_RESTART);


static bool parseBssidString(const char *text, uint8_t out[6], char normalized[18])
{
	if (!text || !out || !normalized) {
		return false;
	}
	unsigned int b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0;
	int n = sscanf(text, "%2x:%2x:%2x:%2x:%2x:%2x", &b0, &b1, &b2, &b3, &b4, &b5);
	if (n != 6) {
		n = sscanf(text, "%2x-%2x-%2x-%2x-%2x-%2x", &b0, &b1, &b2, &b3, &b4, &b5);
		if (n != 6) {
			return false;
		}
	}
	out[0] = static_cast<uint8_t>(b0);
	out[1] = static_cast<uint8_t>(b1);
	out[2] = static_cast<uint8_t>(b2);
	out[3] = static_cast<uint8_t>(b3);
	out[4] = static_cast<uint8_t>(b4);
	out[5] = static_cast<uint8_t>(b5);
	snprintf(normalized, 18, "%02X:%02X:%02X:%02X:%02X:%02X", out[0], out[1], out[2], out[3], out[4], out[5]);
	normalized[17] = '\0';
	return true;
}

const char *getWifiRepeaterBssid()
{
	return gWifiRepeaterBssidLocked ? gWifiRepeaterBssidText : "";
}

bool isWifiRepeaterBssidLocked()
{
	return gWifiRepeaterBssidLocked;
}

static bool applyWifiRepeaterBssid(const String &bssid, bool persist)
{
	String value = bssid;
	value.trim();
	if (!value.length()) {
		const bool hadLock = gWifiRepeaterBssidLocked;
		gWifiRepeaterBssidLocked = false;
		gWifiRepeaterBssidText[0] = '\0';
		memset(gWifiRepeaterBssid, 0, sizeof(gWifiRepeaterBssid));
		if (persist && hadLock && gatewayFsBegin()) {
			GATEWAY_FS.remove(WIFI_REPEATER_BSSID_FILE);
		}
		if (hadLock) {
			dbgprintln(ico_info, "[WiFi] repeater BSSID lock cleared");
		}
		return true;
	}

	uint8_t parsed[6] = {0};
	char normalized[18] = {'\0'};
	if (!parseBssidString(value.c_str(), parsed, normalized)) {
		return false;
	}

	const bool changed = (!gWifiRepeaterBssidLocked) || (strncmp(gWifiRepeaterBssidText, normalized, sizeof(gWifiRepeaterBssidText)) != 0);
	memcpy(gWifiRepeaterBssid, parsed, sizeof(gWifiRepeaterBssid));
	snprintf(gWifiRepeaterBssidText, sizeof(gWifiRepeaterBssidText), "%s", normalized);
	gWifiRepeaterBssidText[sizeof(gWifiRepeaterBssidText) - 1] = '\0';
	gWifiRepeaterBssidLocked = true;

	if (persist && changed) {
		(void)writeTextFile(WIFI_REPEATER_BSSID_FILE, gWifiRepeaterBssidText);
	}
	if (changed) {
		dbgprintf(ico_info, "[WiFi] repeater BSSID lock set to %s", gWifiRepeaterBssidText);
	}
	return true;
}

bool setWifiRepeaterBssid(const String &bssid)
{
	return applyWifiRepeaterBssid(bssid, true);
}

static void loadWifiRepeaterBssidFromStorage()
{
	char buf[24] = {'\0'};
	if (readTextFile(WIFI_REPEATER_BSSID_FILE, buf, sizeof(buf))) {
		String value(buf);
		value.trim();
		if (!applyWifiRepeaterBssid(value, false)) {
			dbgprintf(ico_warning, "[WiFi] invalid stored repeater BSSID ignored: %s", buf);
		}
	}
}

static bool applyWifiAuthFailBackoffMs(uint32_t backoffMs, bool persist)
{
	if (backoffMs < WIFI_AUTHFAIL_BACKOFF_MIN_MS || backoffMs > WIFI_AUTHFAIL_BACKOFF_MAX_MS) {
		return false;
	}
	const bool changed = (gWifiAuthFailBackoffMs != backoffMs);
	gWifiAuthFailBackoffMs = backoffMs;
	if (persist && changed) {
		char buf[16] = {'\0'};
		snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(backoffMs));
		if (!writeTextFile(WIFI_AUTHFAIL_BACKOFF_FILE, buf)) {
			return false;
		}
	}
	if (changed) {
		dbgprintf(ico_info,
				  "[WiFi] AUTH_FAIL backoff set to %lu ms",
				  static_cast<unsigned long>(gWifiAuthFailBackoffMs));
	}
	return true;
}

static void loadWifiAuthFailBackoffFromStorage()
{
	char buf[24] = {'\0'};
	if (!readTextFile(WIFI_AUTHFAIL_BACKOFF_FILE, buf, sizeof(buf))) {
		return;
	}
	char *endp = nullptr;
	const unsigned long v = strtoul(buf, &endp, 10);
	if (endp == buf || !applyWifiAuthFailBackoffMs(static_cast<uint32_t>(v), false)) {
		dbgprintf(ico_warning,
				  "[WiFi] invalid stored AUTH_FAIL backoff ignored: %s (allowed %lu..%lu ms)",
				  buf,
				  static_cast<unsigned long>(WIFI_AUTHFAIL_BACKOFF_MIN_MS),
				  static_cast<unsigned long>(WIFI_AUTHFAIL_BACKOFF_MAX_MS));
	}
}

uint32_t getWifiAuthFailBackoffMs()
{
	return gWifiAuthFailBackoffMs;
}

bool setWifiAuthFailBackoffMs(uint32_t backoffMs)
{
	return applyWifiAuthFailBackoffMs(backoffMs, true);
}

static bool applyWifiAuthFailBackoffThreshold(uint32_t threshold, bool persist)
{
	if (threshold < WIFI_AUTHFAIL_BACKOFF_THRESHOLD_MIN || threshold > WIFI_AUTHFAIL_BACKOFF_THRESHOLD_MAX) {
		return false;
	}
	const uint8_t value = static_cast<uint8_t>(threshold);
	const bool changed = (gWifiAuthFailBackoffThreshold != value);
	gWifiAuthFailBackoffThreshold = value;
	if (persist && changed) {
		char buf[8] = {'\0'};
		snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(value));
		if (!writeTextFile(WIFI_AUTHFAIL_BACKOFF_THRESHOLD_FILE, buf)) {
			return false;
		}
	}
	if (changed) {
		dbgprintf(ico_info,
				  "[WiFi] AUTH_FAIL backoff threshold set to %u",
				  static_cast<unsigned>(gWifiAuthFailBackoffThreshold));
	}
	return true;
}

static void loadWifiAuthFailBackoffThresholdFromStorage()
{
	char buf[24] = {'\0'};
	if (!readTextFile(WIFI_AUTHFAIL_BACKOFF_THRESHOLD_FILE, buf, sizeof(buf))) {
		return;
	}
	char *endp = nullptr;
	const unsigned long v = strtoul(buf, &endp, 10);
	if (endp == buf || !applyWifiAuthFailBackoffThreshold(static_cast<uint32_t>(v), false)) {
		dbgprintf(ico_warning,
				  "[WiFi] invalid stored AUTH_FAIL backoff threshold ignored: %s (allowed %lu..%lu)",
				  buf,
				  static_cast<unsigned long>(WIFI_AUTHFAIL_BACKOFF_THRESHOLD_MIN),
				  static_cast<unsigned long>(WIFI_AUTHFAIL_BACKOFF_THRESHOLD_MAX));
	}
}

uint32_t getWifiAuthFailBackoffThreshold()
{
	return static_cast<uint32_t>(gWifiAuthFailBackoffThreshold);
}

bool setWifiAuthFailBackoffThreshold(uint32_t threshold)
{
	return applyWifiAuthFailBackoffThreshold(threshold, true);
}

static void beginWifiConnect()
{
#if defined(MY_WIFI_SSID) && defined(MY_WIFI_PASSWORD)
	if (gWifiRepeaterBssidLocked && !gWifiForceAnyApFallback) {
		dbgprintf(ico_info, "[WiFi] connect using locked repeater BSSID %s", gWifiRepeaterBssidText);
		WiFi.begin(MY_WIFI_SSID, MY_WIFI_PASSWORD, 0, gWifiRepeaterBssid, true);
	} else {
		if (gWifiRepeaterBssidLocked && gWifiForceAnyApFallback) {
			dbgprintf(ico_warning,
					  "[WiFi] connect fallback: any AP with SSID %s allowed (ignoring lock %s)",
					  MY_WIFI_SSID,
					  gWifiRepeaterBssidText);
		}
		WiFi.begin(MY_WIFI_SSID, MY_WIFI_PASSWORD);
	}
#else
	WiFi.begin();
#endif
}

static unsigned long computeWifiBeginBackoffMs(uint8_t attempts)
{
	const uint8_t shift = (attempts > 4U) ? 4U : attempts;
	const unsigned long scaled = WIFI_BEGIN_BASE_INTERVAL_MS << shift;
	return (scaled > WIFI_BEGIN_MAX_INTERVAL_MS) ? WIFI_BEGIN_MAX_INTERVAL_MS : scaled;
}

static bool shouldAttemptWifiBegin(wl_status_t status, unsigned long now)
{
	if (status == WL_IDLE_STATUS || status == WL_SCAN_COMPLETED) {
		return false;
	}

	if (status != WL_DISCONNECTED &&
		status != WL_CONNECT_FAILED &&
		status != WL_CONNECTION_LOST &&
		status != WL_NO_SSID_AVAIL) {
		return false;
	}

	if (wifiReconnectFirstFailMs == 0 || (now - wifiReconnectFirstFailMs) < WIFI_RECONNECT_MIN_FAIL_MS) {
		return false;
	}

	return now >= wifiNextBeginAllowedMs;
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
	WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
		if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
			const uint8_t reason = info.wifi_sta_disconnected.reason;
			wifiLastDisconnectReason = reason;
			wifiDisconnectedEventPending = true;
			if (reason == 202U || reason == 23U) {
				if (wifiAuthFailStreak < 255U) {
					wifiAuthFailStreak++;
				}
				if (wifiAuthFailStreak >= gWifiAuthFailBackoffThreshold) {
					const unsigned long until = millis() + gWifiAuthFailBackoffMs;
					if (static_cast<long>(until - wifiAuthFailBackoffUntilMs) > 0L) {
						wifiAuthFailBackoffUntilMs = until;
					}
				}
				if (gWifiRepeaterBssidLocked && !gWifiForceAnyApFallback && wifiAuthFailStreak >= 3U) {
					wifiAuthFailFallbackPending = true;
				}
			} else {
				wifiAuthFailStreak = 0U;
			}
		} else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
			wifiGotIpEventPending = true;
			wifiAuthFailStreak = 0U;
			wifiAuthFailFallbackPending = false;
			wifiAuthFailBackoffUntilMs = 0;
			gWifiForceAnyApFallback = false;
		}
	});
}

void loop_Wifi()
{
	const unsigned long now = millis();
	const wl_status_t status = WiFi.status();

	if (wifiDisconnectedEventPending) {
		const uint8_t reason = wifiLastDisconnectReason;
		wifiDisconnectedEventPending = false;
		dbgprintf(ico_error,
				  "[WiFiEvt] disconnected | reason=%u (%s) | status=%s (%d) | RSSI=%d",
				  static_cast<unsigned>(reason),
				  wifiDisconnectReasonToString(reason),
				  wifiStatusToString(status),
				  static_cast<int>(status),
				  WiFi.RSSI());
	}

	if (wifiGotIpEventPending) {
		wifiGotIpEventPending = false;
		dbgprintf(ico_ok,
				  "[WiFiEvt] got-ip | ip=%s | status=%s (%d) | RSSI=%d",
				  WiFi.localIP().toString().c_str(),
				  wifiStatusToString(status),
				  static_cast<int>(status),
				  WiFi.RSSI());
		requestNtpSync(true);
	}

	if (wifiAuthFailFallbackPending) {
		wifiAuthFailFallbackPending = false;
		if (isWifiRepeaterBssidLocked()) {
			dbgprintf(ico_warning,
					  "[WiFi] repeated AUTH_FAIL (%u) with BSSID lock %s -> enable any-AP fallback and retry",
					  static_cast<unsigned>(wifiAuthFailStreak),
					  getWifiRepeaterBssid());
			gWifiForceAnyApFallback = true;
			wifiAuthFailStreak = 0U;
			wifiAuthFailBackoffUntilMs = 0;
			wifiReconnectAttempts = 0;
			wifiReconnectTimeoutCycles = 0;
			wifiStackRecoveryDone = false;
			wifiReconnectFirstFailMs = now - WIFI_RECONNECT_MIN_FAIL_MS;
			wifiNextBeginAllowedMs = now;
			beginWifiConnect();
			wifiLastBeginMs = now;
			dbgprintln(ico_warning, "[WiFi] retrying with any AP of same SSID (BSSID lock kept for later)");
		} else {
			dbgprintf(ico_warning,
					  "[WiFi] repeated AUTH_FAIL (%u) without BSSID lock -> check SSID/password/AP auth policy",
					  static_cast<unsigned>(wifiAuthFailStreak));
		}
	}

	// connected again: clear reconnect state
	if (status == WL_CONNECTED)
	{
		if (wifiReconnectFirstFailMs != 0)
		{
			logWifiStatus("reconnected");
		}
		wifiReconnectFirstFailMs = 0;
		wifiLastBeginMs = 0;
		wifiNextBeginAllowedMs = 0;
		wifiReconnectTimeoutCycles = 0;
		wifiReconnectAttempts = 0;
		wifiStackRecoveryDone = false;
		wifiAuthFailStreak = 0U;
		wifiAuthFailFallbackPending = false;
		wifiAuthFailBackoffUntilMs = 0;
		gWifiForceAnyApFallback = false;
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

	// Start a new connect attempt only after minimum failure age and backoff window.
	if (shouldAttemptWifiBegin(status, now))
	{
		if (wifiAuthFailBackoffUntilMs != 0 && static_cast<long>(now - wifiAuthFailBackoffUntilMs) < 0L) {
			const unsigned long remaining = wifiAuthFailBackoffUntilMs - now;
			static unsigned long lastAuthBackoffLogMs = 0;
			if ((now - lastAuthBackoffLogMs) >= 10000UL) {
				lastAuthBackoffLogMs = now;
				dbgprintf(ico_warning,
						  "[WiFi] AUTH_FAIL backoff active: wait %lu ms before next begin",
						  remaining);
			}
			return;
		}

		const unsigned long disconnectedForMs = (wifiReconnectFirstFailMs == 0) ? 0UL : (now - wifiReconnectFirstFailMs);
		const unsigned long backoffMs = computeWifiBeginBackoffMs(wifiReconnectAttempts);
		dbgprintf(ico_info,
				  "[WiFi] begin attempt=%u status=%s disconnected_for=%lu ms backoff=%lu ms",
				  static_cast<unsigned>(wifiReconnectAttempts + 1U),
				  wifiStatusToString(status),
				  disconnectedForMs,
				  backoffMs);
		beginWifiConnect();
		wifiLastBeginMs = now;
		wifiNextBeginAllowedMs = now + backoffMs;
		if (wifiReconnectAttempts < 10U) {
			wifiReconnectAttempts++;
		}
		counter++;
		dbgprintf(ico_info,
				  "[WiFi] next reconnect try in %lu ms",
				  static_cast<unsigned long>(wifiNextBeginAllowedMs - now));
	}

	yield();
	//delay(10);

	// Stage 1: hard reset WiFi stack once after 5 minutes disconnect.
	if (!wifiStackRecoveryDone && (now - wifiReconnectFirstFailMs) >= WIFI_STACK_RECOVERY_MS)
	{
		dbgprintf(ico_warning,
				  "[WiFi] stage1 recovery after %lu ms disconnect: hard reset WiFi stack",
				  static_cast<unsigned long>(now - wifiReconnectFirstFailMs));
		persistPlannedRestartMarker("wifi-stack-recovery");
		WiFi.disconnect(true);
		delay(200);
		WiFi.mode(WIFI_OFF);
		delay(200);
		WiFi.mode(WIFI_STA);
#if WIFI_DISABLE_POWERSAVE
		WiFi.setSleep(false);
#endif
		beginWifiConnect();
		wifiLastBeginMs = now;
		wifiReconnectAttempts = 1;
		wifiNextBeginAllowedMs = now + computeWifiBeginBackoffMs(wifiReconnectAttempts);
		wifiStackRecoveryDone = true;
		wifiReconnectFirstFailMs = now;
		wifiReconnectTimeoutCycles = 0;
		return;
	}

	// very last resort, with long timeout
	if ((now - wifiReconnectFirstFailMs) >= WIFI_RECONNECT_TIMEOUT_MS)
	{
		wifiReconnectTimeoutCycles++;
		if (wifiReconnectTimeoutCycles >= WIFI_TIMEOUT_CYCLES_BEFORE_RESTART)
		{
			logWifiStatus("reconnect timeout reached repeatedly - restarting");
			persistPlannedRestartMarker("wifi-reconnect-timeout-post-stage1");
			delay(100);
			ESP.restart();
		}
		else
		{
			dbgprintf(ico_warning,
					  "[WiFi] reconnect timeout cycle %u/%u, keeping gateway alive",
					  static_cast<unsigned>(wifiReconnectTimeoutCycles),
					  static_cast<unsigned>(WIFI_TIMEOUT_CYCLES_BEFORE_RESTART));
			wifiReconnectFirstFailMs = now;
		}
	}
}

//=====================================================================
#pragma endregion
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
#if WIFI_DISABLE_POWERSAVE
	WiFi.setSleep(false);
#endif

	delay(10);
	// We start by connecting to a WiFi network
	dbgprintln();
	dbgprintf(ico_ok, "Connecting to %s", MY_WIFI_SSID);

	WiFi.mode(WIFI_STA);
	beginWifiConnect();

	int counter = 0;
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		if (++counter > 100)
		{
			persistPlannedRestartMarker("setup-wifi-timeout");
			ESP.restart();
		}
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
		(void)err_buf;
		Serial.printf_P(PSTR("Pushover: connection failed. Could not connect to api.pushover.net:443.\n"));
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
//---------------------------------------------------------------------
#pragma endregion

#pragma region datetime / midnight handling
//////////////////////////////////////////////////////////////////////////////////////////////////
//
static time_t nextMidnightEpoch = 0;
static int lastMidnightResetYear = -1;
static int lastMidnightResetYday = -1;

static bool scheduleNextMidnightFrom(time_t nowEpoch)
{
	struct tm localNow;
	if (localtime_r(&nowEpoch, &localNow) == nullptr) {
		return false;
	}
	localNow.tm_hour = 0;
	localNow.tm_min = 0;
	localNow.tm_sec = 0;
	localNow.tm_mday += 1;
	const time_t nextEpoch = mktime(&localNow);
	if (nextEpoch <= 0) {
		return false;
	}
	nextMidnightEpoch = nextEpoch;
	return true;
}

boolean checkMidnight()
{
	if (!bootTimeSynced) {
		return false;
	}

	const time_t nowEpoch = time(nullptr);
	if (nowEpoch <= 0) {
		return false;
	}

	if (nextMidnightEpoch == 0) {
		(void)scheduleNextMidnightFrom(nowEpoch);
		return false;
	}

	if (nowEpoch < nextMidnightEpoch) {
		return false;
	}

	struct tm localNow;
	if (localtime_r(&nowEpoch, &localNow) == nullptr) {
		return false;
	}

	// Prevent duplicate resets if time jumps around midnight.
	if (localNow.tm_year == lastMidnightResetYear && localNow.tm_yday == lastMidnightResetYday) {
		(void)scheduleNextMidnightFrom(nowEpoch);
		return false;
	}

	lastMidnightResetYear = localNow.tm_year;
	lastMidnightResetYday = localNow.tm_yday;
	(void)scheduleNextMidnightFrom(nowEpoch);
	return true;
}

void loop_NTP()
{
	static unsigned long lastNtpCheckMs = 0;
	static unsigned long lastDailySyncMs = 0;
	const unsigned long nowMs = millis();

	// Run lightweight NTP scheduling every 5s instead of every loop.
	if ((nowMs - lastNtpCheckMs) < 5000UL) {
		return;
	}
	lastNtpCheckMs = nowMs;

	if (checkMidnight())
	{
		// reset all counter on midnight
		resetDailyStats();
	}

	if (!bootTimeSynced || WiFi.status() != WL_CONNECTED) {
		return;
	}

	if (lastDailySyncMs == 0) {
		lastDailySyncMs = nowMs;
		return;
	}

	// Daily NTP refresh once every 24h while connected.
	if ((nowMs - lastDailySyncMs) >= 86400000UL) {
		requestNtpSync(true);
		lastDailySyncMs = nowMs;
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

#ifdef WITH_WEB_DEBUG
void indication(const indication_t indicator)
{
	if (lockStats()) {
		lastIndicatorCode = static_cast<int>(indicator);
		lastIndicatorMs = millis();
		switch (indicator)
		{
		case INDICATION_GW_TX:
			gatewayTxMessage++;
			rxtxStats.nGwTx++;
			break;

		case INDICATION_GW_RX:
			gatewayRxMessage++;
			rxtxStats.nGwRx++;
			break;

		case INDICATION_TX:
			sensorTxMessage++;
			rxtxStats.nTx++;
			break;

		case INDICATION_RX:
			sensorRxMessage++;
			rxtxStats.nRx++;
			break;

		case INDICATION_ERR_TX:
			rxtxStats.nErr++;
			break;

		default:
			break;
		};

		if (indicator >= 101 && indicator <= 116)
		{
			indicatorTxErrors++;
		}
		unlockStats();
	} else {
		lastIndicatorCode = static_cast<int>(indicator);
		lastIndicatorMs = millis();
		switch (indicator)
		{
		case INDICATION_GW_TX:
			gatewayTxMessage++;
			rxtxStats.nGwTx++;
			break;

		case INDICATION_GW_RX:
			gatewayRxMessage++;
			rxtxStats.nGwRx++;
			break;

		case INDICATION_TX:
			sensorTxMessage++;
			rxtxStats.nTx++;
			break;

		case INDICATION_RX:
			sensorRxMessage++;
			rxtxStats.nRx++;
			break;

		case INDICATION_ERR_TX:
			rxtxStats.nErr++;
			break;

		default:
			break;
		};

		if (indicator >= 101 && indicator <= 116)
		{
			indicatorTxErrors++;
		}
	}

	const bool webDebugEnabled = isWebDebugEnabled();
	if (webDebugEnabled) {
		send_Event((indicator == INDICATION_GW_TX || indicator == INDICATION_TX) ? "&#128994;" :
				  (indicator == INDICATION_GW_RX || indicator == INDICATION_RX || indicator == INDICATION_ERR_TX) ? "&#128308;" : "&#128993;",
				  "led");
	}

	if (!webDebugEnabled) {
		return;
	}

	const char *mysIndication = "";
	if (indicator <= 18)
	{
		mysIndication = mysIndicationErrorCodes0[indicator];
	}
	else if (indicator >= 101 && indicator <= 116)
	{
		mysIndication = mysIndicationErrorCodes100[indicator - 101];
	}
	else
	{
		mysIndication = "Unknown indication";
	}
	const SharedStatsSnapshot s = snapshotStats();

	char msgbuf[192] = {'\0'};
	if (snprintf(msgbuf, sizeof(msgbuf),
				 "%s | gateway: rx: %lu - tx: %lu  | sensors: rx: %lu - tx: %lu  | err: %lu <br />",
				 mysIndication,
				 s.gatewayRxMessage,
				 s.gatewayTxMessage,
				 s.sensorRxMessage,
				 s.sensorTxMessage,
				 s.indicatorTxErrors) < 0)
	{
		strcpy(msgbuf, "<div class=\"error\">error</div>");
	}
	send_Event(msgbuf, "indicator");
	// dbgprintln(ico_info, msgbuf);

	dbgprintf(ico_info,
			  "[INDICATION] code=%d | text=%s | gw(rx=%lu tx=%lu) sensor(rx=%lu tx=%lu) errors=%lu | wifi=%s (%d)",
			  static_cast<int>(indicator),
			  mysIndication,
			  s.gatewayRxMessage,
			  s.gatewayTxMessage,
			  s.sensorRxMessage,
			  s.sensorTxMessage,
			  s.indicatorTxErrors,
			  wifiStatusToString(WiFi.status()),
			  static_cast<int>(WiFi.status()));
}
#endif


//=====================================================================
#pragma region MySensors setup
//////////////////////////////////////////////////////////////////////////////////////////////////
//
void setup()
{
	gResetReasonCode = getResetReasonCode();
	captureResetReasonRaw();
	applyTimeZone();
#if WIFI_DISABLE_POWERSAVE
	WiFi.setSleep(false);
#endif
	gatewayFsBegin();
	loadWifiRepeaterBssidFromStorage();
	loadWifiAuthFailBackoffFromStorage();
	loadWifiAuthFailBackoffThresholdFromStorage();
	loadBootLogMaxEntriesFromStorage();

#ifdef MY_DEBUG
	Serial.begin(MY_BAUD_RATE);
	while (!Serial)
	{
	} // Wait

	Serial.println("---------- begin setup()");
	Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
	Serial.printf("SW version: %s\n", __SWVERSION__);
	
	Serial.printf("Reset reason code: %u\n", static_cast<unsigned>(gResetReasonCode));
	Serial.printf("Reset reason text: %s\n", getResetReasonText(gResetReasonCode));
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

	if (!gStatsMutex) {
		gStatsMutex = xSemaphoreCreateMutex();
	}

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
	gatewayFsBegin();// Filesystem mounten
	initBootCounter();
	consumeLastRestartMarker();

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
	updateHealthSnapshot(bootCount,
							 lastBootEpoch,
							 gResetReasonCode,
							 getResetReasonText(gResetReasonCode),
							 gResetReasonRaw,
							 gLastRestartMarker,
							 bootTimeSynced);
#endif

	// start main loop task on core 0 with higher priority than default
	// to ensure responsive handling of WiFi and other events
	// stack size is set to 12KB to accommodate potential needs of OTA
	// and web server handling
	// pinning to core 0 allows core 1 to be more available for background tasks
	// and WiFi handling
	// Note: if using MY_CORE_ONLY, the main loop runs on the default core
	// and this task is not created
	// The main loop task will yield to other tasks as needed, so it should not starve them
	// The main loop task will also handle the main MySensors loop and related processing
	// This setup allows for better multitasking and responsiveness while maintaining the main loop's functionality
	xTaskCreatePinnedToCore(taskSystemCore,
						  "GatewaySystemCore",
						  12288,
						  nullptr,
						  1,
						  &gSystemTaskHandle,
						  0);
}
//---------------------------------------------------------------------
#pragma endregion


//=====================================================================
#pragma region MySensors loop
//////////////////////////////////////////////////////////////////////////////////////////////////
//
static void loopSystemCore()
{
	beatSystemTask();
	checkTaskHealthWatchdog();

	avgCpuDelta = getCpuDelta();
#ifdef WWW
	const bool webDebugOn = isWebDebugEnabled();
	const bool webLiveUiOn = isWebLiveUiEnabled();
#else
	const bool webDebugOn = false;
	const bool webLiveUiOn = false;
#endif

	delay(10); // https://github.com/espressif/esp-idf/issues/1021
	yield();

	loop_Wifi();

#ifdef WWW
	loop_WebServer();
#endif // WWW

#ifdef TELNET
	if (isTelnetRuntimeEnabled()) {
		loop_Telnet(); // handle telnet server
	}
#endif

#ifdef NTP
	loop_NTP();
	if (!bootTimeSynced) {
		requestNtpSync();
	}
#endif // NTP

#ifdef WWW
#ifdef WITH_MQTT
	{
		static unsigned long lastCtrlStatusProbeMs = 0;
		static unsigned long lastCtrlStatusSendMs = 0;
		static bool ctrlStatusInitialized = false;
		static bool lastCtrlStatus = false;
		const unsigned long nowMs = millis();
		if ((nowMs - lastCtrlStatusProbeMs) >= 5000UL) {
			lastCtrlStatusProbeMs = nowMs;
			const bool controllerUpNow = isTransportReady();
			if (!ctrlStatusInitialized ||
				controllerUpNow != lastCtrlStatus ||
				(nowMs - lastCtrlStatusSendMs) >= 30000UL) {
				lastCtrlStatus = controllerUpNow;
				ctrlStatusInitialized = true;
				lastCtrlStatusSendMs = nowMs;
				send_Event(controllerUpNow ? "1" : "0", "ctrlstatus");
			}
		}
	}
#endif
#endif

#ifdef OTA
	if (isOtaRuntimeEnabled() && WiFi.status() == WL_CONNECTED) {
		const unsigned long nowMs = millis();
		if (!gOtaUpdateInProgress &&
			gOtaWindowUntilMs != 0 &&
			static_cast<long>(nowMs - gOtaWindowUntilMs) >= 0L) {
			gOtaRuntimeEnabled = false;
			gOtaWindowUntilMs = 0;
			dbgprintln(ico_info, "[OTA] runtime window expired");
		}
		if (isOtaRuntimeEnabled() && (nowMs - gLastOtaHandleMs) >= OTA_HANDLE_INTERVAL_MS) {
			gLastOtaHandleMs = nowMs;
			ArduinoOTA.handle(); // throttled handling
		}
	}
#endif // OTA

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
		const SharedStatsSnapshot stats = snapshotStats();
#ifdef WWW
		updateHealthSnapshot(bootCount,
							 lastBootEpoch,
							 gResetReasonCode,
							 getResetReasonText(gResetReasonCode),
							 gResetReasonRaw,
							 gLastRestartMarker,
							 bootTimeSynced);
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

		if (webDebugOn) {
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
			buf[0] = {'\0'};
		}
		timestamp[0] = {'\0'};

		const bool controllerUp = isTransportReady();
#ifdef WITH_MQTT
		const char *controllerType = "mqtt";
#else
		const char *controllerType = "gateway";
#endif

		if (webDebugOn) {
			char telem[320];
			buildTelemetryJson(telem, sizeof(telem),
				heap,
				getHeapFragmentationPct(),
				getMaxFreeBlockBytes(),
				WiFi.RSSI(),
				wifiStatusToString(WiFi.status()),
				stats.lastIndicatorCode,
				stats.gatewayRxMessage,
				stats.gatewayTxMessage,
				stats.sensorRxMessage,
				stats.sensorTxMessage,
				stats.indicatorTxErrors,
				controllerUp,
				controllerType);
			send_Event(telem, "telemetry");
		}

		static unsigned long lastControllerDiagMs = 0;
		if (millis() - lastControllerDiagMs > 60000UL) // alle 60 sekunden
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
					  stats.lastIndicatorCode,
					  (stats.lastIndicatorMs == 0 ? 0UL : millis() - stats.lastIndicatorMs));
		}

		static unsigned long lastInfoSendMs = 0;
		if (/*webLiveUiOn && */millis() - lastInfoSendMs >= 1000UL) {
			lastInfoSendMs = millis();
			updateWebStats();
		}

	}

}

static void loopMySensorsCore()
{
	beatMySensorsTask();

#ifdef WWW
	const bool webDebugOn = isWebDebugEnabled();
#else
	const bool webDebugOn = false;
#endif

	// interval based jobs
	if (millis() - gw_send_prev_time > gw_send_interval)
	{
		gw_send_prev_time = millis();

		sendHeartbeat();
		send(msgUptime.set(uptime()));

		// every now and then, report ARC statistics ("pseudo-RSSI")
		if (webDebugOn) {
			const char* arc = reportArcStatistics();
			send_Event(arc, "debug");
		}
	}

	delay(2);
	yield();
}

static void taskSystemCore(void *pvParameters)
{
	(void)pvParameters;
	for (;;) {
		loopSystemCore();
		vTaskDelay(pdMS_TO_TICKS(2));
	}
}

void loop()
{
	loopMySensorsCore();
}
//---------------------------------------------------------------------
#pragma endregion

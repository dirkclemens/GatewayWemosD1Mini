/*
 *  https://www.mysensors.org/download/serial_api_20
 *  MyMessage.h
 * 
 */

const char *mysCommandCodes[]       = {"C_PRESENTATION", "C_SET", "C_REQ", "C_INTERNAL", "C_STREAM", "C_RESERVED_5", "C_RESERVED_6", "C_INVALID_7"};
const char *mysPresenationCodes[]   = {"S_DOOR", "S_MOTION", "S_SMOKE", "S_BINARY", "S_DIMMER", "S_COVER", "S_TEMP", "S_HUM", "S_BARO", "S_WIND", "S_RAIN", "S_UV", "S_WEIGHT", "S_POWER", "S_HEATER", "S_DISTANCE", "S_LIGHT_LEVEL", "S_ARDUINO_NODE", "S_ARDUINO_REPEATER_NODE", "S_LOCK", "S_IR", "S_WATER", "S_AIR_QUALITY", "S_CUSTOM", "S_DUST", "S_SCENE_CONTROLLER", "S_RGB_LIGHT", "S_RGBW_LIGHT", "S_COLOR_SENSOR", "S_HVAC", "S_MULTIMETER", "S_SPRINKLER", "S_WATER_LEAK", "S_SOUND", "S_VIBRATION", "S_MOISTURE", "S_INFO", "S_GAS", "S_GPS", "S_WATER_QUALITY"};
const char *mysSetReqCodes[]        = {"V_TEMP", "V_HUM", "V_STATUS", "V_PERCENTAGE", "V_PRESSURE", "V_FORECAST", "V_RAIN", "V_RAINRATE", "V_WIND", "V_GUST", "V_DIRECTION", "V_UV", "V_WEIGHT", "V_DISTANCE", "V_IMPEDANCE", "V_ARMED", "V_TRIPPED", "V_WATT", "V_KWH", "V_SCENE_ON", "V_SCENE_OFF", "V_HVAC_FLOW_STATE", "V_HVAC_SPEED", "V_LIGHT_LEVEL", "V_VAR1", "V_VAR2", "V_VAR3", "V_VAR4", "V_VAR5", "V_UP", "V_DOWN", "V_STOP", "V_IR_SEND", "V_IR_RECEIVE", "V_FLOW", "V_VOLUME", "V_LOCK_STATUS ", "V_LEVEL", "V_VOLTAGE", "V_CURRENT", "V_RGB", "V_RGBW", "V_ID", "V_UNIT_PREFIX", "V_HVAC_SETPOINT_COOL", "V_HVAC_SETPOINT_HEAT", "V_HVAC_FLOW_MODE", "V_TEXT", "V_CUSTOM", "V_POSITION", "V_IR_RECORD", "V_PH ", "V_ORP", "V_EC", "V_VAR", "V_VA", "V_POWER_FACTOR"};
const char *mysSetReqUnits[]        = {"°C", "%", " ", "%", "mB", "forecast", "rain", "rainrate", "wind", "gust", "°", " ", "kg", "m", "ohm", " ", " ", "W", "kwh", " ", " ", " ", " ", "lux", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", "m", "m³", "  ", " ", "V", "A", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", "ph ", "mV", "μS/cm", "var", "va", " "};
const char *mysInternalCodes[]      = {"I_BATTERY_LEVEL", "I_TIME", "I_VERSION", "I_ID_REQUEST", "I_ID_RESPONSE", "I_INCLUSION_MODE", "I_CONFIG", "I_FIND_PARENT_REQUEST", "I_FIND_PARENT_RESPONSE", "I_LOG_MESSAGE", "I_CHILDREN", "I_SKETCH_NAME", "I_SKETCH_VERSION", "I_REBOOT", "I_GATEWAY_READY", "I_SIGNING_PRESENTATION", "I_NONCE_REQUEST", "I_NONCE_RESPONSE", "I_HEARTBEAT_REQUEST", "I_PRESENTATION", "I_DISCOVER_REQUEST", "I_DISCOVER_RESPONSE", "I_HEARTBEAT_RESPONSE", "I_LOCKED ", "I_PING", "I_PONG", "I_REGISTRATION_REQUEST", "I_REGISTRATION_RESPONSE", "I_DEBUG", "I_SIGNAL_REPORT_REQUEST", "I_SIGNAL_REPORT_REVERSE", "I_SIGNAL_REPORT_RESPONSE", "I_PRE_SLEEP_NOTIFICATION", "I_POST_SLEEP_NOTIFICATION"};
const char *mysStreamCodes[]        = {"ST_FIRMWARE_CONFIG_REQUEST", "ST_FIRMWARE_CONFIG_RESPONSE", "ST_FIRMWARE_REQUEST", "ST_FIRMWARE_RESPONSE", "ST_SOUND", "ST_IMAGE", "ST_FIRMWARE_CONFIRM", "ST_FIRMWARE_RESPONSE_RLE"};


const char *mysIndicationErrorCodes0[] = {
    "Sent a message.", 
    "Received a message.",
    "Gateway transmit message.", 
    "Gateway receive message.",
    "Start finding parent node.", 
    "Found parent node.",
    "Request node ID.", 
    "Got a node ID.", 
    "Check uplink", 
    "Request node registration.", 
    "Got registration response.", 
    "Rebooting node.", 
    "Presenting node to gateway.", 
    "Clear routing table requested.", 
    "Node goes to sleep.", 
    "Node just woke from sleep.", 
    "Start of OTA firmware update process.", 
    "Received a piece of firmware data.", 
    "Received wrong piece of firmware data."};

const char *mysIndicationErrorCodes100[] = {
	"HW initialization error", 
	"Failed to transmit message.", 
	"Transport failure.", 
	"MySensors transport hardware (radio) init failure.", 
	"Failed to find parent node.", 
	"Failed to receive node ID.", 
	"Failed to check uplink", 
	"Error signing.", 
	"Invalid message length.", 
	"Protocol version mismatch.", 
	"Network full. All node ID's are taken.", 
	"Gateway transport hardware init failure.", 
	"Node is locked.", 
	"Firmware update flash initialisation failure.", 
	"Firmware update timeout.", 
	"Firmware update checksum mismatch."};
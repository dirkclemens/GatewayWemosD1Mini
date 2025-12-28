#pragma once

/* includes */
#include <Arduino.h>
#include <pgmspace.h>               // for flash constants to save ram

// #define __SWVERSION__       "2.0.10"
#define __SWVERSION__       "2.4.1" // HA

#define OTA
#define WWW
#define WWW_PORT            80
#define TELNET
#define TELNET_PORT         23
#define NTP
#define PUSHOVER

// https://fipsok.de/Esp8266-Webserver/ntp-zeit-esp8266.tab
// enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
const char *const PROGMEM TZ_INFO = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
const char *const PROGMEM NTP_SERVER[] = {"fritz.box", "de.pool.ntp.org", "ptbtime1.ptb.de", "europe.pool.ntp.org"};

/** FLASH constants, to save on RAM **/
static const PROGMEM char EOL[] = "\r\n";
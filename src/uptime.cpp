#ifndef UPTIME_H_INCLUDED
#define UPTIME_H_INCLUDED
/*
 *
 */
#include "uptime.h"
#include "common.h"

char bootTime[21] = {'\0'}; // Aug 02 2020 12:34:56 => 20 + \0

char *runtime()
{
	static uint8_t rolloverCounter{0};
	static uint32_t previousMillis{0};
	uint32_t currentMillis{millis()};
	if (currentMillis < previousMillis)
		rolloverCounter++; // prüft Millis Überlauf
	previousMillis = currentMillis;
	uint32_t sek{(0xFFFFFFFF / 1000) * rolloverCounter + (currentMillis / 1000)};
	static char buf[20];
	snprintf(buf, sizeof(buf), "%d day%s %02d:%02d:%02d", sek / 86400, sek < 86400 || sek >= 172800 ? "e" : "", sek / 3600 % 24, sek / 60 % 60, sek % 60);
	return buf;
}

/* 
 *
 */ 
void setBootTime(){
    struct tm btm;
	time_t now = time(&now);
	localtime_r(&now, &btm);
	strftime(bootTime, sizeof(bootTime), "%b %d %Y %H:%M:%S", &btm); // http://www.cplusplus.com/reference/ctime/strftime/
	dbgprintf(ico_info, "boot time: %s", bootTime);
}

/* 
 *
 */ 
String getBootTime(){
    return (bootTime);
}


/*
 * Function made to millis() be an optional parameter
 * https://forum.arduino.cc/index.php?topic=71212.0
 */
char *uptime() { 
    return (char *)uptime(millis()); // call original uptime function with unsigned long millis() value
}
static unsigned int  _rolloverCount   = 0;      // Number of 0xFFFFFFFF rollover we had in millis()
static unsigned long _lastMillis      = 0;      // Value of the last millis() 
static unsigned long _incMillis      = 0;       // Value of the last millis() 
/* 
 *
 */ 
char *uptime(unsigned long milli){
    // If we had a rollover we count that.
    if (milli < _lastMillis) {
        _rolloverCount++;
        _incMillis++;
    }
    // Now store the current number of milliseconds for the next round.
    _lastMillis = milli;    
    // Based on the current milliseconds and the number of rollovers
    // we had in total we calculate here the uptime in seconds since 
    // poweron or reset.
    // Caution: Because we shorten millis to seconds we may miss one 
    // second for every rollover (1 second every 50 days).
    unsigned long secs = (0xFFFFFFFF / 1000 ) * _rolloverCount + (_lastMillis / 1000)  + _incMillis;  
    
    static char _result[24];
    //unsigned long secs = milli / 1000, 
    unsigned long mins = secs / 60;
    unsigned int hours = mins / 60, days = hours / 24 ;
    milli -= secs * 1000;
    secs -= mins * 60;
    mins -= hours * 60;
    hours -= days * 24;
    sprintf(_result, "%d days %2.2d:%2.2d:%2.2d", (int)days, (byte)hours, (byte)mins, (byte)secs);
    return _result;
}

#endif // UPTIME_H_INCLUDED
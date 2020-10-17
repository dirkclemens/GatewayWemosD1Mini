#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED
/*
 *
 */
#include "common.h"

// const
const char *const PROGMEM flashChipMode[] = {"QIO", "QOUT", "DIO", "DOUT", "Unbekannt"};

/* 
 *
 */
void getCurrentTimeString(char *timestamp, const char *format)
{
	struct tm tm;
	static char _timestr[21] = {'\0'}; // Aug 02 2020 12:34:56 => 20 + \0
	// static time_t lastsec = 0;
	time_t now = time(&now);
	localtime_r(&now, &tm);
	// strftime(timestr, sizeof(timestr), "%Y.%m.%d %H:%M:%S", &tm); // http://www.cplusplus.com/reference/ctime/strftime/
	strftime(_timestr, sizeof(_timestr), format, &tm); // http://www.cplusplus.com/reference/ctime/strftime/
	strcpy(timestamp, _timestr);
}

// https://www.w3schools.com/charsets/ref_utf_dingbats.asp
void dbgprintf(dbgIcons icon, const char *format, ...)
{
#ifdef MY_DEBUG
	static char sbuf[1024] = {'\0'}; // For debug lines
	va_list varArgs;								// For variable number of params
	va_start(varArgs, format);						// Prepare parameters
	vsnprintf(sbuf, sizeof(sbuf), format, varArgs); // Format the message
	va_end(varArgs);								// End of using parameters
	static char uniicon[5];
	switch (icon)
	{
	case ico_null:
		strcpy(uniicon, " ");
		break;
	case ico_ok:
		strcpy(uniicon, "\u2705 ");
		break;
	case ico_error:
		strcpy(uniicon, "\u274C ");
		break;
	case ico_warning:
		strcpy(uniicon, "\u2757 ");
		break;
	case ico_info:
		strcpy(uniicon, "\u2771 ");
		break;
	case ico_arrow:
		strcpy(uniicon, "\u2794 ");
		break;
	case ico_dot:
		strcpy(uniicon, "\u2024 ");
		break;
	default:
		strcpy(uniicon, " ");
	}

	char _timestr[24] = {'\0'};
	getCurrentTimeString(_timestr, "%Y.%m.%d %H:%M:%S  -  ");
	Serial.print(_timestr);
	Serial.print(uniicon);
	Serial.println(sbuf); // and the info
#endif
}

void dbgprint(dbgIcons icon, const char *textbuf)
{
#ifdef MY_DEBUG
	static char sbuf[1024] = {'\0'}; // For debug lines
	strcpy(sbuf, textbuf);
	static char uniicon[5];
	switch (icon)
	{
	case ico_null:
		strcpy(uniicon, " ");
		break;
	case ico_ok:
		strcpy(uniicon, "\u2705 ");
		break;
	case ico_error:
		strcpy(uniicon, "\u274C ");
		break;
	case ico_warning:
		strcpy(uniicon, "\u2757 ");
		break;
	case ico_info:
		strcpy(uniicon, "\u2771 ");
		break;
	case ico_arrow:
		strcpy(uniicon, "\u2794 ");
		break;
	case ico_dot:
		strcpy(uniicon, "\u2024 ");
		break;
	default:
		strcpy(uniicon, " ");
	}

	char _timestr[24] = {'\0'};
	getCurrentTimeString(_timestr, "%Y.%m.%d %H:%M:%S  -  ");
	Serial.print(_timestr);
	Serial.print(uniicon);
	Serial.print(sbuf); // and the info
#endif
}

void dbgprintln(dbgIcons icon, const char *textbuf)
{
#ifdef MY_DEBUG
	dbgprint(icon, textbuf);
	Serial.println(); // and the info
#endif
}

void dbgprintln()
{
#ifdef MY_DEBUG
	Serial.println();
#endif
}

const String formatBytes(size_t const &bytes)
{ // lesbare Anzeige der Speichergrößen
	return (bytes < 1024) ? String(bytes) + " Byte" : (bytes < (1024 * 1024)) ? String(bytes / 1024.0) + " KB" : String(bytes / 1024.0 / 1024.0) + " MB";
}

char *ftoa(char *a, double f, int precision)
{
	long p[] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
	char *ret = a;
	long heiltal = (long)f;
	itoa(heiltal, a, 10);
	while (*a != '\0')
		a++;
	*a++ = '.';
	long desimal = abs((long)((f - heiltal) * p[precision]));
	itoa(desimal, a, 10);
	return ret;
} // ftoa()

#endif // COMMON_H_INCLUDED
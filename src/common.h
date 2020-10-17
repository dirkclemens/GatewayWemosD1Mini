/*
 *
 */
#include <Arduino.h>
#include "config.h"

// https://www.w3schools.com/charsets/ref_utf_dingbats.asp
enum dbgIcons {
	ico_null,
	ico_ok,	 		// \u2705   ✅
	ico_error,		// \u274C   ❌
					// \u274E   ❎
	ico_warning, 	// \u2757   ❗ 
					// \u275B   ❛
	ico_info, 		// \u2771   ❱
	ico_arrow,		// \u2794   ➔
					// \u26AA   ⚪
					// \u26AB   ⚫
	ico_dot			// \u2024
};

// utilities
void        dbgprint( dbgIcons icon, const char* textbuf );
void        dbgprintln( );
void        dbgprintln( dbgIcons icon, const char* textbuf );
void        dbgprintf( dbgIcons icon, const char* format, ... );
const String formatBytes(size_t const &bytes);
char 		*ftoa(char *a, double f, int precision);
void		getCurrentTimeString(char *timestamp, const char *format);

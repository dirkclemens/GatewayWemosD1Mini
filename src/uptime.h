#pragma once

#include <Arduino.h>

char *runtime();
char *uptime(); 
char *uptime(unsigned long milli);
void setBootTime();
String getBootTime();

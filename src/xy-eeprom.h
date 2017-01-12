

#ifndef _xy_eeprom
#define _xy_eeprom

#include <EEPROM.h>

#include <ESPAsyncWebServer.h>

#define EEPROM_BYTES_OFS      68
#define EEPROM_BYTES_PER_SSID 70
#define EEPROM_TOTAL_BYTES    348

extern char ap_ssid[33];
extern char ap_pwd[33];

void initeeprom();
int eepromGetStr(char* res, int idx);
int eepromGetIP(IPAddress res, int idx);
int eepromPutStr(const char* str, int idx);
int eepromPutIP(const char* ipStr, int idx);

#endif

/* eeprom map
  2  Magic = 0xedde
  33 AP ssid name
  33 AP password
	4 STA choices (70 each)
		 33 ssid
		 33 pwd
		 4  static ip
	348 bytes total
*/

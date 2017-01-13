

#ifndef _xy_eeprom
#define _xy_eeprom

/* eeprom map
  2  Magic = 0xedde
  33 AP ssid name
  33 AP password
	4 STA choices (82 each)
		 33 ssid
		 33 pwd
		 16 static ip (as str)
	396 bytes total
*/
#define EEPROM_BYTES_OFS      68
#define EEPROM_BYTES_PER_SSID 82
#define EEPROM_TOTAL_BYTES    396

void initeeprom();
int eepromGetStr(char* res, int idx);
int eepromGetIp(char* res, int idx);
int eepromPutStr(const char* str, int idx);
int eepromPutIp(const char* ipStr, int idx);

#endif

#include <EEPROM.h>

#include <Arduino.h>
#include <os_type.h>
#include <ESPAsyncWebServer.h>

#include "IPAddress.h"
#include "xy-eeprom.h"
#include "xy-wifi.h"

void initeeprom() {
	char buf[33];
	int bufidx, eeAddr;
	String ap_ssid_str;
  EEPROM.begin(512);
	if (EEPROM.read(0) != 0xe7 || EEPROM.read(1) != 0xde) {
    Serial.println("initializing empty eeprom, magic was: " +
                    String(EEPROM.read(0),HEX) + String(EEPROM.read(1),HEX));
		EEPROM.write(0, 0xe7);
		EEPROM.write(1, 0xde);
		for (eeAddr=2; eeAddr < EEPROM_TOTAL_BYTES; eeAddr++) EEPROM.write(eeAddr, 0);
    EEPROM.end();
	  ap_ssid_str = "eridien_XY_" + String(ESP.getChipId(), HEX);
		ap_ssid_str.toCharArray(buf, 33);
    eepromPutStr(buf, 2);
    eepromPutStr("eridienxy", 35);
	}
  EEPROM.end();
}

int eepromGetStrWLen(char* res, int idx, int len){
  EEPROM.begin(512);
  int i = 0; char chr;
	do { chr = EEPROM.read(idx+i); res[i] = chr; i++; } while (chr);
  EEPROM.end();
	return idx+len;
}
int eepromGetStr(char* res, int idx){
  return eepromGetStrWLen(res, idx, 33);
}
int eepromGetIP(char* res, int idx){
  return eepromGetStrWLen(res, idx, 16);
}
int eepromPutStrLen(const char* str, int idx, int len){
  EEPROM.begin(512);
  int i = 0; char chr;
	do {EEPROM.write(idx+i, chr=str[i]); i++;} while (chr);
  EEPROM.end();
	return idx+len;
}
int eepromPutStr(const char* ipStr, int idx) {
  return eepromPutStrLen(ipStr, idx, 33);
}
int eepromPutIp(const char* ipStr, int idx) {
  return eepromPutStrLen(ipStr, idx, 16);
}

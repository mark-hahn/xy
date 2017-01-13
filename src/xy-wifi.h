
#ifndef _xy_wifi
#define _xy_wifi

#include <EEPROM.h>

extern char ap_ssid[33];
extern char ap_pwd[33];
extern char sta_ssid[33];
extern char sta_pwd[33];

void find_and_connect_try();
void find_and_connect();

#endif

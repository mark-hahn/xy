#include <EEPROM.h>

#include <ESPAsyncWebServer.h>

#include "xy-wifi.h"
#include "xy-leds.h"
#include "xy-eeprom.h"

char ap_ssid[33];
char ap_pwd[33];
char sta_ssid[33];
char sta_pwd[33];
int best_quality = -1;

void find_and_connect() {
  ledBlink(true);
	int n = WiFi.scanNetworks(), i, j, eepromIdx;
	char eeprom_ssid[33];
	Serial.println(String("\nWifi scan found ") + n + " ssids");
	for(i=0; i<4; i++) {
		eepromIdx = eepromGetStr(eeprom_ssid, EEPROM_BYTES_OFS + i * EEPROM_BYTES_PER_SSID);
		for(j=0; j<n; j++) {
			if(strcmp(WiFi.SSID(j).c_str(), eeprom_ssid) == 0 &&
			      WiFi.encryptionType(j) == ENC_TYPE_NONE) {
        if (WiFi.RSSI(j) > best_quality) {
					strcpy(sta_ssid, eeprom_ssid);
					eepromIdx = eepromGetStr(sta_pwd, eepromIdx);
          best_quality = WiFi.RSSI(j);
				}
			}
		}
	}
	// TODO  REMOVE DEBUG
  if(best_quality == -1) {
    Serial.println("no match found in scan, using debug settings");
		strcpy(sta_ssid, "hahn-fi");
		strcpy(sta_pwd, "NBVcvbasd987");
		best_quality = 101;
	}
  ledBlink(false);
  led_on();
  eepromGetStr(ap_ssid, 2);
  eepromGetStr(ap_pwd, 35);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_pwd);
  Serial.println(String("AP running: ") + ap_ssid);

  ledBlink(true);
	Serial.println(String("Connecting to AP ") + sta_ssid +
                 ", quality " + best_quality);
  WiFi.begin(sta_ssid, sta_pwd);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("STA connection failed");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(sta_ssid, sta_pwd);
  }
	Serial.println(String("AP  address: ") + WiFi.softAPIP().toString());
	Serial.println(String("STA address: ") + WiFi.localIP().toString());
	ledBlink(false);
}

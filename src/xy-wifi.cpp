#include <EEPROM.h>

#include <ESPAsyncWebServer.h>

#include "xy-wifi.h"
#include "xy-leds.h"
#include "xy-eeprom.h"
#include "xy-ajax.h"

char ap_ssid[33];
char ap_pwd[33];
char sta_ssid[33];
char sta_pwd[33];
int best_quality;

void find_and_connect() {
  WiFi.mode(WIFI_AP_STA);
  find_and_connect_try();
}

void find_and_connect_try() {
  connectAfterFormPost = false;
  ledBlink(true);
  eepromGetStr(ap_ssid, 2);
  eepromGetStr(ap_pwd, 35);
  best_quality = -1000;

	int n = WiFi.scanNetworks(), i, j, eepromIdx;
	Serial.println(String("\nWifi scan found ") + n + " ssids");
  char eeprom_ssid[33];
	for(i=0; i<4; i++) {
		eepromIdx = eepromGetStr(eeprom_ssid, EEPROM_BYTES_OFS + i * EEPROM_BYTES_PER_SSID);
    if(eeprom_ssid[0]) {
      for(j=0; j<n; j++) {
  			if(strcmp(WiFi.SSID(j).c_str(), eeprom_ssid) == 0) {
          Serial.println(String("checking ") + eeprom_ssid + ", strength " + WiFi.RSSI(j));
          if (WiFi.RSSI(j) > best_quality) {
  					strcpy(sta_ssid, eeprom_ssid);
  					eepromGetStr(sta_pwd, eepromIdx);
            best_quality = WiFi.RSSI(j);
  				}
  			}
      }
		}
	}
  WiFi.softAP(ap_ssid, ap_pwd);
  Serial.println(String("AP ") + ap_ssid +
                 " running at IP " + WiFi.softAPIP().toString());
  if(best_quality > -1000) {
  	Serial.println(String("Connecting to AP ") + sta_ssid);
    WiFi.disconnect(false);
    WiFi.begin(sta_ssid, sta_pwd);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("STA connection failed");
      WiFi.disconnect(false);
      delay(1000);
      WiFi.begin(sta_ssid, sta_pwd);
    }
    Serial.println(String("STA address: ") + WiFi.localIP().toString());
  } else
    Serial.println("No STA match found in wifi scan");
  ledBlink(false);
}

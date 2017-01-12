#include <EEPROM.h>

#include <ESPAsyncWebServer.h>
#include <ESP8266httpUpdate.h>

#include "xy-eeprom.h"
#include "xy-updates.h"

#define updateServer String("http://hahnca.com/xy/")

AsyncWebServerRequest * firmUpdateReq;
void do_firmware_update(AsyncWebServerRequest *request) {
  firmUpdateReq = (AsyncWebServerRequest *) 0;
	request->send(200, "text/plain", "Started firmware update");
  String url = updateServer + "firmware.bin";
	Serial.println("\nUpdating firmware from " + url);
	delay(1000);
	ESPhttpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return ret = ESPhttpUpdate.update(url);
  switch(ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("Firmware Update Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No firmware Updates");
      break;
    default:
      Serial.println("firmware Updated (you shouldn't see this)");
      break;
  }
}
AsyncWebServerRequest * fsUpdateReq;
void do_spiffs_update(AsyncWebServerRequest *request) {
  fsUpdateReq = (AsyncWebServerRequest *) 0;
	request->send(200, "text/plain", "Started file system update");
  String url = updateServer + "spiffs.bin";
	Serial.println("Updating file system from " + url);
	delay(1000);
	ESPhttpUpdate.rebootOnUpdate(false);
  t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(url);
  switch(ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("FS Update Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No FS Updates");
      break;
    default:
      Serial.println("FS Updated");
      break;
  }
}

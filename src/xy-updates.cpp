#include <EEPROM.h>

#include <ESPAsyncWebServer.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WiFi.h>

#include "xy-updates.h"
#include "xy-eeprom.h"
#include "xy-ajax.h"

#define updateServer String("http://hahnca.com/xy/")

AsyncWebServerRequest * firmUpdateReq;
AsyncWebServerRequest * fsUpdateReq;
AsyncWebServerRequest * mcuUpdateReq;

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
      Serial.printf("Firmware Update Error (%d): %s",
                     ESPhttpUpdate.getLastError(),
                     ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No firmware Updates");
      break;
    default:
      Serial.println("firmware Updated (you shouldn't see this)");
      break;
  }
}

void do_mcu_update(AsyncWebServerRequest *request) {
  mcuUpdateReq = (AsyncWebServerRequest *) 0;

  char mcu = 0xff, numParams = request->params();
  for(char i=0; i < numParams; i++) {
    AsyncWebParameter* p = request->getParam(i);
    if(p->name() == "mcu") mcu = p->value().toInt();
  }
  if(mcu == 0xff) {
    Serial.println(
      String("http request for mcu update missing mcu param: ") + String(request->url()));
    return;
  }
  const char *host = "hahnca.com";
	request->send(200, "text/plain", "Started mcu update");
  Serial.println(String("Started mcu update from ") + request->url());
  WiFiClient client;
  if (!client.connect(host, 80)) {
    Serial.println("connection failed");
    return;
  }
  String url = "/xymcu/xy-mcu.production.hex";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  bool pastHeaders = false;
  while(client.available()){
    String line = client.readStringUntil('\n');
    if(line == "\r") pastHeaders |= true;
    if(pastHeaders) ajaxFlashHexLine(mcu, line.c_str());
  }
}

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

void chkUpdates() {
  if(firmUpdateReq) do_firmware_update(firmUpdateReq);
  if(fsUpdateReq)   do_spiffs_update(fsUpdateReq);
  if(mcuUpdateReq)  do_mcu_update(mcuUpdateReq);
}

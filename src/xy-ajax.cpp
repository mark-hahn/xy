
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "xy-ajax.h"
#include "xy-wifi.h"
#include "xy-eeprom.h"

AsyncWebServerRequest *ssidScanReq;
AsyncWebServerRequest *getEepromReq;
AsyncWebServerRequest *wifiStatusReq;
bool connectAfterFormPost = false;

String eepromssidData;

void responseOK(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/plain", "OK");
  response->addHeader("Access-Control-Allow-Origin","*");
  response->addHeader("Access-Control-Allow-Methods", "POST");
  response->addHeader("Access-Control-Allow-Headers",
    "Content-Type, Access-Control-Allow-Headers, Authorization, X-Requested-With");
  request->send(response);
}

void do_ssidScan(AsyncWebServerRequest *request) {
  ssidScanReq = (AsyncWebServerRequest*) 0;
	int ssidCount = WiFi.scanNetworks();
  Serial.println(String("\ndo_ssidScan: Wifi scan found ") + ssidCount + " ssids");
  String json = "[";
  int ssidIdx; for(ssidIdx=0; ssidIdx<ssidCount; ssidIdx++) {
    json += "{\"ssid\":\"" + WiFi.SSID(ssidIdx)  + "\"," +
             "\"encryptionType\":\"";
    switch (WiFi.encryptionType(ssidIdx)) {
      case 2:  json += "TKIP (WPA)";  break;
      case 5:  json += "WEP";         break;
      case 4:  json += "CCMP (WPA)";  break;
      case 7:  json += "NONE";        break;
      case 8:  json += "AUTO";        break;
      default: json += "UNKNOWN";
    }
    json += String("\",") + "\"rssi\":" + WiFi.RSSI(ssidIdx)  + "}" +
             (ssidIdx == ssidCount-1 ? "" : ",");
	}
  json += "]";
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/json", json);
  response->addHeader("Access-Control-Allow-Origin","*");
  request->send(response);
}

// TODO: REPLACE BODY WITH CONCATENATED STRING INSTEAD OF JSON

void do_getEeprom(AsyncWebServerRequest *request) {
  getEepromReq = (AsyncWebServerRequest*) 0;
  char str[33];
  int ssidIdx, eeidx = 2;
  eeidx = eepromGetStr(str, eeidx);
  String json = String("[{\"apSsid\":\"") + str + "\",";
  eeidx = eepromGetStr(str, eeidx);
  json += String("\"apPwd\":\"") + str + "\"},";
  for(ssidIdx=0; ssidIdx<4; ssidIdx++) {
    eeidx = eepromGetStr(str, eeidx);
    json += String("{\"ssid\":\"") + str  + "\",";
    eeidx = eepromGetStr(str, eeidx);
    json += String("\"password\":\"") + str  + "\",";
    eeidx = eepromGetIp(str, eeidx);
    json += String("\"staticIp\":\"") + str + "\"}" + (ssidIdx == 3 ? "" : ",");
	}
  json += "]";
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/json", json);
  response->addHeader("Access-Control-Allow-Origin","*");
  request->send(response);
}
// TODO: REPLACE BODY WITH CONCATENATED STRING INSTEAD OF JSON

void eepromssidPost() {
  const size_t bufferSize = JSON_ARRAY_SIZE(5) + JSON_OBJECT_SIZE(2) +
                                               4*JSON_OBJECT_SIZE(3);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonArray& root = jsonBuffer.parseArray(eepromssidData);
  if (!root.success()) {
    Serial.println("parse failed");
    return;
  }
  int eeIdx = 2;

  const char* root_apSsid = root[0]["apSsid"];
  const char* root_apPwd  = root[0]["apPwd"];
  eeIdx = eepromPutStr(root_apSsid, eeIdx);
  eeIdx = eepromPutStr(root_apPwd,  eeIdx);

  JsonObject& a = root[1];
  const char* ssid1      = a["ssid"];
  const char* password1  = a["password"];
  const char* staticIp1  = a["staticIp"];
  eeIdx = eepromPutStr(ssid1,     eeIdx);
  eeIdx = eepromPutStr(password1, eeIdx);
  eeIdx = eepromPutIp(staticIp1,  eeIdx);

  JsonObject& b = root[2];
  const char* ssid2      = b["ssid"];
  const char* password2  = b["password"];
  const char* staticIp2  = b["staticIp"];
  eeIdx = eepromPutStr(ssid2,     eeIdx);
  eeIdx = eepromPutStr(password2, eeIdx);
  eeIdx = eepromPutIp(staticIp2,  eeIdx);

  JsonObject& c = root[3];
  const char* ssid3      = c["ssid"];
  const char* password3  = c["password"];
  const char* staticIp3  = c["staticIp"];
  eeIdx = eepromPutStr(ssid3,     eeIdx);
  eeIdx = eepromPutStr(password3, eeIdx);
  eeIdx = eepromPutIp(staticIp3,  eeIdx);

  JsonObject& d = root[4];
  const char* ssid4      = d["ssid"];
  const char* password4  = d["password"];
  const char* staticIp4  = d["staticIp"];
  eeIdx = eepromPutStr(ssid4,     eeIdx);
  eeIdx = eepromPutStr(password4, eeIdx);
  eeIdx = eepromPutIp(staticIp4,  eeIdx);

  connectAfterFormPost = true;
}

void do_wifiStatus(AsyncWebServerRequest *request) {
  wifiStatusReq = (AsyncWebServerRequest*) 0;
  char str[33];
  String json = "{\"apSsid\":\""  + String(ap_ssid)            + "\"," +
                 "\"apIp\":\""    + WiFi.softAPIP().toString() + "\"," +
                 "\"staSsid\":\"" + String(sta_ssid)           + "\"," +
                 "\"staIp\":\""   + WiFi.localIP().toString()  + "\"}";
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/json", json);
  response->addHeader("Access-Control-Allow-Origin","*");
  request->send(response);
}

void initAjaxServer(AsyncWebServer server) {
  server.on("/ajax/wifi-status", HTTP_GET, [](AsyncWebServerRequest *request){
    wifiStatusReq = request;
  });
	server.on("/ajax/get-eeprom", HTTP_GET, [](AsyncWebServerRequest *request){
		getEepromReq = request;
	});
  server.on("/ajax/ssid-scan", HTTP_GET, [](AsyncWebServerRequest *request){
    ssidScanReq = request;
  });

  server.on("/ajax/set-eeprom", HTTP_OPTIONS, responseOK);
  server.on("/ajax/set-eeprom", HTTP_POST, responseOK, 0,
    [](AsyncWebServerRequest *request,
            uint8_t *data, size_t len, size_t index, size_t total) {
    if(!index) eepromssidData = String("");
    char lastChar[2];
    lastChar[0] = data[len-1]; lastChar[1] = 0;
    data[len-1] = 0;
    eepromssidData = eepromssidData + String((char*)data) + lastChar;
    if(index + len == total) {
      eepromssidPost();
      eepromssidData = String("");
    }
  });
}

void chkAjax() {
  if(ssidScanReq)          do_ssidScan(ssidScanReq);
  if(getEepromReq)         do_getEeprom(getEepromReq);
  if(wifiStatusReq)        do_wifiStatus(wifiStatusReq);
  if(connectAfterFormPost) find_and_connect_try();
}

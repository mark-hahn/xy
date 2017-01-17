
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "xy-ajax.h"
#include "xy-wifi.h"
#include "xy-eeprom.h"

AsyncWebServerRequest *ssidRequest;
AsyncWebServerRequest *resetReq;
AsyncWebServerRequest *eepromssidRequest;
AsyncWebServerRequest *wifistatusRequest;
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

void do_resetEeprom(AsyncWebServerRequest *request) {
  ssidRequest = (AsyncWebServerRequest*) 0;
  responseOK(request);
  resetEeprom();
}

void do_ssids(AsyncWebServerRequest *request) {
  ssidRequest = (AsyncWebServerRequest*) 0;
	int ssidCount = WiFi.scanNetworks();
  Serial.println(String("\ndo_ssids: Wifi scan found ") + ssidCount + " ssids");
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

void do_eepromssids(AsyncWebServerRequest *request) {
  eepromssidRequest = (AsyncWebServerRequest*) 0;
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

void do_wifistatus(AsyncWebServerRequest *request) {
  wifistatusRequest = (AsyncWebServerRequest*) 0;
  char str[33];
  String json = "[{\"apSsid\":\""  + String(ap_ssid)           + "\"," +
                  "\"staSsid\":\"" + String(sta_ssid)            + "\"," +
                  "\"apIp\":\""    + WiFi.softAPIP().toString() + "\"," +
                  "\"staIp\":\""   + WiFi.localIP().toString()  + "\"}]";
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/json", json);
  response->addHeader("Access-Control-Allow-Origin","*");
  request->send(response);
}

void chkAjax() {
  if(ssidRequest)          do_ssids(ssidRequest);
  if(resetReq)             do_resetEeprom(resetReq);
  if(eepromssidRequest)    do_eepromssids(eepromssidRequest);
  if(wifistatusRequest)    do_wifistatus(wifistatusRequest);
  if(connectAfterFormPost) find_and_connect_try();
}

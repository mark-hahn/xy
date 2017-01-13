
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "xy-ajax.h"
#include "xy-eeprom.h"

AsyncWebServerRequest *ssidRequest;
AsyncWebServerRequest *eepromssidRequest;
String eepromssidData;

void responseOK(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/plain", "OK");
  response->addHeader("Access-Control-Allow-Origin","*");
  response->addHeader("Access-Control-Allow-Methods", "POST");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type, Access-Control-Allow-Headers, Authorization, X-Requested-With");
  request->send(response);
}

void do_ssids(AsyncWebServerRequest *request) {
  ssidRequest = (AsyncWebServerRequest*) 0;
	int ssidCount = WiFi.scanNetworks();
  Serial.println(String("\nWifi scan found ") + ssidCount + " ssids");
  String json = "[";
  int ssidIdx;
  for(ssidIdx=0; ssidIdx<ssidCount; ssidIdx++) {
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

  request->send(200, "text/json", json);
}

void do_eepromssids(AsyncWebServerRequest *request) {
  eepromssidRequest = (AsyncWebServerRequest*) 0;
  char str[33];
  eepromGetStr(str, 2);
  String json = String("[{\"apSsid\":\"") + str + "\",";
  eepromGetStr(str, 35);
  json += String("\"apPwd\":\"") + str + "\"},";
  int ssidIdx;
  for(ssidIdx=0; ssidIdx<4; ssidIdx++) {
    eepromGetStr(str, EEPROM_BYTES_OFS + ssidIdx * EEPROM_BYTES_PER_SSID);
    json += String("{\"ssid\":\"") + str  + "\",";
    eepromGetStr(str, EEPROM_BYTES_OFS + ssidIdx * EEPROM_BYTES_PER_SSID + 33);
    json += String("\"password\":\"") + str  + "\",";
    eepromGetIP(str, EEPROM_BYTES_OFS + ssidIdx * EEPROM_BYTES_PER_SSID + 66);
    json += String("\"staticIp\":\"") +str  + "\"}" +
                                     (ssidIdx == 3 ? "" : ",");
	}
  json += "]";
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/json", json);
  response->addHeader("Access-Control-Allow-Origin","*");
  request->send(response);

  request->send(200, "text/json", json);
}

void eepromssidPost() {
  Serial.println(String("starting to parse: ") + eepromssidData);
  const size_t bufferSize = JSON_ARRAY_SIZE(5) + JSON_OBJECT_SIZE(2) +
                                               4*JSON_OBJECT_SIZE(3);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonArray& root = jsonBuffer.parseArray(eepromssidData);

  const char* root_apSsid = root[0]["apSsid"]; // "eridien_XY_c3b2f0"
  const char* root_apPwd  = root[0]["apPwd"]; // "eridien"

  Serial.println(root_apSsid);
  Serial.println(root_apPwd);

  eepromPutStr(root_apSsid, 2);
  eepromPutStr(root_apPwd,  35);

  // const char* root_apPwd  = root[0]["apPwd"]; // "eridien"
  // JsonObject& 1 = root[1];
  // const char* 1_ssid = 1["ssid"]; // ""
  // const char* 1_password = 1["password"]; // ""
  // const char* 1_staticIp = 1["staticIp"]; // ""
  // JsonObject& 2 = root[2];
  // const char* 2_ssid = 2["ssid"]; // ""
  // const char* 2_password = 2["password"]; // ""
  // const char* 2_staticIp = 2["staticIp"]; // ""
  // JsonObject& 3 = root[3];
  // const char* 3_ssid = 3["ssid"]; // ""
  // const char* 3_password = 3["password"]; // ""
  // const char* 3_staticIp = 3["staticIp"]; // ""
  // JsonObject& 4 = root[4];
  // const char* 4_ssid = 4["ssid"]; // ""
  // const char* 4_password = 4["password"]; // ""
  // const char* 4_staticIp = 4["staticIp"]; // ""
  Serial.println("done parsing");
  if (!root.success()) {
    Serial.println("parse failed");
    return;
  }
}
void chkAjax() {
  if(ssidRequest)       do_ssids(ssidRequest);
  if(eepromssidRequest) do_eepromssids(eepromssidRequest);
}

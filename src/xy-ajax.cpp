
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Arduino.h>

#include "xy-ajax.h"
#include "xy-server.h"
#include "xy-wifi.h"
#include "xy-eeprom.h"
#include "xy-spi.h"

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

char hexDigit2int(char hex) {
  if(hex <= '9') return hex-'0';
  if(hex <= 'F') return hex-'A'+10;
  if(hex <= 'f') return hex-'a'+10;
  return 0;
}
uint8_t hexByte2int(const char *hex, uint8_t idx) {
  return (hexDigit2int(hex[idx]) << 4) | hexDigit2int(hex[idx+1]);
}

unsigned int upperBytesAddr = 0;

void ajaxFlashHexLine(uint8_t mcu, const char *line) {
  // Serial.println(String("ajaxFlashHexLine: ") + line);
  if(line[0] != ':') return;
  char buf[65];
  uint8_t i, len = hexByte2int(line, 1);
  if (len > 64) return;
  uint8_t addrH = hexByte2int(line, 3);
  uint8_t addrL = hexByte2int(line, 5);
  unsigned int addr = (addrH << 8) | addrL;
  uint8_t type = hexByte2int(line, 7);
  uint8_t cksum = len + addrH + addrL + type;
  for (i=0; i <= len; i++) {
    uint8_t byte = hexByte2int(line, 9+i*2);
    buf[i] = byte; // extra cksum byte is after len bytes
    cksum += byte;
  }
  if(cksum != 0) {
    Serial.println(String("hex line checksum error: ") + line);
    return;
  }
  switch (type) {
    case 4: upperBytesAddr = (buf[0] << 8) | buf[1];
    case 0: flashMcuBytes(mcu, (upperBytesAddr << 16) | addr, buf, len); break;
    case 1:
      Serial.println(String("mcu flashed, reseting it"));
      upperBytesAddr = 0;
      endFlashMcuBytes(mcu);
      break;
  }
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
  String staIp = ((String(sta_ssid).length() == 0) ?
                     "" :  WiFi.localIP().toString());
  String json = "[{\"apSsid\":\""  + String(ap_ssid)            + "\"," +
                  "\"staSsid\":\"" + String(sta_ssid)           + "\"," +
                  "\"apIp\":\""    + WiFi.softAPIP().toString() + "\"," +
                  "\"staIp\":\""   + staIp                      + "\"," +
                  "\"reqFromAp\":" + (reqFromAp ? "true" : "false") + "}]";
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
  if(connectAfterFormPost) find_and_connect_try(true);
}

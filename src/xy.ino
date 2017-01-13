

#define VERSION "version 0.1"

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <SPIFFSEditor.h>
#include <Hash.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

// includes needed for platformIO bug
#include <EEPROM.h>
#include <ESP8266httpUpdate.h>

#include "xy-eeprom.h"
#include "xy-websocket.h"
#include "xy-updates.h"
#include "xy-leds.h"
#include "xy-wifi.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
File fileUpload;
DNSServer dnsServer;


//////////////////////  AJAX  /////////////////////
void responseOK(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/plain", "OK");
  response->addHeader("Access-Control-Allow-Origin","*");
  response->addHeader("Access-Control-Allow-Methods", "POST");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type, Access-Control-Allow-Headers, Authorization, X-Requested-With");
  request->send(response);
}
AsyncWebServerRequest *ssidRequest;
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

AsyncWebServerRequest *eepromssidRequest;
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
String eepromssidData;
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


/////////////  SETUP  /////////////
void setup() {
	delay(1000);

	Serial.begin(115200);
	Serial.println(String("\n\nXY Control App Starting -- ") + VERSION);
	Serial.println(String("Free Code Space: ") + ESP.getFreeSketchSpace());

	initeeprom();
	pinMode(2, OUTPUT);
	SPIFFS.begin();
  ledInit();



/////////////  DNS  /////////////
	const byte DNS_PORT = 53;
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());


/////////////  SERVER  /////////////
  server.addHandler(new SPIFFSEditor("admin","admin"));

  server.on("/f", HTTP_GET, [](AsyncWebServerRequest *request){
    firmUpdateReq = request;
  });
  server.on("/fs", HTTP_GET, [](AsyncWebServerRequest *request){
    fsUpdateReq = request;
  });
  server.on("/ssids", HTTP_GET, [](AsyncWebServerRequest *request){
    ssidRequest = request;
  });

  server.on("/setssids", HTTP_OPTIONS, responseOK);
  server.on("/setssids", HTTP_POST, responseOK, 0,
    [](AsyncWebServerRequest *request,
            uint8_t *data, size_t len, size_t index, size_t total) {
    if(!index) {
      eepromssidData = String("");
    }
    char lastChar[2];
    lastChar[0] = data[len-1]; lastChar[1] = 0;
    data[len-1] = 0;
    eepromssidData = eepromssidData + String((char*)data) + lastChar;
    if(index + len == total) {
      eepromssidPost();
      eepromssidData = String("");
    }
  });

  server.on("/eepromssids", HTTP_GET, [](AsyncWebServerRequest *request){
    eepromssidRequest = request;
  });

  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("generate_204: " + request->url());
    request->send(200, "text/html", String( "<center><div style=\"width:50%\">") +
                        "<p>Use <b>xy.local</b> or <b>192.168.4.1</b> "       +
                        "to access the XY application when using the WiFi access point <b>"  +
                         String(ap_ssid) + "</b> you are using now. </p>"       +
                        "<p>When using your normal WiFi access point, use <b>xy.local</b>"     +
         (sta_ssid[0] ? " or <b>" + WiFi.localIP().toString() + "</b>." :
                        String(". If <b>xy.local</b> does not work, you will need to ") +
                        "find the XY ip address.  See the documentation.")    +
                        "</p></div></center>");
  });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200);
  }, [](AsyncWebServerRequest *request, const String& filename,
        size_t index, uint8_t *data, size_t len, bool final){
    if(!index) {
      if (fileUpload) {
        request->send(404);
        return;
      }
      Serial.println("Upload Start: " + filename);
      String fname = "";
      int i, numParams = request->params();
      for(i=0; i < numParams; i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost() && p->name() == "filename") fname = p->value();
      }
      if(fname.length() == 0) fname = filename;
      fileUpload = SPIFFS.open(fname.startsWith("/") ? fname : "/" + fname, "w");
    }
    if(len) fileUpload.write(data, len);
    if(final) {
      fileUpload.close();
      fileUpload = (File) 0;
      Serial.println("Upload End: " + filename + ", " + (index+len));
    }
  });

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.println("File not found: " + request->url());
    request->send(404);
  });

  server.begin();
	Serial.println("HTTP server started");


/////////////  AP & STA  /////////////
	find_and_connect();


/////////////  MDNS  /////////////
  MDNS.begin("xy");
  MDNS.addService("http", "tcp", 80);


//////////////  WEB SOCKET INIT  ////////////////
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);


/////////////  CLEAN UP SETUP  /////////////
	EEPROM.end();
}


/////////////  LOOP  /////////////
void loop() {
  if(firmUpdateReq)     do_firmware_update(firmUpdateReq);
  if(fsUpdateReq)       do_spiffs_update(fsUpdateReq);
  if(ssidRequest)       do_ssids(ssidRequest);
  if(eepromssidRequest) do_eepromssids(eepromssidRequest);

  dnsServer.processNextRequest();
}

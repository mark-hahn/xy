

#define VERSION "version 0.1"

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <FS.h>
#include <SPIFFSEditor.h>
#include <Hash.h>
#include <DNSServer.h>
#include <os_type.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
char ap_ssid[33];
char ap_pwd[33];
File fileUpload;
DNSServer dnsServer;


//////////////////  WEB SOCKET  //////////////////
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}


///////////////  UPDATES  //////////////
AsyncWebServerRequest * firmUpdateReq;
void do_firmware_update(AsyncWebServerRequest *request) {
  firmUpdateReq = (AsyncWebServerRequest *) 0;
	request->send(200, "text/plain", "Started firmware update");
	Serial.println("\nUpdating firmware from http://hahnca.com/xy/firmware.bin");
	delay(1000);
	ESPhttpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return ret = ESPhttpUpdate.update("http://hahnca.com/xy/firmware.bin");
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
	Serial.println("Updating file system from http://hahnca.com/xy/spiffs.bin");
	delay(1000);
	ESPhttpUpdate.rebootOnUpdate(false);
  t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(
															"http://hahnca.com/xy/spiffs.bin");
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


/////////////  LEDS  /////////////
os_timer_t ledTimer;
bool ledBlinking;
bool blinkLedIsOn;
void led_on()  {digitalWrite(2, 0);}
void led_off() {digitalWrite(2, 1);}
void ledBlinkCallback(void *pArg) {
  blinkLedIsOn = !blinkLedIsOn;
  if(blinkLedIsOn) led_on();
  else             led_off();
}
void ledBlink(bool turnBlinkOn) {
  if(!ledBlinking && turnBlinkOn) {
    ledBlinking = true;
    os_timer_arm(&ledTimer, 100, true);
  } else if (ledBlinking && !turnBlinkOn) {
    os_timer_disarm(&ledTimer);
    ledBlinking = false;
    led_off();
  }
}


/////////////  EEPROM  /////////////
/* eeprom map
  2  Magic = 0xedde
  33 AP ssid name
  33 AP password
	4 STA choices (70 each)
		 33 ssid
		 33 pwd
		 4  static ip
	348 bytes total
*/
#define EEPROM_BYTES_OFS      68
#define EEPROM_BYTES_PER_SSID 70
#define EEPROM_TOTAL_BYTES    348

void initeeprom() {
	char buf[33];
	int bufidx, eeAddr;
	String ap_ssid_str;
  EEPROM.begin(512);
	if (EEPROM.read(0) != 0xed || EEPROM.read(1) != 0xde) {
    Serial.println("initializing empty eeprom, magic was: " +
                    String(EEPROM.read(0),HEX) + String(EEPROM.read(1),HEX));
		EEPROM.write(0, 0xed);
		EEPROM.write(1, 0xde);
		for (eeAddr=2; eeAddr < EEPROM_TOTAL_BYTES; eeAddr++) EEPROM.write(eeAddr, 0);
    EEPROM.end();
	  ap_ssid_str = "eridien_XY_" + String(ESP.getChipId(), HEX);
		ap_ssid_str.toCharArray(buf, 33);
    eepromPutStr(buf, 2);
    eepromPutStr("eridienxy", 35);
	}
  EEPROM.end();
}
int eepromGetStr(char* res, int idx){
  EEPROM.begin(512);
  int i = 0; char chr;
	do { chr = EEPROM.read(idx+i); res[i] = chr; i++; } while (chr);
  EEPROM.end();
	return idx+33;
}
int eepromGetIP(IPAddress res, int idx){
  EEPROM.begin(512);
	int i;
	for(i=0; i<4; i++) res[i] = EEPROM.read(idx+i);
  EEPROM.end();
	return idx+4;
}
int eepromPutStr(const char* str, int idx){
  EEPROM.begin(512);
  int i = 0; char chr;
	do {EEPROM.write(idx+i, chr=str[i]); i++;} while (chr);
  EEPROM.end();
	return idx+33;
}
int eepromPutIP(const char* ipStr, int idx){
  IPAddress ip;
  if(ip.fromString(ipStr)) {
    EEPROM.begin(512);
  	int i; for(i=0; i<4; i++) EEPROM.write(idx+i, ip[i]);
    EEPROM.end();
  }
	return idx+4;
}


/////////////  AP & STA SETUP  /////////////
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
  IPAddress ip;
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
    eepromGetIP(ip, EEPROM_BYTES_OFS + ssidIdx * EEPROM_BYTES_PER_SSID + 66);
    json += String("\"staticIp\":\"") + ip.toString()  + "\"}" +
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
  os_timer_setfn(&ledTimer, ledBlinkCallback, NULL);


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

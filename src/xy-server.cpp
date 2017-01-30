
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <SPIFFSEditor.h>
#include <DNSServer.h>
#include <EEPROM.h>

#include "xy-server.h"
#include "xy-ajax.h"
#include "xy-updates.h"
#include "xy-wifi.h"

AsyncWebServer server(80);
DNSServer dnsServer;
File fileUpload;
bool reqFromAp;

void setupServer() {
	const byte DNS_PORT = 53;
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  MDNS.begin("xy");
  MDNS.addService("http", "tcp", 80);

  server.addHandler(new SPIFFSEditor("admin","admin"));

	server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
		resetReq = request;
	});
  server.on("/update-fw", HTTP_GET, [](AsyncWebServerRequest *request){
    firmUpdateReq = request;
  });
  server.on("/update-fs", HTTP_GET, [](AsyncWebServerRequest *request){
    fsUpdateReq = request;
  });

  server.on("/ajax/ssid-scan", HTTP_GET, [](AsyncWebServerRequest *request){
    ssidRequest = request;
  });

  server.on("/ajax/wifi-status", HTTP_GET, [](AsyncWebServerRequest *request){
    // Serial.println("/ajax/wifi-status");
    wifistatusRequest = request;
  });

  server.on("/ajax/get-eeprom", HTTP_GET, [](AsyncWebServerRequest *request){
	  eepromssidRequest = request;
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

  server.on("/i2c", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(14, 1);
		ajaxToMcu(request);
    digitalWrite(14, 0);
	});

  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
    // Serial.println("generate_204: " + request->url());
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

  server.on("/flashMcu", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200);
	}, 0, [](AsyncWebServerRequest *request,
		    uint8_t *data, size_t len, size_t index, size_t total) {  // handleBody
		static unsigned int flashAddr;
    if(!index) {
			if (!request->params()) {
				Serial.println("flashMcu missing flashAddr param");
				request->send(500);
				return;
			}
			AsyncWebParameter* p = request->getParam(0);  // only one param allowed. byte address
      if(p->name() == String("flashAddr")) {
				flashAddr = p->value().toInt();
        Serial.println(String("flashMcu atart addr, len, index, total: ") +
										   flashAddr  + ", " + len + ", " + index + ", " + total);
      } else {
				Serial.println("invalid param in flashMcu: " + p->name());
				request->send(500);
        return;
			}
    }
    if(len) ajaxFlashMcu(flashAddr+index, (char *) data, len);
  });

	server.on("/resetMcu", HTTP_GET, [](AsyncWebServerRequest *request){
		ajaxResetMcu();
	});

	server.serveStatic("/", SPIFFS, "/")
	      .setFilter([](AsyncWebServerRequest *request){
		reqFromAp = (request->client()->localIP().toString() ==
		             String("192.168.4.1"));
		Serial.println(String("****  request from ") +
										(reqFromAp ? "AP" : "STA") + ": " + String(request->url()));
		return false;
	});

	server.serveStatic("/", SPIFFS, "/") .setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request){
    // Serial.println("File not found: " + request->url());
    request->send(404);
  });

  server.begin();
	Serial.println("HTTP server started");
}

void chkServer() {
  dnsServer.processNextRequest();
}

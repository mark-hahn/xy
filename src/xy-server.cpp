
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

void setupServer() {
	const byte DNS_PORT = 53;
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
	Serial.println("1");
  MDNS.begin("xy");
	Serial.println("2");
  MDNS.addService("http", "tcp", 80);
	Serial.println("3");
	initAjaxServer(server);
	Serial.println("4");
  server.addHandler(new SPIFFSEditor("admin","admin"));

	Serial.println("5");
  server.on("/f", HTTP_GET, [](AsyncWebServerRequest *request){
    firmUpdateReq = request;
  });
	Serial.println("51");
  server.on("/fs", HTTP_GET, [](AsyncWebServerRequest *request){
    fsUpdateReq = request;
  });
	Serial.println("52");
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

	Serial.println("53");
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
	Serial.println("54");
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

	Serial.println("55");
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.println("File not found: " + request->url());
    request->send(404);
  });

	Serial.println("56");
  server.begin();
	Serial.println("HTTP server started");
}

void chkServer() {
  dnsServer.processNextRequest();
}

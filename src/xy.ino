#include "handlers.h"

#define CONN_TIMEOUT 60
#define ledPcb 2

void ledOn()  { digitalWrite(ledPcb, 0); }
void ledOff() { digitalWrite(ledPcb, 1); }

void setup() {
  pinMode(ledPcb, OUTPUT);
  ledOff();

  Serial.begin(115200);
  Serial.println(String("starting XY wifi setup"));

  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }


///////////////// INIT WIFI  //////////////
  WiFiManagerXy WiFiManagerXy;
  WiFiManagerXy.resetSettings();
  WiFiManagerXy.setTimeout(CONN_TIMEOUT);
  WiFiManagerXy.setMinimumSignalQuality(15); // 15%

  Serial.println("trying wifi autoconnect");
  String ssid = WiFiManagerXy.autoConnect();
  // WiFiManagerXy.connectWifi("","");
  // WiFiManagerXy.startConfigPortal();

  if (WiFi.status() != WL_CONNECTED) Serial.print("Waiting for connection");

  // // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    ledOn();
    delay(200);
    ledOff();
    delay(1000);
    Serial.print(".");
  }


///////////////  HTTP SERVER  ///////////////
  Serial.println("starting HTTP server");
  //http://192.168.1.235/list?dir=
  server.on("/list", HTTP_GET, handleFileList);
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  server.on("/edit", HTTP_PUT, handleFileCreate);
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "eridien XY, 404: File Not Found");
  });
  server.on("/misc", HTTP_GET, [](){
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap());
    json += ", \"analog\":"+String(analogRead(A0));
    json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

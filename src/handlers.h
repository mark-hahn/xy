
#ifndef _handlersh
#define _handlersh

#include <DNSServer.h>
#include <WiFiManagerXy.h>       // https://github.com/tzapu/WiFiManagerXy
#include <ESP8266WebServer.h>    // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
#include <WebSocketsServer.h>    // https://github.com/Links2004/arduinoWebSockets
#include <Hash.h>
#include <Wire.h>                // https://github.com/esp8266/Arduino/tree/master/libraries/Wire
#include <FS.h>

extern ESP8266WebServer server;
extern File fsUploadFile;

String formatBytes(size_t bytes);
void handleFileList();
bool handleFileRead(String path);
void handleFileUpload();
void handleFileDelete();
void handleFileCreate();

#endif

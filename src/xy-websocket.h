
#ifndef _xy_websocket
#define _xy_websocket

#include <EEPROM.h>

#include <ESPAsyncWebServer.h>

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void setupWebsocket();

#endif

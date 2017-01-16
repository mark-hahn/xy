
#ifndef _xy_ajax
#define _xy_ajax

extern AsyncWebServerRequest *ssidScanReq;
extern AsyncWebServerRequest *getEepromReq;
extern AsyncWebServerRequest *wifiStatusReq;
extern bool                   connectAfterFormPost;
extern String                 eepromssidData;

void initAjaxServer(AsyncWebServer server);
void chkAjax();

#endif

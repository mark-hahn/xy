
#ifndef _xy_ajax
#define _xy_ajax

#include <ESPAsyncWebServer.h>


extern AsyncWebServerRequest *ssidRequest;
extern AsyncWebServerRequest *eepromssidRequest;
extern AsyncWebServerRequest *wifistatusRequest;
extern AsyncWebServerRequest *resetReq;
extern bool                   connectAfterFormPost;
extern String                 eepromssidData;

void responseOK(AsyncWebServerRequest *request);
void do_ssids(AsyncWebServerRequest *request);
void do_eepromssids(AsyncWebServerRequest *request);
void eepromssidPost();
void ajaxFlashHexLine(char mcu, const char *line);
void ajaxResetMcu();
void chkAjax();

#endif

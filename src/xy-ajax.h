
#ifndef _xy_ajax
#define _xy_ajax

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
void ajaxToMcu(AsyncWebServerRequest *request);
void ajaxFlashHexLine(const char *line);
void ajaxResetMcu();
void chkAjax();


#endif

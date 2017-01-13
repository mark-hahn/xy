
#ifndef _xy_ajax
#define _xy_ajax

extern AsyncWebServerRequest *ssidRequest;
extern AsyncWebServerRequest *eepromssidRequest;
extern String eepromssidData;

void responseOK(AsyncWebServerRequest *request);
void do_ssids(AsyncWebServerRequest *request);
void do_eepromssids(AsyncWebServerRequest *request);
void eepromssidPost();
void chkAjax();


#endif
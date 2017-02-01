
#ifndef _xy_updates
#define _xy_updates

extern AsyncWebServerRequest * firmUpdateReq;
extern AsyncWebServerRequest * fsUpdateReq;
extern AsyncWebServerRequest * mcuUpdateReq;

void do_firmware_update(AsyncWebServerRequest *request);
void do_spiffs_update(AsyncWebServerRequest *request);
void chkUpdates();


#endif


#ifndef _xy_server
#define _xy_server

extern AsyncWebServer server;
extern bool reqFromAp;
extern bool connToXY;

void setupServer();
void chkServer();

#endif

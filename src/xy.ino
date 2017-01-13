

#define VERSION "version 0.1"

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <SPIFFSEditor.h>
#include <Hash.h>
#include <DNSServer.h>

// includes needed for platformIO bug
#include <EEPROM.h>
#include <ESP8266httpUpdate.h>

#include "xy-eeprom.h"
#include "xy-websocket.h"
#include "xy-updates.h"
#include "xy-leds.h"
#include "xy-wifi.h"
#include "xy-ajax.h"
#include "xy-server.h"

void setup() {
	delay(1000);

	Serial.begin(115200);
	Serial.println(String("\n\nXY Control App Starting -- ") + VERSION);
	Serial.println(String("Free Code Space: ") + ESP.getFreeSketchSpace());

	SPIFFS.begin();
	initeeprom();
  ledInit();
  setupServer();
	find_and_connect();
  setupWebsocket();
}

void loop() {
  chkUpdates();
  chkAjax();
	chkServer();
}

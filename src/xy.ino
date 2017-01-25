

#define VERSION "version 0.3"

#define debug true

#include "xy.h"
#include "xy-eeprom.h"
#include "xy-websocket.h"
#include "xy-updates.h"
#include "xy-leds.h"
#include "xy-wifi.h"
#include "xy-ajax.h"
#include "xy-server.h"
#include "xy-i2c.h"

void setup() {
	delay(1000);

	Serial.begin(115200);
	Serial.println(String("\n\nXY Control App Starting -- ") + VERSION);
	Serial.println(String("Free Code Space: ") + ESP.getFreeSketchSpace());

	SPIFFS.begin();
	if (!debug) {
		initeeprom();
	  ledInit();
	  setupServer();
		find_and_connect();
	  setupWebsocket();
	}
	initI2c();
	char buf[1] = {homing};
	writeI2c(1, offsetof(Bank, cmd), buf, 1);
}

void loop() {
	if (!debug) {
		chkServer();
	  chkAjax();
		chkUpdates();
	}
}

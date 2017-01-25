

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

	pinMode(SYNC, OUTPUT_OPEN_DRAIN);
	digitalWrite(SYNC, HIGH);

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
	testI2c();
}

void loop() {
	if (!debug) {
		chkServer();
	  chkAjax();
		chkUpdates();
	}
	delayMicroseconds(50);
	digitalWrite(SYNC, LOW);
	delayMicroseconds(50);
	digitalWrite(SYNC, HIGH);
}

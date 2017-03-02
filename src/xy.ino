

#define VERSION "version 0.4"

#include <SPI.h>

#include "xy.h"
#include "xy-eeprom.h"
#include "xy-websocket.h"
#include "xy-updates.h"
#include "xy-leds.h"
#include "xy-wifi.h"
#include "xy-ajax.h"
#include "xy-server.h"
#include "xy-spi.h"
#include "xy-control.h"

void setup() {
	delay(1000);

	Serial.begin(115200);
	Serial.println(String("\n\nXY Control App Starting -- ") + VERSION);
	Serial.println(String("Free Code Space: ") + ESP.getFreeSketchSpace());

	pinMode(PWRLED, OUTPUT);
	digitalWrite(PWRLED, 0);
  pinMode(PWRON, INPUT);

	// SPIFFS.begin();
	// initeeprom();
  // ledInit();
  setupServer();
	find_and_connect();
  // setupWebsocket();
	initSpi();

	// get status rec, just to see hex in console
	Serial.println("getting status rec");
	char stat;
	do {
		delay(1);
		stat = getMcuStatusRec(0);
		if(stat == 254) break; // status rec too long (?)
	} while (stat != 0);

  for(char i = 0; i < STATUS_REC_LEN; i++) {
		printHex8(statusRec[i]); Serial.print(" ");
	}
  Serial.println();

	// diagonalTest();
}

void loop() {
	chkServer();
	chkAjax();
	chkUpdates();
	chkCtrl();
}

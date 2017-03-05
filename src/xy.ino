

#define VERSION "version 0.4"

#include <Arduino.h>
#include <SPI.h>

#include "xy.h"
#include "mcu-cpu.h"
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

	// // get status rec, just to see hex in console
	// Serial.println("getting status rec");
	// char stat;
	// while(1) {
	// 	delay(1);
	// 	stat = getMcuStatusRec(0);
  //   if(spiCommError(stat)) {
  //     byte2mcuWithSS(0, clearErrorCmd);
  //     continue;
  //   }
	// 	break;
	// }
	// if(stat == 0) {
  //   Serial.println("got status rec, status: " + String(stat, HEX));
	//   for(char i = 0; i < STATUS_REC_LEN; i++) {
	// 		printHex8(statusRec[i]); Serial.print(" ");
	// 	}
	//   Serial.println();
	// } else
  //   Serial.println("status rec non-zero status (hex): " + String(stat, HEX));

	diagonalTest();
}

void loop() {
	chkServer();
	chkAjax();
	chkUpdates();
	chkCtrl();
}

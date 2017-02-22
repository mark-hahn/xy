

#define VERSION "version 0.4"

#define debug true

#include <SPI.h>

#include "xy.h"
#include "xy-eeprom.h"
#include "xy-websocket.h"
#include "xy-updates.h"
#include "xy-leds.h"
#include "xy-wifi.h"
#include "xy-ajax.h"
#include "xy-server.h"
#include "xy-driver.h"
#include "xy-spi.h"
#include "mcu-cpu.h"

unsigned long microStartTime = 0;

void startMicroTimer() {microStartTime = micros();}
long elapsedMicros() {micros() - microStartTime;}

char status;
char lastStatus = 0;
bool_t noResponse = TRUE;
bool_t sentCmd = FALSE;

void setup() {
	delay(1000);

	Serial.begin(115200);
	Serial.println(String("\n\nXY Control App Starting -- ") + VERSION);
	Serial.println(String("Free Code Space: ") + ESP.getFreeSketchSpace());

	// SPIFFS.begin();
	// initeeprom();
  // ledInit();
  setupServer();
	find_and_connect();
  // setupWebsocket();
	initSpi();

	status = word2mcu(resetCmd << 24, 0);
}

void loop() {
	chkServer();
	chkAjax();
	chkUpdates();
	// chkDriver();


	if(!sentCmd) {
		status = word2mcu(homeCmd << 24, 0);
		sentCmd = TRUE;
	}
	else
    status = word2mcu(nopCmd << 24, 0);

	if(status != 0 && status != lastStatus) {
    Serial.print("mcu status: "); Serial.println(status, HEX);
		lastStatus = status;
	}

	if(status != 0xff && (status & 0x0f) != 0) {
		word2mcu(clearErrorCmd << 24, 0);
		sentCmd = FALSE;
	}

  // status = word2mcu(resetCmd << 24, 0);
	// if(status != 0) {
	// 	if(status != lastStatus) {
	//     Serial.print("mcu status: "); Serial.println(status, HEX);
	// 		lastStatus = status;
	// 	}
	// 	if(!noResponse && status == 0xff) {
	// 		Serial.print("mcu not responding ...");
	// 		noResponse = TRUE;
	// 	}
	// 	if(noResponse && status != 0xff) {
	// 		Serial.println(" mcu responding");
	// 		noResponse = FALSE;
	// 	}
	// 	if(!noResponse && status != 0xff && (status & 0x0f) != 0) {
	// 		Serial.print("mcu error: "); Serial.println(status, HEX);
	// 		word2mcu(clearErrorCmd << 24, 0);
	// 	}
	// }
}

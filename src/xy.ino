

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

char status;
char lastStatus = 0;
bool_t noResponse = TRUE;
bool_t sentCmd = FALSE;
char sleepCmdCounter = 0;

void setup() {
	delay(1000);

	Serial.begin(115200);
	Serial.println(String("\n\nXY Control App Starting -- ") + VERSION);
	Serial.println(String("Free Code Space: ") + ESP.getFreeSketchSpace());

  pinMode(PWRON, INPUT);

	// SPIFFS.begin();
	// initeeprom();
  // ledInit();
  setupServer();
	find_and_connect();
  // setupWebsocket();
	initSpi();

	word2mcu(0, 0);
	word2mcu(0, 0);
	word2mcu(sleepCmd << 24, 0);
	word2mcu(0, 0);
	word2mcu(0, 0);
}

void chkStatus(char statusIn) {
	status = statusIn;
	if(status != 0 && status != lastStatus) {
    Serial.print("status: "); Serial.println(status, HEX);
	  lastStatus = status;
	}
}

void loop() {
	chkServer();
	chkAjax();
	chkUpdates();
	// chkDriver();

	if(digitalRead(PWRON) == 0) {
		if(sleepCmdCounter++ == 0)
			chkStatus(word2mcu(sleepCmd << 24, 0));
	  sentCmd = FALSE;
		return;
	}

	chkStatus(word2mcu(0, 0););

	if(status == 0xff) {
		// mcu is not running
		// sentCmd = FALSE;
		return;
	}
	if((status & 0x0f) != 0) {
		word2mcu(0, 0);
		word2mcu(clearErrorCmd << 24, 0);
		// sentCmd = FALSE;
	}
  if(!sentCmd) {
		word2mcu(homeCmd << 24, 0);
		sentCmd = TRUE;
	}
}

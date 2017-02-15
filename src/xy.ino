

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

void setup() {
	delay(1000);

	#define CS  15
	#define SCK 14

	pinMode(CS,  OUTPUT);
	pinMode(SCK, OUTPUT); // sck
	digitalWrite(CS,1);
	pinMode(12, INPUT);
	// pinMode(SYNC, INPUT);

	SPI.begin();
	SPI.setHwCs(false);

	Serial.begin(115200);
	Serial.println(String("\n\nXY Control App Starting -- ") + VERSION);
	Serial.println(String("Free Code Space: ") + ESP.getFreeSketchSpace());

	// SPIFFS.begin();
	// initeeprom();
  // ledInit();
  setupServer();
	find_and_connect();
  // setupWebsocket();
}

char status = 0;
char oldstatus = 0;
bool_t firstCmd = TRUE;

void loop() {
	chkServer();
	chkAjax();
	chkUpdates();
	// chkDriver();

	if(status != oldstatus) {
    oldstatus = status;
		Serial.print("Status from MCU: "); Serial.println(status, HEX);
	}
	if(firstCmd)
	  status = word2mcu(homeCmd << 24);
	else
		status = word2mcu(0);

	if(status != 0) firstCmd = FALSE;

	if(status & 0x0f) {
		oldstatus = status;
    Serial.print("Error from MCU: "); Serial.println(status, HEX);
		delay(1000);
		status = word2mcu(clearErrorCmd << 24);
		delay(1000);
	}
}



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
// #include "xy-i2c.h"
#include "xy-driver.h"
// #include "xy-flash-mcu.h"
#include "xy-spi.h"

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
	// initI2c();
}

void loop() {
	chkServer();
	chkAjax();
	chkUpdates();
	// chkDriver();

	delayMicroseconds(25);
	digitalWrite(CS,0);
	word2mcu(0x12);

	Serial.println(byteBack, HEX);

	delayMicroseconds(25);
	word2mcu(0x34);
	delayMicroseconds(25);
	word2mcu(0x56);
	delayMicroseconds(25);
	word2mcu(0x78);
	digitalWrite(CS,1);
}

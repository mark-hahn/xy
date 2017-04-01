
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
	Serial.print("Flash Chip Real Size: "); Serial.println(ESP.getFlashChipRealSize());
  Serial.print("Sketch size: "); Serial.println(ESP.getSketchSize());
  Serial.print("Free size: ");   Serial.println(ESP.getFreeSketchSpace());
	pinMode(PWRLED, OUTPUT);
	digitalWrite(PWRLED, 0);
  pinMode(PWRON, INPUT);

	SPIFFS.begin();
	initeeprom();
  ledInit();
  setupServer();
	find_and_connect();
  setupWebsocket();
	initSpi();

	test();

}

void loop() {
	chkServer();
	chkAjax();
	chkUpdates();
	chkCtrl();
}

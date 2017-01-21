

#define VERSION "version 0.2"

#include "xy-eeprom.h"
#include "xy-websocket.h"
#include "xy-updates.h"
#include "xy-leds.h"
#include "xy-wifi.h"
#include "xy-ajax.h"
#include "xy-server.h"

// 8266 pins
#define RESET  16 /* D0 */
#define SCL     5 /* D1 */
#define SDA     4 /* D2 */
#define LED    14 /* D5 */
#define SYNC   12 /* D6 */
#define ENABLE 13 /* D7 */


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

	pinMode(SYNC, INPUT);
}

void loop() {
	chkServer();
  chkAjax();
	chkUpdates();
}

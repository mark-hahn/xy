

#define VERSION "version 0.4"

#define debug true

#include <string.h>
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

void setup() {
	delay(1000);

	Serial.begin(115200);
	Serial.println(String("\n\nXY Control App Starting -- ") + VERSION);
	Serial.println(String("Free Code Space: ") + ESP.getFreeSketchSpace());

	pinMode(PWRLED, OUTPUT);
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

char status = 0;
char errorCode = 0;
char errorAxis;
char lastStatus = 0;
char lastErrorCode = 0;
char sleepCmdCounter = 0;
StatusRecU statusRec;
char statusRecInBuf[STATUS_SPI_BYTE_COUNT];
char statusRecInIdx = 0;
char bytesSinceStateByte = 0xff;

// unpack statusRecInBuf into statusRec
void processRecIn() {
	char statusRecIdx = 0;
	char curByte;
  for(int i=0; i < STATUS_SPI_BYTE_COUNT; i++) {
		char sixBits = statusRecInBuf[i];
		switch(i % 4) {
			case 0:
			  curByte = (sixBits << 2);
			  break;

			case 1:
			  curByte |= ((sixBits & 0x30) >> 4);
				statusRec.bytes[statusRecIdx++] = curByte;
				curByte  = ((sixBits & 0x0f) << 4);
			  break;

			case 2:
			  curByte |= ((sixBits & 0x3c) >> 2);
				statusRec.bytes[statusRecIdx++] = curByte;
				curByte  = ((sixBits & 0x03) << 6);
				break;

			case 3:
				statusRec.bytes[statusRecIdx++] = curByte | sixBits;;
			  break;
		}
	}
  for(int i=0; i < STATUS_SPI_BYTE_COUNT; i++)
	  Serial.println(String(i) + ": " + String(statusRecInBuf[i], HEX));

  Serial.println(String("apiVers: ")   + (int) statusRec.rec.apiVers);
  Serial.println(String("mfr: ")       + (int) statusRec.rec.mfr);
  Serial.println(String("prod: ")      + (int) statusRec.rec.prod);
  Serial.println(String("vers: ")      + (int) statusRec.rec.vers);
  Serial.println(String("homeDistX: ") + statusRec.rec.homeDistX);
  Serial.println(String("homeDistY: ") + statusRec.rec.homeDistY);
}

bool_t sentCmd = FALSE;  // debug to send one command at power on

void chkStatus(char statusIn) {
	// Serial.println(statusIn, HEX);
	if(statusIn == 0xff) {
		status = statusNoResponse;
		if(bytesSinceStateByte != 0xff) bytesSinceStateByte++;
		return;
	}
  if((statusIn & 0xc0) == typeState) {
	  status = statusIn & spiStateByteMask;
	  if((statusIn & spiStateByteErrFlag) == 0) errorCode = 0;
    if(bytesSinceStateByte == STATUS_SPI_BYTE_COUNT &&
			 statusRecInIdx == STATUS_SPI_BYTE_COUNT)
			processRecIn();
		bytesSinceStateByte = statusRecInIdx = 0;
	}
	else {
		if((statusIn & 0xc0) == typeData) {
      if(bytesSinceStateByte != statusRecInIdx ||
			   statusRecInIdx >= STATUS_SPI_BYTE_COUNT)
				statusRecInIdx = 0;
			else
			  statusRecInBuf[statusRecInIdx++] = (statusIn & 0x3f);
		}
	  else if((statusIn & 0xc0) == typeError) {
			errorAxis = statusIn & 0x01;
		  errorCode = statusIn & 0x3e;
		} // else top two bits are zero
		if(bytesSinceStateByte != 0xff) bytesSinceStateByte++;
	}
	if(status != 0 && status != lastStatus) {
    Serial.print("Status: "); Serial.println(status, HEX);
  }
	if(errorCode != 0 && errorCode != lastErrorCode) {
		Serial.println(String("Error, code: ") + String(errorCode, DEC) +
									             ", axis: "  + String(errorAxis, DEC));
		word2mcu(0, 0);
		word2mcu(clearErrorCmd << 24, 0);
	}
	lastStatus = status;
	lastErrorCode = errorCode;
}

void loop() {
	chkServer();
	chkAjax();
	chkUpdates();
	// chkDriver();

	if(digitalRead(PWRON) == 0) {
		digitalWrite(PWRLED, 0);
		if(sleepCmdCounter++ == 0)
			chkStatus(word2mcu(sleepCmd << 24, 0));
	  sentCmd = FALSE; // debug, send test command on power switch on
		return;
	}
	else digitalWrite(PWRLED, 1);

	chkStatus(word2mcu(0, 0));

	if(status == statusNoResponse) {
		// mcu is not running
		// sentCmd = FALSE;
		return;
	}

  if(!sentCmd) {
		word2mcu(statusCmd << 24, 0);
		// word2mcu(homeCmd << 24, 0);
		sentCmd = TRUE;
	}
}

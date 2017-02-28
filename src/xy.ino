

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
	digitalWrite(PWRLED, 0);

  pinMode(PWRON, INPUT);

	// SPIFFS.begin();
	// initeeprom();
  // ledInit();
  setupServer();
	find_and_connect();
  // setupWebsocket();
	initSpi();

  setWordDelay(300);
	zero2mcu(0);
	cmd2mcu(0, sleepCmd);
	zero2mcu(0);
	setWordDelay(0);
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
  Serial.println(String("apiVers: ")   + (int) statusRec.rec.apiVers);
  Serial.println(String("mfr: ")       + (int) statusRec.rec.mfr);
  Serial.println(String("prod: ")      + (int) statusRec.rec.prod);
  Serial.println(String("vers: ")      + (int) statusRec.rec.vers);
  Serial.println(String("homeDistX: ") + statusRec.rec.homeDistX);
  Serial.println(String("homeDistY: ") + statusRec.rec.homeDistY);

	setWordDelay(0);
}

bool_t sentHome = TRUE;
bool_t sentMove = TRUE;
char errorCount = 0;

void chkStatus(char statusIn) {
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
	if(errorCode != lastErrorCode) {
		if( errorCode)
			Serial.println(String("Error, code: ") + String(errorCode, DEC) +
										             ", axis: "  + String(errorAxis, DEC));
		else
		  Serial.println("Error clear");
  }
	if(errorCode) {
		if(errorCount == 0) {
			errorCount = 1;
	  	setWordDelay(300);
		  zero2mcu(0);
			cmd2mcu(0, clearErrorCmd);
		  zero2mcu(0);
	  	setWordDelay(0);
		}
	}
	if(status) lastStatus = status;
	lastErrorCode = errorCode;
}

void loop() {
	chkServer();
	chkAjax();
	chkUpdates();
	// chkDriver();

	if(digitalRead(PWRON) == 0) {
		// power switch off
		digitalWrite(PWRLED, 0);
		if(sleepCmdCounter++ == 0) chkStatus(cmd2mcu(0, sleepCmd));
	  sentHome  = FALSE; // debug, send test commands on power switch on
		sentMove = FALSE;
	}
	else {
		// power switch on
		digitalWrite(PWRLED, 1);
    if(status == statusSleeping) {
			// power switch just switched on
		  chkStatus(cmd2mcu(0, resetCmd));
			chkStatus(zero2mcu(0));
			Serial.println("Sent cmd: resetCmd");
		}

		chkStatus(zero2mcu(0));

	  if(status != statusNoResponse && !errorCode && !sentHome) {
			// void vec2mcu(char mcu, char axis, char dir, char ustep,
			//              uint16_t usecsPerPulse, uint16_t pulseCount) {
	    vec2mcu(0, X, FORWARD, 2, 1000, 1023);  // 100 mm but only 1023 each vec
	    vec2mcu(0, X, FORWARD, 2, 1000, 100*20-1023);
			eof2mcu(0, X);

	    vec2mcu(0, Y, FORWARD, 2, 1000, 1023);  // 100 mm but only 1023 each vec
	    vec2mcu(0, Y, FORWARD, 2, 1000, 100*20-1023);
			eof2mcu(0, Y);
			Serial.println("Sent vectors");

			delay(500);
			cmd2mcu(0, homeCmd);
			Serial.println("Sent home cmd");
			sentHome = TRUE;
			sentMove = FALSE;
	  }
		else if(status == statusLocked && !errorCode && !sentMove) {
			delay(500);
			cmd2mcu(0, moveCmd);
			Serial.println("Sent move cmd");
			sentMove = TRUE;
			sentHome = FALSE;
		}
	}
}

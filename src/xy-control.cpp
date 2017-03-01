
#include <Arduino.h>

#include "xy-control.h"
#include "xy.h"
#include "xy-spi.h"
#include "mcu-cpu.h"

// void printHex8(char byte) {
// 	String byteStr = String((int) byte, HEX);
// 	if(byteStr.length() < 2) byteStr = "0" + byteStr;
// 	Serial.print(byteStr);
// }
// void printHex16(uint16_t i) {
// 	printHex8(i >> 8); printHex8(i & 0x00ff);
// }
//
// void printHex32(uint32_t w) {
// 	printHex16(w >> 16); Serial.print(" "); printHex16(w & 0x0000ffff);
//   Serial.println();
// }

bool_t runningDiagonalTest = FALSE;

char status = 0;
char errorCode = 0;
char errorAxis;
char lastStatus = 0;
char lastErrorCode = 0;
char sleepCmdCounter = 0;

// status rec
StatusRecU statusRec;
char statusRecInBuf[STATUS_SPI_BYTE_COUNT];
char statusRecInIdx = 0;
char bytesSinceStateByte = 0xff; // must match statusRecInIdx

bool_t sentHome = TRUE;
bool_t sentMove = TRUE;
int8_t skipErrors = -1; // for debug only

void initCtrl() {
	initSpi();

  setWordDelay(300);
	zero2mcu(0);
	cmd2mcu(0, sleepCmd);
	zero2mcu(0);
	setWordDelay(0);
}

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
  // Serial.println(String("apiVers: ")   + (int) statusRec.rec.apiVers);
  // Serial.println(String("mfr: ")       + (int) statusRec.rec.mfr);
  // Serial.println(String("prod: ")      + (int) statusRec.rec.prod);
  // Serial.println(String("vers: ")      + (int) statusRec.rec.vers);
  Serial.println(String("homeDistX: ") + statusRec.rec.homeDistX);
  Serial.println(String("homeDistY: ") + statusRec.rec.homeDistY);
	//
  // printHex32(*((uint32_t *) &statusRec.bytes[0]));
  // printHex32(*((uint32_t *) &statusRec.bytes[4]));
  // printHex32(*((uint32_t *) &statusRec.bytes[8]));

	setWordDelay(0);
}

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
			errorCode = statusIn & 0x3e;
			errorAxis = statusIn & 0x01;
		} // else top two bits are zero
		if(bytesSinceStateByte != 0xff) bytesSinceStateByte++;
	}
	if(status != lastStatus) {
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
		if(skipErrors == -1) {
	  	setWordDelay(300);
		  zero2mcu(0);
			cmd2mcu(0, clearErrorCmd);
		  zero2mcu(0);
	  	setWordDelay(0);
		}
		else skipErrors--;
	}
	if(status) lastStatus = status;
	lastErrorCode = errorCode;
}

void chkDiagonalTest();

void chkCtrl(){
  digitalWrite(PWRLED, digitalRead(PWRON));
  char statusIn = zero2mcu(0);
  if(statusIn) chkStatus(statusIn);
  if (runningDiagonalTest) chkDiagonalTest();
}

// state machine
char state = 0;

void diagonalTest() {
  runningDiagonalTest = TRUE;
  state = 0;
}

void chkDiagonalTest() {
  if (errorCode || status == statusNoResponse) {
    state = 0;
    return;
  }
  if(digitalRead(PWRON) == 0) state = 0;

  switch (state) {
    case 0:
      if(digitalRead(PWRON) == 1) {
        // power switch just switched on
        cmd2mcu(0, resetCmd);
        state = 10;
      }
      else {
        // power switch off
        if(sleepCmdCounter++ == 0)
            cmd2mcu(0, sleepCmd);
      }
      break;

    case 10:
      if(status == statusSleeping ||
         status == statusUnlocked) {
        //
        // void vec2mcu(char mcu, char axis, char dir, char ustep,
        //              uint16_t usecsPerPulse, uint16_t pulseCount) {
        vec2mcu(0, X, FORWARD, 2, 1000, 1023);  // 12" but only 1023 each vec
        vec2mcu(0, X, FORWARD, 2, 1000, 1023);
        vec2mcu(0, X, FORWARD, 2, 1000, 1023);
        vec2mcu(0, X, FORWARD, 2, 1000, 1023);
        vec2mcu(0, X, FORWARD, 2, 1000, 1023);
        vec2mcu(0, X, FORWARD, 2, 1000, 6096-5*1023);

        eof2mcu(0, X);

        vec2mcu(0, Y, FORWARD, 2, 1000, 1023);  // 12" but only 1023 each vec
        vec2mcu(0, Y, FORWARD, 2, 1000, 1023);
        vec2mcu(0, Y, FORWARD, 2, 1000, 1023);
        vec2mcu(0, Y, FORWARD, 2, 1000, 1023);
        vec2mcu(0, Y, FORWARD, 2, 1000, 1023);
        vec2mcu(0, Y, FORWARD, 2, 1000, 6096-5*1023);

        eof2mcu(0, Y);
        Serial.println("Sent vectors");

        delay(500);
        cmd2mcu(0, homeCmd);
        Serial.println("Sent home cmd");
        state = 20;
      }
      break;

    case 20:
      if(status == statusLocked) {
        cmd2mcu(0, statusCmd);
				Serial.println("Sent status cmd");

        delay(500);
        cmd2mcu(0, moveCmd);
        Serial.println("Sent move cmd");
        state = 30;
      }
      break;

    case 30:
      if(status == statusMoved) state = 0;
      break;
  }
}

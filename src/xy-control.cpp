
#include <Arduino.h>

#include "xy-control.h"
#include "xy.h"
#include "xy-spi.h"
#include "mcu-cpu.h"

#define digitalRead(PWRON) 1

void printHex8(char byte) {
	String byteStr = String((int) byte, HEX);
	if(byteStr.length() < 2) byteStr = "0" + byteStr;
	Serial.print(byteStr);
}

// void printHex16(uint16_t i) {
// 	printHex8(i >> 8); printHex8(i & 0x00ff);
// }
//
// void printHex32(uint32_t w) {
// 	printHex16(w >> 16); Serial.print(" "); printHex16(w & 0x0000ffff);
//   Serial.println();
// }


char status = 0;
char errorCode = 0;
char errorAxis;
char lastStatus = 0;
char lastErrorCode = 0;

int8_t skipErrors = -1; // for debug only

void chkStatus(char statusIn) {
	if((statusIn & RET_TYPE_MASK) == typeError) {
		errorCode = (statusIn & 0x1f);
	}
  else if((statusIn & RET_TYPE_MASK) == typeState) {
	  status = statusIn & spiStateByteMask;
	  if((statusIn & spiStateByteErrFlag) == 0) errorCode = 0;
		if(status == statusFlashing) errorCode = errorMcuFlashing;
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
		  zero2mcu(0);
			cmd2mcu(0, clearErrorCmd);
		  zero2mcu(0);
		}
		else skipErrors--;
	}
	if(status) lastStatus = status;
	lastErrorCode = errorCode;
}

void chkDiagonalTest();
bool_t runningDiagonalTest = FALSE;

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
  if (errorCode) {
    state = 0;
    return;
  }
  if(digitalRead(PWRON) == 0) state = 0;

  switch (state) {
    case 0:
      if(digitalRead(PWRON) == 1) {
        // power switch just switched on
				delay(100);  // wait 100 ms for mcu and motor drivers to wake up
        cmd2mcu(0, resetCmd);
				cmd2mcu(0, homeCmd);
        state = 10;
      }
      break;

    case 10:
			if(status == statusLocked) {
				char stat;
				// get status rec, just to see hex in console
				do {
					stat = getMcuStatusRec(0);
					if(stat == 254) {
						Serial.println("status rec too long");
						break;
					}
					if((stat & RET_TYPE_MASK) == typeError) return;
				} while (stat != 0);

			  for(char i = 0; i < STATUS_REC_LEN; i++) {
			    printHex8(statusRecInBuf[i]); Serial.print(" ");
				}
			  Serial.println();

				digitalWrite(PWRLED, 1);

        //
        // void vec2mcu(char mcu, char axis, char dir, char ustep,
        //              uint16_t usecsPerPulse, uint16_t pulseCount) {
        vec2mcu(0, X, FORWARD, 2, 1000, 920);      // 45 mm, max 1023 pulses each vec
        eof2mcu(0, X);
        Serial.println("Sent fwd vector");

				delay(7000);
        cmd2mcu(0, moveCmd);
        Serial.println("Sent move cmd");
        state = 20;
				digitalWrite(PWRLED, 0);
      }
      break;

    case 20:
      if(status == statusMoved) {
				digitalWrite(PWRLED, 1);
				cmd2mcu(0, idleCmd);
        Serial.println("Sent idle cmd");

        //
        // void vec2mcu(char mcu, char axis, char dir, char ustep,
        //              uint16_t usecsPerPulse, uint16_t pulseCount) {
        vec2mcu(0, X, BACKWARDS, 2, 1000, 920);     // 45 mm, max 1023 pulses each vec
        eof2mcu(0, X);
        Serial.println("Sent back vector");

				delay(7000);
        cmd2mcu(0, moveCmd);
        Serial.println("Sent move cmd");
        state = 30;
				digitalWrite(PWRLED, 0);
      }
      break;

    case 30:
      if(status == statusMoved) {
				cmd2mcu(0, idleCmd);
				state = 10;
			}
			break;
	}
}

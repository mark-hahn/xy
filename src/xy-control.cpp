
#include <Arduino.h>

#include "xy-control.h"
#include "xy.h"
#include "xy-spi.h"
#include "mcu-cpu.h"
#include "xy-spi-encode.h"

void printHex8(uint8_t byte) {
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

uint8_t status[2] = {0,0};
uint8_t errorCode[2] = {0,0};
uint8_t errorAxis[2] = {0,0};
uint8_t lastStatus[2] = {0,0};
uint8_t lastErrorCode[2] = {0,0};

void chkStatus(int mcu, uint8_t statusIn) {
	if((statusIn & RET_TYPE_MASK) == typeError)
		errorCode[mcu] = (statusIn & 0x3f);

  else if((statusIn & RET_TYPE_MASK) == typeState) {
	  status[mcu] = statusIn & spiStateByteMask;
	  if((statusIn & spiStateByteErrFlag) == 0) errorCode[mcu] = 0;
		if(status[mcu] == statusFlashing) errorCode[mcu] = errorMcuFlashing;
		if(status[mcu] != lastStatus[mcu]) {
	    Serial.print(String("mcu") + mcu+" status: "); Serial.println(status[mcu]);
		}
	}
	if(errorCode[mcu] != lastErrorCode[mcu]) {
		if(errorCode[mcu]) {
			switch(errorCode[mcu] & 0x3e) {
				case errorMcuFlashing:
				  Serial.println(String("mcu") + mcu + " is waiting for flashing"); break;
				case errorReset:
				  Serial.println(String("mcu") + mcu + " was reset"); break;
				case 0x3e:
				  Serial.println(String("mcu") + mcu + " not responding"); break;
        default:
				  if(errorCode[mcu] >= errorSpiByteSync)
            Serial.println(String("mcu") + mcu + " comm error: ");
					Serial.println(String(String("mcu") + mcu + " error, code: ") +
					               String(errorCode[mcu], DEC) +
												 ", axis: "  + String(errorAxis[mcu], DEC));
			}
		}
		else Serial.println(String("mcu") + mcu + " error clear");
  }
	if(errorCode[mcu]) {
		  zero2mcu(mcu);
			byte2mcu(mcu, clearErrorCmd);
		  zero2mcu(mcu);
	}
	lastStatus[mcu] = status[mcu];
	lastErrorCode[mcu] = errorCode[mcu];
}

void chkTest();
bool_t chkMcu1 = TRUE;
bool_t runningTest = FALSE;

/////////////////// action loop  ////////////////////
bool_t lastPwr = 0;

void chkCtrl(){
  digitalWrite(PWRLED, pwrSw);
	if(pwrSw != lastPwr) {
    lastPwr = pwrSw;
		Serial.print("pwr: "); Serial.println(lastPwr, DEC);
	}

  status[0] = zero2mcu(0);
  if(status[0]) chkStatus(0, status[0]);

	if(chkMcu1) {
	  status[1] = zero2mcu(1);
	  if(status[1]) {
			chkStatus(1, status[1]);
			chkMcu1 = FALSE;
		}
	}
  if (runningTest) chkTest();
}


/////////////////// tests  ////////////////////
uint8_t state[2] = {0,0};

void test() {
	state[0] = 0;
  runningTest = TRUE;
}

void chkTest() {
  if (errorCode[0] || pwrSw == 0) {
    state[0] = 0;
    return;
  }

  switch (state[0]) {
    case 0:
      if(pwrSw == 1) {
				delay(100);  // wait 100 ms for mcu and motor drivers to wake up
				Serial.println("sending clear-error, idle, and home cmds");
        byte2mcu(0, clearErrorCmd);
        byte2mcu(0, idleCmd);
        byte2mcu(0, homeCmd);
        state[0] = 10;
      }
      break;

    case 10:
			if(status[0] == statusLocked) {
				Serial.println("locked, sending vel2mcu, eof2mcu, and move");

				// uint8_t stat;
				// get status rec, just to see hex in console
				// do {
				// 	stat = getMcuStatusRec(0);
				// 	if(stat == 254) {
				// 		Serial.println("status rec too long");
				// 		break;
				// 	}
				// 	if((stat & RET_TYPE_MASK) == typeError) return;
				// } while (stat != 0);
				//
			  // for(uint8_t i = 0; i < STATUS_REC_LEN; i++) {
			  //   printHex8(statusRecInBuf[i]); Serial.print(" ");
				// }
			  // Serial.println();

 				vel2mcu(0, X, FORWARD, 3, 1000, 1000);
        eof2mcu(0, X);
        eof2mcu(0, Y);
        Serial.println("Sent fwd/eof vector");

        byte2mcu(0, moveCmd);
        Serial.println("Sent move cmd");
        state[0] = 20;
      }
      break;

    case 20:
      if(status[0] == statusMoved) {
				byte2mcu(0, idleCmd);
 				vel2mcu(0, X, BACKWARDS, 3, 1000, 1000);
        eof2mcu(0, X);
        eof2mcu(0, Y);
        Serial.println("Sent back/eof vector");

        byte2mcu(0, moveCmd);
        Serial.println("Sent move cmd");

        state[0] = 30;
      }
      break;

    case 30:
      if(status[0] == statusMoved) {
				byte2mcu(0, idleCmd);
				state[0] = 10;
			}
			break;
	}
}

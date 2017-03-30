
#include <Arduino.h>

#include "xy-control.h"
#include "xy.h"
#include "xy-spi.h"
#include "mcu-cpu.h"

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

uint8_t skipErrors = 0; // for debug only

void chkStatus(int mcu, uint8_t statusIn) {
	if((statusIn & RET_TYPE_MASK) == typeError) {
		errorCode[mcu] = (statusIn & 0x3f);
	}
  else if((statusIn & RET_TYPE_MASK) == typeState) {
	  status[mcu] = statusIn & spiStateByteMask;
	  if((statusIn & spiStateByteErrFlag) == 0) errorCode[mcu] = 0;
		if(status[mcu] == statusFlashing) errorCode[mcu] = errorMcuFlashing;
	}
	if(status[mcu] != lastStatus[mcu]) {
		if(lastStatus[mcu] == 0xff) lastStatus[mcu] = status[mcu];
    Serial.print(String("mcu") + mcu+" status: "); Serial.println(status[mcu], DEC);
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
		else {
		  Serial.println(String("mcu") + mcu + " error clear");
			lastStatus[mcu] = 0xff;
		}
  }
	if(errorCode[mcu]) {
		if(skipErrors == 0) {
		  zero2mcu(mcu);
			byte2mcu(mcu, clearErrorCmd);
		  zero2mcu(mcu);
		}
		else skipErrors--;
	}
	if(status[mcu] && lastStatus[mcu] != 0xff)
	  lastStatus[mcu] = status[mcu];
	lastErrorCode[mcu] = errorCode[mcu];
}

void chkDiagonalTest();
bool_t runningDiagonalTest = FALSE;

void chkZTest();
bool_t runningZTest = FALSE;


/////////////////// action loop  ////////////////////
bool_t lastPwr = 0;

void chkCtrl(){
  digitalWrite(PWRLED, pwrSw);
	if(pwrSw != lastPwr) {
    lastPwr = pwrSw;
		Serial.print("pwr: "); Serial.println(lastPwr, DEC);
	}

  uint8_t statusIn = zero2mcu(0);
  if(statusIn) chkStatus(0, statusIn);

  // statusIn = zero2mcu(1);
  // if(statusIn) chkStatus(1, statusIn);
	//
  // if (runningDiagonalTest) chkDiagonalTest();
	// if (runningZTest) chkZTest();
}

//
// /////////////////// tests  ////////////////////
// uint8_t state[2] = {0,0};
//
// void diagonalTest() {
// 	if(runningZTest) return;
// 	state[0] = 0;
//   runningDiagonalTest = TRUE;
// }
//
// void chkDiagonalTest() {
//   uint8_t statusIn = zero2mcu(0);
//   if(statusIn) chkStatus(0,statusIn);
//
//   if (errorCode) {
//     state[0] = 0;
//     return;
//   }
//   if(pwrSw == 0) state[0] = 0;
//
//   switch (state[0]) {
//     case 0:
//       if(pwrSw == 1) {
//         // power switch just switched on
// 				delay(100);  // wait 100 ms for mcu and motor drivers to wake up
//         byte2mcu(0, clearErrorCmd);
//         byte2mcu(0, idleCmd);
//         byte2mcu(0, homeCmd);
//         state[0] = 10;
//       }
//       break;
//
//     case 10:
// 			if(status[0] == statusLocked) {
// 				uint8_t stat;
// 				// get status rec, just to see hex in console
// 				do {
// 					stat = getMcuStatusRec(0);
// 					if(stat == 254) {
// 						Serial.println("status rec too long");
// 						break;
// 					}
// 					if((stat & RET_TYPE_MASK) == typeError) return;
// 				} while (stat != 0);
//
// 			  for(uint8_t i = 0; i < STATUS_REC_LEN; i++) {
// 			    printHex8(statusRecInBuf[i]); Serial.print(" ");
// 				}
// 			  Serial.println();
//
// 				digitalWrite(PWRLED, 1);
//
//         //
//         // void vec2mcu(uint8_t mcu, uint8_t axis, uint8_t dir, uint8_t ustep,
//         //              uint16_t usecsPerPulse, uint16_t pulseCount) {
//         vec2mcu(0, X, FORWARD, 2, 1000, 920);      // 45 mm, max 1023 pulses each vec
//         eof2mcu(0, X);
//         Serial.println("Sent fwd vector");
//
// 				delay(7000);
//         byte2mcu(0, moveCmd);
//         Serial.println("Sent move cmd");
//         state[0] = 20;
// 				digitalWrite(PWRLED, 0);
//       }
//       break;
//
//     case 20:
//       if(status[0] == statusMoved) {
// 				digitalWrite(PWRLED, 1);
// 				byte2mcu(0, idleCmd);
//         Serial.println("Sent idle cmd");
//
//         //
//         // void vec2mcu(uint8_t mcu, uint8_t axis, uint8_t dir, uint8_t ustep,
//         //              uint16_t usecsPerPulse, uint16_t pulseCount) {
//         vec2mcu(0, X, BACKWARDS, 2, 1000, 920);     // 45 mm, max 1023 pulses each vec
//         eof2mcu(0, X);
//         Serial.println("Sent back vector");
//
// 				delay(7000);
//         byte2mcu(0, moveCmd);
//         Serial.println("Sent move cmd");
//         state[0] = 30;
// 				digitalWrite(PWRLED, 0);
//       }
//       break;
//
//     case 30:
//       if(status[0] == statusMoved) {
// 				byte2mcu(0, idleCmd);
// 				state[0] = 10;
// 			}
// 			break;
// 	}
// }
//
//
// ///////////////////////// Z TEST /////////////////////////
// void ZTest() {
// 	if(runningDiagonalTest) return;
// 	state[1] = 0;
//   runningZTest = TRUE;
// }
//
// void chkZTest(){
//   uint8_t statusIn = zero2mcu(1);
//   if(statusIn) chkStatus(1,statusIn);
//
//   // if (errorCode[1]) {
//   //   state[1] = 0;
//   //   return;
//   // }
//   if(pwrSw == 0) state[1] = 0;
//
//   switch (state[1]) {
//     case 0:
//       if(pwrSw == 1) {
// 				Serial.println("power switch just switched on");
//         // power switch just switched on
//         // byte2mcu(1, clearErrorCmd);
//         // byte2mcu(1, idleCmd);
//         byte2mcu(1, homeCmd);
// 				Serial.println("sent home cmd to Z");
//         state[1] = 10;
//       }
//       break;
// 	}
// }

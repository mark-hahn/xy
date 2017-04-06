
#include <Arduino.h>

#include "xy-control.h"
#include "xy.h"
#include "xy-spi.h"
#include "mcu-cpu.h"
#include "xy-spi-encode.h"

// test params

// noisy
// uint8_t  ustep       = 0;
// uint16_t pulseCount  = 750;
// uint16_t ppsX        = 4000;   // 800 mm/sec
// uint16_t ppsY        = 2300;   // 460 mm/sec
// int8_t   accell      = 10;
// uint16_t startPPS    = 10;    //    2 mm/sec
// uint8_t  rampFactor  = 25;

// runs quiet and reliabily
// needs 1.5 A motor current
uint8_t  ustep       = 1;
uint16_t pulseCount  = 1000;
uint16_t ppsX        = 4000;   // 400 mm/sec
uint16_t ppsY        = 2400;   // 240 mm/sec
int8_t   accell      = 10;
uint16_t startPPS    = 10;     //    1 mm/sec
uint8_t  rampFactor  = 50;

uint8_t  rampPulseCountX = ppsX/rampFactor;
uint8_t  rampPulseCountY = ppsY/rampFactor;

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
		  getMcuState(mcu);
			cmd2mcu(mcu, clearErrorCmd);
		  getMcuState(mcu);
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

  status[0] = getMcuState(0);
  chkStatus(0, status[0]);

	if(chkMcu1) {
	  status[1] = getMcuState(1);
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

				cmd2mcu(0, idleCmd);
				delay(1000);
				Serial.println("sending home cmd");
				cmd2mcu(0, clearDistance);
        cmd2mcu(0, homeCmd);
        state[0] = 10;
      }
      break;

    case 10:
			if(status[0] == statusMoved) {

				rampPulseCountX = ppsX/rampFactor;
				rampPulseCountY = ppsY/rampFactor;

				cmd2mcu(0, idleCmd);

				// uint8_t stat;
				// do {
				// 	stat = getMcuStatusRec(0);
				// 	if(stat == 254) {
				// 		Serial.println("status rec too long");
				// 		break;
				// 	}
				// 	if((stat & RET_TYPE_MASK) == typeError) return;
				// } while (stat != 0);
				// dumpStatusRec();

				settingsVector2mcu(0, X, FORWARD, ustep, startPPS, accell);
			  moveVector2mcu(    0, X, FORWARD, ustep,      ppsX, pulseCount);
			  moveVector2mcu(    0, X, FORWARD, ustep,      ppsX, pulseCount-rampPulseCountX);
			  moveVector2mcu(    0, X, FORWARD, ustep, startPPS, rampPulseCountX);
				eofVector2mcu(     0, X);
				eofVector2mcu(     0, Y);

        cmd2mcu(0, moveCmd);
        state[0] = 20;
      }
      break;

    case 20:
      if(status[0] == statusMoved) {
				cmd2mcu(0, idleCmd);

				eofVector2mcu(     0, X);
				settingsVector2mcu(0, Y, FORWARD, ustep,   startPPS, accell);
			  moveVector2mcu(    0, Y, FORWARD, ustep, ppsY, pulseCount);
			  moveVector2mcu(    0, Y, FORWARD, ustep, ppsY, pulseCount-rampPulseCountY);
			  moveVector2mcu(    0, Y, FORWARD, ustep,   startPPS, rampPulseCountY);
				eofVector2mcu(     0, Y);
        cmd2mcu(0, moveCmd);
        state[0] = 30;
      }
      break;

    case 30:
			if(status[0] == statusMoved) {
				cmd2mcu(0, idleCmd);

				// uint8_t stat;
				// do {
				// 	stat = getMcuStatusRec(0);
				// 	if(stat == 254) {
				// 		Serial.println("status rec too long");
				// 		break;
				// 	}
				// 	if((stat & RET_TYPE_MASK) == typeError) return;
				// } while (stat != 0);
				// dumpStatusRec();

				settingsVector2mcu(0, X, BACKWARDS, ustep, startPPS, accell);
			  moveVector2mcu(    0, X, BACKWARDS, ustep,      ppsX, pulseCount);
			  moveVector2mcu(    0, X, BACKWARDS, ustep,      ppsX, pulseCount-rampPulseCountX);
			  moveVector2mcu(    0, X, BACKWARDS, ustep, startPPS, rampPulseCountX);
				eofVector2mcu(     0, X);
				eofVector2mcu(     0, Y);

        cmd2mcu(0, moveCmd);
        state[0] = 40;
      }
      break;

    case 40:
      if(status[0] == statusMoved) {
				cmd2mcu(0, idleCmd);

				eofVector2mcu(     0, X);
				settingsVector2mcu(0, Y, BACKWARDS, ustep,   startPPS, accell);
			  moveVector2mcu(    0, Y, BACKWARDS, ustep, ppsY, pulseCount);
			  moveVector2mcu(    0, Y, BACKWARDS, ustep, ppsY, pulseCount-rampPulseCountY);
			  moveVector2mcu(    0, Y, BACKWARDS, ustep,   startPPS, rampPulseCountY);
				eofVector2mcu(     0, Y);

        cmd2mcu(0, moveCmd);
        state[0] = 50;
      }
      break;

    case 50:
      if(status[0] == statusMoved) {
  			cmd2mcu(0, idleCmd);
				state[0] = 10;

				// accell += 2;
				// Serial.print("accell: "); Serial.println(accell);
			}
			break;
	}
}

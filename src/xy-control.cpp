
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
// uint8_t  RAMP_FACTOR  = 25;

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
					               String(errorCode[mcu] & 0x3e, DEC) +
												 ", axis: "  + String(errorCode[mcu], DEC));
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

  status[XY] = getMcuState(XY);
  chkStatus(XY, status[XY]);

	if(chkMcu1) {
	  status[Z] = getMcuState(Z);
	  if(status[Z]) {
			chkStatus(Z, status[Z]);
			// chkMcu1 = FALSE;
		}
	}
  if (runningTest) chkTest();
}

//////////////////////////  action routines  ////////////////////
#define XY 0
#define  Z 1
#define  X 0
#define  Y 1
#define  UP FORWARD
#define  DN BACKWARDS
#define  RAMP_FACTOR 50
bool_t penIsUp = FALSE;
uint8_t state = 0;

bool_t busy(uint8_t unused, bool_t unlockedOK) {
  if( (status[XY] == statusLocked || status[XY] == statusMoved ||
	        (unlockedOK && (status[XY] == statusUnlocked))) &&
			(status[Z] == statusLocked || status[Z] == statusMoved ||
		 	    (unlockedOK && (status[Z] == statusUnlocked)))) {
  	Serial.print("State: ");  Serial.println(state);
		return FALSE;
	}
	return TRUE;
}
bool_t homeXY() {
	if(busy(XY, TRUE)) return FALSE;
	delay(1000);
	Serial.println("sending XY home cmd");
	cmd2mcu(XY, clearDistance);
  cmd2mcu(XY, homeCmd);
  return TRUE;
}

bool_t homePen() {
	if(busy(Z, TRUE)) return FALSE;
	Serial.println("sending Z home cmd");
	cmd2mcu(Z, clearDistance);
  cmd2mcu(Z, homeCmd);
  return TRUE;
}

void setting2mcu(uint8_t mcu, uint8_t setting, int16_t value) {
  uint8_t paramBytes[3];
	paramBytes[0] = setting;
	paramBytes[1] = ((uint8_t *) &value)[1];
	paramBytes[2] = ((uint8_t *) &value)[0];
	cmdWParams2mcu(mcu, settingsCmd, 3, paramBytes);
}

void settings() {
	Serial.println("");
	Serial.println("");
	Serial.println("");
	Serial.println("");
  setting2mcu(XY, homeOfsX, 100);
  setting2mcu(XY, homeOfsY, 100);
  settingsVector2mcu(XY, X, 0, 1, 10, 8);
  settingsVector2mcu(XY, Y, 0, 1, 10, 8);
  settingsVector2mcu( Z, 0, 0, 0, 10, 8);
}

bool_t move(int16_t mmDistX, int16_t mmDistY) {
	if(busy(XY, FALSE)) return FALSE;
	uint16_t rampCount;
	uint8_t dirX = (mmDistX >= 0);
	uint8_t dirY = (mmDistY >= 0);
	if (!dirX) mmDistX = -1 * mmDistX;
	if (!dirY) mmDistY = -1 * mmDistY;

  settingsVector2mcu(XY, X, 0, 1, 10, 8);
  settingsVector2mcu(XY, Y, 0, 1, 10, 8);

	if(penIsUp) {
		rampCount = 2000/RAMP_FACTOR;
		Serial.println("move with pen up");
		Serial.print("rampCount ");Serial.println(rampCount);
		Serial.print("dirX ");Serial.println(dirX);
		Serial.print("dirY ");Serial.println(dirY);
		Serial.print("mmDistX ");Serial.println(mmDistX);
		Serial.print("mmDistY ");Serial.println(mmDistY);
		if(mmDistX && rampCount > mmDistX*10) {
		  Serial.println("badramp X");
			delay(10000);
		}
		if(mmDistX) {
	    moveVector2mcu(XY, X, dirX, 1, 2000, mmDistX*10 - rampCount);
	    moveVector2mcu(XY, X, dirX, 1,   10, rampCount);
		}
		if(mmDistY && rampCount > mmDistY*10) {
		  Serial.println("badramp Y");
			delay(10000);
		}
		if(mmDistY) {
	    moveVector2mcu(XY, Y, dirY, 1, 2000, mmDistY*10 - rampCount);
	    moveVector2mcu(XY, Y, dirY, 1,   10, rampCount);
		}
	} else {
		rampCount = 1000/RAMP_FACTOR;
		Serial.println("move with pen down");
		Serial.print("rampCount");Serial.println(rampCount);
		Serial.print("dirX ");Serial.println(dirX);
		Serial.print("dirY ");Serial.println(dirY);
		Serial.print("mmDistX ");Serial.println(mmDistX);
		Serial.print("mmDistY ");Serial.println(mmDistY);
		if(mmDistX) {
	    moveVector2mcu(XY, X, dirX, 1, 1000, mmDistX*10 - rampCount);
	    moveVector2mcu(XY, X, dirX, 1,   10, rampCount);
		}
		if(mmDistY) {
	    moveVector2mcu(XY, Y, dirY, 1, 1000, mmDistY*10-rampCount);
      moveVector2mcu(XY, Y, dirY, 1,   10, rampCount);
		}
	}
	eofVector2mcu(XY, X);
	eofVector2mcu(XY, Y);
  cmd2mcu(XY, moveCmd);
	return TRUE;
}

bool_t penUp(uint16_t mmDist) {
  if(busy(Z, TRUE)) return FALSE;
	Serial.println("penUp");
	penIsUp = TRUE;
  settingsVector2mcu(Z, 0, FORWARD, 0, 10, 8);
  moveVector2mcu(Z, 0, FORWARD,  0, 1000, mmDist*5);
	eofVector2mcu(Z, 0);
  cmd2mcu(Z, moveCmd);
	return TRUE;
}

bool_t penDn(uint16_t mmDist) {
  if(busy(Z, FALSE)) return FALSE;
	penIsUp = FALSE;
  settingsVector2mcu(Z, 0, FORWARD, 0, 10, 8);
  moveVector2mcu(Z, 0, BACKWARDS, 0, 1000, mmDist*5);
	eofVector2mcu(Z, 0);
  cmd2mcu(Z, moveCmd);
	return TRUE;
}




////////////////////////////  test  /////////////////////////////

void test() {
	state = 0;
  runningTest = TRUE;
}

uint16_t moveOfs  = 20;
uint16_t pressure = 10;

void chkTest() {
  if (errorCode[0] || errorCode[1] || pwrSw == 0) {
    state = 0;
		cmd2mcu(XY, unlockCmd);
		cmd2mcu( Z, unlockCmd);
    return;
  }

  switch (state) {
    case   0: settings();                       state =  10; break;
		case  10: cmd2mcu(Z, lockCmd);							state =  12; break;

		case  12: if (penUp(20))   								  state =  15; break;
    case  15: if (homeXY())  										state =  17; break;
	  case  17: if (move(30, 10)) 						    state =  19; break;
    case  19: if (homePen()) 										state =  21; break;
		case  21: if (penUp(3)) 										state =  30; break;

// -- loop --
	  case  30: if (move(moveOfs, 0)) 						state =  40; break;

		case  40: if (penDn(pressure)) 						  state =  50; break;
	  case  50: if (move(   0, moveOfs)) 					state =  70; break;
	  case  70: if (move(-moveOfs,   0))				  state = 100; break;

		case 100: if (penUp(pressure)) 						  state = 105; break;
	  case 105: if (move(   0, -moveOfs)) 				state = 110; break;

    case 110: moveOfs += 10;                    state = 30; break;
  }
}


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

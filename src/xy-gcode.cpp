#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <Arduino.h>
#include "xy-gcode.h"
#include "xy-gcode-parse.h"
#include "mcu-cpu.h"

#define EMPTY_PARAM 0x7fff0000

/////////////////  GCode param current values  /////////////////
uint32_t lineNumber = 0;
uint8_t  gc_ustepX = 2, gc_ustepY = 2, gc_ustepZ = 2;  // 1, 1/2, 1/4,... microstep
bool_t   gc_usingInches = FALSE, gc_relativePos = FALSE;

int32_t  gc_feedRate = 6000;  //  mm/min
uint8_t  gc_feedRateExp = 0

int16_t  gc_accelX = gc_accelY = gc_accelZ = 1000; // integer mm/sec/sec
int16_t  gc_jerkXY = gc_jerkZ = 10; // integer mm/sec

int32_t  gc_ofsManX = 0, gc_ofsManY, gc_ofsManZ = 0;
int8_t   gc_ofsExpX = 0, gc_ofsExpY, gc_ofsExpZ = 0;

bool_t   reverseX = FALSE, bool_t reverseY = FALSE;


/////////////////  state of mcu/motors  /////////////////
int32_t mcu_posX = 0, mcu_posY = 0, mcu_posZ= 0; // 1/32 step units, relative to home
uint8_t homeSubStepX, homeSubStepY;              // substep ofs of home (0-31)
uint16_t mcu_accelX = mcu_accelY = mcu_accelZ = 0xffff; // integer mm/sec/sec


/////////////////  global temp parameter values  /////////////////
int32_t paramManS, paramManP, paramManX, paramManY, paramManZ;
uint8_t paramExpS, paramExpP, paramExpX, paramExpY, paramExpZ;


/////////////////  utility routines  /////////////////
void gcodeErr(char *lineOut, char *msg, char cmdLtr, uint16_t cmdNum) {
  sprintf(lineOut, "Error: %s, %c%d (Line %d)\r\n", msg, cmdLtr, cmdNum, lineNumber);
  Serial.print(lineOut);
}

// find param letter idx, ignores [0] which is always G or M
uint8_t findParam(char pltr) {
  for(int8_t i=1; i<numParams; i++)
    if(paramLetter[i] == pltr) return i;
  return 0;
}

void handleParams() {
  paramManS = paramManP = paramManX = paramManY = paramManZ = EMPTY_PARAM;
  for(uint8_t idx=1; idx < numParams; idx++) {
    switch(paramLetter[idx]) {
      'F': gc_feedRate = paramMan[idx]; gc_feedRateExp = paramExp[idx]; break;
      'S': paramManS = paramMan[idx]; paramExpS = paramExp[idx]; break;
      'P': paramManP = paramMan[idx]; paramExpP = paramExp[idx]; break;
      'X': paramManX = paramMan[idx]; paramExpX = paramExp[idx]; break;
      'Y': paramManY = paramMan[idx]; paramExpY = paramExp[idx]; break;
      'Z': paramManZ = paramMan[idx]; paramExpZ = paramExp[idx]; break;
      default: continue;
    }
    paramLetter[idx] = 0;  // param was handled
  }
}

// convert gcode ustep format to mcu format
// return 99 if invalid
uint8_t ustepGc2Mcu(uint8_t gc_ustep) {
  switch(gc_ustep) {
    case  1: return 0;
    case  2: return 1;
    case  4: return 2;
    case  8: return 3;
    case 16: return 4;
    case 32: return 5;
  }
  return 99;
}

// set ustep value, if present; return TRUE on error
bool_t handleUstep(char paramLtr, uint8_t *ustepPtr, char *lineOut) {
  uint8_t idx = findParam(paramLtr);
  if(idx == 0) return FALSE;
  uint8_t ustep = (uint8_t) decFp2Int(paramMan[idx], paramExp[idx]);
  if(ustepGc2Mcu(ustep) == 99) {
      gcodeErr(lineOut, "bad microstep value", paramLtr, paramMan[idx]);
      return TRUE;
  }
  *ustepPtr = ustep;
  return FALSE;
}


/////////////////  exec line  /////////////////
// on error, returns -1 and  line starts with ERROR:
// return of 0 means to try again
// return of 1 means to move on to next line
int8_t execGCodeLine(char *lineIn, char *lineOut) {
  int16_t ret = parseGCodeLine(lineIn);
  if(ret == 0) { // empty line
    *lineOut = 0;
    return 1;
  }
  if (ret < 0) {
    gcodeErr(lineOut, "GCode syntax", "", -ret);
    return -1;
  }
  uint16_t cmd = (paramLetter[0] == 'G' ? 10000 : 20000) + (uint16_t) paramMan[0];

  // do we need to wait?
  if (cmd != 10000 && cmd != 10001 &&
       (getMcuState(XY) == moving || getMcuState(Z) == moving ||
        getMcuState(XY) == homing || getMcuState(Z) == homing)
    return 0;

  lineNumber++;
  switch (cmd) {
    case 10000: // G0 -- move
    case 10001: // G1
      handleParams();
      if(gc_accelX != mcu_accelX) {
        settingsVector2mcu(XY, X, 0, gc_ustepX, 0, gc_accelX); // no dir or pps
        mcu_accelX = gc_accelX;
      }
      if(gc_accelY != mcu_accelY) {
        settingsVector2mcu(XY, Y, 0, gc_ustepY, 0, gc_accelY);
        mcu_accelY = gc_accelY;
      }
      if(gc_accelZ != mcu_accelZ) {
        settingsVector2mcu(Z, 0, 0, gc_ustepZ, 0, gc_accelZ); // no axis, dir, or pps
        mcu_accelZ = gc_accelZ;
      }
      // int32_t mcu_newPosX = mcu_posX +

      break;

    case 20017: // M17: Enable/Power all stepper motors
      cmd2mcu(lockCmd);
      break;
    case 20018: // M18: disable all stepper motors
      cmd2mcu(unlockCmd);
      break;

    case 20201: // M201: Set max acceleration (integer mm/sec/sec)
      handleParams();
      if(paramManX != EMPTY_PARAM) gc_accelX = decFp2Int(paramManX, paramExpX);
      if(paramManY != EMPTY_PARAM) gc_accelY = decFp2Int(paramManY, paramExpY);
      if(paramManZ != EMPTY_PARAM) gc_accelZ = decFp2Int(paramManZ, paramExpZ);
      break;

    case 20205: // M205: Set max jerk (integer mm/sec)
      handleParams();
      if(paramManX != EMPTY_PARAM) gc_jerkXY = decFp2Int(paramManX, paramExpX);
      if(paramManZ != EMPTY_PARAM) gc_jerkZ  = decFp2Int(paramManZ, paramExpZ);
      break;

    case 20206: // M206: Offset axes (change home position)
      handleParams();
      if(paramManX != EMPTY_PARAM) {
        gc_ofsManX = paramManX;
        gc_ofsExpX = paramExpX;
      }
      if(paramManY != EMPTY_PARAM) {
        gc_ofsManY = paramManY;
        gc_ofsExpY = paramExpY;
      }
      if(paramManZ != EMPTY_PARAM) {
        gc_ofsManZ = paramManZ;
        gc_ofsExpZ = paramExpZ;
      }
      break;

    case 10020: // G20: Set Units to Inches
      gcodeErr(lineOut, "inch units not supported", paramLetter[0], paramMan[0]);
      return -1;
      // gc_usingInches = TRUE;
      // break;
    case 10021: // G21: Set Units to Millimeters
      gc_usingInches = FALSE;
      break;

    case 10090: // G90: Set positioning absolute
      gc_relativePos = FALSE;
      break;
    case 10091: // G21: Set positioning relative
      gcodeErr(lineOut, "relative positioning not supported", paramLetter[0], paramMan[0]);
      return -1;
      // gc_relativePos = TRUE;
      // break;

    case 20350: // M350: set ustep
      if(findParam('S')) {
        if(handleUstep('S', &gc_ustepX, lineOut)) return -1;
        if(handleUstep('S', &gc_ustepY, lineOut)) return -1;
        if(handleUstep('S', &gc_ustepZ, lineOut)) return -1;
      }
      if(handleUstep('X', &gc_ustepX, lineOut)) return -1;
      if(handleUstep('Y', &gc_ustepY, lineOut)) return -1;
      if(handleUstep('Z', &gc_ustepZ, lineOut)) return -1;
      break;

    default:
      gcodeErr(lineOut, "unknown command", paramLetter[0], paramMan[0]);
  }
  strCpy(lineOut, "OK");
}

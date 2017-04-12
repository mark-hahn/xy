

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <Arduino.h>
#include "xy-gcode.h"
#include "mcu-cpu.h"
#include "xy-dec-float.h"

#define MAX_LINE_LEN 64
#define MAX_PARAMS   10

uint8_t  numParams;
char     paramLetter[MAX_PARAMS];
int32_t  paramMan[MAX_PARAMS];  // mantissa, val = paramMan * (10 **(-paramExp))
uint8_t  paramExp[MAX_PARAMS];  // number of digits after dec point

// returns number of params or negative means syntax error code(code * 100 + param number)
int16_t parseGCodeLine(char *chr) {
  char *end = chr + MAX_LINE_LEN;
  char sign;
  bool_t bol = TRUE, haveSign = FALSE, haveDot = FALSE, inNum = FALSE;
  int8_t paramIdx = 0;
  paramLetter[0] = 0;
  for(; *chr && chr < end; chr++) {
    switch(*chr) {
    case ' ':
    case '\t': continue;

    case '\r':
    case '\n':
    case ';':
    case '*':
    case '(':
    case '%':
    case '/':
      if(!bol && haveSign && sign == '-') paramMan[paramIdx] *= -1;
      numParams = paramIdx+1;
      return numParams + 1;

    case 'N':
      if(!bol) return -500 - paramIdx;
      continue;

    case 'M':
    case 'G':
      if(!bol) return -600 - paramIdx;
      bol = FALSE;
      paramLetter[0] = *chr;
      paramMan[0] = 0;
      paramExp[0] = 0;
      continue;

    case '+':
    case '-':
      if(bol || haveSign || inNum) return -100 - paramIdx;
      sign = *chr;
      haveSign = TRUE;
      inNum    = TRUE;
      continue;

    case '.':
      if(bol || haveDot) return -200 - paramIdx;
      if(!haveSign) {
        haveSign = TRUE;
        sign = '+';
      }
      haveDot = TRUE;
      inNum   = TRUE;
      continue;

    default:
      if(*chr >= '0' && *chr <= '9') {
        if(bol) continue;
        if(!haveSign) {
          haveSign = TRUE;
          sign = '+';
        }
        inNum = TRUE;
        paramMan[paramIdx] *= 10;
        paramMan[paramIdx] += (*chr - '0');
        if(haveDot) paramExp[paramIdx]++;
        continue;
      }
      else if(*chr >= 'A' && *chr <= 'Z') {
        if(bol) return -300 - paramIdx;
        if(haveSign && sign == '-') paramMan[paramIdx] *= -1;
        haveSign = haveDot = inNum = FALSE;
        paramIdx++;
        paramLetter[paramIdx] = *chr;
        paramMan[paramIdx] = 0;
        paramExp[paramIdx] = 0;
        continue;
      }
      else return -400 - paramIdx;
      continue;
    }
  }
  if(!bol) {
    if(haveSign && sign == '-') paramMan[paramIdx] *= -1;
    numParams = paramIdx+1;
    return numParams;
  }
  return 0;
}

void testGCodeLine(char *line, uint32_t lineNum) {
  int16_t numParams = parseGCodeLine(line);
  if(numParams < 0) {
    Serial.print("G-Code parse error: "); Serial.println(line);
    Serial.print("LineNum, code: ");
    Serial.print(lineNum) + Serial.print(":"); Serial.println(-numParams+1);
  } else {
    for(uint8_t i=0; i<numParams; i++) {
        Serial.print(paramLetter[i]); Serial.print(": ");
        Serial.print(paramMan[i]); Serial.print(", "); Serial.println(paramExp[i]);
    }
  }
}

///////////////////////////////  EMULATION  ///////////////////////////////
// GCode param current values
uint32_t lineNumber = 0;
uint8_t  gc_ustepX = 2, gc_ustepY = 2, gc_ustepZ = 2,
bool_t   gc_usingInches = FALSE;
int32_t  gc_feedRate = 60000, accel = 1000;
uint8_t  gc_feedRateExp = 0, accelExp = 0;
int32_t  ofsX = 0, ofsY = 0;
bool_t   reverseX = FALSE, bool_t reverseY = FALSE;

// state of mcu/motors
int32_t posX = 0, posY = 0;          // 1/32 step units, relative to home
uint8_t homeSubStepX, homeSubStepY;  // substep state of home (0-31)

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
  for(uint8_t idx=1; idx < numParams; idx++) {
    switch(paramLetter[idx]) {
      'F': gc_feedRate = paramMan[idx]; gc_feedRateExp = paramExp[idx]; break;


      default: continue;
    }
    paramLetter[idx] = 0;  // param was handled
  }
}

// set ustep value, if present; return TRUE on error
bool_t handleUstep(char paramLtr, uint8_t *ustepPtr, char *lineOut) {
  uint8_t idx = findParam(paramLtr);
  if(idx == 0) return FALSE;
  uint8_t ustep;
  switch(paramMan[idx]) {
    case  1: ustep = 0; break;
    case  2: ustep = 1; break;
    case  4: ustep = 2; break;
    case  8: ustep = 3; break;
    case 16: ustep = 4; break;
    case 32: ustep = 5; break;
    default:
      gcodeErr(lineOut, "bad microstep value", paramLtr, paramMan[idx]);
      return TRUE;
  }
  *ustepPtr = ustep;
  return FALSE;
}

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

      break;

    case 20017: // M17: Enable/Power all stepper motors
      cmd2mcu(lockCmd);
      break;

    case 10020: // G20: Set Units to Inches
      gc_usingInches = TRUE;
      break;

    case 10021: // G21: Set Units to Millimeters
      gc_usingInches = FALSE;
      break;

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

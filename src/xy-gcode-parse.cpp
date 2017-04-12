
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <Arduino.h>
#include "xy-gcode-parse.h"
#include "mcu-cpu.h"

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

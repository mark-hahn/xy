#ifndef GCPARSE
#define GCPARSE

#define MAX_LINE_LEN 64
#define MAX_PARAMS   10

extern uint8_t  numParams;
extern char     paramLetter[MAX_PARAMS];
extern int32_t  paramMan[MAX_PARAMS];  // mantissa, val = paramMan * (10 **(-paramExp))
extern uint8_t  paramExp[MAX_PARAMS];  // number of digits after dec point

// returns number of params or negative means syntax error code(code * 100 + param number)
int16_t parseGCodeLine(char *chr);
void testGCodeLine(char *line, uint32_t lineNum);

#endif

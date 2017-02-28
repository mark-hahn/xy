
#include <SPI.h>
#include "xy-spi.h"
#include "mcu-cpu.h"

#define BYTE_DELAY     10   // usecs between  8-bit bytes
#define DEF_WORD_DELAY 100   // usecs between 32-bit words (too short causes err 18)

#define SCK 14

int32 speedByMcu[3] = {4000000, 4000000, 4000000};
char  ssPinByMcu[3] = {15, 16, 0};
uint16_t wordDelay = DEF_WORD_DELAY;

void initSpi() {
  pinMode(SCK, OUTPUT); // why?
  for (char mcu=0; mcu < 3; mcu++) {
    digitalWrite(ssPinByMcu[mcu],1); // should this be before or after pinMode ?
    pinMode(ssPinByMcu[mcu], OUTPUT);
    digitalWrite(ssPinByMcu[mcu],1);
  }
	SPI.begin();
	SPI.setHwCs(false);  // our code will drive SS, not library
}

void setWordDelay(uint16_t wordDelayIn) {
  wordDelay = wordDelayIn;
  if(wordDelay == 0) wordDelay = DEF_WORD_DELAY;
}

char byte2mcu(char mcu, char byte) {
  SPI.beginTransaction(SPISettings(speedByMcu[mcu], MSBFIRST, SPI_MODE0));
  char byteBack = SPI.transfer(byte);
  SPI.endTransaction();
  return byteBack;
}

// array is little-endian (unusual)
// spi is big-endian
char bytes2mcu(char mcu, char *bytes) {
	digitalWrite(ssPinByMcu[mcu],0);
  char status = byte2mcu(bytes[3], mcu);
	delayMicroseconds(BYTE_DELAY);

	byte2mcu(bytes[2], mcu);
  delayMicroseconds(BYTE_DELAY);

	byte2mcu(bytes[1], mcu);
	delayMicroseconds(BYTE_DELAY);

	byte2mcu(bytes[0], mcu);
  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelay);
  return status;
}

/// endian flip is needed in each int
char ints2mcu(char mcu, uint16_t int1, uint16_t int2) {
	digitalWrite(ssPinByMcu[mcu],0);
  char status = byte2mcu(mcu, ((char *) &int1)[1]);
	delayMicroseconds(BYTE_DELAY);

	byte2mcu(mcu, ((char *) &int1)[0]);
  delayMicroseconds(BYTE_DELAY);

  byte2mcu(mcu, ((char *) &int2)[1]);
	delayMicroseconds(BYTE_DELAY);

	byte2mcu(mcu, ((char *) &int2)[0]);
  delayMicroseconds(BYTE_DELAY);

  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelay);
  return status;
}

// word and array are both little-endian
char word2mcu(char mcu, uint32_t word) {
  return bytes2mcu(mcu, (char *) &word);
}

// send all zero word, spi idle
// used to get status or just delay
char zero2mcu(char mcu) {
  return word2mcu(mcu, 0);
}

// send one byte immediate command, not per-axis
char cmd2mcu(char mcu, char cmd) {
	digitalWrite(ssPinByMcu[mcu],0);
  char status = byte2mcu(mcu, cmd);
	delayMicroseconds(BYTE_DELAY);

	byte2mcu(mcu, 0);
  delayMicroseconds(BYTE_DELAY);

  byte2mcu(mcu, 0);
	delayMicroseconds(BYTE_DELAY);

  byte2mcu(mcu, 0);
  delayMicroseconds(BYTE_DELAY);

  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelay);
  return status;
}

// absolute vector for straight line
// max 65.536 ms per pulse (65536 usec) and max 1023 pulses
// add another vec2mcu for more pulses
// add delay2mcu for longer usecs/single-pulse
char vec2mcu(char mcu, char axis, char dir, char ustep,
             uint16_t usecsPerPulse, uint16_t pulseCount) {
  // Serial.print(  String( (axis ? 0x4000 : 0x8000) | (dir << 13) | (ustep << 10) | pulseCount, HEX) );
  // Serial.println(String(usecsPerPulse, HEX));
  return ints2mcu(mcu,
                 (axis ? 0x4000 : 0x8000) | (dir << 13) | (ustep << 10) | pulseCount,
                 usecsPerPulse);
}

// outputs no pulse, just delay
// max 65.536 ms (65536 usec)
char delay2mcu(char mcu, char axis, uint16_t delayUsecs) {
  return vec2mcu(mcu, axis, 0, 0, delayUsecs, 0);
}

// last vector in move, change mcu state from moving to locked
char eof2mcu(char mcu, char axis) {
  return vec2mcu(mcu, axis, 0, 0, 1, 0);
}

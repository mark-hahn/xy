
#include <SPI.h>
#include "xy-spi.h"
#include "mcu-cpu.h"

#define BYTE_DELAY     20   // usecs between  8-bit bytes
#define DEF_WORD_DELAY 50   // usecs between 32-bit words

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

char byteBack;

void byte2mcu(char byte, char mcu) {
  SPI.beginTransaction(SPISettings(speedByMcu[mcu], MSBFIRST, SPI_MODE0));
  byteBack = SPI.transfer(byte);
  SPI.endTransaction();
}

void setWordDelay(uint16_t wordDelayIn) {
  wordDelay = wordDelayIn;
  if(wordDelay == 0) wordDelay = DEF_WORD_DELAY;
}

char word2mcu(uint32_t word, char mcu) {
	digitalWrite(ssPinByMcu[mcu],0);

	byte2mcu(word >> 24, mcu);
	delayMicroseconds(BYTE_DELAY);
  char status = byteBack;

	byte2mcu((word & 0x00ff0000) >> 16, mcu);
	delayMicroseconds(BYTE_DELAY);

	byte2mcu((word & 0x0000ff00) >> 8, mcu);
	delayMicroseconds(BYTE_DELAY);

  byte2mcu(word & 0x000000ff, mcu);

  digitalWrite(ssPinByMcu[mcu],1);

  delayMicroseconds(wordDelay);
  return status;
}

void vec2mcu(char mcu, char axis, char dir, char ustep,
             uint16_t usecsPerPulse, uint16_t pulseCount) {
  VectorU vec;
  vec.vec.usecsPerPulse = usecsPerPulse;
  vec.vec.ctrlWord = (axis ? 0x4000 : 0x8000) |
                     (dir << 13) | (ustep << 10) | pulseCount;
  Serial.println(String((uint32_t) vec.word, HEX));
  word2mcu(vec.word, mcu);
}

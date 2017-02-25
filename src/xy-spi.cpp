
#include <SPI.h>
#include "xy-spi.h"
#include "mcu-cpu.h"

#define BYTE_DELAY 30   // usecs between  8-bit bytes
#define WORD_DELAY 200   // usecs between 32-bit words

#define SCK 14

int32 speedByMcu[3] = {4000000, 4000000, 4000000};
char  ssPinByMcu[3] = {15, 16, 0};

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

char word2mcu(uint32_t word, char mcu) {
	delayMicroseconds(WORD_DELAY);
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

  return status;
}

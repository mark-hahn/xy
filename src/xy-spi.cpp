
#include <SPI.h>
#include "xy-spi.h"
#include "mcu-cpu.h"

#define wordDelay 25  // microseconds between words
#define byteDelay 10  // microseconds between bytes

uint8_t byte2mcuWByteBack(uint8_t byte) {
  char bb;
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  bb = SPI.transfer(byte);
  SPI.endTransaction();
  return bb;
}

void byte2mcu(uint8_t byte) {
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  SPI.write(byte);
  SPI.endTransaction();
}

// returns first and only reply byte
uint8_t word2mcu(uint32_t word) {
  char byteBack;
  // it's a shame to have to do this first delay
  delayMicroseconds(wordDelay);

  digitalWrite(CS, 0); // slave select low
  byteBack = byte2mcuWByteBack(word >> 24);

  delayMicroseconds(byteDelay);
  byte2mcu((word & 0x00ff0000) >> 16);

  delayMicroseconds(byteDelay);
  byte2mcu((word & 0x0000ff00) >> 8);

  delayMicroseconds(byteDelay);
  byte2mcu(word & 0x000000ff);

  digitalWrite(CS, 1); // slave select high
  return byteBack;
}

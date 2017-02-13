
#include <SPI.h>
#include "xy-spi.h"

char byteBack;

// void word2mcu(unsigned long word) {
void word2mcu(char byte) {
  /* cpol, cphase, output edge, input edge
    SPI_MODE0	0	0	Falling	Rising
    SPI_MODE1	0	1	Rising	Falling
    SPI_MODE2	1	0	Rising	Falling
    SPI_MODE3	1	1	Falling	Rising
  */
  // uint8_t out[4];
  // uint8_t in[4];
  //
  // *((unsigned long*) &out) = word;
  // Serial.println(out[0], HEX);
  // Serial.println(out[1], HEX);
  // Serial.println(out[2], HEX);
  // Serial.println(out[3], HEX);

  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  // SPI.write32(word);
  byteBack = SPI.transfer(byte);
  SPI.endTransaction();
  // Serial.println(b, HEX);
  // Serial.println(in[0], HEX);
  // Serial.println(in[1], HEX);
  // Serial.println(in[2], HEX);
  // Serial.println(in[3], HEX);
}

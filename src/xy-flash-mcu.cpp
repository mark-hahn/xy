

#include <Arduino.h>
#include "xy-flash-mcu.h"
#include "xy-i2c.h"

void sendMcuAddr(unsigned int flashWordAddr) {
  char buf[2];
  buf[0] = flashWordAddr >> 8;
  buf[1] = flashWordAddr & 0xff;
  writeI2cRaw(12 /*addrI2cWriteAddr*/, buf, 2);
}

void sendMcuData(char *buf) {
  writeI2cRaw(dataI2cWriteAddr, buf, WRITE_FLASH_BLOCKSIZE*2);
}

void triggerMcuErase() {
  char buf[1];
  readI2cRaw(eraseI2cReadAddr, buf, 1);
}

void triggerMcuFlash() {
  char buf[1];
  readI2cRaw(flashI2cReadAddr, buf, 1);
}

void flashMcu64Bytes(unsigned int flashByteAddr, char *buf) {
  sendMcuAddr(flashByteAddr >> 1);
  sendMcuData(buf);
  triggerMcuErase();
  triggerMcuFlash();
}

void resetMcu() {
  sendMcuAddr(0xffff);
}



#include <Arduino.h>
#include "xy-flash-mcu.h"
#include "xy-i2c.h"

void sendMcuAddr(unsigned int flashWordAddr) {
  char buf[2];
  buf[0] = flashWordAddr >> 8;
  buf[1] = flashWordAddr & 0xff;
  writeI2cRaw(addrI2cWriteAddr, buf, 2);
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

unsigned int lastBlockByteAddr = 0xffff;
char mcuBuf[WRITE_FLASH_BLOCKSIZE*2], mcuBufEmpty=1;

void flashMcuBytes(unsigned int flashByteAddr, char *buf, char qty) {
  char i;
  if(mcuBufEmpty)
    for(i=0; i<WRITE_FLASH_BLOCKSIZE*2; i++) mcuBuf[i] = 0xff;
  unsigned int blockByteAddr = flashByteAddr & WRITE_FLASH_BLOCK_LEFT_MASK;
  if(blockByteAddr != lastBlockByteAddr && lastBlockByteAddr != 0xffff) {

    Serial.println(String("sendMcuAddr, blockByteAddr, lastBlockByteAddr, mcuBufEmpty: ") +
        blockByteAddr + ", " + lastBlockByteAddr +   ", " + (int) mcuBufEmpty);

    sendMcuAddr(lastBlockByteAddr >> 1);
    sendMcuData(mcuBuf);
    triggerMcuErase();
    triggerMcuFlash();
    for(i=0; i<WRITE_FLASH_BLOCKSIZE*2; i++) mcuBuf[i] = 0xff;
    mcuBufEmpty = 1;
  }
  for(i=0; i < qty; i++)
    mcuBuf[(flashByteAddr + i) & WRITE_FLASH_BLOCK_RIGHT_MASK] = buf[i];
  mcuBufEmpty = 0;
  lastBlockByteAddr = blockByteAddr;
}
void endFlashMcuBytes() {
  Serial.print("endFlashMcuBytes, ");
  if(!mcuBufEmpty) {

    Serial.println(String("lastBlockByteAddr, 0-4: ") +
           lastBlockByteAddr + ", " +
           (int) mcuBuf[0]   + ", " +
           (int) mcuBuf[1]   + ", " +
           (int) mcuBuf[2]   + ", " +
           (int) mcuBuf[3]   + ", " +
           (int) mcuBuf[4]);

    sendMcuAddr(lastBlockByteAddr >> 1);
    sendMcuData(mcuBuf);
    triggerMcuErase();
    triggerMcuFlash();
    mcuBufEmpty = 1;
    lastBlockByteAddr = 0xffff;
  }
}

void resetMcu() {
  sendMcuAddr(0xffff);
}


#ifndef _SPI
#define _SPI

#include <Arduino.h>

#define STATUS_REC_LEN 30
#define STATUS_REC_BUF_LEN ((STATUS_REC_LEN * 4) / 3)

extern uint8_t statusRec[STATUS_REC_LEN];
extern uint8_t statusRecInBuf[STATUS_REC_BUF_LEN];

void initSpi();

uint8_t byte2mcu(uint8_t mcu, uint8_t byte);
uint8_t bytes2mcu(uint8_t mcu, uint8_t *bytes);
uint8_t word2mcu(uint8_t mcu, uint32_t word);
uint8_t ints2mcu(uint8_t mcu, uint16_t int1, uint16_t int2);
uint8_t cmdWParams2mcu(uint8_t mcu, uint8_t cmd, uint8_t paramCount, uint8_t *params);
uint8_t zero2mcu(uint8_t mcu);

uint8_t getMcuState(uint8_t mcu);
uint8_t getMcuStatusRec(uint8_t mcu);

void flashMcuBytes(uint8_t mcu, unsigned int addr, char *buf, int len);
void endFlashMcuBytes(uint8_t mcu);

#endif

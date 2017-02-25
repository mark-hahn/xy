
#ifndef _SPI
#define _SPI

extern char byteBack;

void initSpi();
char word2mcu(uint32_t word, char mcu, uint16_t delay);

#endif

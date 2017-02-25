
#ifndef _SPI
#define _SPI

extern char byteBack;

void initSpi();
void setWordDelay(uint16_t wordDelayIn);
char word2mcu(uint32_t word, char mcu);

#endif

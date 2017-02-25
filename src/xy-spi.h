
#ifndef _SPI
#define _SPI

extern char byteBack;

void initSpi();
void setWordDelay(uint16_t wordDelayIn);
char word2mcu(uint32_t word, char mcu);
void vec2mcu(char mcu, char axis, char dir, char ustep,
             uint16_t usecsPerPulse, uint16_t pulseCount);

#endif

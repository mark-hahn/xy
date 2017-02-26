
#ifndef _SPI
#define _SPI

void initSpi();
void setWordDelay(uint16_t wordDelayIn);
char word2mcu(char mcu, uint32_t word);
char bytes2mcu(char mcu, char *bytes);
char zero2mcu(char mcu);
char cmd2mcu(char mcu, char cmd);
char vec2mcu(char mcu, char axis, char dir, char ustep,
             uint16_t usecsPerPulse, uint16_t pulseCount);
char delay2mcu(char mcu, char axis, uint16_t delayUsecs);
char eof2mcu(char mcu, char axis);

#endif


#ifndef _SPI
#define _SPI

#define STATUS_REC_LEN 30
#define STATUS_REC_BUF_LEN ((STATUS_REC_LEN * 4) / 3)

extern char statusRec[STATUS_REC_LEN];
extern char statusRecInBuf[STATUS_REC_BUF_LEN];

void initSpi();
char word2mcu(char mcu, uint32_t word);
char bytes2mcu(char mcu, char *bytes);
char zero2mcu(char mcu);
char cmd2mcu(char mcu, char cmd);
char vec2mcu(char mcu, char axis, char dir, char ustep,
             uint16_t usecsPerPulse, uint16_t pulseCount);
char delay2mcu(char mcu, char axis, uint16_t delayUsecs);
char eof2mcu(char mcu, char axis);

char getMcuState(char mcu);
char getMcuStatusRec(char mcu);

#endif

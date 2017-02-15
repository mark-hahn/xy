
#ifndef _SPI
#define _SPI

#include "mcu-cpu.h"

#define CS 15  // pin D8, aka SS

uint8_t word2mcu(uint32_t word);

#endif

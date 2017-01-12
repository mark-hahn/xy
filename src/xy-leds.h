
#ifndef _xy_leds
#define _xy_leds

#include <EEPROM.h>

void ledInit();
void led_on();
void led_off();
void ledBlink(bool turnBlinkOn);

#endif

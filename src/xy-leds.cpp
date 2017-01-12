#include <EEPROM.h>

#include <Arduino.h>
#include <os_type.h>

#include "xy-leds.h"

os_timer_t ledTimer;
bool ledBlinking;
bool blinkLedIsOn;

void ledBlinkCallback(void *pArg) {
  blinkLedIsOn = !blinkLedIsOn;
  if(blinkLedIsOn) led_on();
  else             led_off();
}
void ledInit() {
  os_timer_setfn(&ledTimer, ledBlinkCallback, NULL);
}

void led_on()  {digitalWrite(2, 0);}
void led_off() {digitalWrite(2, 1);}

void ledBlink(bool turnBlinkOn) {
  if(!ledBlinking && turnBlinkOn) {
    ledBlinking = true;
    os_timer_arm(&ledTimer, 100, true);
  } else if (ledBlinking && !turnBlinkOn) {
    os_timer_disarm(&ledTimer);
    ledBlinking = false;
    led_off();
  }
}

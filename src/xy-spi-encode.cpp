
#include <stdint.h>
#include "xy-spi.h"
#include "xy-spi-encode.h"

// mcu immediate command
// see cmdWParams2mcu in xy-spi.h
// 1:  100i iiii  -- 5-bit immediate cmd
uint8_t cmd2mcu(uint8_t mcu, uint8_t cmd) {
  return byte2mcu(mcu, (0x80 | cmd));
}

// settings vector
// set acceleration, start velocity, ustep, and direction
// does not cause any action, just defaults
// acceleration of
// 0:  010d vvvv vvvv vvvv uuua 0000 xxxx xxxx  -- settings,   (5 unused bits)
uint8_t settingsVector2mcu(uint8_t mcu, uint8_t axis, uint8_t dir,
                           uint8_t ustep, uint16_t pps, uint8_t acceleration) {
  uint16_t int1 = 0x4000        | (dir  << 12) | pps;
  uint16_t int2 = (ustep << 13) | (axis << 12) | acceleration;
  return ints2mcu(mcu, int1, int2);
}

// move vector
// max 65.536 ms per pulse (65536 usec) and max 4096 pulses
// add another vel2mcu for more pulses
// add delay2mcu for longer usecs/single-pulse
// uses acceleration for to go to specified velocity but no decelleration at end
// 0:  001d vvvv vvvv vvvv uuua cccc cccc cccc  -- move,       (1 unused bit)
uint8_t moveVector2mcu(uint8_t mcu, uint8_t axis, uint8_t dir, uint8_t ustep,
                uint16_t pps, uint16_t pulseCount) {
  uint16_t int1 = 0x2000        | (dir  << 12) | pps;
  uint16_t int2 = (ustep << 13) | (axis << 12) | pulseCount;
  return ints2mcu(mcu, int1, int2);
}

// delay vector, no pulses
// max delay 65.536 ms
// move vector with pulse count of zero, uuudvvvvvvvvvvvv is 16-bit usecs delay (not pps)
uint8_t delayVector2mcu(uint8_t mcu, uint8_t axis, uint16_t delayUsecs) {
  uint16_t int1 = 0x2000 | (delayUsecs & 0x1fff);
  uint16_t int2 = (delayUsecs & 0xe000) | (axis << 12);
  return ints2mcu(mcu, int1, int2);
}

// ------------ TODO -------------
// array of signed pps changes (2x8 -> 9x3)
// dir and ustep are same as last vector
uint8_t curveVector2mcu(uint8_t mcu, uint8_t axis, uint8_t count, int8_t *a) {
  uint8_t b[4];
  switch(count) {
    case 2: b[0] = 0xff; b[1] = 0xfc | axis;      // 2x8
       b[2] = a[0]; b[3] = a[1];
       break;
    case 3: b[0] = 0xff;                          // 3*7
       b[1] = (0x80 + (axis << 5)) | ((a[0] >> 2) & 0x1f);
       b[2] = (a[0] << 6)          | ((a[1] >> 1) & 0x3f);
       b[3] = (a[1] << 7)          |  (a[2] & 0x7f);
       break;
    case 4: b[0] = 0xff;                          // 4*5
       b[1] = 0xc0         |  (axis << 4)         | ((a[0] >> 1) & 0x0f);
       b[2] =  (a[0] << 7) | ((a[1] << 2) & 0x7c) | ((a[2] >> 3) & 0x03);
       b[3] =  (a[2] << 5) |  (a[3] & 0x1f);
       break;                                     // 5x5
    case 5: b[0] = 0xf0    | (axis << 1)           | ((a[0] >> 4) & 0x01);
       b[1] = (a[0] << 4)  | ((a[1] >> 1) & 0x0f);
       b[2] = (a[1] << 7)  | ((a[2] << 2) & 0x7c)  | ((a[3] >> 3) & 0x03);
       b[3] = (a[3] << 5)  | (a[4] & 0x1f);
       break;
    case 6: b[0] = 0xf8   | axis;                 // 6x4
       b[1] = (a[0] << 4) | (a[1] & 0x0f);
       b[2] = (a[2] << 4) | (a[3] & 0x0f);
       b[3] = (a[4] << 4) | (a[5] & 0x0f);
       break;                                     // 7x4
    case 7: b[0] = 0xc0   | (axis << 4)     | (a[0] & 0x0f);
       b[1] = (a[1] << 4) | (a[2] & 0x0f);
       b[2] = (a[3] << 4) | (a[4] & 0x0f);
       b[3] = (a[5] << 4) | (a[6] & 0x0f);
       break;
    case 8: b[0] = 0xfc           | axis;         // 8x3
       b[1] = (a[0] << 5)         | ((a[1] << 2) & 0x1c) |
             ((a[2] >> 1) & 0x03);
       b[2] = (a[2] << 7)         | ((a[3] << 4) & 0x70) |
             ((a[4] << 1) & 0x0e) | ((a[5] >> 2) & 0x01);
       b[3] = (a[5] << 6)         | ((a[6] << 3) & 0x38) |
              (a[7] & 0x07);
       break;                                     // 9x3
    case 9: b[0] = 0xe0           | (axis << 3) | (a[0] & 07);
       b[1] = (a[1] << 5)         | ((a[2] << 2) & 0x1c) |
             ((a[3] >> 1) & 0x03);
       b[2] = (a[3] << 7)         | ((a[4] << 4) & 0x70) |
             ((a[5] << 1) & 0x0e) | ((a[6] >> 2) & 0x01);
       b[3] = (a[6] << 6)         | ((a[7] << 3) & 0x38) |
              (a[8] & 0x07);
       break;
  }
  return bytes2mcu(mcu, b);
}

// last vector in move, change mcu state from moving to locked
uint8_t eofVector2mcu(uint8_t mcu, uint8_t axis) {
  return word2mcu(mcu, (0xffffffcf | (axis << 4)));
}


#include <stdint.h>
#include "xy-spi.h"
#include "xy-spi-encode.h"

// velocity vector for straight line
// max 65.536 ms per pulse (65536 usec) and max 4096 pulses
// add another vel2mcu for more pulses
// add delay2mcu for longer usecs/single-pulse
uint8_t vel2mcu(uint8_t mcu, uint8_t axis, uint8_t dir, uint8_t ustep,
              uint16_t pps, uint16_t pulseCount) {
  uint16_t int1 = 0x8000        | (dir  << 12) | (pps & 0x0fff);
  uint16_t int2 = (ustep << 13) | (axis << 12) | (pulseCount & 0x0fff);
  return ints2mcu(mcu, int1, int2);
}

// delay vector, no pulses
// max delay 65.536 ms
uint8_t delay2mcu(uint8_t mcu, uint8_t axis, uint16_t delayUsecs) {
  uint16_t int1 = 0x8000 | (delayUsecs & 0x1fff);
  uint16_t int2 = (delayUsecs & 0xe000) | (axis << 12);
  return ints2mcu(mcu, int1, int2);
}

// signed acceleration vector
// acceleration is 8-bit signed change to pps
// add another accel2mcu for more pulses
uint8_t accel2mcu(uint8_t mcu, uint8_t axis, uint8_t ustep,
               int8_t accel, uint16_t pulseCount) {
  uint16_t int1 = 0xfe00 | accel;
  uint16_t int2 = (ustep << 13) | (axis << 12) | (pulseCount & 0x0fff);
  return ints2mcu(mcu, int1, int2);
}

// array of signed acceleration to pps (2x8 -> 9x3)
// dir and ustep are same as last vector
uint8_t accels2mcu(uint8_t mcu, uint8_t axis, uint8_t count, int8_t *a) {
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
uint8_t eof2mcu(uint8_t mcu, uint8_t axis) {
  return word2mcu(mcu, (0xffffffcf | (axis << 4)));
}


/*                 ---- PROTOCOL ----

iiiiiii:        7-bit immediate cmd
a:              axis, X (0) or Y (1)
d:              direction (0: backwards, 1:forwards)
uuu:            microstep, 0 (1x) to 5 (32x)
xxxxxxxx:        8-bit signed acceleration in pulses/sec/sec
vvvvvvvvvvvv:   12-bit velocity in pulses/sec
cccccccccccc:   12-bit pulse count
E-M: curve acceleration field, signed
zzzz: vector list markers
  15: eof, end of moving


Number before : is number of leading 1's

 0:  0iii iiii  -- 7-bit immediate cmd - more bytes may follow
 1:  100d vvvv vvvv vvvv uuua cccc cccc cccc  -- velocity vector  (1 unused bit)
     if pulse count is zero then uuudvvvvvvvvvvvv is 16-bit usecs delay (not pps)

 7:  1111 1110 xxxx xxxx uuua cccc cccc cccc  -- acceleration vector

Curve vectors, each field is one pulse of signed acceleration ...

 3:  1110 aEEE  FFFG GGHH  HIII JJJK  KKLL LMMM --  9 3-bit
 6:  1111 110a  FFFG GGHH  HIII JJJK  KKLL LMMM --  8 3-bit
 2:  110a EEEE  FFFF GGGG  HHHH IIII  JJJJ KKKK --  7 4-bit
 5:  1111 100a  EEEE FFFF  GGGG HHHH  IIII JJJJ --  6 4-bit (1 unused bit)
 4:  1111 00aF  FFFF GGGG  GHHH HHII  IIIJ JJJJ --  5 5-bit (1 unused bit)
10:  1111 1111  110a FFFF  FGGG GGHH  HHHI IIII --  4 5-bit
 9:  1111 1111  10aF FFFF  FFGG GGGG  GHHH HHHH --  3 7-bit
14:  1111 1111  1111 110a  GGGG GGGG  HHHH HHHH --  2 8-bit

26:  1111 1111 1111 1111 1111 1111 110a zzzz  -- 4-bit vector marker

Sample Calculations (unfinished) ...

Typical case:

1000 mm/sec/sec =>  mm/ms/sec, assuming 1000 pps this is 1 mm/sec of velocity change each ms.  To get to a speed of 100 mm/sec, it would take 100 ms in 100 pulses.  This would cover 10 mm.

at 3000 mm/sec/sec, 3mm/ms/sec, and 3000 pps, it would be 3mm/sec vel increment. to get to 270 mm/sec would require 90 steps in 30 ms, covering 5000 (185*90) mm, or 15 meters.

given
  1) accel: mm/sec/sec  constant acceleration, typ. 1000
  2) vel:   mm/sec      end target speed,      typ. 100
  3) ppmm:  pulses/mm   constant ratio,        typ. 20  (1/4 step per pulse)

  vel*ppmm => pps: end pulses/sec  typ. 2000
  avg pps = 1000


each pulse:
  1ms -> 1mm/ms  0.5 mm
  2ms -> 2mm/ms  2 mm
  3ms -> 3mm/ms  4.5
  4ms -> 4mm/ms  8

  time: ms =>

avg vel: 50 mm/sec
avg pps: 1000


pps pulse rate (final update rate), you get mm/sec vel inc, number of steps, time, and distance.


10-bit (was 8) signed acceleration in PPS/sec, 3000 mm/s/s => 3mm/ms incr which is at most

12-bit pulse count
 */

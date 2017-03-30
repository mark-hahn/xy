
#ifndef _SPI_ENCODE
#define _SPI_ENCODE

uint8_t vel2mcu(uint8_t mcu, uint8_t axis, uint8_t dir, uint8_t ustep,
              uint16_t pps, uint16_t pulseCount);
uint8_t accel2mcu(uint8_t mcu, uint8_t axis, uint8_t ustep,
               int8_t accel, uint16_t pulseCount);
uint8_t accels2mcu(uint8_t mcu, uint8_t axis, uint8_t count, int8_t *a);
uint8_t delay2mcu(uint8_t mcu, uint8_t axis, uint16_t delayUsecs);
uint8_t eof2mcu(uint8_t mcu, uint8_t axis);

#endif

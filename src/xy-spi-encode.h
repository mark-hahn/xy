
#ifndef _SPI_ENCODE
#define _SPI_ENCODE

uint8_t cmd2mcu(uint8_t mcu, uint8_t cmd);
uint8_t settingsVector2mcu(uint8_t mcu, uint8_t axis, uint8_t dir,
                           uint8_t ustep, uint16_t pps, uint8_t acceleration);
uint8_t moveVector2mcu(uint8_t mcu, uint8_t axis, uint8_t dir, uint8_t ustep,
                uint16_t pps, uint16_t pulseCount);
uint8_t delayVector2mcu(uint8_t mcu, uint8_t axis, uint16_t delayUsecs);
uint8_t curveVector2mcu(uint8_t mcu, uint8_t axis, uint8_t count, int8_t *a);
uint8_t eofVector2mcu(uint8_t mcu, uint8_t axis);

#endif

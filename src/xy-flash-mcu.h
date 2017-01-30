
#ifndef FLASH_MCU
#define FLASH_MCU

#define addrI2cWriteAddr 4 /* i2c addr for write that sets
                               word addr for erase and flash writes.
                               word addr of 0xffff triggers reset */
#define dataI2cWriteAddr 8 /* i2c addr for writing 32 words (64 bytes, high-endian */
#define eraseI2cReadAddr 4 /* i2c addr for read that triggers erasing */
#define flashI2cReadAddr 8 /* i2c addr for read that triggers actual flashing
                               and bumps word address for next erase */

#define ERASE_FLASH_BLOCKSIZE 32 /* words */
#define WRITE_FLASH_BLOCKSIZE 32 /* words */
#define APP_FLASH_ADDR     0x200 /* 512 byte bootloader */

void sendMcuAddr(unsigned int flashWordAddr);  // for testing

void flashMcu64Bytes(unsigned int flashAddr, char *buf);
void resetMcu();

#endif

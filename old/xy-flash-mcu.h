
#ifndef FLASH_MCU
#define FLASH_MCU

#define addrI2cWriteAddr 10 /* i2c addr for write that sets
                               word addr for erase and flash writes.
                               word addr of 0xffff triggers reset */
#define dataI2cWriteAddr 11 /* i2c addr for writing 32 words (64 bytes, high-endian */
#define eraseI2cReadAddr 10 /* i2c addr for read that triggers erasing */
#define flashI2cReadAddr 11 /* i2c addr for read that triggers actual flashing
                               and bumps word address for next erase */

#define ERASE_FLASH_BLOCKSIZE 32 /* words */
#define WRITE_FLASH_BLOCKSIZE 32 /* words */
#define WRITE_FLASH_BLOCK_LEFT_MASK  0xffc0 /* bytes */
#define WRITE_FLASH_BLOCK_RIGHT_MASK 0x003f /* bytes */
#define APP_FLASH_ADDR     0x200 /* 512 byte bootloader */

void sendMcuAddr(unsigned int flashWordAddr);  // for testing
void flashMcuBytes(unsigned int flashByteAddr, char *buf, char qty);
void endFlashMcuBytes();
void resetMcu();

#endif

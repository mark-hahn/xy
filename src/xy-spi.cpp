
#include <Arduino.h>
#include <SPI.h>
#include "xy-spi.h"
#include "mcu-cpu.h"

// MCU 0 timing
#define MCU0_BIT_RATE  4000000 // bit rate (4 mbits)
#define MCU0_BYTE_DELAY     10 // usecs between  8-bit bytes
#define MCU0_WORD_DELAY    100 // usecs between words (too short causes errorSpiByteOverrun)

// add-on initial timing
// add-on timing is slow until device id is known
#define DEF_BIT_RATE    800000 // (0.8 mbits)
#define DEF_BYTE_DELAY      10
#define DEF_WORD_DELAY     100

// status rec
uint8_t statusRec[STATUS_REC_LEN];
uint8_t statusRecInBuf[STATUS_REC_BUF_LEN];

#define SCK 14
uint8_t ssPinByMcu[3] = {15, 16, 0};

int32    speedByMcu[3]     = {MCU0_BIT_RATE,   DEF_BIT_RATE,   DEF_BIT_RATE  };
uint16_t byteDelayByMcu[3] = {MCU0_BYTE_DELAY, DEF_BYTE_DELAY, DEF_BYTE_DELAY};
uint16_t wordDelayByMcu[3] = {MCU0_WORD_DELAY, DEF_WORD_DELAY, DEF_WORD_DELAY};

void initSpi() {
  pinMode(SCK, OUTPUT); // why?
	SPI.begin();
	SPI.setHwCs(false);  // our code will drive SS, not library

  // start all three MCU SS high
  for (uint8_t mcu=0; mcu < 3; mcu++) {
    pinMode(ssPinByMcu[mcu], OUTPUT);
    digitalWrite(ssPinByMcu[mcu],1);
  }
}

uint8_t trans2mcu(uint8_t mcu, uint8_t byte) {
  SPI.beginTransaction(SPISettings(speedByMcu[mcu], MSBFIRST, SPI_MODE0));
  uint8_t byteBack = SPI.transfer(byte);
  SPI.endTransaction();
  return byteBack;
}

uint8_t byte2mcu(uint8_t mcu, uint8_t byte) {
	digitalWrite(ssPinByMcu[mcu],0);
  uint8_t byteBack = trans2mcu(mcu, byte);
  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  return byteBack;
}

// array is little-endian (unusual)
// spi is big-endian
uint8_t bytes2mcu(uint8_t mcu, uint8_t *bytes) {
	digitalWrite(ssPinByMcu[mcu],0);
  uint8_t status = trans2mcu(bytes[3], mcu);
	delayMicroseconds(byteDelayByMcu[mcu]);

	trans2mcu(bytes[2], mcu);
  delayMicroseconds(byteDelayByMcu[mcu]);

	trans2mcu(bytes[1], mcu);
	delayMicroseconds(byteDelayByMcu[mcu]);

	trans2mcu(bytes[0], mcu);
  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  return status;
}

/// endian flip is needed in each int
uint8_t ints2mcu(uint8_t mcu, uint16_t int1, uint16_t int2) {
	digitalWrite(ssPinByMcu[mcu],0);
  uint8_t status = trans2mcu(mcu, ((uint8_t *) &int1)[1]);
	delayMicroseconds(byteDelayByMcu[mcu]);

	trans2mcu(mcu, ((uint8_t *) &int1)[0]);
  delayMicroseconds(byteDelayByMcu[mcu]);

  trans2mcu(mcu, ((uint8_t *) &int2)[1]);
	delayMicroseconds(byteDelayByMcu[mcu]);

	trans2mcu(mcu, ((uint8_t *) &int2)[0]);
  delayMicroseconds(byteDelayByMcu[mcu]);

  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  return status;
}

// word and array are both little-endian
uint8_t word2mcu(uint8_t mcu, uint32_t word) {
  return bytes2mcu(mcu, (uint8_t *) &word);
}

// send all zero word, spi idle
// used to get status or just delay
uint8_t zero2mcu(uint8_t mcu) {
  return byte2mcu(mcu, 0);
}

// send one byte immediate command, not per-axis
uint8_t cmd2mcu(uint8_t mcu, uint8_t cmd) {
	digitalWrite(ssPinByMcu[mcu],0);
  uint8_t status = trans2mcu(mcu, cmd);
	delayMicroseconds(byteDelayByMcu[mcu]);

	trans2mcu(mcu, 0);
  delayMicroseconds(byteDelayByMcu[mcu]);

  trans2mcu(mcu, 0);
	delayMicroseconds(byteDelayByMcu[mcu]);

  trans2mcu(mcu, 0);
  delayMicroseconds(byteDelayByMcu[mcu]);

  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  return status;
}

// absolute vector for straight line
// max 65.536 ms per pulse (65536 usec) and max 4096 pulses
// add another move2mcu for more pulses
// add delay2mcu for longer usecs/single-pulse
uint8_t move2mcu(uint8_t mcu, uint8_t axis, uint8_t dir, uint8_t ustep,
              uint16_t pps, uint16_t pulseCount) {
  uint8_t bytes[4] = {
    (uint8_t)  (0x80 | (axis << 4) | (dir << 3) | ustep),
    (uint8_t)  (pps >> 4),
    (uint8_t)(((pps << 4) & 0xf0) | (pulseCount >> 8)),
    (uint8_t)  (pulseCount & 0xff) };
  return bytes2mcu(mcu, bytes);
}

// signed acceleration vector
// acceleration is 8-bit signed change to pps
// add another accel2mcu for more pulses
uint8_t accel2mcu(uint8_t mcu, uint8_t axis, uint8_t ustep,
               int8_t accel, uint16_t pulseCount) {
  uint8_t bytes[4] = { 0xfe,
    (uint8_t) ((axis << 7) | (ustep << 6) | (accel >> 4)),
    (uint8_t) ((accel << 4) | (pulseCount >> 4)),
    (uint8_t) (pulseCount & 0xff) };
  return bytes2mcu(mcu, bytes);
}

// array of signed acceleration to pps (9x3 -> 2x8)
// dir and ustep are same as last vector
uint8_t accels2mcu(uint8_t mcu, uint8_t axis, uint8_t count, int8_t *a) {
  int8_t b[9];
  switch(count) {
    case 2: b[0] = 0xff; b[1] = 0xfc | axis;            // 2x8
       b[2] = a[0]; b[3] = a[1];
       break;
    case 3: b[0] = 0xff;                                // 3*7
       b[1] = (0x80 + (axis << 5)) | ((a[0] >> 2) & 0x1f);
       b[2] = (a[0] << 6)          | ((a[1] >> 1) & 0x3f);
       b[3] = (a[1] << 7)          |  (a[2] & 0x7f);
       break;
    case 4: b[0] = 0xff;                                // 4*5
       b[1] = 0xc0                 |  (axis << 4)         | ((a[0] >> 1) & 0x0f);
       b[2] =  (a[0] << 7)         | ((a[1] << 2) & 0x7c) | ((a[2] >> 3) & 0x03);
       b[3] =  (a[2] << 5)         |  (a[3] & 0x1f);
       break;                                      // 5x5
    case 5: b[0] = 0xf0                 | (axis << 1)          | ((a[0] >> 4) & 0x01);
       b[1] = (a[0] << 4)          | ((a[1] >> 1) & 0x0f);
       b[2] = (a[1] << 7)          | ((a[2] << 2) & 0x7c) | ((a[3] >> 3) & 0x03);
       b[3] = (a[3] << 5)          | (a[4] & 0x1f);
       break;
    case 6: b[0] = 0xf8   | axis;                   // 6x4
       b[1] = (a[0] << 4) | (a[1] & 0x0f);
       b[2] = (a[2] << 4) | (a[3] & 0x0f);
       b[3] = (a[4] << 4) | (a[5] & 0x0f);
       break;                                      // 7x4
    case 7: b[0] = 0xc0   | (axis << 3)     | ((a[0] << 4) & 0x0f);
       b[1] = (a[1] << 4) | (a[2] & 0x0f);
       b[2] = (a[3] << 4) | (a[4] & 0x0f);
       b[3] = (a[5] << 4) | (a[6] & 0x0f);
       break;

    case 8: b[0] = 0xfc           | axis;           // 8x3
       b[1] = (a[0] << 5)         | ((a[1] << 2) & 0x1c) |
             ((a[2] >> 1) & 0x03);
       b[2] = (a[2] << 7)         | ((a[3] << 4) & 0x70) |
             ((a[4] << 1) & 0xe0) | ((a[5] >> 2) & 0x01);
       b[3] = (a[5] << 6)         | ((a[6] << 3) & 0x38) |
              (a[7] & 0x03);
       break;
                                                  // 9x3
    case 9: b[0] = 0xe0                | (axis << 3) | (a[0] & 07);
       b[1] = (a[1] << 5)         | ((a[2] << 2) & 0x1c) |
             ((a[3] >> 1) & 0x03);
       b[2] = (a[3] << 7)         | ((a[4] << 4) & 0x70) |
             ((a[5] << 1) & 0xe0) | ((a[6] >> 2) & 0x01);
       b[3] = (a[6] << 6)         | ((a[7] << 3) & 0x38) |
              (a[8] & 0x03);
       break;
  }
  Serial.println( String(b[0], HEX) + " " + String(b[1], HEX) + " " +
                  String(b[2], HEX) + " " + String(b[3], HEX) );
  return 0; // bytes2mcu(mcu, bytes);
}

// outputs no pulse, just delay
// max 65.535 ms
uint8_t delay2mcu(uint8_t mcu, uint8_t axis, uint16_t delayUsecs) {
  uint8_t topNibble = (delayUsecs >> 12);
  return move2mcu(mcu, axis,
    (topNibble >> 3),
    (topNibble & 0x07),
    (delayUsecs & 0x0fff), 0);
}

// last vector in move, change mcu state from moving to locked
uint8_t eof2mcu(uint8_t mcu, uint8_t axis) {
  return word2mcu(mcu, (0xffffffcf | (axis << 4)));
}

uint8_t getMcuState(uint8_t mcu) {
  uint8_t mcu_state;
  do {
  	digitalWrite(ssPinByMcu[mcu], 0);
    mcu_state = trans2mcu(mcu, nopCmd);
  	digitalWrite(ssPinByMcu[mcu], 1);
    delayMicroseconds(wordDelayByMcu[mcu]);
    if(mcu_state == 0) delay(1);
  } while(mcu_state == 0);
  return mcu_state;
}

// unpack statusRecInBuf into statusRec
void unpackRec(uint8_t recLen) {
	uint8_t statusRecIdx = 0;
	uint8_t curByte;
  for(int i=0; i < recLen; i++) {
		uint8_t sixBits = statusRecInBuf[i];
		switch(i % 4) {
			case 0:
			  curByte = (sixBits << 2);
			  break;

			case 1:
			  curByte |= ((sixBits & 0x30) >> 4);
				statusRec[statusRecIdx++] = curByte;
				curByte  = ((sixBits & 0x0f) << 4);
			  break;

			case 2:
			  curByte |= ((sixBits & 0x3c) >> 2);
				statusRec[statusRecIdx++] = curByte;
				curByte  = ((sixBits & 0x03) << 6);
				break;

			case 3:
				statusRec[statusRecIdx++] = curByte | sixBits;;
			  break;
		}
	}
}

// looks for state byte, state rec, and a final state byte
// if not found state byte is returned
// if found then zero is returned;
// if rec longer than buffer, 254 is returned
// 255 returned on missing mcu
uint8_t getMcuStatusRec(uint8_t mcu) {
  uint8_t byteIn;
  uint8_t recLen = 255, recLen1;
  uint8_t statusRecInIdx = 0;
  bool_t foundStateByte = FALSE;
  uint8_t nonTypeCount = 0;

  // request status rec
  byte2mcu(mcu, statusCmd);

  // scan for data byte skipping state bytes
  while(1){
    byteIn = getMcuState(mcu);
    if((byteIn & 0xc0) == typeData) break;
    if(++nonTypeCount > 10) return getMcuState(mcu);
    // Serial.println("byteIn: " + String(byteIn, HEX));

    if(((byteIn & 0xc0) != typeState) ||
        ((byteIn & spiStateByteErrFlag) != 0))
      // must be clean state byte to continue
      return byteIn;
    foundStateByte = TRUE;
  }
  if(!foundStateByte) return getMcuState(mcu);

  // we have the first data byte that followed a state byte
  while(1) {
    if((byteIn & 0xc0) == typeData &&
         statusRecInIdx < STATUS_REC_BUF_LEN) {
      uint8_t statusData = (byteIn & 0x3f);
      statusRecInBuf[statusRecInIdx++] = statusData;

      // first byte is rec len
      if(statusRecInIdx == 1)
        recLen1 = statusData << 2;
      else if(statusRecInIdx == 2) {
        recLen = recLen1 | (statusData & 0x30) >> 4;
        if (recLen > STATUS_REC_BUF_LEN) return 254;
      }
    }
    else {
      if((byteIn & 0xc0) == typeState &&
        ((byteIn & spiStateByteErrFlag) == 0) &&
          statusRecInIdx == recLen) {
        unpackRec(recLen);
        return 0;  // success
      }
      else
        return byteIn; // status rec aborted
    }
    byteIn = byte2mcu(mcu, nopCmd);
    // data bytes must be sequential
    if(!byteIn) return getMcuState(mcu);
  }
}

// === these definitions much match the ones in pic-spi-ldr/spi.h ===
// these packets have all bytes in one SS low period
// bottom nibble zero in case CPU sends while not flashing
#define ERASE_CMD  0x10  // cmd byte + 2-byte word address
#define WRITE_CMD  0x20  // cmd byte + 2-byte word address and 32 words(64 bytes)
#define RESET_CMD  0x30  // cmd byte only, reset processor, issue when finished
#define MAX_BYTES_IN_BLOCK 64  // always writes this size

uint8_t flashBuf[MAX_BYTES_IN_BLOCK];
bool_t flashBufDirty = FALSE;
unsigned int lastBlkAddr = 0xffff;

void flashBlk(uint8_t mcu, unsigned int blkAddr) {
  // erase block
  digitalWrite(ssPinByMcu[mcu],0);
  trans2mcu(mcu, ERASE_CMD);
  delayMicroseconds(byteDelayByMcu[mcu]);
  trans2mcu(mcu, blkAddr >> 8);
  delayMicroseconds(byteDelayByMcu[mcu]);
  trans2mcu(mcu, blkAddr & 0xff);
  delayMicroseconds(byteDelayByMcu[mcu]);
  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  // wait for erase to finish
  delay(1);
  while(getMcuState(mcu) != statusFlashing);

  // write block
  digitalWrite(ssPinByMcu[mcu],0);
  trans2mcu(mcu, WRITE_CMD);
  delayMicroseconds(byteDelayByMcu[mcu]);
  trans2mcu(mcu, blkAddr >> 8);
  delayMicroseconds(byteDelayByMcu[mcu]);
  trans2mcu(mcu, blkAddr & 0xff);
  for(uint8_t i=0; i < MAX_BYTES_IN_BLOCK; i++) {
    delayMicroseconds(byteDelayByMcu[mcu]);
    trans2mcu(mcu, flashBuf[i]);
  }
  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  // wait for write to finish
  delay(1);
  while(getMcuState(mcu) != statusFlashing);

  memset(flashBuf, 0xff, MAX_BYTES_IN_BLOCK);
  flashBufDirty = FALSE;
}

void flashMcuBytes(uint8_t mcu, unsigned int addr, char *buf, int len){
  if(lastBlkAddr == 0xffff) {
    Serial.println("erasing block to start boot loader, updateFlashCodecmd");
    // start boot loader in mcu
    uint8_t status = byte2mcu(mcu, updateFlashCode);
    delay(1000);
    uint8_t stat = getMcuState(mcu); // ignore first response
    Serial.println(String("first resp after updateFlashCodecmd: ") + String(stat, HEX));
    while(getMcuState(mcu) != 7);
    Serial.println("have 7");
    memset(flashBuf, 0xff, MAX_BYTES_IN_BLOCK);
  }
  // lastBlkAddr = 0;
  // return;

  unsigned int blkAddr = ((addr / MAX_BYTES_IN_BLOCK) * MAX_BYTES_IN_BLOCK) / 2;
  // Serial.println("flashMcuBytes(blkAddr): " + String(blkAddr, HEX));

  if(lastBlkAddr != 0xffff && lastBlkAddr != blkAddr)
     flashBlk(mcu, lastBlkAddr);
  memcpy(&flashBuf[addr % MAX_BYTES_IN_BLOCK], buf, len);
  flashBufDirty = TRUE;
  lastBlkAddr = blkAddr;
}

void endFlashMcuBytes(uint8_t mcu) {
  if(flashBufDirty) flashBlk(mcu, lastBlkAddr);

  // make sure mcu is ready and flashing
  while(getMcuState(mcu) != statusFlashing);
  // reset mcu
  digitalWrite(ssPinByMcu[mcu],0);
  trans2mcu(mcu, RESET_CMD);
  digitalWrite(ssPinByMcu[mcu],1);
  // no response since mcu is rebooting
  Serial.println("mcu reset");
}

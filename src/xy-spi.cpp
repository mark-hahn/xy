
#include <SPI.h>
#include "xy-spi.h"
#include "mcu-cpu.h"

// MCU 0 timing
#define MCU0_BIT_RATE  4000000 // bit rate (4 mbits)
#define MCU0_BYTE_DELAY     10 // usecs between  8-bit bytes
#define MCU0_WORD_DELAY    100 // usecs between 32-bit words (too short causes err 18)

// add-on initial timing
// add-on timing is slow until device id is known
#define DEF_BIT_RATE  100000 // 100 kbs
#define DEF_BYTE_DELAY    50
#define DEF_WORD_DELAY   200

// status rec
char statusRec[STATUS_REC_LEN];
char statusRecInBuf[STATUS_REC_BUF_LEN];

#define SCK 14
char ssPinByMcu[3] = {15, 16, 0};

int32    speedByMcu[3]     = {MCU0_BIT_RATE,   DEF_BIT_RATE,   DEF_BIT_RATE  };
uint16_t byteDelayByMcu[3] = {MCU0_BYTE_DELAY, DEF_BYTE_DELAY, DEF_BYTE_DELAY};
uint16_t wordDelayByMcu[3] = {MCU0_WORD_DELAY, DEF_WORD_DELAY, DEF_WORD_DELAY};

void initSpi() {
  pinMode(SCK, OUTPUT); // why?
  for (char mcu=0; mcu < 3; mcu++) {
    digitalWrite(ssPinByMcu[mcu],1); // should this be before or after pinMode ?
    pinMode(ssPinByMcu[mcu], OUTPUT);
    digitalWrite(ssPinByMcu[mcu],1);
  }
	SPI.begin();
	SPI.setHwCs(false);  // our code will drive SS, not library
}

char byte2mcu(char mcu, char byte) {
  SPI.beginTransaction(SPISettings(speedByMcu[mcu], MSBFIRST, SPI_MODE0));
  char byteBack = SPI.transfer(byte);
  SPI.endTransaction();
  return byteBack;
}

char byte2mcuWithSS(char mcu, char byte) {
	digitalWrite(ssPinByMcu[mcu],0);
  char byteBack = byte2mcu(mcu, byte);
  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  return byteBack;
}

// array is little-endian (unusual)
// spi is big-endian
char bytes2mcu(char mcu, char *bytes) {
	digitalWrite(ssPinByMcu[mcu],0);
  char status = byte2mcu(bytes[3], mcu);
	delayMicroseconds(byteDelayByMcu[mcu]);

	byte2mcu(bytes[2], mcu);
  delayMicroseconds(byteDelayByMcu[mcu]);

	byte2mcu(bytes[1], mcu);
	delayMicroseconds(byteDelayByMcu[mcu]);

	byte2mcu(bytes[0], mcu);
  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  return status;
}

/// endian flip is needed in each int
char ints2mcu(char mcu, uint16_t int1, uint16_t int2) {
	digitalWrite(ssPinByMcu[mcu],0);
  char status = byte2mcu(mcu, ((char *) &int1)[1]);
	delayMicroseconds(byteDelayByMcu[mcu]);

	byte2mcu(mcu, ((char *) &int1)[0]);
  delayMicroseconds(byteDelayByMcu[mcu]);

  byte2mcu(mcu, ((char *) &int2)[1]);
	delayMicroseconds(byteDelayByMcu[mcu]);

	byte2mcu(mcu, ((char *) &int2)[0]);
  delayMicroseconds(byteDelayByMcu[mcu]);

  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  return status;
}

// word and array are both little-endian
char word2mcu(char mcu, uint32_t word) {
  return bytes2mcu(mcu, (char *) &word);
}

// send all zero word, spi idle
// used to get status or just delay
char zero2mcu(char mcu) {
  return word2mcu(mcu, 0);
}

// send one byte immediate command, not per-axis
char cmd2mcu(char mcu, char cmd) {
	digitalWrite(ssPinByMcu[mcu],0);
  char status = byte2mcu(mcu, cmd);
	delayMicroseconds(byteDelayByMcu[mcu]);

	byte2mcu(mcu, 0);
  delayMicroseconds(byteDelayByMcu[mcu]);

  byte2mcu(mcu, 0);
	delayMicroseconds(byteDelayByMcu[mcu]);

  byte2mcu(mcu, 0);
  delayMicroseconds(byteDelayByMcu[mcu]);

  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  return status;
}

// absolute vector for straight line
// max 65.536 ms per pulse (65536 usec) and max 1023 pulses
// add another vec2mcu for more pulses
// add delay2mcu for longer usecs/single-pulse
char vec2mcu(char mcu, char axis, char dir, char ustep,
             uint16_t usecsPerPulse, uint16_t pulseCount) {
  // Serial.print(  String( (axis ? 0x4000 : 0x8000) | (dir << 13) | (ustep << 10) | pulseCount, HEX) );
  // Serial.println(String(usecsPerPulse, HEX));
  return ints2mcu(mcu,
                 (axis ? 0x4000 : 0x8000) | (dir << 13) | (ustep << 10) | pulseCount,
                 usecsPerPulse);
}

// outputs no pulse, just delay
// max 65.536 ms (65536 usec)
char delay2mcu(char mcu, char axis, uint16_t delayUsecs) {
  return vec2mcu(mcu, axis, 0, 0, delayUsecs, 0);
}

// last vector in move, change mcu state from moving to locked
char eof2mcu(char mcu, char axis) {
  return vec2mcu(mcu, axis, 0, 0, 1, 0);
}

char getMcuState(char mcu) {
  char mcu_state;
  do {
  	digitalWrite(ssPinByMcu[mcu], 0);
    mcu_state = byte2mcu(mcu, nopCmd);
  	digitalWrite(ssPinByMcu[mcu], 1);
    delayMicroseconds(wordDelayByMcu[mcu]);
    if(mcu_state == 0) delay(1);
  } while(mcu_state == 0);
  return mcu_state;
}

// unpack statusRecInBuf into statusRec
void unpackRec(char recLen) {
	char statusRecIdx = 0;
	char curByte;
  for(int i=0; i < recLen; i++) {
		char sixBits = statusRecInBuf[i];
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
char getMcuStatusRec(char mcu) {
  char byteIn;
  char recLen = 255, recLen1;
  char statusRecInIdx = 0;
  bool_t foundStateByte = FALSE;
  char nonTypeCount = 0;

  // request status rec
  byte2mcuWithSS(mcu, statusCmd);

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
      char statusData = (byteIn & 0x3f);
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
    byteIn = byte2mcuWithSS(mcu, nopCmd);
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

void flashMcuBytes(char mcu, unsigned int addr, char *buf, int len){
  if(len != 64 || (addr % 32) != 0) {
    Serial.println(String("invalid flashMcuBytes params: ") + addr + ", " + len);
    return;
  }
  while(getMcuState(mcu) != statusFlashing) {
    // start boot loader in mcu
    char status = byte2mcuWithSS(mcu, updateFlashCode);
    // give some time for flash write and reset
    delay(100);
    getMcuState(mcu); // ignore first response
  }

  // erase block
  digitalWrite(ssPinByMcu[mcu],0);
  byte2mcu(mcu, ERASE_CMD);
  delayMicroseconds(byteDelayByMcu[mcu]);
  byte2mcu(mcu, addr >> 8);
  delayMicroseconds(byteDelayByMcu[mcu]);
  byte2mcu(mcu, addr & 0xff);
  delayMicroseconds(byteDelayByMcu[mcu]);
  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  // wait for erase to finish
  while(getMcuState(mcu) != statusFlashing);

  // write block
  digitalWrite(ssPinByMcu[mcu],0);
  byte2mcu(mcu, ERASE_CMD);
  delayMicroseconds(byteDelayByMcu[mcu]);
  byte2mcu(mcu, addr >> 8);
  delayMicroseconds(byteDelayByMcu[mcu]);
  byte2mcu(mcu, addr & 0xff);
  for(char i=0; i < len; i++) {
    delayMicroseconds(byteDelayByMcu[mcu]);
    byte2mcu(mcu, buf[i]);
  }
  digitalWrite(ssPinByMcu[mcu],1);
  delayMicroseconds(wordDelayByMcu[mcu]);
  // wait for flash write to finish
  while(getMcuState(mcu) != statusFlashing);
}

void endFlashMcuBytes(char mcu) {
  // make sure mcu is ready and flashing
  while(getMcuState(mcu) != statusFlashing);
  // reset mcu
  digitalWrite(ssPinByMcu[mcu],0);
  byte2mcu(mcu, RESET_CMD);
  digitalWrite(ssPinByMcu[mcu],1);
  // doesn't wait for response since mcu is rebooting
}

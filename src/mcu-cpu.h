
#ifndef MCUCPU_H
#define	MCUCPU_H

#include <stdint.h>

// This file contains all definitions shared by CPU and MCU
// and should be included in CPU and MCU apps
// If this file must change, make sure it is backwards compatible
// Keep this file in both apps the same (except for MCU_H and CPU_H defines)


/////////////////////////////////  Definitions  ///////////////////////////

#define X 0  /* idx for X axis */
#define Y 1  /* idx for Y axis */

#define FORWARD   1 // motor dir bit
#define BACKWARDS 0

typedef uint8_t bool_t;
#define TRUE  1
#define FALSE 0

// position, unit: 0.00625 mm (160/mm), 1/32 step, (smallest microstep)
// max position is +- 52 meters
#ifdef   MCU_H
typedef signed short long pos_t; // 24 bits signed
#else // CPU_H
typedef long pos_t; // 32 bits signed
#endif


//////////////////////  Immediate Commands  ///////////////////////

// immediate 5-bit command -- top is 0b100
// may be followed by param bytes
typedef enum Cmd {
  nopCmd               =  0, // does nothing except get status
  statusCmd            =  1, // requests status rec returned
  clearErrorCmd        =  2, // on error, no activity until this command
  settingsCmd          =  3, // byte 2 is setting idx, 3 & 4 are int16 value
  updateFlashCode      =  4, // set flash bytes as no-app flag and reboot

  // commands specific to one add-on start at 10
  idleCmd              = 10, // abort any commands, clear vec buffers
  lockCmd              = 11, // set reset pins on motors to low
  unlockCmd            = 12, // clear reset pins on motors to high
  homeCmd              = 13, // goes home and saves homing distance
  moveCmd              = 14, // enough vectors need to be loaded to do this
  clearDistance        = 19  // clear distance counters
} Cmd;


/////////////////////////////////  SETTINGS  ///////////////////////////

// all of these are int16_t
typedef enum Settings {
  motorCurrent,    // meaning varies between MCUs
  directionLevels, // x dir: d1, y dir: d0
  disableLimitX,
  defaultAccell,
  moveJerk,
  homingUstep,
  homingPps,
  homeBkupUstep,
  homeBkupPps,
  homeAccel,
  homeJerk,
  homeOfsX,
  homeOfsY,
  NUM_SETTINGS
} Settings;


/////////////////////////////////  VECTORS  ///////////////////////////
// all unsigned but one, E-M
//
//iiiii:          5-bit immediate cmd
//a:              axis, X (0) or Y (1)
//d:              direction (0: backwards, 1:forwards)
//uuu:            microstep, 0 (1x) to 5 (32x)
//xxxxxxxx:        8-bit acceleration in pulses/sec/sec
//vvvvvvvvvvvv:   12-bit velocity in pulses/sec
//cccccccccccc:   12-bit pulse count
//E-M: curve pps change field, signed
//zzzz: vector list markers
//  15: eof, end of moving

//Number before : is number of leading 1's


// 1:  100i iiii  -- 5-bit immediate cmd - more bytes may follow
// 0:  010d vvvv vvvv vvvv uuua 0000 xxxx xxxx  -- settings,   (5 unused bits)
//     if v... pps is zero, don't set it
// 0:  001d vvvv vvvv vvvv uuua cccc cccc cccc  -- move,       (1 unused bit)
//  if pulse count is zero then uuudvvvvvvvvvvvv is 16-bit usecs delay (not pps)
//  move vec d29 is 1 to ensure zero bytes aren't interpreted as commands

//Curve vectors, each field is one pulse of signed pps change ...
//
// 3:  1110 aEEE  FFFG GGHH  HIII JJJK  KKLL LMMM --  9 3-bit
// 6:  1111 110a  FFFG GGHH  HIII JJJK  KKLL LMMM --  8 3-bit
// 2:  110a EEEE  FFFF GGGG  HHHH IIII  JJJJ KKKK --  7 4-bit
// 5:  1111 100a  EEEE FFFF  GGGG HHHH  IIII JJJJ --  6 4-bit (1 unused bit)
// 4:  1111 00aF  FFFF GGGG  GHHH HHII  IIIJ JJJJ --  5 5-bit (1 unused bit)
//10:  1111 1111  110a FFFF  FGGG GGHH  HHHI IIII --  4 5-bit
// 9:  1111 1111  10aF FFFF  FFGG GGGG  GHHH HHHH --  3 7-bit
//14:  1111 1111  1111 110a  GGGG GGGG  HHHH HHHH --  2 8-bit
//
//26:  1111 1111 1111 1111 1111 1111 110a zzzz  -- 4-bit vector marker



/////////////////////////////////  MCU => CPU  ///////////////////////////

// only first returned (mcu to cpu) byte of word is used
// rest are zero      (mcu has no buffering in that direction)

// mcu states
// values are valid even when error is set, tells what was happening
// 3 bits
typedef enum Status {
  statusUnlocked    = 1, // idle with motor reset pins low
  statusHoming      = 2, // auto homing
  statusLocked      = 3, // idle with motor current
  statusMoving      = 4, // executing vector moves from vecBuf
  statusMoved       = 5, // same as statusLocked but after move finishes
  statusFlashing    = 7  // waiting in boot loader for flash bytes
} Status;

// state byte...
// d7-d6: typeState, 0b00
// d5:    error flag
// d4-d3: vector buf high-water flags for X and Y
// d2-d0: status code (mcu_state)

// error byte ...
// d7-d6: typeState, 0b11
// d5-d1: error code
//    d0: axis flag

// byte type in top 2 bits of returned byte

#define RET_TYPE_MASK 0xc0
#define typeState  0x00  // state byte returned by default
#define typeState2 0x40  // reserved for possible extended state
#define typeData   0x80  // status rec data in bottom 6 bits
#define typeError  0xc0  // err code: d5-d1, axis flag: d0

// state byte includes flag indicating error
#define spiStateByteErrFlag 0x20
#define spiStateByteMask    0x1f

// type of rec inside rec, matches command
#define STATUS_REC 1

// this record is returned to the CPU when requested by statusCmd
// must be sequential with status byte before and after
// future api versions may extend record
// words must be word aligned
typedef struct StatusRec {
  uint8_t len;            // number of SPI bytes in rec, NOT sizeof(StatusRec)
  uint8_t type;           // type of record (always STATUS_REC for now)
  uint8_t mfr;            // manufacturer code (1 == eridien)
  uint8_t prod;           // product id (1 = XY base)
  uint8_t vers;           // XY (code and hw) version
  uint8_t padding[3];
  int32_t distanceX;      // position since clearDistance
  int32_t distanceY;
} StatusRec;

#define STATUS_SPI_BYTE_COUNT \
  (((sizeof(StatusRec) * 4) / 3) + (uint8_t)((sizeof(StatusRec) % 3) != 0))

typedef union StatusRecU {
  StatusRec rec;
  uint8_t   bytes[sizeof(StatusRec)];
} StatusRecU;

// top 2 bits are 0b11 (typeError)
// lsb of error byte reserved for error axis
// 6-bit code (including axis bit)
typedef enum Error {
  // these must be supported by all add-ons
  errorMcuFlashing =    2,
  errorReset       =    4,  // always in this state after mcu reset

  // errors specific to add-on start at 10
  errorFault             = 10, // driver chip fault
  errorLimit             = 12, // hit error limit switch during move backwards
  errorVecBufOverflow    = 14,
  errorVecBufUnderflow   = 16,

  // comm errors must start at errorSpiByteSync and be last
  errorSpiByteSync       = 48,
  errorSpiByteOverrun    = 50,
  errorSpiBytesOverrun   = 52,
  errorSpiOvflw          = 54,
  errorSpiWcol           = 56,
  errorSpuriousInt       = 58,

  errorNoResponse  = 0x3f // miso pull-up returns 0xff when no mcu
} Error;

#define spiCommError(byte) ((byte >= (0xc0 | errorSpiByteSync)))

#endif

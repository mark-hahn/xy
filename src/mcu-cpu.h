
#ifndef CPU_H
#define	CPU_H

#define API_VERSION 1

// This file contains all definitions shared by CPU and MCU
// and should be included in CPU and MCU apps
// If this file must change, make sure it is backwards compatible
// Keep this file in both apps the same (except for MCU_H and CPU_H defines)

// notes for CPU ...
//   for max stepper speed calculator see ...
//     http://techref.massmind.org/techref/io/stepper/estimate.htm
//   assuming 1A, 2.6mH, 12V, and 200 steps per rev; min is 433 usecs full step

// motor notes ... distance per step
// 0: 0.2 mm
// 1: 0.1 mm
// 2: 0.05 mm
// 3: 0.025 mm
// 4: 0.0125 mm
// 5: 0.00625 mm

#define X 0  /* idx for X axis */
#define Y 1  /* idx for Y axis */

typedef char bool_t;
#define TRUE 1
#define FALSE 0

// time, unit: usec
// max time is 65 ms
typedef unsigned int shortTime_t; // 16 bits unsigned

// position, unit: 0.00625 mm, 1/32 step distance (smallest microstep)
// max position is +- 52 meters
#ifdef MCU_H
typedef signed char         int8_t;
typedef unsigned long     uint32_t;
typedef signed short long    pos_t; // 24 bits signed
#else // CPU_H
typedef long pos_t; // 32 bits signed
#endif

#define FORWARD   1 // motor dir bit
#define BACKWARDS 0

// immediate command 32-bit words -- top 2 bits are zero
// command codes enumerated here are in bottom nibble of first byte
// set homing speeds have microstep index in byte 2 and speed param in 3-4
// setMotorCurrent has param in bottom 5 bits
// all others have no params

typedef enum Cmd {
  // zero is not used so blank SPI words are ignored
  nopCmd               =  0, // does nothing except get status
  statusCmd            =  1, // requests entire state rec returned
  sleepCmd             =  2, // clear state & set all motor pins low
  resetCmd             =  3, // clear state & hold reset pins on motors low
  idleCmd              =  4, // abort any commands, clear vec buffers
  homeCmd              =  5, // goes home and saves homing distance
  moveCmd              =  6, // enough vectors need to be loaded to do this
  clearErrorCmd        =  7, // on error, no activity until this command
  setHomingSpeed       =  8, // set homeUIdx & homeUsecPerPulse settings
  setHomingBackupSpeed =  9, // set homeBkupUIdx & homeBkupUsecPerPulse settings
  setMotorCurrent      = 10, // set motorCurrent (0 to 31) immediately
  setDirectionLevelXY  = 11  // set direction for each motor
} Cmd;

// general mcu states
// values are valid even when error flag is set, tells what was happening
// 5 bits
typedef enum Status {
  statusNoResponse  = 1, // no response from mcu (cpu is receiving 0xff)
  statusSleeping    = 2, // idle, all motor pins low
  statusUnlocked    = 3, // idle with motor reset pins low
  statusHoming      = 4, // auto homing
  statusLocked      = 5, // idle with motor current
  statusMoving      = 6  // executing vector moves from vecBuf
} Status;

// only first return byte of 32-bit word is used
// byte type in top 2 bits of returned byte
#define typeState  0x40  // state, if errorcode then d5 = 1
#define typeData   0x80  // status rec data in bottom 6 bits
#define typeError  0xc0  // err code: d5-d1, mcu flag: d0

// state byte also includes flag indicating error
#define spiStateByteErrFlag 0x20
#define spiStateByteMask    0x1f

// this record is returned to the CPU when requested by statusCmd
// must be sequential with status byte before and after
// future api versions may extend record
typedef struct StatusRec {
  char apiVers;        // version of this API
  char mfr;            // manufacturer code (1 == eridien)
  char prod;           // product id (1 = XY base)
  char vers;           // XY (code or hw) version
  uint32_t homeDistX;  // homing distance of last home operation
  uint32_t homeDistY;
} StatusRec;

typedef union StatusRecU {
  StatusRec rec;
  char      bytes[sizeof(StatusRec)];
} StatusRecU;

#define STATUS_SPI_BYTE_COUNT           \
  (((sizeof(StatusRec) % 3) == 0 ?      \
   ((sizeof(StatusRec)*4)/3) : (((sizeof(StatusRec)*4)/3) + 1)))

// 5 bit code, lsb of error byte reserved for error axis
typedef enum Error {
  errorFault             =  2, // driver chip fault
  errorLimit             =  4, // hit error limit switch during move
  errorVecBufOverflow    =  6,
  errorVecBufUnderflow   =  8,
  errorMoveWhenUnlocked  = 10,
  errorMoveWithNoVectors = 12,
  errorSpiByteSync       = 14,
  errorSpiByteOverrun    = 16,
  errorSpiWordOverrun    = 18,
  errorSpiOvflw          = 20,
  errorSpiWcol           = 22
} Error;


#ifdef CPU_H

// absolute vector 32-bit words -- constant speed travel
typedef struct Vector {
  uint16_t  usecsPerPulse;
  // ctrlWord has five bit fields, from msb to lsb ...
  //   1 bit: axis X vector, both X and Y clr means command, not vector
  //   1 bit: axis Y vector, both X and Y set means delta, not absolute, vector
  //   1 bit: dir (0: backwards, 1: forwards)
  //   3 bits: ustep idx, 0 (full-step) to 5 (1/32 step)
  //  10 bits: pulse count
  uint16_t ctrlWord;
} Vector;

#else  // MCU_H

typedef struct Vector {
  uint16_t ctrlWord;
  uint16_t  usecsPerPulse;
} Vector;

#endif

typedef union VectorU {
  Vector   vec;
  uint32_t word;
} VectorU;

// delta 32-bit words -- varying speed travel
// this word appears after an absolute vector with multiple vectors per word
// it has same axis, dir, and micro-index as the previous vector
// delta values are the difference in usecsPerPulse
// there are 4, 3, and 2 deltas possible per word
// the letter s below is delta sign, 1: minus, 0: plus
// w: first delta, x: 2nd, y: 3rd, z: 4th
// 4 delta format,  7 bits each: 11s0 wwww wwwX XXXX XXyy yyyy yZZZ ZZZZ
// 3 delta format,  9 bits each: 11s1 0www wwww wwXX XXXX XXXy yyyy yyyy
// 2 delta format, 13 bits each: 11s1 10ww wwww wwww wwwX XXXX XXXX XXXX

// the last vector of axis move: 1111 1111 1111 1111 1111 1111 1111 111A
// where A is the axis
// when both axis have reached this marker then the move is finished

#endif

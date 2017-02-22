
#ifndef _I2C
#define _I2C

typedef enum State {
  unlocked,
  locked,
  // active
  homing,
  moving,
  stopping,
  // errors
  beforeErrors,
  errorFault,
  errorLimit,
  errorMinPos,
  errorMaxPos
} State;

typedef enum Cmd {
  none,
  home,
  move,
  stop,
  reset,
  startSync = 0x80,
  stopSync  = 0x81
} Cmd;

typedef struct Bank {
  long actualPos;
  long accel;
  long velocity;
  long targetPos;
  Cmd  cmd;
  char reserved[3];  // for long alignment
  // settings only in bank 0
  long homeApproachVel;
  long homeBackupVel;
  long homeSlowApproachVel;
  char motorCurrent;
} Bank;

typedef union RegBank {
  Bank bank;
  long longs[8];
  char chars[32];
} RegBank;

void initI2c();
void writeI2c(char mcu, char bankAddr, char *buf, char qty);
void writeI2cRaw(char addr, char *buf, int qty);
char readI2cRaw(char addr, char *buf, int qty);

#endif

#include <stdint.h>
#include "xy-dec-float.h"

// return signed 32-bit int from decfp number, truncates value
int32_t decfp2Int(int32_t man, int8_t exp) {
  switch(exp) {
    case -8: return man / 100000000;
    case -7: return man / 10000000;
    case -6: return man / 1000000;
    case -5: return man / 100000;
    case -4: return man / 10000;
    case -3: return man / 1000;
    case -2: return man / 100;
    case -1: return man / 10;
    case  0: return man;
    case  1: return man * 10;
    case  2: return man * 100;
    case  3: return man * 1000;
    case  4: return man * 10000;
    case  5: return man * 100000;
    case  6: return man * 1000000;
    case  7: return man * 10000000;
  }
  return 0; // won't get here
}
//
// // multiple decfp number times signed 32-bit number
// // returns signed 32-bit number
// // no check for overflow
// // this keeps as much precision as possible
// int32_t decfpTimesS32(decfp_t dfp, int32_t multiplicand) {
//   if(dfp >= -134,217,728 && dfp < +134,217,728)
//     return dfp * multiplicand;
//   int32_t dfpDec = (dfp & 0x07ffffff);
//   if(dfp < 0) dfpDec = dfpDec | 0xf8000000;
//   int64_t num = (int64_t) dfpDec * multiplicand;
//   switch(expFromDecfp(dfp)) {
//     case -8: return num / 100000000;
//     case -7: return num / 10000000;
//     case -6: return num / 1000000;
//     case -5: return num / 100000;
//     case -4: return num / 10000;
//     case -3: return num / 1000;
//     case -2: return num / 100;
//     case -1: return num / 10;
//     case  0: return num;
//     case  1: return num * 10;
//     case  2: return num * 100;
//     case  3: return num * 1000;
//     case  4: return num * 10000;
//     case  5: return num * 100000;
//     case  6: return num * 1000000;
//     case  7: return num * 10000000;
//   }
//   return 0; // won't get here
// }

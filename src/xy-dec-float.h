
#ifndef DECFP
#define DECFP

#include <stdint.h>

// math routines for custom decimal floating point numbers
// man (mantissa): a signed 32-bit integer
// exp (exponent): total number is mantisaa times (10 ** exp)
// range is -0.0000001 to +999,999,990,000,000
// 8-digit precision

// truncate fraction and return signed 32-bit integer
int32_t decfp2Int(int32_t man, int8_t exp);

#endif

/**
 * \file typedef.h
 * Type definitions for integer and floating point variables.
 *
 * \author Steven Bell
 * \date 22 July 2010
 */

#ifndef TYPEDEF_H_
#define TYPEDEF_H_

#include <inttypes.h>

typedef uint8_t uint8; ///< 8-bit unsigned value
typedef int8_t int8; ///< 8-bit signed value
typedef uint16_t uint16; ///< 16-bit unsigned value
typedef int16_t int16; ///< 16-bit signed value
typedef uint32_t uint32; ///< 32-bit unsigned value
typedef int32_t int32; ///< 32-bit signed value
typedef uint64_t uint64; ///< 64-bit unsigned value
typedef int64_t int64; ///< 64-bit signed value

/**
 * \brief 32-bit floating-point value
 *
 * Both floats and doubles are 32-bit floating point values, so
 * there is no float64.
 */
typedef float float32;

#endif /* TYPEDEF_H_ */

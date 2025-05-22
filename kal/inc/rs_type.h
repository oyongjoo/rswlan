// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_TYPE_H
#define RS_TYPE_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/types.h>

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////
// Data Type

// NULL
#ifndef NULL
#define NULL ((void *)0)
#endif

// TRUE
#ifndef TRUE
#define TRUE (1)
#endif

// FALSE
#ifndef FALSE
#define FALSE (0)
#endif

// Signed 8 bit
// -128 ~ 127
#ifndef s8
typedef __s8 s8;
#endif

// Unsigned 8 bit
// 0 ~ 255 (0xFF)
#ifndef u8
typedef __u8 u8;
#endif

// Signed 16 bit
// -32,768 ~ 32,767
#ifndef s16
typedef __s16 s16;
#endif

// Unsigned 16 bit
// 0 ~ 65,535 (0xFFFF)
#ifndef u16
typedef __u16 u16;
#endif

// Signed 32 bit
// -2,147,483,648 ~ 2,147,483,647
#ifndef s32
typedef __s32 s32;
#endif

// Unsigned 32 bit
// 0 ~ 4,294,967,295 (0xFFFFFFFF)
#ifndef u32
typedef __u32 u32;
#endif

// Float 32 bit
// 1.175494351e-38 to 3.40282347e+38 (normalized values)
#ifndef f32
typedef float f32;
#endif

// Signed 64 bit
// –9,223,372,036,854,775,808 ~ 9,223,372,036,854,775,807
#ifndef s64
typedef __s64 s64;
#endif

// Unsigned 64 bit
// 0 ~ 18,446,744,073,709,551,615 (0xFFFFFFFFFFFFFFFF)
#ifndef u64
typedef __u64 u64;
#endif

// Double 64 bit
// 2.22507385850720138e-308 to 1.79769313486231571e+308 (normalized values)
#ifndef d64
typedef double d64;
#endif

// Boolean 8 bit
// TRUE or FALSE
// #ifndef bool
// typedef u8 bool;
// #endif

#define RS_BIT(pos) (1UL << (pos))

// Octets in one ethernet addr
#ifndef ETH_ADDR_LEN
#define ETH_ADDR_LEN (6)
#endif

#ifndef MAC_ADDRESS_STR
#define MAC_ADDRESS_STR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifndef MAC_ADDR_ARRAY
#define MAC_ADDR_ARRAY(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif

// Get ret from dev status
#define RS_GET_DEV_RET(ret) ((ret) * (-1))

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////
// function return value

typedef enum
{
	RS_SUCCESS = 0, // Success

	RS_FAIL = -1, // Fail
	RS_EMPTY = -2, // Empty
	RS_FULL = -3, // Full
	RS_INVALID_PARAM = -4, // Invalid parameter
	RS_NOT_FOUND = -5, // Not found
	RS_NO_MORE = -6, // No more data
	RS_NOT_IN_USE = -7, // Not in use
	RS_BUSY = -8, // Busy
	RS_IN_PROGRESS = -9, // In progress
	RS_MEMORY_FAIL = -10, // Memory error
	RS_NOT_SUPPORT = -11, // Not support
	RS_EXIST = -12, // Already Exist
} rs_ret;

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

#endif /* RS_TYPE_H */

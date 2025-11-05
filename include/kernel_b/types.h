#ifndef KERNELB_TYPES_H
#define KERNELB_TYPES_H

// Unsigned types
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

// Signed types
typedef signed char        i8;
typedef signed short       i16;
typedef signed int         i32;
typedef signed long long   i64;

// Size and pointer types
typedef unsigned long size_t;
typedef long ssize_t;

// Boolean
typedef int bool;
#define true  1
#define false 0

// Null
#define NULL ((void*)0)

#endif

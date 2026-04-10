#ifndef TYPES_H
#define TYPES_H


typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

typedef signed char         i8;
typedef signed short        i16;
typedef signed int          i32;
typedef signed long long    i64;

typedef u32  usize;
typedef i32  isize;
typedef u32  uintptr_t;

#define NULL  ((void *)0)
#define TRUE  1
#define FALSE 0

/* Compiler hints */
#define UNUSED(x)       ((void)(x))
#define PACKED          __attribute__((packed))
#define ALIGNED(n)      __attribute__((aligned(n)))
#define NORETURN        __attribute__((noreturn))
#define ALWAYS_INLINE   __attribute__((always_inline))

#endif 
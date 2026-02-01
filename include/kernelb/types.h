#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef uint32_t size_t;
typedef int32_t ssize_t;
typedef uint32_t off_t;
typedef uint32_t mode_t;
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;

#define NULL ((void*)0)
#define bool int
#define true 1
#define false 0

#endif
#ifndef _TYPES_H
#define _TYPES_H

#define NULL ((void*)0)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed char int8_t;
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
typedef long time_t;
typedef unsigned int dev_t;
typedef unsigned int ino_t;
typedef unsigned int nlink_t;
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
typedef long time_t;
typedef long suseconds_t;
typedef unsigned int size_t;
typedef int ssize_t;
typedef long off_t;
typedef int mode_t;

typedef struct {
    time_t tv_sec;
    suseconds_t tv_usec;
} timeval;

typedef struct {
    time_t tv_sec;
    long tv_nsec;
} timespec;




#endif
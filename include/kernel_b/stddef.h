#ifndef KERNELB_STDDEF_H
#define KERNELB_STDDEF_H

#include "types.h"

#define offsetof(type, member) ((size_t)&(((type*)0)->member))
#define NULL ((void*)0)

#endif

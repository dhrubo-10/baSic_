#ifndef _STDDEF_H
#define _STDDEF_H

#include "types.h"

#define offsetof(type, member) ((size_t)&((type*)0)->member)

#endif
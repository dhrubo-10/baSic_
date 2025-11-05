#ifndef KERNELB_STDLIB_H
#define KERNELB_STDLIB_H

#include "types.h"

void abort(void) __attribute__((noreturn));
int atoi(const char *s);
void itoa(int value, char *buf, int base);

#endif

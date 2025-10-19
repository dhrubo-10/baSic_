#ifndef CLOCK_H
#define CLOCK_H

#include "types.h"

void clock_init(void);
u64 clock_ticks(void);
void clock_sleep(u64 ms);
void clock_now(char *buf, size_t len);

#endif

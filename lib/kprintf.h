#ifndef KPRINTF_H
#define KPRINTF_H

/* kernel printf — supports: %s %d %u %x %c %% */
void kprintf(const char *fmt, ...);

#endif
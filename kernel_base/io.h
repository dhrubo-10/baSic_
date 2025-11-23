#ifndef IO_H
#define IO_H

// assembler defined routines from iR_1.s */
extern void error(void);
extern int betwen(int val, int low, int high);
extern void putw(int w);
extern void putc(char c);

#endif

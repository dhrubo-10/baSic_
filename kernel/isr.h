#ifndef ISR_H
#define ISR_H
 
#include "idt.h"
 
void isr_handler(registers_t *regs);
 
#endif
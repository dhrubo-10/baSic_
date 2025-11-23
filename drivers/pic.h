#ifndef DRV_PIC_H
#define DRV_PIC_H

#include "../inc/types.h"

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1

void pic_remap(u8 offset1, u8 offset2);
void pic_send_eoi(u8 irq);

#endif

#ifndef DRV_MOUSE_H
#define DRV_MOUSE_H

#include "../inc/types.h"

typedef struct {
    i8 dx;
    i8 dy;
    u8 buttons; 
} mouse_packet_t;

void mouse_init(void);
int mouse_read(mouse_packet_t *pkt);
#endif 

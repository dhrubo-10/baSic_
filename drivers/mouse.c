#include "mouse.h"
#include "io.h"
#include "pic.h"


static volatile u8 mouse_cycle = 0;
static volatile i8 mouse_buf[3];

static void mouse_wait(u8 a_type) {
    u32 timeout = 100000;
    if (a_type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

void mouse_init(void) {
    /* enable auxiliary device - mouse */
    mouse_wait(1);
    outb(0x64, 0xA8);
    /* enable mouse interrupts on PIC (IRQ12) */
    u8 mask = inb(PIC2_DATA);
    mask &= ~(1 << 4); /* IRQ12 is bit 4 on slave PIC */
    outb(PIC2_DATA, mask);

    /* tell controller to use default settings and enable streaming */
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, 0xF4); /* enable packet streaming */
}

int mouse_read(mouse_packet_t *pkt) {
    if ((inb(0x64) & 1) == 0) return 0;
    u8 data = inb(0x60);
    mouse_buf[mouse_cycle++] = data;
    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        pkt->buttons = mouse_buf[0] & 0x07;
        pkt->dx = mouse_buf[1];
        pkt->dy = mouse_buf[2];
        return 1;
    }
    return 0;
}

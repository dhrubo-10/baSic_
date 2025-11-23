#include "timer.h"
#include "io.h"
#include "pic.h"

/* PIT ports */
#define PIT_CHANNEL0 0x40
#define PIT_CMD      0x43
static volatile u64 ticks = 0;

/* irq handler stub needs to be connected to IDT/ISR table by kernel.
   For early testing call timer_tick() from  IRQ wrapper. 
*/


void timer_tick_isr(void) {
    ticks++;
    pic_send_eoi(0);
}

void timer_init(u32 freq_hz) {
    u32 divisor = 1193180 / freq_hz;
    outb(PIT_CMD, 0x36); 
    outb(PIT_CHANNEL0, (u8)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (u8)((divisor >> 8) & 0xFF));
   
}

u64 timer_get_ticks(void) {
    return ticks;
}

void timer_sleep(u32 ms) {
    u64 target = ticks + (ms);
    while (timer_get_ticks() < target) {
        asm volatile("hlt");
    }
}

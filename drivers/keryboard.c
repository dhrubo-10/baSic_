#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "video.h"

/* keyboard buffer simple ring */
#define KBD_BUF_SIZE 64
static volatile u8 kbd_buf[KBD_BUF_SIZE];
static volatile u8 kbd_head = 0;
static volatile u8 kbd_tail = 0;

/* basic scancode -> ASCII map for set 1*/
static const char scancode_map[128] = {

    /* wrote this part using deepseek */
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', /* Backspace */
    '\t', /* Tab */
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', /* Enter */
    0, /* control */
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, /* left shift */
    '\\','z','x','c','v','b','n','m',',','.','/',
    0, /* right shift */
    '*',
    0, /* alt */
    ' ', /* space */
    0, /* caps lock */
    /* rest set to 0 */
    /* done */
};

void keyboard_isr(void) {
    u8 sc = inb(0x60);
    u8 next_head = (kbd_head + 1) % KBD_BUF_SIZE;
    if (next_head != kbd_tail) {
        kbd_buf[kbd_head] = sc;
        kbd_head = next_head;
    }
    pic_send_eoi(1);
}

/* enable keyboard irq and optionally set keyboard controller settings */
void keyboard_init(void) {
    /* unmask IRQ1 on PIC (clear bit 1) */
    u8 mask = inb(PIC1_DATA);
    mask &= ~(1 << 1);
    outb(PIC1_DATA, mask);
}

u8 keyboard_getchar_nonblock(void) {
    if (kbd_head == kbd_tail) return 0;
    u8 sc = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return scancode_to_ascii(sc);
}

/* blocking get */
char keyboard_getchar(void) {
    while (kbd_head == kbd_tail) {
        asm volatile("hlt");
    }
    u8 sc = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return scancode_to_ascii(sc);
}

char scancode_to_ascii(u8 sc) {
    if (sc > 127) return 0;
    return scancode_map[sc];
}

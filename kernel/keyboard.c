/* baSic_ - kernel/keyboard.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * PS/2 keyboard driver: scancode set 1 to ASCII
 * hooks into IRQ 1 via the IRQ dispatch table
 */

#include "keyboard.h"
#include "irq.h"
#include "idt.h"
#include "../lib/kprintf.h"

#define KBD_DATA_PORT   0x60    /* read scancodes from here */

static const char sc_ascii[] = {
/*  0     1     2     3     4     5     6     7  */
    0,    0,   '1',  '2',  '3',  '4',  '5',  '6',
/*  8     9     A     B     C     D     E     F  */
   '7',  '8',  '9',  '0',  '-',  '=',  '\b',  '\t',
/* 10    11    12    13    14    15    16    17  */
   'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
/* 18    19    1A    1B    1C    1D    1E    1F  */
   'o',  'p',  '[',  ']',  '\n',  0,   'a',  's',
/* 20    21    22    23    24    25    26    27  */
   'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
/* 28    29    2A    2B    2C    2D    2E    2F  */
   '\'',  '`',  0,   '\\',  'z',  'x',  'c',  'v',
/* 30    31    32    33    34    35    36    37  */
   'b',  'n',  'm',  ',',  '.',  '/',   0,    '*',
/* 38    39  */
    0,   ' ',
};

/* shifted scancode → ASCII */
static const char sc_ascii_shift[] = {
    0,    0,   '!',  '@',  '#',  '$',  '%',  '^',
   '&',  '*',  '(',  ')',  '_',  '+',  '\b',  '\t',
   'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
   'O',  'P',  '{',  '}',  '\n',  0,   'A',  'S',
   'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
   '"',  '~',   0,   '|',  'Z',  'X',  'C',  'V',
   'B',  'N',  'M',  '<',  '>',  '?',   0,   '*',
    0,   ' ',
};

#define SC_TABLE_SIZE   ((int)(sizeof(sc_ascii)))

static volatile char last_char = 0;
static int shift_held = 0;

static inline u8 inb(u16 port)
{
    u8 val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* IRQ 1 handler — called on every keypress/release */
static void keyboard_irq_handler(registers_t *regs)
{
    (void)regs;

    u8 sc = inb(KBD_DATA_PORT);

    /* bit 7 set = key release */
    if (sc & 0x80) {
        u8 released = sc & 0x7F;
        if (released == 0x2A || released == 0x36)
            shift_held = 0;
        return;
    }

    /* track shift */
    if (sc == 0x2A || sc == 0x36) {
        shift_held = 1;
        return;
    }

    if (sc >= SC_TABLE_SIZE)
        return;

    char c = shift_held ? sc_ascii_shift[sc] : sc_ascii[sc];
    if (c) {
        last_char = c;
    }
}

void keyboard_init(void)
{
    last_char = 0;
    shift_held = 0;
    irq_register(1, keyboard_irq_handler);
}

char keyboard_getchar(void)
{
    char c = last_char;
    last_char = 0;
    return c;
}
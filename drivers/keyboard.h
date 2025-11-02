#ifndef DRV_KEYBOARD_H
#define DRV_KEYBOARD_H

#include "../inc/types.h"

void keyboard_init(void);

/* Non-blocking: returns 0 if no key available, otherwise ASCII code or scancode mapping */
u8 keyboard_getchar_nonblock(void);

/* Blocking: waits and returns ASCII (or 0 for non-printable keys) */
char keyboard_getchar(void);

char scancode_to_ascii(u8 sc);

#endif

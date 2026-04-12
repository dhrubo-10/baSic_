/* baSic_ - kernel/keyboard.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * PS/2 keyboard driver interface
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../include/types.h"

void keyboard_init(void);

/* returns the last ASCII character typed, or 0 if none */
char keyboard_getchar(void);

#endif
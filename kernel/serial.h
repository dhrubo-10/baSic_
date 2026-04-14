/* baSic_ - kernel/serial.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * COM1 serial port driver
 */

#ifndef SERIAL_H
#define SERIAL_H

#include "../include/types.h"

void serial_init(void);
void serial_putchar(char c);
void serial_print(const char *s);

#endif
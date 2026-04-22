/* baSic_ - kernel/terminal.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * terminal input driver: VT100 escape sequence decoder
 * translates raw scancodes into terminal events
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include "../include/types.h"

typedef enum {
    TERM_CHAR   = 0,    /* regular ASCII character */
    TERM_UP,            /* arrow up    */
    TERM_DOWN,          /* arrow down  */
    TERM_LEFT,          /* arrow left  */
    TERM_RIGHT,         /* arrow right */
    TERM_HOME,
    TERM_END,
    TERM_DEL,
    TERM_PGUP,
    TERM_PGDN,
    TERM_NONE,
} term_key_type_t;

typedef struct {
    term_key_type_t type;
    char            ch;     /* valid when type == TERM_CHAR */
} term_event_t;

void term_init(void);
term_event_t term_poll(void);   /* non-blocking — type == TERM_NONE if no key */

#endif
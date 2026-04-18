/* baSic_ - kernel/pipe.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * in..kernel pipe: circular byte buffer connecting two commands
 */

#ifndef PIPE_H
#define PIPE_H

#include "../include/types.h"

#define PIPE_BUF_SIZE   512

typedef struct {
    u8   buf[PIPE_BUF_SIZE];
    u32  read_pos;
    u32  write_pos;
    u32  count;
} pipe_t;

void pipe_init(pipe_t *p);
u32  pipe_write(pipe_t *p, const u8 *data, u32 len);
u32  pipe_read(pipe_t *p, u8 *out, u32 max);
int  pipe_empty(pipe_t *p);

#endif
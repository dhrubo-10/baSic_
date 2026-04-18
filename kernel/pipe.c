/* baSic_ - kernel/pipe.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * circular byte-buffer pipe
 * used to connect shell commands: cmd1 | cmd2
 */

#include "pipe.h"
#include "../lib/string.h"

void pipe_init(pipe_t *p)
{
    memset(p, 0, sizeof(pipe_t));
}

u32 pipe_write(pipe_t *p, const u8 *data, u32 len)
{
    u32 written = 0;
    while (len-- && p->count < PIPE_BUF_SIZE) {
        p->buf[p->write_pos] = *data++;
        p->write_pos = (p->write_pos + 1) % PIPE_BUF_SIZE;
        p->count++;
        written++;
    }
    return written;
}

u32 pipe_read(pipe_t *p, u8 *out, u32 max)
{
    u32 n = 0;
    while (n < max && p->count > 0) {
        out[n++] = p->buf[p->read_pos];
        p->read_pos = (p->read_pos + 1) % PIPE_BUF_SIZE;
        p->count--;
    }
    return n;
}

int pipe_empty(pipe_t *p)
{
    return p->count == 0;
}
#ifndef DRV_VIDEO_H
#define DRV_VIDEO_H

#include "../inc/types.h"

/* color codes for VGA text mode, for now this should work. */
typedef enum {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW = 14,
    VGA_WHITE = 15
} vga_color_t;

void video_init(void);
void video_clear(void);
void video_putc(char c);
void video_puts(const char *s);
void video_set_color(vga_color_t fg, vga_color_t bg);
void video_move_cursor(u16 row, u16 col);

#endif

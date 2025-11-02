#include "video.h"
#include "io.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
static volatile u16 *vga_buf = (u16*)0xB8000;
static u16 cursor_row = 0, cursor_col = 0;
static u8 vga_color = (VGA_LIGHT_GREY | VGA_BLACK << 4);

static void update_cursor() {
    u16 pos = cursor_row * VGA_WIDTH + cursor_col;
    outb(0x3D4, 14);
    outb(0x3D5, (u8)(pos >> 8));
    outb(0x3D4, 15);
    outb(0x3D5, (u8)(pos & 0xFF));
}

void video_init(void) {
    video_clear();
    update_cursor();
}

void video_clear(void) {
    u16 blank = (u16)(' ' | (vga_color << 8));
    for (u16 i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) vga_buf[i] = blank;
    cursor_row = 0; cursor_col = 0;
    update_cursor();
}

static void scroll_if_needed(void) {
    if (cursor_row >= VGA_HEIGHT) {
        /* move all lines up one */
        for (u16 r = 1; r < VGA_HEIGHT; ++r) {
            for (u16 c = 0; c < VGA_WIDTH; ++c) {
                vga_buf[(r-1)*VGA_WIDTH + c] = vga_buf[r*VGA_WIDTH + c];
            }
        }
        /* clr last line */
        u16 blank = (u16)(' ' | (vga_color << 8));
        for (u16 c = 0; c < VGA_WIDTH; ++c) {
            vga_buf[(VGA_HEIGHT-1)*VGA_WIDTH + c] = blank;
        }
        cursor_row = VGA_HEIGHT - 1;
    }
}

void video_putc(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            vga_buf[cursor_row * VGA_WIDTH + cursor_col] = (u16)(' ' | (vga_color << 8));
        }
    } else {
        vga_buf[cursor_row * VGA_WIDTH + cursor_col] = (u16)(c | (vga_color << 8));
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }
    scroll_if_needed();
    update_cursor();
}

void video_puts(const char *s) {
    for (; *s; ++s) video_putc(*s);
}

void video_set_color(vga_color_t fg, vga_color_t bg) {
    vga_color = (u8)(fg | bg << 4);
}

void video_move_cursor(u16 row, u16 col) {
    cursor_row = row;
    cursor_col = col;
    update_cursor();
}
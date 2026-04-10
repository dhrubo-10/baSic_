/* baSic_ - kernel/vga.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 */ 


#include "vga.h"

/* VGA text mode framebuffer: 80 columns x 25 rows, each cell is 2 bytes.
 * Low byte  = ASCII character
 * High byte = color attribute (bg << 4 | fg)
 */
#define VGA_BASE    ((volatile u16 *)0xB8000)
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

static int     cursor_x   = 0;
static int     cursor_y   = 0;
static u8      cur_attr   = 0;   /* color attribute byte */

/* helper: pack fg + bg into a VGA attribute byte */
static inline u8 make_attr(vga_color_t fg, vga_color_t bg)
{
    return (u8)((bg << 4) | (fg & 0x0F));
}

/* helper: write a cell directly into the framebuffer */
static inline void fb_write(int col, int row, char c, u8 attr)
{
    VGA_BASE[row * VGA_WIDTH + col] = (u16)((attr << 8) | (u8)c);
}

void vga_init(void)
{
    cur_attr = make_attr(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_clear(void)
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_BASE[i] = (u16)((cur_attr << 8) | ' ');

    cursor_x = 0;
    cursor_y = 0;
}

void vga_set_color(vga_color_t fg, vga_color_t bg)
{
    cur_attr = make_attr(fg, bg);
}

/* scroll the screen up by one row */
static void scroll(void)
{
    /* copy every row upward by one */
    for (int row = 1; row < VGA_HEIGHT; row++)
        for (int col = 0; col < VGA_WIDTH; col++)
            VGA_BASE[(row - 1) * VGA_WIDTH + col] =
                VGA_BASE[row * VGA_WIDTH + col];

    /* blank the last row */
    for (int col = 0; col < VGA_WIDTH; col++)
        fb_write(col, VGA_HEIGHT - 1, ' ', cur_attr);

    cursor_y = VGA_HEIGHT - 1;
}

void vga_putchar(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;    /* align to next 8-column tab stop */
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            fb_write(cursor_x, cursor_y, ' ', cur_attr);
        }
    } else {
        fb_write(cursor_x, cursor_y, c, cur_attr);
        cursor_x++;
    }

    /* wrap line */
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    /* scroll if we've gone past the last row */
    if (cursor_y >= VGA_HEIGHT)
        scroll();
}

void vga_print(const char *str)
{
    while (*str)
        vga_putchar(*str++);
}

/* print a 32-bit value in hexadecimal, e.g. "0xDEADBEEF" */
void vga_print_hex(u32 value)
{
    const char hex_chars[] = "0123456789ABCDEF";
    char buf[11];   /* "0x" + 8 hex digits + '\0' */
    int  pos = 10;

    buf[10] = '\0';
    if (value == 0) {
        vga_print("0x00000000");
        return;
    }

    while (value > 0 && pos > 2) {
        buf[--pos] = hex_chars[value & 0xF];
        value >>= 4;
    }
    /* pad with leading zeros */
    while (pos > 2)
        buf[--pos] = '0';

    buf[1] = 'x';
    buf[0] = '0';
    vga_print(buf);
}

/* print a 32-bit value in decimal */
void vga_print_dec(u32 value)
{
    char buf[11];   /* max u32 = 4294967295 (10 digits) + '\0' */
    int  pos = 10;

    buf[10] = '\0';
    if (value == 0) {
        vga_putchar('0');
        return;
    }

    while (value > 0) {
        buf[--pos] = '0' + (value % 10);
        value /= 10;
    }
    vga_print(&buf[pos]);
}
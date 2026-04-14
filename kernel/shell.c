/* baSic_ - kernel/shell.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * interactive shell — fixed banner, scrolling body, built-in commands
 */

#include "shell.h"
#include "vga.h"
#include "timer.h"
#include "keyboard.h"
#include "rtc.h"
#include "../mm/pmm.h"
#include "../fs/vfs.h"
#include "../fs/ramfs.h"
#include "../fs/fd.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"
#include "../include/types.h"

#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define HEADER_ROWS     5
#define SHELL_TOP       6
#define PROMPT_ROW      23
#define CMD_BUF_SIZE    80

#define VGA_BASE  ((volatile u16 *)0xB8000)

static char cwd[VFS_PATH_MAX] = "/";

static inline void vga_write_at(int col, int row, char c, u8 fg, u8 bg)
{
    VGA_BASE[row * VGA_WIDTH + col] = (u16)(((bg << 4 | fg) << 8) | (u8)c);
}

static void vga_str_at(int col, int row, const char *s, u8 fg, u8 bg)
{
    while (*s)
        vga_write_at(col++, row, *s++, fg, bg);
}

static void vga_clear_row(int row, u8 fg, u8 bg)
{
    for (int c = 0; c < VGA_WIDTH; c++)
        vga_write_at(c, row, ' ', fg, bg);
}

static void draw_header(void)
{
    for (int r = 0; r < HEADER_ROWS; r++)
        vga_clear_row(r, VGA_COLOR_BLACK, VGA_COLOR_BLACK);

    vga_str_at(34, 1, "baSic_ v1.0",       VGA_COLOR_WHITE,      VGA_COLOR_BLACK);
    vga_str_at(31, 2, "by Shahriar Dhrubo", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    for (int c = 0; c < VGA_WIDTH; c++)
        vga_write_at(c, 4, '-', VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
}

static void write_dd(int col, int row, u8 val, u8 fg)
{
    vga_write_at(col,     row, '0' + val / 10, fg, VGA_COLOR_BLACK);
    vga_write_at(col + 1, row, '0' + val % 10, fg, VGA_COLOR_BLACK);
}

static void update_clock(void)
{
    rtc_time_t t;
    rtc_read(&t);

    write_dd(58, 0, t.hour,   VGA_COLOR_WHITE);
    vga_write_at(60, 0, ':', VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    write_dd(61, 0, t.minute, VGA_COLOR_WHITE);
    vga_write_at(63, 0, ':', VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    write_dd(64, 0, t.second, VGA_COLOR_WHITE);

    vga_write_at(55, 1, '2',  VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_write_at(56, 1, '0',  VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    write_dd(57, 1, t.year,   VGA_COLOR_LIGHT_GREY);
    vga_write_at(59, 1, '-',  VGA_COLOR_DARK_GREY,  VGA_COLOR_BLACK);
    write_dd(60, 1, t.month,  VGA_COLOR_LIGHT_GREY);
    vga_write_at(62, 1, '-',  VGA_COLOR_DARK_GREY,  VGA_COLOR_BLACK);
    write_dd(63, 1, t.day,    VGA_COLOR_LIGHT_GREY);

    u32 total_s = timer_ticks() / 1000;
    u8  h = (u8)(total_s / 3600);
    u8  m = (u8)((total_s % 3600) / 60);
    u8  s = (u8)(total_s % 60);

    vga_str_at(55, 2, "up ", VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    write_dd(58, 2, h, VGA_COLOR_YELLOW);
    vga_write_at(60, 2, ':', VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    write_dd(61, 2, m, VGA_COLOR_YELLOW);
    vga_write_at(63, 2, ':', VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    write_dd(64, 2, s, VGA_COLOR_YELLOW);
}

static void shell_splash(void)
{
    vga_clear();
    vga_str_at(18, 8,  "  _               _      ", VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_str_at(18, 9,  " | |__   __ _ ___(_) ___ ", VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_str_at(18, 10, " | '_ \\ / _` / __| |/ __|", VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_str_at(18, 11, " | |_) | (_| \\__ \\ | (__ ", VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_str_at(18, 12, " |_.__/ \\__,_|___/_|\\___|", VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_str_at(36, 14, "v1.0",              VGA_COLOR_WHITE,      VGA_COLOR_BLACK);
    vga_str_at(27, 15, "by Shahriar Dhrubo", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    timer_sleep(2000);
    vga_clear();
}

static int shell_row = SHELL_TOP;

static void shell_scroll(void)
{
    for (int r = SHELL_TOP; r < PROMPT_ROW - 1; r++)
        for (int c = 0; c < VGA_WIDTH; c++)
            VGA_BASE[r * VGA_WIDTH + c] = VGA_BASE[(r + 1) * VGA_WIDTH + c];
    vga_clear_row(PROMPT_ROW - 1, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    if (shell_row > PROMPT_ROW - 1)
        shell_row = PROMPT_ROW - 1;
}

static void shell_puts(const char *s, u8 fg)
{
    int col = 0;
    while (*s) {
        if (*s == '\n' || col >= VGA_WIDTH) {
            col = 0;
            shell_row++;
            if (shell_row >= PROMPT_ROW - 1)
                shell_scroll();
        }
        if (*s != '\n')
            vga_write_at(col++, shell_row, *s, fg, VGA_COLOR_BLACK);
        s++;
    }
    shell_row++;
    if (shell_row >= PROMPT_ROW - 1)
        shell_scroll();
}

static char cmd_buf[CMD_BUF_SIZE];
static int  cmd_len = 0;

static void prompt_redraw(void)
{
    vga_clear_row(PROMPT_ROW, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_str_at(0, PROMPT_ROW, "baSic_> ", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    for (int i = 0; i < cmd_len; i++)
        vga_write_at(8 + i, PROMPT_ROW, cmd_buf[i], VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write_at(8 + cmd_len, PROMPT_ROW, '_', VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

static void cmd_help(void)
{
    shell_puts("commands:", VGA_COLOR_YELLOW);
    shell_puts("  help              — this message",         VGA_COLOR_LIGHT_GREY);
    shell_puts("  clear             — clear shell area",     VGA_COLOR_LIGHT_GREY);
    shell_puts("  uptime            — time since boot",      VGA_COLOR_LIGHT_GREY);
    shell_puts("  time              — current date/time",    VGA_COLOR_LIGHT_GREY);
    shell_puts("  mem               — memory usage",         VGA_COLOR_LIGHT_GREY);
    shell_puts("  ls                — list directory",       VGA_COLOR_LIGHT_GREY);
    shell_puts("  cat <path>        — print file",           VGA_COLOR_LIGHT_GREY);
    shell_puts("  write <f> <text>  — write file",           VGA_COLOR_LIGHT_GREY);
    shell_puts("  about             — about baSic_",         VGA_COLOR_LIGHT_GREY);
    shell_puts("  halt              — stop the system",      VGA_COLOR_LIGHT_GREY);
}

static void cmd_clear(void)
{
    for (int r = SHELL_TOP; r < PROMPT_ROW; r++)
        vga_clear_row(r, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    shell_row = SHELL_TOP;
}

static void cmd_time(void)
{
    rtc_time_t t;
    rtc_read(&t);
    char buf[48];
    int i = 0;
    const char *p = "time: ";
    while (*p) buf[i++] = *p++;
    buf[i++] = '0' + t.hour / 10;   buf[i++] = '0' + t.hour % 10;
    buf[i++] = ':';
    buf[i++] = '0' + t.minute / 10; buf[i++] = '0' + t.minute % 10;
    buf[i++] = ':';
    buf[i++] = '0' + t.second / 10; buf[i++] = '0' + t.second % 10;
    buf[i++] = ' '; buf[i++] = ' ';
    p = "date: 20";
    while (*p) buf[i++] = *p++;
    buf[i++] = '0' + t.year / 10;   buf[i++] = '0' + t.year % 10;
    buf[i++] = '-';
    buf[i++] = '0' + t.month / 10;  buf[i++] = '0' + t.month % 10;
    buf[i++] = '-';
    buf[i++] = '0' + t.day / 10;    buf[i++] = '0' + t.day % 10;
    buf[i] = '\0';
    shell_puts(buf, VGA_COLOR_LIGHT_CYAN);
}

static void cmd_uptime(void)
{
    u32 total_s = timer_ticks() / 1000;
    u8  h = (u8)(total_s / 3600);
    u8  m = (u8)((total_s % 3600) / 60);
    u8  s = (u8)(total_s % 60);
    char buf[32];
    int i = 0;
    const char *p = "uptime: ";
    while (*p) buf[i++] = *p++;
    buf[i++] = '0' + h / 10; buf[i++] = '0' + h % 10; buf[i++] = 'h';
    buf[i++] = ' ';
    buf[i++] = '0' + m / 10; buf[i++] = '0' + m % 10; buf[i++] = 'm';
    buf[i++] = ' ';
    buf[i++] = '0' + s / 10; buf[i++] = '0' + s % 10; buf[i++] = 's';
    buf[i] = '\0';
    shell_puts(buf, VGA_COLOR_LIGHT_CYAN);
}

static void cmd_mem(void)
{
    u32 total = pmm_total_frames() * 4;
    u32 free  = pmm_free_frames()  * 4;
    u32 used  = total - free;
    kprintf("  total : %d KB\n", total);
    kprintf("  used  : %d KB\n", used);
    kprintf("  free  : %d KB\n", free);
}

static void cmd_ls(void)
{
    vfs_node_t *dir = vfs_resolve(cwd);
    if (!dir || !(dir->flags & VFS_DIR)) {
        shell_puts("ls: not a directory", VGA_COLOR_LIGHT_RED);
        return;
    }

    typedef struct {
        u8         *buf;
        u32         capacity;
        vfs_node_t *children[32];
        u32         child_count;
    } rdata_t;

    rdata_t *rd = (rdata_t *)dir->inode;
    if (!rd || rd->child_count == 0) {
        shell_puts("(empty)", VGA_COLOR_LIGHT_GREY);
        return;
    }
    for (u32 i = 0; i < rd->child_count; i++) {
        vfs_node_t *child = rd->children[i];
        char line[VFS_NAME_MAX + 8];
        int  j = 0;
        if (child->flags & VFS_DIR) {
            line[j++] = '['; line[j++] = 'd'; line[j++] = ']'; line[j++] = ' ';
        } else {
            line[j++] = '['; line[j++] = 'f'; line[j++] = ']'; line[j++] = ' ';
        }
        const char *n = child->name;
        while (*n) line[j++] = *n++;
        line[j] = '\0';
        shell_puts(line, (child->flags & VFS_DIR) ? VGA_COLOR_LIGHT_CYAN : VGA_COLOR_WHITE);
    }
}

static void cmd_cat(const char *path)
{
    if (!path || !*path) {
        shell_puts("usage: cat <path>", VGA_COLOR_LIGHT_RED);
        return;
    }
    char full[VFS_PATH_MAX];
    if (path[0] == '/') {
        strncpy(full, path, VFS_PATH_MAX - 1);
        full[VFS_PATH_MAX - 1] = '\0';
    } else {
        strncpy(full, cwd, VFS_PATH_MAX - 1);
        full[VFS_PATH_MAX - 1] = '\0';
        usize cwdlen = strlen(full);
        if (cwdlen < VFS_PATH_MAX - 2 && full[cwdlen - 1] != '/') {
            full[cwdlen++] = '/';
            full[cwdlen]   = '\0';
        }
        strncpy(full + strlen(full), path, VFS_PATH_MAX - strlen(full) - 1);
    }
    vfs_node_t *node = vfs_resolve(full);
    if (!node || !(node->flags & VFS_FILE)) {
        shell_puts("cat: file not found", VGA_COLOR_LIGHT_RED);
        return;
    }
    u8  buf[128];
    u32 off = 0, n;
    while ((n = vfs_read(node, off, sizeof(buf) - 1, buf)) > 0) {
        buf[n] = '\0';
        shell_puts((char *)buf, VGA_COLOR_WHITE);
        off += n;
    }
}

static void cmd_write(const char *fname, const char *data)
{
    if (!fname || !*fname || !data) {
        shell_puts("usage: write <name> <text>", VGA_COLOR_LIGHT_RED);
        return;
    }
    vfs_node_t *parent = vfs_resolve(cwd);
    if (!parent) { shell_puts("write: bad cwd", VGA_COLOR_LIGHT_RED); return; }

    vfs_node_t *node = vfs_finddir(parent, fname);
    if (!node)
        node = ramfs_mkfile(parent, fname);
    if (!node) { shell_puts("write: could not create file", VGA_COLOR_LIGHT_RED); return; }

    vfs_write(node, 0, strlen(data), (u8 *)data);
    shell_puts("written.", VGA_COLOR_LIGHT_GREEN);
}

static void cmd_about(void)
{
    shell_puts("baSic_ v1.0 — original OS, written from scratch", VGA_COLOR_WHITE);
    shell_puts("author : Shahriar Dhrubo",           VGA_COLOR_LIGHT_GREY);
    shell_puts("arch   : x86 32-bit protected mode", VGA_COLOR_LIGHT_GREY);
    shell_puts("license: GPL v2",                    VGA_COLOR_LIGHT_GREY);
}

static void cmd_halt(void)
{
    shell_puts("halting system...", VGA_COLOR_LIGHT_RED);
    __asm__ volatile ("cli; hlt");
}

static void dispatch(void)
{
    cmd_buf[cmd_len] = '\0';
    if (cmd_len == 0) return;

    if (!strcmp(cmd_buf, "help"))   { cmd_help();   return; }
    if (!strcmp(cmd_buf, "clear"))  { cmd_clear();  return; }
    if (!strcmp(cmd_buf, "uptime")) { cmd_uptime(); return; }
    if (!strcmp(cmd_buf, "time"))   { cmd_time();   return; }
    if (!strcmp(cmd_buf, "mem"))    { cmd_mem();    return; }
    if (!strcmp(cmd_buf, "ls"))     { cmd_ls();     return; }
    if (!strcmp(cmd_buf, "about"))  { cmd_about();  return; }
    if (!strcmp(cmd_buf, "halt"))   { cmd_halt();   return; }

    if (!strncmp(cmd_buf, "cat ", 4)) {
        cmd_cat(cmd_buf + 4);
        return;
    }

    if (!strncmp(cmd_buf, "write ", 6)) {
        char *sp = cmd_buf + 6;
        while (*sp && *sp != ' ') sp++;
        if (*sp == ' ') {
            *sp = '\0';
            cmd_write(cmd_buf + 6, sp + 1);
        } else {
            shell_puts("usage: write <name> <text>", VGA_COLOR_LIGHT_RED);
        }
        return;
    }

    char msg[CMD_BUF_SIZE + 12];
    int i = 0;
    const char *pre = "unknown: ";
    while (*pre) msg[i++] = *pre++;
    for (int j = 0; j < cmd_len; j++) msg[i++] = cmd_buf[j];
    msg[i] = '\0';
    shell_puts(msg, VGA_COLOR_LIGHT_RED);
}

void shell_init(void)
{
    shell_splash();
    draw_header();
    cmd_len = 0;
    memset(cmd_buf, 0, CMD_BUF_SIZE);
    shell_puts("baSic_ shell ready. type 'help' for commands.", VGA_COLOR_LIGHT_GREY);
    prompt_redraw();
}

void shell_run(void)
{
    u32 last_s = (u32)-1;

    for (;;) {
        u32 now_s = timer_ticks() / 1000;
        if (now_s != last_s) {
            update_clock();
            last_s = now_s;
        }

        char c = keyboard_getchar();
        if (!c) {
            __asm__ volatile ("hlt");
            continue;
        }

        if (c == '\n') {
            dispatch();
            cmd_len = 0;
            memset(cmd_buf, 0, CMD_BUF_SIZE);
            prompt_redraw();
        } else if (c == '\b') {
            if (cmd_len > 0) {
                cmd_len--;
                prompt_redraw();
            }
        } else if (cmd_len < CMD_BUF_SIZE - 1) {
            cmd_buf[cmd_len++] = c;
            prompt_redraw();
        }
    }
}
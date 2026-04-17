/* baSic_ - kernel/shell.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * interactive shell
 */

#include "shell.h"
#include "vga.h"
#include "timer.h"
#include "keyboard.h"
#include "rtc.h"
#include "serial.h"
#include "process.h"
#include "shooter.h"
#include "editor.h"
#include "calc.h"
#include "../mm/pmm.h"
#include "../fs/vfs.h"
#include "../fs/ramfs.h"
#include "../fs/fd.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"
#include "../include/types.h"

#define HEADER_ROWS     5
#define SHELL_TOP       6
#define PROMPT_ROW      23
#define CMD_BUF_SIZE    80
#define HISTORY_SIZE    16

#define VGA_BASE  ((volatile u16 *)0xB8000)

static char cwd[VFS_PATH_MAX] = "/";

static char history[HISTORY_SIZE][CMD_BUF_SIZE];
static int  history_count = 0;
static int  history_idx   = -1;

static u8 shell_fg = VGA_COLOR_WHITE;
static u8 shell_bg = VGA_COLOR_BLACK;

static inline void vga_write_at(int col, int row, char c, u8 fg, u8 bg)
{
    VGA_BASE[row * VGA_W + col] = (u16)(((bg << 4 | fg) << 8) | (u8)c);
}

static void vga_str_at(int col, int row, const char *s, u8 fg, u8 bg)
{
    while (*s && col < VGA_W)
        vga_write_at(col++, row, *s++, fg, bg);
}

static void vga_clear_row(int row, u8 fg, u8 bg)
{
    for (int c = 0; c < VGA_W; c++)
        vga_write_at(c, row, ' ', fg, bg);
}

static void draw_header(void)
{
    for (int r = 0; r < HEADER_ROWS; r++)
        vga_clear_row(r, VGA_COLOR_BLACK, VGA_COLOR_BLACK);
    vga_str_at(34, 1, "baSic_ v1.0",       VGA_COLOR_WHITE,      VGA_COLOR_BLACK);
    vga_str_at(31, 2, "by Shahriar Dhrubo", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    for (int c = 0; c < VGA_W; c++)
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
        for (int c = 0; c < VGA_W; c++)
            VGA_BASE[r * VGA_W + c] = VGA_BASE[(r + 1) * VGA_W + c];
    vga_clear_row(PROMPT_ROW - 1, shell_fg, shell_bg);
    if (shell_row > PROMPT_ROW - 1)
        shell_row = PROMPT_ROW - 1;
}

static void shell_puts(const char *s, u8 fg)
{
    int col = 0;
    while (*s) {
        if (*s == '\n' || col >= VGA_W) {
            col = 0;
            shell_row++;
            if (shell_row >= PROMPT_ROW - 1)
                shell_scroll();
        }
        if (*s != '\n')
            vga_write_at(col++, shell_row, *s, fg, shell_bg);
        s++;
    }
    shell_row++;
    if (shell_row >= PROMPT_ROW - 1)
        shell_scroll();
}

static char cmd_buf[CMD_BUF_SIZE];
static int  cmd_len = 0;

/* returns the last component of cwd for the prompt */
static const char *prompt_dirname(void)
{
    if (cwd[0] == '/' && cwd[1] == '\0') return "baSic_";
    const char *last = cwd;
    for (const char *p = cwd; *p; p++)
        if (*p == '/') last = p + 1;
    return (*last) ? last : "baSic_";
}

static void prompt_redraw(void)
{
    vga_clear_row(PROMPT_ROW, shell_fg, shell_bg);
    const char *pname = prompt_dirname();
    int col = 0;
    const char *p = pname;
    while (*p && col < 20)
        vga_write_at(col++, PROMPT_ROW, *p++, VGA_COLOR_LIGHT_GREEN, shell_bg);
    vga_write_at(col++, PROMPT_ROW, '>', shell_bg == VGA_COLOR_BLACK ?
                 VGA_COLOR_LIGHT_GREEN : VGA_COLOR_DARK_GREY, shell_bg);
    vga_write_at(col++, PROMPT_ROW, ' ', shell_fg, shell_bg);
    int text_start = col;
    for (int i = 0; i < cmd_len && col < VGA_W - 1; i++)
        vga_write_at(col++, PROMPT_ROW, cmd_buf[i], shell_fg, shell_bg);
    (void)text_start;
    if (col < VGA_W)
        vga_write_at(col, PROMPT_ROW, '_', VGA_COLOR_LIGHT_GREY, shell_bg);
}

static void history_push(void)
{
    if (cmd_len == 0) return;
    int slot = history_count % HISTORY_SIZE;
    strncpy(history[slot], cmd_buf, CMD_BUF_SIZE - 1);
    history[slot][CMD_BUF_SIZE - 1] = '\0';
    history_count++;
    history_idx = -1;
}

static void history_up(void)
{
    if (history_count == 0) return;
    if (history_idx == -1) history_idx = history_count - 1;
    else if (history_idx > 0) history_idx--;
    int slot = history_idx % HISTORY_SIZE;
    strncpy(cmd_buf, history[slot], CMD_BUF_SIZE - 1);
    cmd_len = (int)strlen(cmd_buf);
    prompt_redraw();
}

/* build full path from cwd + relative name */
static void make_full(char *out, const char *path)
{
    if (path[0] == '/') {
        strncpy(out, path, VFS_PATH_MAX - 1);
        out[VFS_PATH_MAX - 1] = '\0';
    } else {
        strncpy(out, cwd, VFS_PATH_MAX - 1);
        out[VFS_PATH_MAX - 1] = '\0';
        usize cwdlen = strlen(out);
        if (cwdlen < VFS_PATH_MAX - 2 && out[cwdlen - 1] != '/') {
            out[cwdlen++] = '/';
            out[cwdlen]   = '\0';
        }
        strncpy(out + strlen(out), path, VFS_PATH_MAX - strlen(out) - 1);
    }
}

static void cmd_help(void)
{
    shell_puts("baSic_ commands:", VGA_COLOR_YELLOW);
    shell_puts("  help              — this message",            VGA_COLOR_LIGHT_GREY);
    shell_puts("  clear             — clear shell",             VGA_COLOR_LIGHT_GREY);
    shell_puts("  echo <text>       — print text",              VGA_COLOR_LIGHT_GREY);
    shell_puts("  calc <expr>       — calculator: 2+3*4",       VGA_COLOR_LIGHT_GREY);
    shell_puts("  color <0-7>       — change shell color",      VGA_COLOR_LIGHT_GREY);
    shell_puts("  uptime            — time since boot",         VGA_COLOR_LIGHT_GREY);
    shell_puts("  time              — current date/time",       VGA_COLOR_LIGHT_GREY);
    shell_puts("  mem               — memory usage",            VGA_COLOR_LIGHT_GREY);
    shell_puts("  sysinfo           — system information",      VGA_COLOR_LIGHT_GREY);
    shell_puts("  ps                — list processes",          VGA_COLOR_LIGHT_GREY);
    shell_puts("  history           — command history",         VGA_COLOR_LIGHT_GREY);
    shell_puts("  pwd               — print working directory", VGA_COLOR_LIGHT_GREY);
    shell_puts("  cd <dir>          — change directory",        VGA_COLOR_LIGHT_GREY);
    shell_puts("  ls                — list directory",          VGA_COLOR_LIGHT_GREY);
    shell_puts("  cat <path>        — print file",              VGA_COLOR_LIGHT_GREY);
    shell_puts("  write <f> <text>  — write to file",           VGA_COLOR_LIGHT_GREY);
    shell_puts("  mkdir <name>      — make directory",          VGA_COLOR_LIGHT_GREY);
    shell_puts("  edit <file>       — open text editor",        VGA_COLOR_LIGHT_CYAN);
    shell_puts("  shoot             — shooter game",            VGA_COLOR_LIGHT_CYAN);
    shell_puts("  about             — about baSic_",            VGA_COLOR_LIGHT_GREY);
    shell_puts("  reboot            — reboot system",           VGA_COLOR_LIGHT_GREY);
    shell_puts("  halt              — stop system",             VGA_COLOR_LIGHT_GREY);
    shell_puts("  Ctrl-P: history up   Ctrl-U: clear line",    VGA_COLOR_DARK_GREY);
}

static void cmd_clear(void)
{
    for (int r = SHELL_TOP; r < PROMPT_ROW; r++)
        vga_clear_row(r, shell_fg, shell_bg);
    shell_row = SHELL_TOP;
}

static void cmd_echo(const char *text)
{
    shell_puts(text ? text : "", shell_fg);
}

static void cmd_calc(const char *expr)
{
    if (!expr || !*expr) {
        shell_puts("usage: calc <expression>  e.g. calc 2+3*4", VGA_COLOR_LIGHT_RED);
        return;
    }
    i32 result;
    if (!calc_eval(expr, &result)) {
        shell_puts("calc: invalid expression", VGA_COLOR_LIGHT_RED);
        return;
    }
    /* print "= result" */
    char buf[32];
    int i = 0;
    buf[i++] = '='; buf[i++] = ' ';
    i32 v = result;
    if (v < 0) { buf[i++] = '-'; v = -v; }
    char tmp[12]; int ti = 10; tmp[11] = '\0';
    if (v == 0) { tmp[ti--] = '0'; }
    else { while (v > 0) { tmp[ti--] = '0' + v % 10; v /= 10; } }
    const char *tp = &tmp[ti + 1];
    while (*tp) buf[i++] = *tp++;
    buf[i] = '\0';
    shell_puts(buf, VGA_COLOR_LIGHT_GREEN);
}

static void cmd_color(const char *arg)
{
    if (!arg || *arg < '0' || *arg > '7') {
        shell_puts("usage: color <0-7>", VGA_COLOR_LIGHT_RED);
        shell_puts("  0=black 1=blue 2=green 3=cyan", VGA_COLOR_LIGHT_GREY);
        shell_puts("  4=red   5=magenta 6=brown 7=grey", VGA_COLOR_LIGHT_GREY);
        return;
    }
    shell_bg = (u8)(*arg - '0');
    shell_fg = (shell_bg == VGA_COLOR_BLACK) ? VGA_COLOR_WHITE : VGA_COLOR_BLACK;
    cmd_clear();
    shell_puts("color changed.", shell_fg);
}

static void cmd_time(void)
{
    rtc_time_t t; rtc_read(&t);
    char buf[48]; int i = 0;
    const char *p = "time: ";
    while (*p) buf[i++] = *p++;
    buf[i++]='0'+t.hour/10;   buf[i++]='0'+t.hour%10;   buf[i++]=':';
    buf[i++]='0'+t.minute/10; buf[i++]='0'+t.minute%10; buf[i++]=':';
    buf[i++]='0'+t.second/10; buf[i++]='0'+t.second%10;
    buf[i++]=' '; buf[i++]=' ';
    p="date: 20"; while(*p) buf[i++]=*p++;
    buf[i++]='0'+t.year/10;  buf[i++]='0'+t.year%10;  buf[i++]='-';
    buf[i++]='0'+t.month/10; buf[i++]='0'+t.month%10; buf[i++]='-';
    buf[i++]='0'+t.day/10;   buf[i++]='0'+t.day%10;
    buf[i]='\0';
    shell_puts(buf, VGA_COLOR_LIGHT_CYAN);
}

static void cmd_uptime(void)
{
    u32 s = timer_ticks() / 1000;
    u8  h=(u8)(s/3600), m=(u8)((s%3600)/60), sec=(u8)(s%60);
    char buf[32]; int i=0;
    const char *p="uptime: "; while(*p) buf[i++]=*p++;
    buf[i++]='0'+h/10;buf[i++]='0'+h%10;buf[i++]='h';buf[i++]=' ';
    buf[i++]='0'+m/10;buf[i++]='0'+m%10;buf[i++]='m';buf[i++]=' ';
    buf[i++]='0'+sec/10;buf[i++]='0'+sec%10;buf[i++]='s';buf[i]='\0';
    shell_puts(buf, VGA_COLOR_LIGHT_CYAN);
}

static void cmd_mem(void)
{
    u32 total = pmm_total_frames() * 4;
    u32 free  = pmm_free_frames()  * 4;
    u32 used  = total - free;
    char buf[48];

    /* helper lambda via nested function not available — inline it */
    #define PRINT_MEM_LINE(label, val, color) do { \
        int _i = 0; const char *_p = label; \
        while (*_p) buf[_i++] = *_p++; \
        u32 _v = val; char _t[12]; int _ti = 10; _t[11]='\0'; \
        if (!_v){_t[_ti--]='0';}else{while(_v){_t[_ti--]='0'+_v%10;_v/=10;}} \
        _p=&_t[_ti+1]; while(*_p) buf[_i++]=*_p++; \
        _p=" KB"; while(*_p) buf[_i++]=*_p++; buf[_i]='\0'; \
        shell_puts(buf, color); \
    } while(0)

    PRINT_MEM_LINE("  total : ", total, VGA_COLOR_LIGHT_GREY);
    PRINT_MEM_LINE("  used  : ", used,  VGA_COLOR_LIGHT_GREY);
    PRINT_MEM_LINE("  free  : ", free,  VGA_COLOR_LIGHT_GREEN);
    #undef PRINT_MEM_LINE
}

static void cmd_sysinfo(void)
{
    shell_puts("  +---------------------------------+", VGA_COLOR_LIGHT_CYAN);
    shell_puts("  |         baSic_ v1.0            |", VGA_COLOR_LIGHT_CYAN);
    shell_puts("  +---------------------------------+", VGA_COLOR_LIGHT_CYAN);
    shell_puts("  author   : Shahriar Dhrubo",         VGA_COLOR_WHITE);
    shell_puts("  arch     : x86 32-bit protected mode",VGA_COLOR_WHITE);
    shell_puts("  kernel   : baSic_ (original, not Unix/Linux)", VGA_COLOR_WHITE);
    shell_puts("  license  : GPL v2",                  VGA_COLOR_WHITE);
    u32 total = pmm_total_frames() * 4;
    char buf[32];
    int i = 0;
    const char *p = "  memory  : ";
    while (*p) buf[i++] = *p++;
    u32 v = total; char tmp[12]; int ti = 10; tmp[11]='\0';
    if (!v){tmp[ti--]='0';}else{while(v){tmp[ti--]='0'+v%10;v/=10;}}
    p=&tmp[ti+1]; while(*p) buf[i++]=*p++;
    p=" KB total"; while(*p) buf[i++]=*p++; buf[i]='\0';
    shell_puts(buf, VGA_COLOR_WHITE);
    cmd_uptime();
}

static void cmd_history(void)
{
    if (history_count == 0) { shell_puts("no history", VGA_COLOR_LIGHT_GREY); return; }
    int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
    for (int i = start; i < history_count; i++) {
        char line[CMD_BUF_SIZE + 6];
        int j = 0;
        /* index number */
        u32 idx = (u32)(i + 1);
        char tmp[8]; int ti = 6; tmp[7]='\0';
        if(!idx){tmp[ti--]='0';}else{while(idx){tmp[ti--]='0'+idx%10;idx/=10;}}
        const char *tp=&tmp[ti+1];
        while(*tp) line[j++]=*tp++;
        line[j++]=' '; line[j++]=' ';
        const char *h = history[i % HISTORY_SIZE];
        while (*h) line[j++] = *h++;
        line[j] = '\0';
        shell_puts(line, VGA_COLOR_LIGHT_GREY);
    }
}

static void cmd_ps(void)
{
    shell_puts("  PID  STATE    NAME", VGA_COLOR_YELLOW);
    shell_puts("  1    running  shell", VGA_COLOR_LIGHT_GREY);
}

static void cmd_pwd(void)
{
    shell_puts(cwd, VGA_COLOR_WHITE);
}

static void cmd_cd(const char *path)
{
    if (!path || !*path) {
        strncpy(cwd, "/", VFS_PATH_MAX);
        prompt_redraw();
        return;
    }
    char full[VFS_PATH_MAX];
    make_full(full, path);
    vfs_node_t *node = vfs_resolve(full);
    if (!node || !(node->flags & VFS_DIR)) {
        shell_puts("cd: no such directory", VGA_COLOR_LIGHT_RED);
        return;
    }
    strncpy(cwd, full, VFS_PATH_MAX - 1);
    cwd[VFS_PATH_MAX - 1] = '\0';
}

static void cmd_ls(void)
{
    vfs_node_t *dir = vfs_resolve(cwd);
    if (!dir || !(dir->flags & VFS_DIR)) {
        shell_puts("ls: not a directory", VGA_COLOR_LIGHT_RED);
        return;
    }
    typedef struct {
        u8 *buf; u32 capacity;
        vfs_node_t *children[32];
        u32 child_count;
    } rdata_t;
    rdata_t *rd = (rdata_t *)dir->inode;
    if (!rd || rd->child_count == 0) { shell_puts("(empty)", VGA_COLOR_LIGHT_GREY); return; }
    for (u32 i = 0; i < rd->child_count; i++) {
        vfs_node_t *child = rd->children[i];
        char line[VFS_NAME_MAX + 8];
        int j = 0;
        if (child->flags & VFS_DIR) {
            line[j++]='['; line[j++]='d'; line[j++]=']'; line[j++]=' ';
        } else {
            line[j++]='['; line[j++]='f'; line[j++]=']'; line[j++]=' ';
        }
        const char *n = child->name;
        while (*n) line[j++] = *n++;
        line[j] = '\0';
        shell_puts(line, (child->flags & VFS_DIR) ? VGA_COLOR_LIGHT_CYAN : VGA_COLOR_WHITE);
    }
}

static void cmd_cat(const char *path)
{
    if (!path || !*path) { shell_puts("usage: cat <path>", VGA_COLOR_LIGHT_RED); return; }
    char full[VFS_PATH_MAX];
    make_full(full, path);
    vfs_node_t *node = vfs_resolve(full);
    if (!node || !(node->flags & VFS_FILE)) { shell_puts("cat: not found", VGA_COLOR_LIGHT_RED); return; }
    u8 buf[128]; u32 off = 0, n;
    while ((n = vfs_read(node, off, sizeof(buf) - 1, buf)) > 0) {
        buf[n] = '\0';
        shell_puts((char *)buf, shell_fg);
        off += n;
    }
}

static void cmd_write(const char *fname, const char *data)
{
    if (!fname || !*fname || !data) { shell_puts("usage: write <n> <text>", VGA_COLOR_LIGHT_RED); return; }
    vfs_node_t *parent = vfs_resolve(cwd);
    if (!parent) { shell_puts("write: bad cwd", VGA_COLOR_LIGHT_RED); return; }
    vfs_node_t *node = vfs_finddir(parent, fname);
    if (!node) node = ramfs_mkfile(parent, fname);
    if (!node) { shell_puts("write: failed", VGA_COLOR_LIGHT_RED); return; }
    vfs_write(node, 0, strlen(data), (u8 *)data);
    shell_puts("written.", VGA_COLOR_LIGHT_GREEN);
}

static void cmd_mkdir(const char *name)
{
    if (!name || !*name) { shell_puts("usage: mkdir <name>", VGA_COLOR_LIGHT_RED); return; }
    vfs_node_t *parent = vfs_resolve(cwd);
    if (!parent) { shell_puts("mkdir: bad cwd", VGA_COLOR_LIGHT_RED); return; }
    if (!ramfs_mkdir(parent, name)) shell_puts("mkdir: failed", VGA_COLOR_LIGHT_RED);
    else shell_puts("directory created.", VGA_COLOR_LIGHT_GREEN);
}

static void cmd_edit(const char *path)
{
    if (!path || !*path) { shell_puts("usage: edit <path>", VGA_COLOR_LIGHT_RED); return; }
    char full[VFS_PATH_MAX];
    make_full(full, path);
    editor_open(full);
    vga_clear();
    draw_header();
    for (int r = SHELL_TOP; r < PROMPT_ROW; r++)
        vga_clear_row(r, shell_fg, shell_bg);
    shell_row = SHELL_TOP;
    shell_puts("returned from editor.", VGA_COLOR_LIGHT_GREY);
    prompt_redraw();
}

static void cmd_shoot(void)
{
    shell_puts("launching shooter... (Q to quit)", VGA_COLOR_LIGHT_CYAN);
    timer_sleep(600);
    shooter_run();
    vga_clear();
    draw_header();
    for (int r = SHELL_TOP; r < PROMPT_ROW; r++)
        vga_clear_row(r, shell_fg, shell_bg);
    shell_row = SHELL_TOP;
    shell_puts("returned from shooter.", VGA_COLOR_LIGHT_GREY);
    prompt_redraw();
}

static void cmd_about(void)
{
    shell_puts("baSic_ v1.0 — original OS, written from scratch", VGA_COLOR_WHITE);
    shell_puts("not Unix. not Linux. its own thing.",              VGA_COLOR_LIGHT_CYAN);
    shell_puts("author : Shahriar Dhrubo",                        VGA_COLOR_LIGHT_GREY);
    shell_puts("arch   : x86 32-bit protected mode",              VGA_COLOR_LIGHT_GREY);
    shell_puts("license: GPL v2",                                  VGA_COLOR_LIGHT_GREY);
}

static void cmd_reboot(void)
{
    shell_puts("rebooting...", VGA_COLOR_YELLOW);
    serial_print("baSic_: reboot requested\n");
    timer_sleep(500);
    /* pulse the 8042 keyboard controller reset line */
    __asm__ volatile (
        "mov $0xFE, %%al\n"
        "outb %%al, $0x64\n"
        : : : "al"
    );
    for (;;) __asm__ volatile ("hlt");
}

static void cmd_halt(void)
{
    serial_print("baSic_: system halted\n");
    shell_puts("halting...", VGA_COLOR_LIGHT_RED);
    __asm__ volatile ("cli; hlt");
}

static void dispatch(void)
{
    cmd_buf[cmd_len] = '\0';
    if (cmd_len == 0) return;

    if (!strcmp(cmd_buf, "help"))    { cmd_help();    return; }
    if (!strcmp(cmd_buf, "clear"))   { cmd_clear();   return; }
    if (!strcmp(cmd_buf, "uptime"))  { cmd_uptime();  return; }
    if (!strcmp(cmd_buf, "time"))    { cmd_time();    return; }
    if (!strcmp(cmd_buf, "mem"))     { cmd_mem();     return; }
    if (!strcmp(cmd_buf, "sysinfo")) { cmd_sysinfo(); return; }
    if (!strcmp(cmd_buf, "ps"))      { cmd_ps();      return; }
    if (!strcmp(cmd_buf, "history")) { cmd_history(); return; }
    if (!strcmp(cmd_buf, "pwd"))     { cmd_pwd();     return; }
    if (!strcmp(cmd_buf, "ls"))      { cmd_ls();      return; }
    if (!strcmp(cmd_buf, "about"))   { cmd_about();   return; }
    if (!strcmp(cmd_buf, "shoot"))   { cmd_shoot();   return; }
    if (!strcmp(cmd_buf, "reboot"))  { cmd_reboot();  return; }
    if (!strcmp(cmd_buf, "halt"))    { cmd_halt();    return; }

    if (!strncmp(cmd_buf, "echo ",   5)) { cmd_echo(cmd_buf + 5);   return; }
    if (!strncmp(cmd_buf, "calc ",   5)) { cmd_calc(cmd_buf + 5);   return; }
    if (!strncmp(cmd_buf, "color ",  6)) { cmd_color(cmd_buf + 6);  return; }
    if (!strncmp(cmd_buf, "cd ",     3)) { cmd_cd(cmd_buf + 3);     return; }
    if (!strncmp(cmd_buf, "mkdir ",  6)) { cmd_mkdir(cmd_buf + 6);  return; }
    if (!strncmp(cmd_buf, "cat ",    4)) { cmd_cat(cmd_buf + 4);    return; }
    if (!strncmp(cmd_buf, "edit ",   5)) { cmd_edit(cmd_buf + 5);   return; }

    if (!strncmp(cmd_buf, "write ", 6)) {
        char *sp = cmd_buf + 6;
        while (*sp && *sp != ' ') sp++;
        if (*sp == ' ') { *sp = '\0'; cmd_write(cmd_buf + 6, sp + 1); }
        else shell_puts("usage: write <n> <text>", VGA_COLOR_LIGHT_RED);
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
    cmd_len       = 0;
    history_count = 0;
    history_idx   = -1;
    shell_fg      = VGA_COLOR_WHITE;
    shell_bg      = VGA_COLOR_BLACK;
    memset(cmd_buf, 0, CMD_BUF_SIZE);
    memset(history, 0, sizeof(history));
    shell_puts("baSic_ shell ready. type 'help' for commands.", VGA_COLOR_LIGHT_GREY);
    prompt_redraw();
}

void shell_run(void)
{
    u32 last_s = (u32)-1;
    for (;;) {
        u32 now_s = timer_ticks() / 1000;
        if (now_s != last_s) { update_clock(); last_s = now_s; }

        char c = keyboard_getchar();
        if (!c) { __asm__ volatile ("hlt"); continue; }

        if (c == '\n') {
            history_push();
            dispatch();
            cmd_len = 0;
            memset(cmd_buf, 0, CMD_BUF_SIZE);
            prompt_redraw();
        } else if (c == '\b') {
            if (cmd_len > 0) { cmd_len--; prompt_redraw(); }
        } else if (c == 'u' - 96) {
            cmd_len = 0; memset(cmd_buf, 0, CMD_BUF_SIZE); prompt_redraw();
        } else if (c == 'p' - 96) {
            history_up();
        } else if (cmd_len < CMD_BUF_SIZE - 1) {
            cmd_buf[cmd_len++] = c;
            prompt_redraw();
        }
    }
}
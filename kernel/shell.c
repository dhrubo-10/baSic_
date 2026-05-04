/* baSic_ - kernel/shell.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * interactive shell — Linux-style terminal interface
 */

#include "shell.h"
#include "vga.h"
#include "timer.h"
#include "keyboard.h"
#include "terminal.h"
#include "rtc.h"
#include "serial.h"
#include "process.h"
#include "signal.h"
#include "shooter.h"
#include "editor.h"
#include "calc.h"
#include "env.h"
#include "klog.h"
#include "pipe.h"
#include "disk.h"
#include "fat12.h"
#include "filemeta.h"
#include "disksync.h"
#include "profiler.h"
#include "watchdog.h"
#include "elf.h"
#include "e1000.h"
#include "../mm/pmm.h"
#include "../fs/vfs.h"
#include "../fs/ramfs.h"
#include "../fs/fd.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"
#include "../include/types.h"

#define TERM_TOP       0
#define TERM_BOT       22
#define PROMPT_ROW     23
#define STATUS_ROW     24
#define CMD_BUF_SIZE   78
#define HISTORY_SIZE   32

#define VGA_BASE  ((volatile u16 *)0xB8000)

/* my country timezone = UTC+6 */
#define TZ_OFFSET_H    6
#define HISTORY_FILE "HISTORY"

static char cwd[VFS_PATH_MAX] = "/";
static char history[HISTORY_SIZE][CMD_BUF_SIZE];
static int  history_count = 0;
static int  history_idx   = -1;
static int  shell_row     = TERM_TOP;
static void shell_newline(void);   
static void shell_exec_line(char *line);
static void dispatch(void);

static inline void put(int col, int row, char c, u8 fg, u8 bg)
{
    if (col < 0 || col >= VGA_W || row < 0 || row >= VGA_H) return;
    VGA_BASE[row * VGA_W + col] = (u16)(((bg << 4 | fg) << 8) | (u8)c);
}

static void fill_row(int row, char c, u8 fg, u8 bg)
{
    for (int i = 0; i < VGA_W; i++)
        put(i, row, c, fg, bg);
}

static void str_at(int col, int row, const char *s, u8 fg, u8 bg)
{
    while (*s && col < VGA_W)
        put(col++, row, *s++, fg, bg);
}

static void draw_status(void)
{
    fill_row(STATUS_ROW, ' ', VGA_COLOR_BLACK, VGA_COLOR_DARK_GREY);

    /* left: baSic_ v1.0 */
    str_at(1, STATUS_ROW, "baSic_ v1.0", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_DARK_GREY);
    str_at(13, STATUS_ROW, "|", VGA_COLOR_DARK_GREY, VGA_COLOR_DARK_GREY);

    /* middle: cwd */
    str_at(15, STATUS_ROW, cwd, VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY);

    /* right: time (BD UTC+6) + uptime */
    rtc_time_t t;
    rtc_read(&t);

    /* apply BD UTC+6 offset */
    int hour = (int)t.hour + TZ_OFFSET_H;
    if (hour >= 24) hour -= 24;

    char tbuf[32];
    int i = 0;
    tbuf[i++] = '0' + hour / 10;
    tbuf[i++] = '0' + hour % 10;
    tbuf[i++] = ':';
    tbuf[i++] = '0' + t.minute / 10;
    tbuf[i++] = '0' + t.minute % 10;
    tbuf[i++] = ':';
    tbuf[i++] = '0' + t.second / 10;
    tbuf[i++] = '0' + t.second % 10;
    tbuf[i++] = ' ';
    tbuf[i++] = 'B'; tbuf[i++] = 'D';
    tbuf[i] = '\0';

    str_at(55, STATUS_ROW, tbuf, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_DARK_GREY);

    /* uptime */
    u32 s = timer_ticks() / 1000;
    u8  uh = (u8)(s / 3600);
    u8  um = (u8)((s % 3600) / 60);
    u8  us = (u8)(s % 60);
    char ubuf[16];
    int j = 0;
    ubuf[j++] = '|'; ubuf[j++] = ' ';
    ubuf[j++] = '0' + uh / 10; ubuf[j++] = '0' + uh % 10; ubuf[j++] = ':';
    ubuf[j++] = '0' + um / 10; ubuf[j++] = '0' + um % 10; ubuf[j++] = ':';
    ubuf[j++] = '0' + us / 10; ubuf[j++] = '0' + us % 10;
    ubuf[j] = '\0';
    str_at(68, STATUS_ROW, ubuf, VGA_COLOR_YELLOW, VGA_COLOR_DARK_GREY);
}

// static void shell_splash(void)
// {
//     vga_clear();

//     /* full-screen centered splash */
//     u8 c1 = VGA_COLOR_LIGHT_CYAN;
//     u8 c2 = VGA_COLOR_WHITE;
//     u8 c4 = VGA_COLOR_DARK_GREY;
//     u8 bk = VGA_COLOR_BLACK;

//     str_at(18, 7,  "  _               _      ", c1, bk);
//     str_at(18, 8,  " | |__   __ _ ___(_) ___ ", c1, bk);
//     str_at(18, 9,  " | '_ \\ / _` / __| |/ __|", c1, bk);
//     str_at(18, 10, " | |_) | (_| \\__ \\ | (__ ", c1, bk);
//     str_at(18, 11, " |_.__/ \\__,_|___/_|\\___|", c1, bk);

//     str_at(34, 13, "v1.0",               c2, bk);

//     /* decorative separator */
//     for (int col = 10; col < 70; col++)
//         put(col, 16, '-', c4, bk);

//     str_at(23, 18, "booting baSic_...", c4, bk);

//     timer_sleep(2000);
//     vga_clear();
// }

static void shell_splash(void)
{
    vga_clear();
    /* splash disabled for debugging */
}

static void scroll(void)
{
    for (int r = TERM_TOP; r < TERM_BOT; r++)
        for (int c = 0; c < VGA_W; c++)
            VGA_BASE[r * VGA_W + c] = VGA_BASE[(r + 1) * VGA_W + c];
    fill_row(TERM_BOT, ' ', VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    if (shell_row > TERM_BOT) shell_row = TERM_BOT;
}

static void shell_puts(const char *s, u8 fg)
{
    int col = 0;
    while (*s) {
        if (*s == '\n' || col >= VGA_W) {
            col = 0; shell_row++;
            if (shell_row > TERM_BOT) { scroll(); shell_row = TERM_BOT; }
        }
        if (*s != '\n')
            put(col++, shell_row, *s, fg, VGA_COLOR_BLACK);
        s++;
    }
    shell_newline();
}

static void shell_newline(void)
{
    shell_row++;
    if (shell_row > TERM_BOT) { scroll(); shell_row = TERM_BOT; }
}

static char  cmd_buf[CMD_BUF_SIZE];
static int   cmd_len    = 0;
static int   cursor_pos = 0;

/* build "root@baSic_:/path# " */
static int build_prompt(char *out)
{
    int i = 0;
    const char *user = "root";
    const char *host = "baSic_";
    while (*user) out[i++] = *user++;
    out[i++] = '@';
    while (*host) out[i++] = *host++;
    out[i++] = ':';
    const char *p = cwd;
    while (*p) out[i++] = *p++;
    out[i++] = '#';
    out[i++] = ' ';
    out[i] = '\0';
    return i;
}

static void history_save(void)
{
    /* build one big buffer: each entry newline separated */
    static u8 hbuf[HISTORY_SIZE * CMD_BUF_SIZE];
    u32 off = 0;
    int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
    for (int i = start; i < history_count; i++) {
        const char *h = history[i % HISTORY_SIZE];
        usize hlen = strlen(h);
        if (off + hlen + 1 >= sizeof(hbuf)) break;
        memcpy(hbuf + off, h, hlen);
        off += hlen;
        hbuf[off++] = '\n';
    }
    if (!off) return;
    int n = fat12_write(HISTORY_FILE, hbuf, off);
    if (n < 0)
        kprintf("[shell] history save failed\n");
    else
        kprintf("[shell] history saved (%d bytes)\n", off);
}

static void history_load(void)
{
    static u8 hbuf[HISTORY_SIZE * CMD_BUF_SIZE];
    int n = fat12_read(HISTORY_FILE, hbuf, sizeof(hbuf) - 1);
    if (n <= 0) return;
    hbuf[n] = '\0';

    char *line = (char *)hbuf;
    while (*line && history_count < HISTORY_SIZE * 2) {
        char *end = line;
        while (*end && *end != '\n') end++;
        char saved = *end;
        *end = '\0';
        if (strlen(line) > 0) {
            int slot = history_count % HISTORY_SIZE;
            strncpy(history[slot], line, CMD_BUF_SIZE - 1);
            history[slot][CMD_BUF_SIZE - 1] = '\0';
            history_count++;
        }
        *end = saved;
        line = (*end) ? end + 1 : end;
    }
    kprintf("[shell] history loaded (%d entries)\n", history_count);
}



static void prompt_redraw(void)
{
    fill_row(PROMPT_ROW, ' ', VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    char prompt[48];
    int  plen = build_prompt(prompt);

    /* "root@baSic_" in green, ":/path" in white, "# " in bright white */
    int col = 0;
    /* user@host */
    for (int i = 0; prompt[i] && prompt[i] != ':'; i++)
        put(col++, PROMPT_ROW, prompt[i], VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    /* :path */
    int j = 0;
    while (prompt[j] && prompt[j] != ':') j++;
    while (prompt[j] && prompt[j] != '#')
        put(col++, PROMPT_ROW, prompt[j++], VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    /* # */
    put(col++, PROMPT_ROW, '#', VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    put(col++, PROMPT_ROW, ' ', VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    int text_start = col;
    (void)plen;

    for (int i = 0; i < cmd_len && col < VGA_W - 1; i++) {
        u8 fg = (i == cursor_pos) ? VGA_COLOR_BLACK : VGA_COLOR_WHITE;
        u8 bg = (i == cursor_pos) ? VGA_COLOR_WHITE : VGA_COLOR_BLACK;
        put(col++, PROMPT_ROW, cmd_buf[i], fg, bg);
    }
    if (cursor_pos >= cmd_len && col < VGA_W)
        put(col, PROMPT_ROW, ' ', VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);

    (void)text_start;
}

/* echo the current command into the terminal scroll area (like a real terminal) */
static void echo_input(void)
{
    char prompt[48];
    int plen = build_prompt(prompt);
    (void)plen;

    /* print prompt into scroll area */
    shell_row++;
    if (shell_row > TERM_BOT) { scroll(); shell_row = TERM_BOT; }

    int col = 0;
    for (int i = 0; prompt[i] && prompt[i] != ':'; i++)
        put(col++, shell_row, prompt[i], VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    int j = 0;
    while (prompt[j] && prompt[j] != ':') j++;
    while (prompt[j] && prompt[j] != '#')
        put(col++, shell_row, prompt[j++], VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    put(col++, shell_row, '#', VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    put(col++, shell_row, ' ', VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    for (int i = 0; i < cmd_len && col < VGA_W; i++)
        put(col++, shell_row, cmd_buf[i], VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

static void history_push(void)
{
    if (!cmd_len) return;
    int slot = history_count % HISTORY_SIZE;
    strncpy(history[slot], cmd_buf, CMD_BUF_SIZE - 1);
    history[slot][CMD_BUF_SIZE - 1] = '\0';
    history_count++;
    history_idx = -1;
}

static void history_up(void)
{
    if (!history_count) return;
    if (history_idx == -1) history_idx = history_count - 1;
    else if (history_idx > 0) history_idx--;
    strncpy(cmd_buf, history[history_idx % HISTORY_SIZE], CMD_BUF_SIZE - 1);
    cmd_len = cursor_pos = (int)strlen(cmd_buf);
    prompt_redraw();
}

static void history_down(void)
{
    if (history_idx == -1) return;
    history_idx++;
    if (history_idx >= history_count) {
        history_idx = -1; cmd_len = cursor_pos = 0;
        memset(cmd_buf, 0, CMD_BUF_SIZE);
    } else {
        strncpy(cmd_buf, history[history_idx % HISTORY_SIZE], CMD_BUF_SIZE - 1);
        cmd_len = cursor_pos = (int)strlen(cmd_buf);
    }
    prompt_redraw();
}

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
            out[cwdlen++] = '/'; out[cwdlen] = '\0';
        }
        strncpy(out + strlen(out), path, VFS_PATH_MAX - strlen(out) - 1);
    }
}

static void tab_complete(void)
{
    static const char *cmds[] = {
        "help","clear","echo","calc","color","env","export","unset",
        "uptime","time","mem","sysinfo","dmesg","top","ps","kill","spawn",
        "history","pwd","cd","ls","cat","write","mkdir","find","grep",
        "diskls","diskcat","diskwrite","diskdel","disksync","chmod",
        "touch","rm","mv","cp","wc","head","tail","netinfo",
        "edit","shoot","about","reboot","halt", NULL
    };
    cmd_buf[cmd_len] = '\0';
    const char *match = NULL;
    int matches = 0;
    usize plen = strlen(cmd_buf);
    for (int i = 0; cmds[i]; i++) {
        if (!strncmp(cmds[i], cmd_buf, plen)) { match = cmds[i]; matches++; }
    }
    if (matches == 1) {
        strncpy(cmd_buf, match, CMD_BUF_SIZE - 1);
        cmd_len = cursor_pos = (int)strlen(cmd_buf);
        if (cmd_len < CMD_BUF_SIZE - 1) { cmd_buf[cmd_len++] = ' '; cursor_pos = cmd_len; }
        prompt_redraw();
    } else if (matches > 1) {
        shell_newline();
        for (int i = 0; cmds[i]; i++) {
            if (!strncmp(cmds[i], cmd_buf, plen)) {
                shell_puts(cmds[i], VGA_COLOR_LIGHT_GREY);
                shell_newline();
            }
        }
        prompt_redraw();
    }
}

static void cmd_help(void)
{
    shell_puts("baSic_ built-in commands:", VGA_COLOR_YELLOW);
    shell_newline();
    shell_puts("  system   : help clear sysinfo about uptime time mem top dmesg", VGA_COLOR_LIGHT_GREY);
    shell_puts("  process  : ps kill spawn", VGA_COLOR_LIGHT_GREY);
    shell_puts("  env      : env export unset", VGA_COLOR_LIGHT_GREY);
    shell_puts("  math     : calc", VGA_COLOR_LIGHT_GREY);
    shell_puts("  display  : color", VGA_COLOR_LIGHT_GREY);
    shell_puts("  nav      : pwd cd ls", VGA_COLOR_LIGHT_GREY);
    shell_puts("  files    : cat write mkdir find grep edit touch rm mv cp wc head tail", VGA_COLOR_LIGHT_GREY);
    shell_puts("  net      : netinfo", VGA_COLOR_LIGHT_GREY);
    shell_puts("  script   : run", VGA_COLOR_LIGHT_GREY);
    shell_puts("  disk     : diskls diskcat diskwrite diskdel disksync chmod", VGA_COLOR_LIGHT_GREY);
    shell_puts("  fun      : shoot", VGA_COLOR_LIGHT_CYAN);
    shell_puts("  power    : reboot halt poweroff", VGA_COLOR_LIGHT_GREY);
    shell_newline();
    shell_puts("  keys: Tab=complete  Up/Down=history  Left/Right=cursor", VGA_COLOR_DARK_GREY);
}

static void cmd_clear(void)
{
    for (int r = TERM_TOP; r <= TERM_BOT; r++)
        fill_row(r, ' ', VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    shell_row = TERM_TOP;
}

static void cmd_echo(const char *t) { if (t&&*t) shell_puts(t, VGA_COLOR_WHITE); shell_newline(); }

static void cmd_calc(const char *expr)
{
    if (!expr||!*expr) { shell_puts("usage: calc <expr>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    i32 result;
    if (!calc_eval(expr, &result)) { shell_puts("error: invalid expression", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    char buf[32]; int i=0;
    buf[i++]='='; buf[i++]=' ';
    i32 v=result;
    if (v<0){buf[i++]='-';v=-v;}
    char tmp[12]; int ti=10; tmp[11]='\0';
    if(!v){tmp[ti--]='0';}else{while(v){tmp[ti--]='0'+v%10;v/=10;}}
    const char *tp=&tmp[ti+1]; while(*tp) buf[i++]=*tp++;
    buf[i]='\0';
    shell_puts(buf, VGA_COLOR_LIGHT_GREEN);
    shell_newline();
}

static void cmd_color(const char *arg)
{
    /* color changes status bar accent only — terminal stays black */
    shell_puts("color: terminal stays black for readability.", VGA_COLOR_LIGHT_GREY);
    shell_newline();
    (void)arg;
}

static void cmd_env(void)  { env_dump(); shell_newline(); }

static void cmd_export(const char *arg)
{
    if (!arg||!*arg) { shell_puts("usage: export KEY=value", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    char key[ENV_KEY_MAX]; int i=0;
    while (arg[i]&&arg[i]!='='&&i<ENV_KEY_MAX-1){key[i]=arg[i];i++;}
    key[i]='\0';
    if (arg[i]!='=') { shell_puts("error: missing '='", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    if (env_set(key,arg+i+1)) shell_puts("OK", VGA_COLOR_LIGHT_GREEN);
    else shell_puts("error: table full", VGA_COLOR_LIGHT_RED);
    shell_newline();
}

static void cmd_unset(const char *key)
{
    if (!key||!*key) { shell_puts("usage: unset KEY", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    env_unset(key);
    shell_puts("OK", VGA_COLOR_LIGHT_GREEN);
    shell_newline();
}

static void cmd_time(void)
{
    rtc_time_t t; rtc_read(&t);
    int hour = (int)t.hour + TZ_OFFSET_H;
    if (hour >= 24) hour -= 24;
    char buf[64]; int i=0;
    const char *p="time  : "; while(*p) buf[i++]=*p++;
    buf[i++]='0'+hour/10;      buf[i++]='0'+hour%10;      buf[i++]=':';
    buf[i++]='0'+t.minute/10;  buf[i++]='0'+t.minute%10;  buf[i++]=':';
    buf[i++]='0'+t.second/10;  buf[i++]='0'+t.second%10;
    buf[i++]=' '; buf[i++]='B'; buf[i++]='D'; buf[i++]='T';
    buf[i++]=' '; buf[i++]='('; buf[i++]='U'; buf[i++]='T';
    buf[i++]='C'; buf[i++]='+'; buf[i++]='6'; buf[i++]=')';
    buf[i++]=' '; buf[i++]=' ';
    p="2026-"; while(*p) buf[i++]=*p++;
    buf[i++]='0'+t.month/10; buf[i++]='0'+t.month%10; buf[i++]='-';
    buf[i++]='0'+t.day/10;   buf[i++]='0'+t.day%10;
    buf[i]='\0';
    shell_puts(buf, VGA_COLOR_LIGHT_CYAN);
    shell_newline();
}

static void cmd_uptime(void)
{
    u32 s=timer_ticks()/1000;
    u8 h=(u8)(s/3600),m=(u8)((s%3600)/60),sec=(u8)(s%60);
    char buf[32]; int i=0;
    const char *p="uptime: "; while(*p) buf[i++]=*p++;
    buf[i++]='0'+h/10; buf[i++]='0'+h%10; buf[i++]='h'; buf[i++]=' ';
    buf[i++]='0'+m/10; buf[i++]='0'+m%10; buf[i++]='m'; buf[i++]=' ';
    buf[i++]='0'+sec/10;buf[i++]='0'+sec%10;buf[i++]='s';buf[i]='\0';
    shell_puts(buf, VGA_COLOR_LIGHT_CYAN);
    shell_newline();
}

static void cmd_mem(void)
{
    u32 total=pmm_total_frames()*4,free=pmm_free_frames()*4,used=total-free;
    char buf[48];
    #define ML(lbl,val,col) do { \
        int _i=0; const char *_p=lbl; while(*_p) buf[_i++]=*_p++; \
        u32 _v=val; char _t[12]; int _ti=10; _t[11]='\0'; \
        if(!_v){_t[_ti--]='0';}else{while(_v){_t[_ti--]='0'+_v%10;_v/=10;}} \
        _p=&_t[_ti+1]; while(*_p) buf[_i++]=*_p++; \
        _p=" KB"; while(*_p) buf[_i++]=*_p++; buf[_i]='\0'; \
        shell_puts(buf,col); shell_newline(); \
    } while(0)
    ML("total  : ",total,VGA_COLOR_WHITE);
    ML("used   : ",used, VGA_COLOR_LIGHT_RED);
    ML("free   : ",free, VGA_COLOR_LIGHT_GREEN);
    #undef ML
}

static void cmd_sysinfo(void)
{
    shell_puts("baSic_ v1.0", VGA_COLOR_LIGHT_CYAN);        shell_newline();
    shell_puts("  arch    : x86 32-bit protected mode", VGA_COLOR_WHITE); shell_newline();
    shell_puts("  kernel  : baSic_ original", VGA_COLOR_WHITE); shell_newline();
    shell_puts("  license : GPL v2", VGA_COLOR_WHITE); shell_newline();
    shell_puts("  tz      : Asia/Dhaka (BDT UTC+6)", VGA_COLOR_WHITE); shell_newline();
    cmd_uptime();
}

static void cmd_dmesg(void) { klog_dump(); shell_newline(); }

static void cmd_top(void)
{
    shell_puts("PROCESSES:", VGA_COLOR_YELLOW); shell_newline();
    shell_puts("  PID  STATE    NAME",        VGA_COLOR_LIGHT_GREY); shell_newline();
    shell_puts("  1    running  shell",        VGA_COLOR_WHITE); shell_newline();
    shell_newline();
    shell_puts("KERNEL PROFILER:", VGA_COLOR_YELLOW); shell_newline();
    prof_dump();
    shell_newline();
    shell_puts("MEMORY:", VGA_COLOR_YELLOW); shell_newline();
    cmd_mem();
    if (watchdog_fired())
        { shell_puts("WATCHDOG: TIMEOUT!", VGA_COLOR_LIGHT_RED); shell_newline(); }
    else
        { shell_puts("watchdog: OK", VGA_COLOR_LIGHT_GREEN); shell_newline(); }
}

static void cmd_ps(void)
{
    shell_puts("  PID  STATE    NAME", VGA_COLOR_LIGHT_GREY); shell_newline();
    shell_puts("  1    running  shell", VGA_COLOR_WHITE); shell_newline();
}

static void cmd_kill(u32 pid, int sig)
{
    signal_send(pid, sig);
    shell_puts("signal sent.", VGA_COLOR_LIGHT_GREEN);
    shell_newline();
}

static void cmd_spawn(const char *path)
{
    if (!path||!*path) { shell_puts("usage: spawn <file>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    u8 *elf_buf = (u8 *)0x400000;
    int n = fat12_read(path, elf_buf, 0x100000);
    if (n <= 0) {
        char full[VFS_PATH_MAX]; make_full(full, path);
        vfs_node_t *node = vfs_resolve(full);
        if (!node||!(node->flags&VFS_FILE)) {
            shell_puts("spawn: file not found", VGA_COLOR_LIGHT_RED); shell_newline(); return;
        }
        n = (int)vfs_read(node, 0, 0x100000, elf_buf);
    }
    if (n<=0) { shell_puts("spawn: empty file", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    u32 entry = elf_load(elf_buf, (u32)n);
    if (!entry) { shell_puts("spawn: not a valid ELF", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    process_t *proc = proc_create(path, entry);
    if (!proc) { shell_puts("spawn: process table full", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    proc->state = PROC_READY;
    shell_puts("process queued.", VGA_COLOR_LIGHT_GREEN); shell_newline();
}

static void cmd_history(void)
{
    if (!history_count) { shell_puts("(no history)", VGA_COLOR_LIGHT_GREY); shell_newline(); return; }
    int start=(history_count>HISTORY_SIZE)?history_count-HISTORY_SIZE:0;
    for (int i=start; i<history_count; i++) {
        char line[CMD_BUF_SIZE+6]; int j=0;
        u32 idx=(u32)(i+1); char tmp[8]; int ti=6; tmp[7]='\0';
        if(!idx){tmp[ti--]='0';}else{while(idx){tmp[ti--]='0'+idx%10;idx/=10;}}
        const char *tp=&tmp[ti+1]; while(*tp) line[j++]=*tp++;
        line[j++]=' '; line[j++]=' ';
        const char *h=history[i%HISTORY_SIZE]; while(*h&&j<CMD_BUF_SIZE+4) line[j++]=*h++;
        line[j]='\0';
        shell_puts(line, VGA_COLOR_LIGHT_GREY); shell_newline();
    }
}

static void cmd_pwd(void)  { shell_puts(cwd, VGA_COLOR_WHITE); shell_newline(); }

static void cmd_cd(const char *path)
{
    if (!path||!*path) { strncpy(cwd,"/",VFS_PATH_MAX); draw_status(); return; }
    char full[VFS_PATH_MAX]; make_full(full,path);
    vfs_node_t *node=vfs_resolve(full);
    if (!node||!(node->flags&VFS_DIR)) {
        shell_puts("cd: no such directory", VGA_COLOR_LIGHT_RED); shell_newline(); return;
    }
    strncpy(cwd,full,VFS_PATH_MAX-1); cwd[VFS_PATH_MAX-1]='\0';
    draw_status();
}

static void cmd_ls(void)
{
    vfs_node_t *dir=vfs_resolve(cwd);
    if (!dir||!(dir->flags&VFS_DIR)) { shell_puts("ls: not a directory", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    typedef struct { u8 *buf; u32 capacity; vfs_node_t *children[32]; u32 child_count; } rdata_t;
    rdata_t *rd=(rdata_t*)dir->inode;
    if (!rd||!rd->child_count) { shell_puts("(empty)", VGA_COLOR_LIGHT_GREY); shell_newline(); return; }
    /* print inline like ls */
    int col = 0;
    for (u32 i=0; i<rd->child_count; i++) {
        vfs_node_t *child=rd->children[i];
        u8 fg=(child->flags&VFS_DIR)?VGA_COLOR_LIGHT_CYAN:VGA_COLOR_WHITE;
        const char *n=child->name;
        usize nlen=strlen(n);
        if (col + (int)nlen + 3 >= VGA_W) { shell_newline(); col=0; }
        str_at(col, shell_row, n, fg, VGA_COLOR_BLACK);
        col += (int)nlen;
        if (child->flags&VFS_DIR) put(col++, shell_row, '/', VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        put(col++, shell_row, ' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
        put(col++, shell_row, ' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
    }
    shell_newline();
}

static void cmd_cat(const char *path)
{
    if (!path||!*path) { shell_puts("usage: cat <path>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    char full[VFS_PATH_MAX]; make_full(full,path);
    vfs_node_t *node=vfs_resolve(full);
    if (!node||!(node->flags&VFS_FILE)) { shell_puts("cat: no such file", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    u8 buf[128]; u32 off=0,n;
    while ((n=vfs_read(node,off,sizeof(buf)-1,buf))>0) {
        buf[n]='\0'; shell_puts((char*)buf,VGA_COLOR_WHITE); off+=n;
    }
    shell_newline();
}

static void cmd_write(const char *fname, const char *data)
{
    if (!fname||!*fname||!data) { shell_puts("usage: write <n> <text>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    vfs_node_t *parent=vfs_resolve(cwd);
    if (!parent) { shell_puts("write: bad cwd", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    vfs_node_t *node=vfs_finddir(parent,fname);
    if (!node) node=ramfs_mkfile(parent,fname);
    if (!node) { shell_puts("write: failed", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    vfs_write(node,0,strlen(data),(u8*)data);
    shell_puts("OK", VGA_COLOR_LIGHT_GREEN); shell_newline();
}

static void cmd_mkdir(const char *name)
{
    if (!name||!*name) { shell_puts("usage: mkdir <n>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    vfs_node_t *parent=vfs_resolve(cwd);
    if (!parent) { shell_puts("mkdir: bad cwd", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    if (!ramfs_mkdir(parent,name)) { shell_puts("mkdir: failed", VGA_COLOR_LIGHT_RED); shell_newline(); }
    else { shell_puts("OK", VGA_COLOR_LIGHT_GREEN); shell_newline(); }
}

static void cmd_touch(const char *name)
{
    if (!name||!*name) { shell_puts("usage: touch <file>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    vfs_node_t *parent = vfs_resolve(cwd);
    if (!parent) { shell_puts("touch: bad cwd", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    vfs_node_t *node = vfs_finddir(parent, name);
    if (node) { shell_puts("touch: already exists", VGA_COLOR_LIGHT_GREY); shell_newline(); return; }
    node = ramfs_mkfile(parent, name);
    if (!node) { shell_puts("touch: failed", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    shell_puts("OK", VGA_COLOR_LIGHT_GREEN); shell_newline();
}

static void cmd_rm(const char *name)
{
    if (!name||!*name) { shell_puts("usage: rm <file>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    vfs_node_t *parent = vfs_resolve(cwd);
    if (!parent) { shell_puts("rm: bad cwd", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    typedef struct { u8 *buf; u32 cap; vfs_node_t *ch[32]; u32 cnt; } rd_t;
    rd_t *rd = (rd_t *)parent->inode;
    if (!rd) { shell_puts("rm: bad dir", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    for (u32 i = 0; i < rd->cnt; i++) {
        if (!strcmp(rd->ch[i]->name, name)) {
            if (rd->ch[i]->flags & VFS_DIR) {
                shell_puts("rm: is a directory", VGA_COLOR_LIGHT_RED); shell_newline(); return;
            }
            /* ~~..shift children down */
            for (u32 j = i; j < rd->cnt - 1; j++)
                rd->ch[j] = rd->ch[j + 1];
            rd->ch[rd->cnt - 1] = NULL;
            rd->cnt--;
            shell_puts("OK", VGA_COLOR_LIGHT_GREEN); shell_newline();
            return;
        }
    }
    shell_puts("rm: not found", VGA_COLOR_LIGHT_RED); shell_newline();
}

static void cmd_mv(const char *src, const char *dst)
{
    if (!src||!*src||!dst||!*dst) 
    { 
        shell_puts("usage: mv <src> <dst>", VGA_COLOR_LIGHT_RED); 
        shell_newline(); return; 
    }
    vfs_node_t *parent = vfs_resolve(cwd);
    if (!parent) { shell_puts("mv: bad cwd", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    vfs_node_t *node = vfs_finddir(parent, src);
    if (!node) { shell_puts("mv: src not found", VGA_COLOR_LIGHT_RED); shell_newline(); return; }

    /* check if dst is an existing directory if yes.. move into it */
    vfs_node_t *dstdir = vfs_finddir(parent, dst);
    if (dstdir && (dstdir->flags & VFS_DIR)) {
        /* remove from current parent */
        typedef struct { u8 *buf; u32 cap; vfs_node_t *ch[32]; u32 cnt; } rd_t;
        rd_t *rd = (rd_t *)parent->inode;
        for (u32 i = 0; i < rd->cnt; i++) {
            if (rd->ch[i] == node) {
                for (u32 j = i; j < rd->cnt - 1; j++) rd->ch[j] = rd->ch[j+1];
                rd->ch[rd->cnt-1] = NULL; rd->cnt--; break;
            }
        }
        /* add to dst directory */
        rd_t *drd = (rd_t *)dstdir->inode;
        if (drd->cnt < 32) { drd->ch[drd->cnt++] = node; node->parent = dstdir; }
        else { shell_puts("mv: dst dir full", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    } else {
        /* just rename */
        strncpy(node->name, dst, VFS_NAME_MAX - 1);
        node->name[VFS_NAME_MAX - 1] = '\0';
    }
    shell_puts("OK", VGA_COLOR_LIGHT_GREEN); shell_newline();
}

static void cmd_cp(const char *src, const char *dst)
{
    if (!src||!*src||!dst||!*dst) 
    { 
        shell_puts("usage: cp <src> <dst>", VGA_COLOR_LIGHT_RED); 
        shell_newline(); return; 
    }
    vfs_node_t *parent = vfs_resolve(cwd);
    if (!parent) { shell_puts("cp: bad cwd", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    vfs_node_t *snode = vfs_finddir(parent, src);
    if (!snode||!(snode->flags&VFS_FILE)) 
    { 
        shell_puts("cp: src not found", VGA_COLOR_LIGHT_RED); 
        shell_newline(); return; 
    }

    /* check if dst is an existing directory.. if yes copy into it with same name */
    vfs_node_t *dstdir = vfs_finddir(parent, dst);
    vfs_node_t *dnode;
    if (dstdir && (dstdir->flags & VFS_DIR)) {
        dnode = vfs_finddir(dstdir, src);
        if (!dnode) dnode = ramfs_mkfile(dstdir, src);
    } else {
        dnode = vfs_finddir(parent, dst);
        if (!dnode) dnode = ramfs_mkfile(parent, dst);
    }
    if (!dnode) { shell_puts("cp: failed to create dst", VGA_COLOR_LIGHT_RED); shell_newline(); return; }

    u8 tmp[128]; u32 off = 0; u32 n;
    dnode->length = 0;
    while ((n = vfs_read(snode, off, sizeof(tmp), tmp)) > 0) {
        vfs_write(dnode, off, n, tmp);
        off += n;
    }
    shell_puts("OK", VGA_COLOR_LIGHT_GREEN); shell_newline();
}

static void find_recurse(vfs_node_t *dir, const char *name, const char *path, int *found)
{
    typedef struct { u8 *buf; u32 cap; vfs_node_t *ch[32]; u32 cnt; } rd_t;
    rd_t *rd=(rd_t*)dir->inode; if (!rd) return;
    for (u32 i=0; i<rd->cnt; i++) {
        vfs_node_t *child=rd->ch[i];
        char cpath[VFS_PATH_MAX]; strncpy(cpath,path,VFS_PATH_MAX-1); cpath[VFS_PATH_MAX-1]='\0';
        usize plen=strlen(cpath);
        if (plen<VFS_PATH_MAX-2&&cpath[plen-1]!='/'){cpath[plen++]='/';cpath[plen]='\0';}
        strncpy(cpath+plen,child->name,VFS_PATH_MAX-plen-1);
        if (!strcmp(child->name,name)){shell_puts(cpath,VGA_COLOR_LIGHT_GREEN);shell_newline();(*found)++;}
        if (child->flags&VFS_DIR) find_recurse(child,name,cpath,found);
    }
}

static void cmd_wc(const char *path)
{
    if (!path||!*path) { 
        shell_puts("usage: wc <file>", VGA_COLOR_LIGHT_RED); 
        shell_newline(); 
        return; 
    }
    char full[VFS_PATH_MAX]; make_full(full, path);
    vfs_node_t *node = vfs_resolve(full);
    if (!node||!(node->flags&VFS_FILE)) { shell_puts("wc: not found", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    u8 tmp[128]; u32 off=0,n;
    u32 lines=0, words=0, bytes=0;
    int in_word = 0;
    while ((n = vfs_read(node, off, sizeof(tmp), tmp)) > 0) {
        for (u32 i = 0; i < n; i++) {
            u8 c = tmp[i];
            bytes++;
            if (c == '\n') lines++;
            if (c == ' '||c == '\t'||c == '\n') in_word = 0;
            else if (!in_word) { in_word = 1; words++; }
        }
        off += n;
    }
    /* print the lines, words & bytes. */
    char buf[64]; int bi = 0;
    /* lines go here. */
    char tmp2[12]; int ti = 10; tmp2[11]='\0';
    u32 v = lines;
    if (!v){tmp2[ti--]='0';}else{while(v){tmp2[ti--]='0'+v%10;v/=10;}}
    const char *tp=&tmp2[ti+1]; while(*tp) buf[bi++]=*tp++;
    buf[bi++]=' ';
    /* word go here */
    ti=10; v=words;
    if (!v){tmp2[ti--]='0';}else{while(v){tmp2[ti--]='0'+v%10;v/=10;}}
    tp=&tmp2[ti+1]; while(*tp) buf[bi++]=*tp++;
    buf[bi++]=' ';
    /* and now bytes */
    ti=10; v=bytes;
    if (!v){tmp2[ti--]='0';}else{while(v){tmp2[ti--]='0'+v%10;v/=10;}}
    tp=&tmp2[ti+1]; while(*tp) buf[bi++]=*tp++;
    buf[bi++]=' '; const char *pp=path; while(*pp) buf[bi++]=*pp++;
    buf[bi]='\0';
    shell_puts(buf, VGA_COLOR_WHITE); shell_newline();
}

static void cmd_head(const char *path, int count)
{
    if (!path||!*path) { shell_puts("usage: head <file>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    char full[VFS_PATH_MAX]; make_full(full, path);
    vfs_node_t *node = vfs_resolve(full);
    if (!node||!(node->flags&VFS_FILE)) { shell_puts("head: not found", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    u8 tmp[128]; u32 off=0; u32 n;
    int lines = 0;
    char line[128]; int lpos = 0;
    while (lines < count && (n = vfs_read(node, off, sizeof(tmp), tmp)) > 0) {
        for (u32 i = 0; i < n && lines < count; i++) {
            char c = (char)tmp[i];
            if (c == '\n') {
                line[lpos] = '\0';
                shell_puts(line, VGA_COLOR_WHITE); shell_newline();
                lpos = 0; lines++;
            } else if (lpos < 127) line[lpos++] = c;
        }
        off += n;
    }
}

static void cmd_tail(const char *path, int count)
{
    if (!path||!*path) { shell_puts("usage: tail <file>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    char full[VFS_PATH_MAX]; make_full(full, path);
    vfs_node_t *node = vfs_resolve(full);
    if (!node||!(node->flags&VFS_FILE)) { shell_puts("tail: not found", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    /* what we are donig here is : just collect all the lines and take into a ring */
    char ring[32][128]; int rcount = 0;
    u8 tmp[128]; u32 off=0; u32 n;
    char line[128]; int lpos = 0;
    while ((n = vfs_read(node, off, sizeof(tmp), tmp)) > 0) {
        for (u32 i = 0; i < n; i++) {
            char c = (char)tmp[i];
            if (c == '\n') {
                line[lpos] = '\0';
                strncpy(ring[rcount % 32], line, 127);
                rcount++; lpos = 0;
            } else if (lpos < 127) line[lpos++] = c;
        }
        off += n;
    }
    int start = (rcount > count) ? rcount - count : 0;
    for (int i = start; i < rcount; i++) {
        shell_puts(ring[i % 32], VGA_COLOR_WHITE); shell_newline();
    }
}

static void cmd_find(const char *name)
{
    if (!name||!*name) { shell_puts("usage: find <n>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    vfs_node_t *root=vfs_root();
    if (!root) { shell_puts("find: no fs", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    int found=0; find_recurse(root,name,"/",&found);
    if (!found) { shell_puts("not found.", VGA_COLOR_LIGHT_GREY); shell_newline(); }
}

static void cmd_grep(const char *pattern, const char *path)
{
    if (!pattern||!*pattern||!path||!*path) { shell_puts("usage: grep <p> <path>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    char full[VFS_PATH_MAX]; make_full(full,path);
    vfs_node_t *node=vfs_resolve(full);
    if (!node||!(node->flags&VFS_FILE)) { shell_puts("grep: not found", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    u8 fbuf[128]; char line[128]; int lpos=0,found=0; u32 off=0;
    usize plen=strlen(pattern); u32 n;
    while ((n=vfs_read(node,off,sizeof(fbuf)-1,fbuf))>0) {
        for (u32 i=0; i<n; i++) {
            char c=(char)fbuf[i];
            if (c=='\n'||lpos>=126) {
                line[lpos]='\0'; usize llen=strlen(line);
                for (usize j=0; j+plen<=llen; j++) {
                    if (!strncmp(line+j,pattern,plen)){shell_puts(line,VGA_COLOR_LIGHT_GREEN);shell_newline();found++;break;}
                }
                lpos=0;
            } else line[lpos++]=c;
        }
        off+=n;
    }
    if (!found) { shell_puts("no matches.", VGA_COLOR_LIGHT_GREY); shell_newline(); }
}

static void cmd_diskls(void)
{
    fat12_dirent_t entries[32];
    int n=fat12_list("/",entries,32);
    if (!n) { shell_puts("disk: no FAT12 or empty", VGA_COLOR_LIGHT_GREY); shell_newline(); return; }
    for (int i=0; i<n; i++) {
        u8 fg=entries[i].is_dir?VGA_COLOR_LIGHT_CYAN:VGA_COLOR_WHITE;
        shell_puts(entries[i].name, fg);
        if (entries[i].is_dir) shell_puts("/", VGA_COLOR_LIGHT_CYAN);
        shell_puts("  ", VGA_COLOR_BLACK);
    }
    shell_newline();
}

static void cmd_diskcat(const char *path)
{
    if (!path||!*path) { shell_puts("usage: diskcat <file>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    u8 buf[2048]; int n=fat12_read(path,buf,sizeof(buf)-1);
    if (n<=0) { shell_puts("diskcat: not found", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    buf[n]='\0'; shell_puts((char*)buf,VGA_COLOR_WHITE); shell_newline();
}

static void cmd_diskwrite(const char *fname, const char *data)
{
    if (!fname||!*fname||!data) { shell_puts("usage: diskwrite <f> <text>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    int n=fat12_write(fname,(const u8*)data,strlen(data));
    if (n<0) { shell_puts("diskwrite: failed", VGA_COLOR_LIGHT_RED); }
    else { shell_puts("OK", VGA_COLOR_LIGHT_GREEN); }
    shell_newline();
}

static void cmd_diskdelete(const char *fname)
{
    if (!fname||!*fname) { shell_puts("usage: diskdel <file>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    if (fat12_delete(fname)) { shell_puts("OK", VGA_COLOR_LIGHT_GREEN); }
    else { shell_puts("diskdel: not found", VGA_COLOR_LIGHT_RED); }
    shell_newline();
}

static void cmd_disksync(void)
{
    disksync_flush();
    shell_puts("disk flushed.", VGA_COLOR_LIGHT_GREEN); shell_newline();
}

static void cmd_chmod(const char *path, const char *mode)
{
    if (!path||!mode) { shell_puts("usage: chmod <path> <rwx>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    u8 perms=0;
    for (int i=0; mode[i]; i++) {
        if (mode[i]=='r') perms|=PERM_READ;
        if (mode[i]=='w') perms|=PERM_WRITE;
        if (mode[i]=='x') perms|=PERM_EXEC;
    }
    filemeta_set(path,perms);
    shell_puts("OK", VGA_COLOR_LIGHT_GREEN); shell_newline();
}

static void cmd_edit(const char *path)
{
    if (!path||!*path) { shell_puts("usage: edit <path>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    char full[VFS_PATH_MAX]; make_full(full,path);
    editor_open(full);
    vga_clear();
    shell_row = TERM_TOP;
    draw_status();
    shell_puts("returned from editor.", VGA_COLOR_LIGHT_GREY); shell_newline();
    prompt_redraw();
}

static void cmd_shoot(void)
{
    shell_puts("launching shooter... press Q to quit", VGA_COLOR_LIGHT_CYAN); shell_newline();
    timer_sleep(500);
    shooter_run();
    vga_clear();
    shell_row = TERM_TOP;
    draw_status();
    shell_puts("returned from shooter.", VGA_COLOR_LIGHT_GREY); shell_newline();
    prompt_redraw();
}

static void cmd_about(void)
{
    shell_puts("baSic_ v1.0", VGA_COLOR_LIGHT_CYAN); shell_newline();
    shell_puts("  author  : Shahriar Dhrubo", VGA_COLOR_WHITE); shell_newline();
    shell_puts("  arch    : x86 32-bit protected mode", VGA_COLOR_WHITE); shell_newline();
    shell_puts("  license : GPL v2", VGA_COLOR_WHITE); shell_newline();
}

/* shell scripts. */
static void cmd_run(const char *path)
{
    if (!path||!*path) { shell_puts("usage: run <script>", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
    char full[VFS_PATH_MAX]; make_full(full, path);
    vfs_node_t *node = vfs_resolve(full);
    if (!node||!(node->flags&VFS_FILE)) {
        /* do try FAT12 disk */
        static u8 sbuf[1024];
        int n = fat12_read(path, sbuf, sizeof(sbuf)-1);
        if (n <= 0) { shell_puts("run: script not found", VGA_COLOR_LIGHT_RED); shell_newline(); return; }
        sbuf[n] = '\0';
        char *line = (char *)sbuf;
        while (*line) {
            char *end = line;
            while (*end && *end != '\n') end++;
            char saved = *end; *end = '\0';
            if (line[0] && line[0] != '#') shell_exec_line(line);
            *end = saved;
            line = (*end) ? end + 1 : end;
        }
        return;
    }
    u8 sbuf[1024]; u32 n = vfs_read(node, 0, sizeof(sbuf)-1, sbuf);
    sbuf[n] = '\0';
    char *line = (char *)sbuf;
    while (*line) {
        char *end = line;
        while (*end && *end != '\n') end++;
        char saved = *end; *end = '\0';
        if (line[0] && line[0] != '#') shell_exec_line(line);
        *end = saved;
        line = (*end) ? end + 1 : end;
    }
}

static void shell_exec_line(char *line)
{
    usize len = strlen(line);
    if (len >= CMD_BUF_SIZE) len = CMD_BUF_SIZE - 1;
    memcpy(cmd_buf, line, len);
    cmd_buf[len] = '\0';
    cmd_len = (int)len;
    dispatch();
    cmd_len = 0;
    memset(cmd_buf, 0, CMD_BUF_SIZE);
}

static void cmd_netinfo(void)
{
    u8 mac[6];
    e1000_get_mac(mac);
    char buf[48]; int i = 0;
    const char *p = "MAC: "; while (*p) buf[i++] = *p++;
    for (int j = 0; j < 6; j++) {
        u8 b = mac[j];
        buf[i++] = "0123456789abcdef"[b >> 4];
        buf[i++] = "0123456789abcdef"[b & 0xF];
        if (j < 5) buf[i++] = ':';
    }
    buf[i] = '\0';
    shell_puts(buf, VGA_COLOR_LIGHT_CYAN);
    shell_newline();
}

static void cmd_reboot(void)
{
    history_save();
    disksync_flush();
    shell_puts("rebooting...", VGA_COLOR_YELLOW); shell_newline();
    serial_print("baSic_: reboot\n");
    timer_sleep(500);
    __asm__ volatile ("mov $0xFE, %%al; outb %%al, $0x64" : : : "al");
    for (;;) __asm__ volatile ("hlt");
}

static void cmd_halt(void)
{
    history_save();
    disksync_flush();
    serial_print("baSic_: halt\n");
    shell_puts("halting...", VGA_COLOR_LIGHT_RED); shell_newline();
    __asm__ volatile ("cli; hlt");
}

/* finally powerOFF!. */
static void cmd_poweroff(void)
{
    history_save();
    disksync_flush();
    serial_print("baSic_: poweroff\n");
    shell_puts("powering off...", VGA_COLOR_LIGHT_RED);
    shell_newline();
    timer_sleep(500);

    /*
        for peace of my mind:
        mov ax, 0x2000
        mov dx, 0x604
        out dx, ax
    */
    __asm__ volatile ("outw %w0, %w1" : : "a"((u16)0x2000), "Nd"((u16)0x604));

    __asm__ volatile ("outw %w0, %w1" : : "a"((u16)0x0|0x2000), "Nd"((u16)0xB004));

    /* if still running just halt */
    __asm__ volatile ("cli; hlt");
}

static void cmd_alias(void)
{
    int found = 0;
    for (int i = 0; i < ALIAS_MAX; i++) {
        if (alias_table[i].used) {
            char line[ALIAS_NAME_MAX + ALIAS_CMD_MAX + 6];
            int j = 0;
            const char *n = alias_table[i].name;
            const char *c = alias_table[i].cmd;
            while (*n) line[j++] = *n++;
            line[j++] = ' '; line[j++] = '-'; line[j++] = '>'; line[j++] = ' ';
            while (*c) line[j++] = *c++;
            line[j] = '\0';
            shell_puts(line, VGA_COLOR_WHITE);
            shell_newline();
            found++;
        }
    }
    if (!found) { shell_puts("no aliases.", VGA_COLOR_LIGHT_GREY); shell_newline(); }
}

static void dispatch(void)
{
    cmd_buf[cmd_len]='\0';
    if (!cmd_len) return;
    draw_status();   
    watchdog_kick();

    if (!strcmp(cmd_buf,"help"))     { cmd_help();    return; }
    if (!strcmp(cmd_buf,"clear"))    { cmd_clear();   return; }
    if (!strcmp(cmd_buf,"uptime"))   { cmd_uptime();  return; }
    if (!strcmp(cmd_buf,"time"))     { cmd_time();    return; }
    if (!strcmp(cmd_buf,"mem"))      { cmd_mem();     return; }
    if (!strcmp(cmd_buf,"sysinfo"))  { cmd_sysinfo(); return; }
    if (!strcmp(cmd_buf,"dmesg"))    { cmd_dmesg();   return; }
    if (!strcmp(cmd_buf,"top"))      { cmd_top();     return; }
    if (!strcmp(cmd_buf,"ps"))       { cmd_ps();      return; }
    if (!strcmp(cmd_buf,"history"))  { cmd_history(); return; }
    if (!strcmp(cmd_buf,"pwd"))      { cmd_pwd();     return; }
    if (!strcmp(cmd_buf,"ls"))       { cmd_ls();      return; }
    if (!strcmp(cmd_buf,"env"))      { cmd_env();     return; }
    if (!strcmp(cmd_buf,"diskls"))   { cmd_diskls();  return; }
    if (!strcmp(cmd_buf,"disksync")) { cmd_disksync();return; }
    if (!strcmp(cmd_buf,"about"))    { cmd_about();   return; }
    if (!strcmp(cmd_buf,"shoot"))    { cmd_shoot();   return; }
    if (!strcmp(cmd_buf,"reboot"))   { cmd_reboot();  return; }
    if (!strcmp(cmd_buf,"halt"))     { cmd_halt();    return; }
    if (!strcmp(cmd_buf, "alias"))   { cmd_alias();   return; }
    if (!strcmp(cmd_buf, "netinfo")) { cmd_netinfo(); return; }

    if (!strcmp(cmd_buf, "poweroff")) { cmd_poweroff(); return; }

    if (!strncmp(cmd_buf,"cp ",   3)) {
        char *sp = cmd_buf+3; while(*sp&&*sp!=' ') sp++;
        if (*sp==' ')
        {
            *sp='\0';cmd_cp(cmd_buf+3,sp+1);
        }
        else {
            shell_puts("usage: cp <src> <dst>", VGA_COLOR_LIGHT_RED); 
            shell_newline(); 
        }
    return;
    }

    if (!strncmp(cmd_buf,"mv ",   3)) {
        char *sp = cmd_buf+3; 
        while(*sp&&*sp!=' ') sp++;
        if (*sp==' ')
            {
                *sp='\0';cmd_mv(cmd_buf+3,sp+1);
            }
        else { 
            shell_puts("usage: mv <src> <dst>", VGA_COLOR_LIGHT_RED); shell_newline(); 
        }
        return;
    }
    /* from now will use sizeof */
    if (!strncmp(cmd_buf,"wc ",   3)) { cmd_wc(cmd_buf+3);             return; }
    if (!strncmp(cmd_buf,"head ", 5)) { cmd_head(cmd_buf+5, 10);       return; }
    if (!strncmp(cmd_buf,"tail ", 5)) { cmd_tail(cmd_buf+5, 10);       return; }

    if (!strncmp(cmd_buf,"run ",    4)) { cmd_run(cmd_buf+4);     return; }
    if (!strncmp(cmd_buf,"touch ",  6)) { cmd_touch(cmd_buf+6);   return; }
    if (!strncmp(cmd_buf,"rm ",     3)) { cmd_rm(cmd_buf+3);      return; }
    if (!strncmp(cmd_buf,"echo ",   5)) { cmd_echo(cmd_buf+5);    return; }
    if (!strncmp(cmd_buf,"calc ",   5)) { cmd_calc(cmd_buf+5);    return; }
    if (!strncmp(cmd_buf,"color ",  6)) { cmd_color(cmd_buf+6);   return; }
    if (!strncmp(cmd_buf,"cd ",     3)) { cmd_cd(cmd_buf+3);      return; }
    if (!strncmp(cmd_buf,"mkdir ",  6)) { cmd_mkdir(cmd_buf+6);   return; }
    if (!strncmp(cmd_buf,"cat ",    4)) { cmd_cat(cmd_buf+4);     return; }
    if (!strncmp(cmd_buf,"find ",   5)) { cmd_find(cmd_buf+5);    return; }
    if (!strncmp(cmd_buf,"export ", 7)) { cmd_export(cmd_buf+7);  return; }
    if (!strncmp(cmd_buf,"unset ",  6)) { cmd_unset(cmd_buf+6);   return; }
    if (!strncmp(cmd_buf,"edit ",   5)) { cmd_edit(cmd_buf+5);    return; }
    if (!strncmp(cmd_buf,"diskcat ",8)) { cmd_diskcat(cmd_buf+8); return; }
    if (!strncmp(cmd_buf,"diskdel ",8)) { cmd_diskdelete(cmd_buf+8); return; }
    if (!strncmp(cmd_buf,"spawn ",  6)) { cmd_spawn(cmd_buf+6);   return; }

    if (!strncmp(cmd_buf,"kill ",4)) {
        u32 pid=0; int sig=SIGTERM;
        char *p=cmd_buf+5;
        while(*p>='0'&&*p<='9'){pid=pid*10+(*p-'0');p++;}
        if (*p==' '){p++;sig=0;while(*p>='0'&&*p<='9'){sig=sig*10+(*p-'0');p++;}}
        cmd_kill(pid,sig); return;
    }
    if (!strncmp(cmd_buf,"grep ",4)) {
        char *sp=cmd_buf+5; while(*sp&&*sp!=' ') sp++;
        if (*sp==' '){*sp='\0';cmd_grep(cmd_buf+5,sp+1);}
        else { shell_puts("usage: grep <p> <path>", VGA_COLOR_LIGHT_RED); shell_newline(); }
        return;
    }
    if (!strncmp(cmd_buf,"write ",6)) {
        char *sp=cmd_buf+6; while(*sp&&*sp!=' ') sp++;
        if (*sp==' '){*sp='\0';cmd_write(cmd_buf+6,sp+1);}
        else { shell_puts("usage: write <n> <text>", VGA_COLOR_LIGHT_RED); shell_newline(); }
        return;
    }
    if (!strncmp(cmd_buf,"diskwrite ",10)) {
        char *sp=cmd_buf+10; while(*sp&&*sp!=' ') sp++;
        if (*sp==' '){*sp='\0';cmd_diskwrite(cmd_buf+10,sp+1);}
        else { shell_puts("usage: diskwrite <f> <text>", VGA_COLOR_LIGHT_RED); shell_newline(); }
        return;
    }
    if (!strncmp(cmd_buf,"chmod ",6)) {
        char *sp=cmd_buf+6; while(*sp&&*sp!=' ') sp++;
        if (*sp==' '){*sp='\0';cmd_chmod(cmd_buf+6,sp+1);}
        else { shell_puts("usage: chmod <path> <rwx>", VGA_COLOR_LIGHT_RED); shell_newline(); }
        return;
    }

    /* unknown */
    shell_puts(cmd_buf, VGA_COLOR_LIGHT_RED);
    shell_puts(": command not found", VGA_COLOR_LIGHT_RED);
    shell_newline();
}

void shell_init(void)
{
    shell_splash();
    vga_clear();

    cmd_len=0; cursor_pos=0;
    history_count=0; history_idx=-1;
    memset(cmd_buf,0,CMD_BUF_SIZE);
    memset(history,0,sizeof(history));
    shell_row = TERM_TOP;

    draw_status();
    history_load();

    /* apply boot path from INIT.CFG */
    const char *bp = disksync_boot_path();
    if (bp && bp[0] == '/') {
        vfs_node_t *bpnode = vfs_resolve(bp);
        if (bpnode && (bpnode->flags & VFS_DIR)) {
            strncpy(cwd, bp, VFS_PATH_MAX - 1);
            draw_status();
        }
    }

    /* motd */
    vfs_node_t *etc = vfs_finddir(vfs_root(), "etc");
    if (etc) {
        vfs_node_t *motd = vfs_finddir(etc, "motd");
        if (motd) {
            u8 buf[128]; u32 n = vfs_read(motd, 0, sizeof(buf)-1, buf);
            if (n > 0) { buf[n]='\0'; shell_puts((char*)buf, VGA_COLOR_LIGHT_GREY); shell_newline(); }
        }
    }

    shell_puts("baSic_ v1.0  |  type 'help' for commands", VGA_COLOR_DARK_GREY);
    shell_newline();
    prompt_redraw();
}

// static void draw_status_time(void)
// {
//     static u8 last_sec = 0xFF;

//     rtc_time_t t;
//     rtc_read(&t);

//     if (t.second == last_sec)
//         return;
//     last_sec = t.second;
//     wait_vretrace();

//     int hour = (int)t.hour + TZ_OFFSET_H;
//     if (hour >= 24) hour -= 24;

//     char tbuf[32]; int i = 0;
//     tbuf[i++] = '0' + hour/10;     tbuf[i++] = '0' + hour%10;     tbuf[i++] = ':';
//     tbuf[i++] = '0' + t.minute/10; tbuf[i++] = '0' + t.minute%10; tbuf[i++] = ':';
//     tbuf[i++] = '0' + t.second/10; tbuf[i++] = '0' + t.second%10;
//     tbuf[i++] = ' '; tbuf[i++] = 'B'; tbuf[i++] = 'D'; tbuf[i] = '\0';
//     str_at(55, STATUS_ROW, tbuf, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_DARK_GREY);

//     u32 s = timer_ticks() / 1000;
//     u8 uh=(u8)(s/3600), um=(u8)((s%3600)/60), us=(u8)(s%60);
//     char ubuf[16]; int j = 0;
//     ubuf[j++]='|'; ubuf[j++]=' ';
//     ubuf[j++]='0'+uh/10; ubuf[j++]='0'+uh%10; ubuf[j++]=':';
//     ubuf[j++]='0'+um/10; ubuf[j++]='0'+um%10; ubuf[j++]=':';
//     ubuf[j++]='0'+us/10; ubuf[j++]='0'+us%10; ubuf[j]='\0';
//     str_at(68, STATUS_ROW, ubuf, VGA_COLOR_YELLOW, VGA_COLOR_DARK_GREY);
// }

void shell_run(void)
{
    u32 last_s = (u32)-1;

    for (;;) {
        u32 now_s = timer_ticks() / 1000;
        if (now_s != last_s) {
            draw_status();
            last_s = now_s;
            watchdog_kick();
        }
        term_event_t ev = term_poll();
        if (ev.type == TERM_NONE) {
            char c = keyboard_getchar();
            if (!c) { __asm__ volatile ("hlt"); continue; }
            ev.type = TERM_CHAR; ev.ch = c;
        }

        switch (ev.type) {
        case TERM_UP:    history_up();   break;
        case TERM_DOWN:  history_down(); break;
        case TERM_LEFT:
            if (cursor_pos > 0) { cursor_pos--; prompt_redraw(); }
            break;
        case TERM_RIGHT:
            if (cursor_pos < cmd_len) { cursor_pos++; prompt_redraw(); }
            break;
        case TERM_HOME:  cursor_pos=0;       prompt_redraw(); break;
        case TERM_END:   cursor_pos=cmd_len; prompt_redraw(); break;
        case TERM_DEL:
            if (cursor_pos < cmd_len) {
                for (int i=cursor_pos; i<cmd_len-1; i++) cmd_buf[i]=cmd_buf[i+1];
                cmd_len--; cmd_buf[cmd_len]='\0'; prompt_redraw();
            }
            break;
        case TERM_CHAR:
            switch (ev.ch) {
            case '\n':
                echo_input();
                shell_newline();
                history_push();
                dispatch();
                cmd_len=0; cursor_pos=0;
                memset(cmd_buf,0,CMD_BUF_SIZE);
                prompt_redraw();
                break;
            case '\b':
                if (cursor_pos > 0) {
                    for (int i=cursor_pos-1; i<cmd_len-1; i++) cmd_buf[i]=cmd_buf[i+1];
                    cmd_len--; cursor_pos--;
                    cmd_buf[cmd_len]='\0'; prompt_redraw();
                }
                break;
            case '\t':
                tab_complete();
                break;
            default:
                if (ev.ch == ('u'-96)) {
                    cmd_len=0; cursor_pos=0;
                    memset(cmd_buf,0,CMD_BUF_SIZE); prompt_redraw();
                } else if (ev.ch >= 32 && ev.ch < 127 && cmd_len < CMD_BUF_SIZE-1) {
                    for (int i=cmd_len; i>cursor_pos; i--) cmd_buf[i]=cmd_buf[i-1];
                    cmd_buf[cursor_pos++]=ev.ch;
                    cmd_len++;
                    prompt_redraw();
                }
                break;
            }
            break;
        default: break;
        }
    }
}
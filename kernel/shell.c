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
#include "env.h"
#include "klog.h"
#include "pipe.h"
#include "disk.h"
#include "fat12.h"
#include "filemeta.h"
#include "disksync.h"
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
static u8   shell_fg = VGA_COLOR_WHITE;
static u8   shell_bg = VGA_COLOR_BLACK;

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
    vga_str_at(36, 14, "v1.0",               VGA_COLOR_WHITE,      VGA_COLOR_BLACK);
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
    vga_write_at(col++, PROMPT_ROW, '>',
        shell_bg == VGA_COLOR_BLACK ? VGA_COLOR_LIGHT_GREEN : VGA_COLOR_DARK_GREY,
        shell_bg);
    vga_write_at(col++, PROMPT_ROW, ' ', shell_fg, shell_bg);
    for (int i = 0; i < cmd_len && col < VGA_W - 1; i++)
        vga_write_at(col++, PROMPT_ROW, cmd_buf[i], shell_fg, shell_bg);
    if (col < VGA_W)
        vga_write_at(col, PROMPT_ROW, '_', VGA_COLOR_LIGHT_GREY, shell_bg);
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
    cmd_len = (int)strlen(cmd_buf);
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
    shell_puts("  calc <expr>       — calculator",              VGA_COLOR_LIGHT_GREY);
    shell_puts("  color <0-7>       — shell color",             VGA_COLOR_LIGHT_GREY);
    shell_puts("  env               — environment vars",        VGA_COLOR_LIGHT_GREY);
    shell_puts("  export KEY=val    — set env var",             VGA_COLOR_LIGHT_GREY);
    shell_puts("  unset KEY         — remove env var",          VGA_COLOR_LIGHT_GREY);
    shell_puts("  uptime / time     — time info",               VGA_COLOR_LIGHT_GREY);
    shell_puts("  mem               — memory usage",            VGA_COLOR_LIGHT_GREY);
    shell_puts("  sysinfo           — system info",             VGA_COLOR_LIGHT_GREY);
    shell_puts("  dmesg             — kernel log",              VGA_COLOR_LIGHT_GREY);
    shell_puts("  ps                — processes",               VGA_COLOR_LIGHT_GREY);
    shell_puts("  history           — command history",         VGA_COLOR_LIGHT_GREY);
    shell_puts("  pwd / cd <dir>    — navigate",                VGA_COLOR_LIGHT_GREY);
    shell_puts("  ls / cat / mkdir  — filesystem",              VGA_COLOR_LIGHT_GREY);
    shell_puts("  write <f> <text>  — write file",              VGA_COLOR_LIGHT_GREY);
    shell_puts("  find <n>          — recursive search",        VGA_COLOR_LIGHT_GREY);
    shell_puts("  grep <p> <path>   — search file",             VGA_COLOR_LIGHT_GREY);
    shell_puts("  diskls            — list disk (FAT12)",       VGA_COLOR_LIGHT_GREY);
    shell_puts("  diskcat <file>    — read file from disk",     VGA_COLOR_LIGHT_GREY);
    shell_puts("  diskwrite <f> <t> — write file to disk",      VGA_COLOR_LIGHT_GREY);
    shell_puts("  diskdel <file>    — delete file from disk",   VGA_COLOR_LIGHT_GREY);
    shell_puts("  disksync          — flush disk cache",        VGA_COLOR_LIGHT_GREY);
    shell_puts("  chmod <p> <rwx>   — set permissions",         VGA_COLOR_LIGHT_GREY);
    shell_puts("  edit <file>       — text editor",             VGA_COLOR_LIGHT_CYAN);
    shell_puts("  shoot             — shooter game",            VGA_COLOR_LIGHT_CYAN);
    shell_puts("  about             — about baSic_",            VGA_COLOR_LIGHT_GREY);
    shell_puts("  reboot / halt     — power",                   VGA_COLOR_LIGHT_GREY);
    shell_puts("  Ctrl-P: history   Ctrl-U: clear line",        VGA_COLOR_DARK_GREY);
}

static void cmd_clear(void)
{
    for (int r = SHELL_TOP; r < PROMPT_ROW; r++)
        vga_clear_row(r, shell_fg, shell_bg);
    shell_row = SHELL_TOP;
}

static void cmd_echo(const char *text)  { shell_puts(text ? text : "", shell_fg); }

static void cmd_calc(const char *expr)
{
    if (!expr || !*expr) { shell_puts("usage: calc <expr>", VGA_COLOR_LIGHT_RED); return; }
    i32 result;
    if (!calc_eval(expr, &result)) { shell_puts("calc: invalid", VGA_COLOR_LIGHT_RED); return; }
    char buf[32]; int i = 0;
    buf[i++]='='; buf[i++]=' ';
    i32 v = result;
    if (v < 0) { buf[i++]='-'; v=-v; }
    char tmp[12]; int ti=10; tmp[11]='\0';
    if(!v){tmp[ti--]='0';}else{while(v){tmp[ti--]='0'+v%10;v/=10;}}
    const char *tp=&tmp[ti+1]; while(*tp) buf[i++]=*tp++;
    buf[i]='\0';
    shell_puts(buf, VGA_COLOR_LIGHT_GREEN);
}

static void cmd_color(const char *arg)
{
    if (!arg || *arg < '0' || *arg > '7') {
        shell_puts("usage: color <0-7>  0=black 1=blue 2=green 3=cyan 4=red 5=mag 6=brown 7=grey",
                   VGA_COLOR_LIGHT_RED);
        return;
    }
    shell_bg = (u8)(*arg - '0');
    shell_fg = (shell_bg == VGA_COLOR_BLACK) ? VGA_COLOR_WHITE : VGA_COLOR_BLACK;
    cmd_clear();
    shell_puts("color changed.", shell_fg);
}

static void cmd_env(void)   { env_dump(); }

static void cmd_export(const char *arg)
{
    if (!arg || !*arg) { shell_puts("usage: export KEY=value", VGA_COLOR_LIGHT_RED); return; }
    char key[ENV_KEY_MAX]; int i = 0;
    while (arg[i] && arg[i] != '=' && i < ENV_KEY_MAX - 1) { key[i] = arg[i]; i++; }
    key[i] = '\0';
    if (arg[i] != '=') { shell_puts("export: missing '='", VGA_COLOR_LIGHT_RED); return; }
    if (env_set(key, arg + i + 1)) shell_puts("set.", VGA_COLOR_LIGHT_GREEN);
    else shell_puts("export: table full", VGA_COLOR_LIGHT_RED);
}

static void cmd_unset(const char *key)
{
    if (!key || !*key) { shell_puts("usage: unset KEY", VGA_COLOR_LIGHT_RED); return; }
    env_unset(key);
    shell_puts("unset.", VGA_COLOR_LIGHT_GREEN);
}

static void cmd_time(void)
{
    rtc_time_t t; rtc_read(&t);
    char buf[48]; int i=0;
    const char *p="time: "; while(*p) buf[i++]=*p++;
    buf[i++]='0'+t.hour/10;   buf[i++]='0'+t.hour%10;   buf[i++]=':';
    buf[i++]='0'+t.minute/10; buf[i++]='0'+t.minute%10; buf[i++]=':';
    buf[i++]='0'+t.second/10; buf[i++]='0'+t.second%10;
    buf[i++]=' '; buf[i++]=' ';
    p="date: 20"; while(*p) buf[i++]=*p++;
    buf[i++]='0'+t.year/10;  buf[i++]='0'+t.year%10;  buf[i++]='-';
    buf[i++]='0'+t.month/10; buf[i++]='0'+t.month%10; buf[i++]='-';
    buf[i++]='0'+t.day/10;   buf[i++]='0'+t.day%10;   buf[i]='\0';
    shell_puts(buf, VGA_COLOR_LIGHT_CYAN);
}

static void cmd_uptime(void)
{
    u32 s=timer_ticks()/1000;
    u8 h=(u8)(s/3600),m=(u8)((s%3600)/60),sec=(u8)(s%60);
    char buf[32]; int i=0;
    const char *p="uptime: "; while(*p) buf[i++]=*p++;
    buf[i++]='0'+h/10; buf[i++]='0'+h%10; buf[i++]='h'; buf[i++]=' ';
    buf[i++]='0'+m/10; buf[i++]='0'+m%10; buf[i++]='m'; buf[i++]=' ';
    buf[i++]='0'+sec/10; buf[i++]='0'+sec%10; buf[i++]='s'; buf[i]='\0';
    shell_puts(buf, VGA_COLOR_LIGHT_CYAN);
}

static void cmd_mem(void)
{
    u32 total=pmm_total_frames()*4, free=pmm_free_frames()*4, used=total-free;
    char buf[48];
    #define ML(lbl,val,col) do { \
        int _i=0; const char *_p=lbl; while(*_p) buf[_i++]=*_p++; \
        u32 _v=val; char _t[12]; int _ti=10; _t[11]='\0'; \
        if(!_v){_t[_ti--]='0';}else{while(_v){_t[_ti--]='0'+_v%10;_v/=10;}} \
        _p=&_t[_ti+1]; while(*_p) buf[_i++]=*_p++; \
        _p=" KB"; while(*_p) buf[_i++]=*_p++; buf[_i]='\0'; \
        shell_puts(buf,col); \
    } while(0)
    ML("  total : ", total, VGA_COLOR_LIGHT_GREY);
    ML("  used  : ", used,  VGA_COLOR_LIGHT_GREY);
    ML("  free  : ", free,  VGA_COLOR_LIGHT_GREEN);
    #undef ML
}

static void cmd_sysinfo(void)
{
    shell_puts("  +---------------------------------+", VGA_COLOR_LIGHT_CYAN);
    shell_puts("  |         baSic_ v1.0            |", VGA_COLOR_LIGHT_CYAN);
    shell_puts("  +---------------------------------+", VGA_COLOR_LIGHT_CYAN);
    shell_puts("  author  : Shahriar Dhrubo",          VGA_COLOR_WHITE);
    shell_puts("  arch    : x86 32-bit protected mode", VGA_COLOR_WHITE);
    shell_puts("  kernel  : baSic_ (original)",        VGA_COLOR_WHITE);
    shell_puts("  license : GPL v2",                   VGA_COLOR_WHITE);
    cmd_uptime();
}

static void cmd_dmesg(void)    { klog_dump(); }

static void cmd_history(void)
{
    if (!history_count) { shell_puts("no history", VGA_COLOR_LIGHT_GREY); return; }
    int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
    for (int i = start; i < history_count; i++) {
        char line[CMD_BUF_SIZE + 6]; int j=0;
        u32 idx=(u32)(i+1); char tmp[8]; int ti=6; tmp[7]='\0';
        if(!idx){tmp[ti--]='0';}else{while(idx){tmp[ti--]='0'+idx%10;idx/=10;}}
        const char *tp=&tmp[ti+1]; while(*tp) line[j++]=*tp++;
        line[j++]=' '; line[j++]=' ';
        const char *h=history[i%HISTORY_SIZE]; while(*h) line[j++]=*h++;
        line[j]='\0';
        shell_puts(line, VGA_COLOR_LIGHT_GREY);
    }
}

static void cmd_ps(void)
{
    shell_puts("  PID  STATE    NAME", VGA_COLOR_YELLOW);
    shell_puts("  1    running  shell", VGA_COLOR_LIGHT_GREY);
}

static void cmd_pwd(void) { shell_puts(cwd, VGA_COLOR_WHITE); }

static void cmd_cd(const char *path)
{
    if (!path || !*path) { strncpy(cwd, "/", VFS_PATH_MAX); return; }
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
    if (!rd || !rd->child_count) { shell_puts("(empty)", VGA_COLOR_LIGHT_GREY); return; }
    for (u32 i = 0; i < rd->child_count; i++) {
        vfs_node_t *child = rd->children[i];
        char line[VFS_NAME_MAX + 8]; int j=0;
        if (child->flags & VFS_DIR) { line[j++]='[';line[j++]='d';line[j++]=']';line[j++]=' '; }
        else                        { line[j++]='[';line[j++]='f';line[j++]=']';line[j++]=' '; }
        const char *n=child->name; while(*n) line[j++]=*n++;
        line[j]='\0';
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
    u8 buf[128]; u32 off=0, n;
    while ((n=vfs_read(node,off,sizeof(buf)-1,buf))>0) {
        buf[n]='\0'; shell_puts((char*)buf, shell_fg); off+=n;
    }
}

static void cmd_write(const char *fname, const char *data)
{
    if (!fname||!*fname||!data) { shell_puts("usage: write <n> <text>", VGA_COLOR_LIGHT_RED); return; }
    vfs_node_t *parent=vfs_resolve(cwd);
    if (!parent) { shell_puts("write: bad cwd", VGA_COLOR_LIGHT_RED); return; }
    vfs_node_t *node=vfs_finddir(parent,fname);
    if (!node) node=ramfs_mkfile(parent,fname);
    if (!node) { shell_puts("write: failed", VGA_COLOR_LIGHT_RED); return; }
    vfs_write(node,0,strlen(data),(u8*)data);
    shell_puts("written.", VGA_COLOR_LIGHT_GREEN);
}

static void cmd_mkdir(const char *name)
{
    if (!name||!*name) { shell_puts("usage: mkdir <n>", VGA_COLOR_LIGHT_RED); return; }
    vfs_node_t *parent=vfs_resolve(cwd);
    if (!parent) { shell_puts("mkdir: bad cwd", VGA_COLOR_LIGHT_RED); return; }
    if (!ramfs_mkdir(parent,name)) shell_puts("mkdir: failed", VGA_COLOR_LIGHT_RED);
    else shell_puts("directory created.", VGA_COLOR_LIGHT_GREEN);
}

static void find_recurse(vfs_node_t *dir, const char *name,
                         const char *path, int *found)
{
    typedef struct {
        u8 *buf; u32 cap;
        vfs_node_t *ch[32];
        u32 cnt;
    } rd_t;
    rd_t *rd = (rd_t *)dir->inode;
    if (!rd) return;
    for (u32 i = 0; i < rd->cnt; i++) {
        vfs_node_t *child = rd->ch[i];
        char cpath[VFS_PATH_MAX];
        strncpy(cpath, path, VFS_PATH_MAX - 1);
        cpath[VFS_PATH_MAX - 1] = '\0';
        usize plen = strlen(cpath);
        if (plen < VFS_PATH_MAX - 2 && cpath[plen-1] != '/') {
            cpath[plen++]='/'; cpath[plen]='\0';
        }
        strncpy(cpath+plen, child->name, VFS_PATH_MAX-plen-1);
        if (!strcmp(child->name, name)) {
            shell_puts(cpath, VGA_COLOR_LIGHT_GREEN);
            (*found)++;
        }
        if (child->flags & VFS_DIR)
            find_recurse(child, name, cpath, found);
    }
}

static void cmd_find(const char *name)
{
    if (!name||!*name) { shell_puts("usage: find <n>", VGA_COLOR_LIGHT_RED); return; }
    vfs_node_t *root = vfs_root();
    if (!root) { shell_puts("find: no filesystem", VGA_COLOR_LIGHT_RED); return; }
    int found = 0;
    find_recurse(root, name, "/", &found);
    if (!found) shell_puts("not found.", VGA_COLOR_LIGHT_GREY);
}

static void cmd_grep(const char *pattern, const char *path)
{
    if (!pattern||!*pattern||!path||!*path) {
        shell_puts("usage: grep <pattern> <path>", VGA_COLOR_LIGHT_RED);
        return;
    }
    char full[VFS_PATH_MAX];
    make_full(full, path);
    vfs_node_t *node = vfs_resolve(full);
    if (!node||!(node->flags & VFS_FILE)) {
        shell_puts("grep: file not found", VGA_COLOR_LIGHT_RED);
        return;
    }
    u8   fbuf[128];
    char line[128];
    int  lpos=0, found=0;
    u32  off=0;
    usize plen = strlen(pattern);
    u32 n;
    while ((n=vfs_read(node,off,sizeof(fbuf)-1,fbuf))>0) {
        for (u32 i=0; i<n; i++) {
            char c=(char)fbuf[i];
            if (c=='\n'||lpos>=126) {
                line[lpos]='\0';
                usize llen=strlen(line);
                for (usize j=0; j+plen<=llen; j++) {
                    if (!strncmp(line+j,pattern,plen)) {
                        shell_puts(line, VGA_COLOR_LIGHT_GREEN);
                        found++; break;
                    }
                }
                lpos=0;
            } else {
                line[lpos++]=c;
            }
        }
        off+=n;
    }
    if (!found) shell_puts("no matches.", VGA_COLOR_LIGHT_GREY);
}

static void cmd_diskls(void)
{
    fat12_dirent_t entries[32];
    int n = fat12_list("/", entries, 32);
    if (n == 0) { shell_puts("disk: no FAT12 or empty", VGA_COLOR_LIGHT_GREY); return; }
    for (int i = 0; i < n; i++) {
        char line[FAT12_NAME_MAX + 8]; int j=0;
        if (entries[i].is_dir) { line[j++]='[';line[j++]='d';line[j++]=']';line[j++]=' '; }
        else                   { line[j++]='[';line[j++]='f';line[j++]=']';line[j++]=' '; }
        const char *nm=entries[i].name; while(*nm) line[j++]=*nm++;
        line[j]='\0';
        shell_puts(line, entries[i].is_dir ? VGA_COLOR_LIGHT_CYAN : VGA_COLOR_WHITE);
    }
}

static void cmd_diskcat(const char *path)
{
    if (!path||!*path) { shell_puts("usage: diskcat <file>", VGA_COLOR_LIGHT_RED); return; }
    u8 buf[2048];
    int n = fat12_read(path, buf, sizeof(buf)-1);
    if (n<=0) { shell_puts("diskcat: not found", VGA_COLOR_LIGHT_RED); return; }
    buf[n]='\0';
    shell_puts((char*)buf, shell_fg);
}

static void cmd_diskwrite(const char *fname, const char *data)
{
    if (!fname||!*fname||!data) { shell_puts("usage: diskwrite <f> <text>", VGA_COLOR_LIGHT_RED); return; }
    int n = fat12_write(fname, (const u8*)data, strlen(data));
    if (n<0) shell_puts("diskwrite: failed", VGA_COLOR_LIGHT_RED);
    else shell_puts("written to disk.", VGA_COLOR_LIGHT_GREEN);
}

static void cmd_diskdelete(const char *fname)
{
    if (!fname||!*fname) { shell_puts("usage: diskdel <file>", VGA_COLOR_LIGHT_RED); return; }
    if (fat12_delete(fname)) shell_puts("deleted from disk.", VGA_COLOR_LIGHT_GREEN);
    else shell_puts("diskdel: not found", VGA_COLOR_LIGHT_RED);
}

static void cmd_disksync(void)
{
    disksync_flush();
    shell_puts("disk flushed.", VGA_COLOR_LIGHT_GREEN);
}

static void cmd_chmod(const char *path, const char *mode)
{
    if (!path||!mode) { shell_puts("usage: chmod <path> <rwx>", VGA_COLOR_LIGHT_RED); return; }
    u8 perms=0;
    for (int i=0; mode[i]; i++) {
        if (mode[i]=='r') perms|=PERM_READ;
        if (mode[i]=='w') perms|=PERM_WRITE;
        if (mode[i]=='x') perms|=PERM_EXEC;
    }
    filemeta_set(path, perms);
    shell_puts("permissions set.", VGA_COLOR_LIGHT_GREEN);
}

static void cmd_edit(const char *path)
{
    if (!path||!*path) { shell_puts("usage: edit <path>", VGA_COLOR_LIGHT_RED); return; }
    char full[VFS_PATH_MAX];
    make_full(full, path);
    editor_open(full);
    vga_clear(); draw_header();
    for (int r=SHELL_TOP; r<PROMPT_ROW; r++) vga_clear_row(r, shell_fg, shell_bg);
    shell_row=SHELL_TOP;
    shell_puts("returned from editor.", VGA_COLOR_LIGHT_GREY);
    prompt_redraw();
}

static void cmd_shoot(void)
{
    shell_puts("launching shooter... (Q to quit)", VGA_COLOR_LIGHT_CYAN);
    timer_sleep(600);
    shooter_run();
    vga_clear(); draw_header();
    for (int r=SHELL_TOP; r<PROMPT_ROW; r++) vga_clear_row(r, shell_fg, shell_bg);
    shell_row=SHELL_TOP;
    shell_puts("returned from shooter.", VGA_COLOR_LIGHT_GREY);
    prompt_redraw();
}

static void cmd_about(void)
{
    shell_puts("baSic_ v1.0", VGA_COLOR_WHITE);
    shell_puts("author : Shahriar Dhrubo", VGA_COLOR_LIGHT_GREY);
    shell_puts("arch   : x86 32-bit protected mode", VGA_COLOR_LIGHT_GREY);
    shell_puts("license: GPL v2", VGA_COLOR_LIGHT_GREY);
}

static void cmd_reboot(void)
{
    disksync_flush();
    shell_puts("rebooting...", VGA_COLOR_YELLOW);
    serial_print("baSic_: reboot\n");
    timer_sleep(500);
    __asm__ volatile ("mov $0xFE, %%al; outb %%al, $0x64" : : : "al");
    for (;;) __asm__ volatile ("hlt");
}

static void cmd_halt(void)
{
    disksync_flush();
    serial_print("baSic_: halt\n");
    shell_puts("halting...", VGA_COLOR_LIGHT_RED);
    __asm__ volatile ("cli; hlt");
}

static void dispatch(void)
{
    cmd_buf[cmd_len]='\0';
    if (!cmd_len) return;

    if (!strcmp(cmd_buf,"help"))    { cmd_help();    return; }
    if (!strcmp(cmd_buf,"clear"))   { cmd_clear();   return; }
    if (!strcmp(cmd_buf,"uptime"))  { cmd_uptime();  return; }
    if (!strcmp(cmd_buf,"time"))    { cmd_time();    return; }
    if (!strcmp(cmd_buf,"mem"))     { cmd_mem();     return; }
    if (!strcmp(cmd_buf,"sysinfo")) { cmd_sysinfo(); return; }
    if (!strcmp(cmd_buf,"dmesg"))   { cmd_dmesg();   return; }
    if (!strcmp(cmd_buf,"ps"))      { cmd_ps();      return; }
    if (!strcmp(cmd_buf,"history")) { cmd_history(); return; }
    if (!strcmp(cmd_buf,"pwd"))     { cmd_pwd();     return; }
    if (!strcmp(cmd_buf,"ls"))      { cmd_ls();      return; }
    if (!strcmp(cmd_buf,"env"))     { cmd_env();     return; }
    if (!strcmp(cmd_buf,"diskls"))  { cmd_diskls();  return; }
    if (!strcmp(cmd_buf,"disksync")){ cmd_disksync();return; }
    if (!strcmp(cmd_buf,"about"))   { cmd_about();   return; }
    if (!strcmp(cmd_buf,"shoot"))   { cmd_shoot();   return; }
    if (!strcmp(cmd_buf,"reboot"))  { cmd_reboot();  return; }
    if (!strcmp(cmd_buf,"halt"))    { cmd_halt();    return; }

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

    if (!strncmp(cmd_buf,"grep ",4)) {
        char *sp=cmd_buf+5;
        while (*sp&&*sp!=' ') sp++;
        if (*sp==' ') { *sp='\0'; cmd_grep(cmd_buf+5,sp+1); }
        else shell_puts("usage: grep <pattern> <path>", VGA_COLOR_LIGHT_RED);
        return;
    }
    if (!strncmp(cmd_buf,"write ",6)) {
        char *sp=cmd_buf+6;
        while (*sp&&*sp!=' ') sp++;
        if (*sp==' ') { *sp='\0'; cmd_write(cmd_buf+6,sp+1); }
        else shell_puts("usage: write <n> <text>", VGA_COLOR_LIGHT_RED);
        return;
    }
    if (!strncmp(cmd_buf,"diskwrite ",10)) {
        char *sp=cmd_buf+10;
        while (*sp&&*sp!=' ') sp++;
        if (*sp==' ') { *sp='\0'; cmd_diskwrite(cmd_buf+10,sp+1); }
        else shell_puts("usage: diskwrite <f> <text>", VGA_COLOR_LIGHT_RED);
        return;
    }
    if (!strncmp(cmd_buf,"chmod ",6)) {
        char *sp=cmd_buf+6;
        while (*sp&&*sp!=' ') sp++;
        if (*sp==' ') { *sp='\0'; cmd_chmod(cmd_buf+6,sp+1); }
        else shell_puts("usage: chmod <path> <rwx>", VGA_COLOR_LIGHT_RED);
        return;
    }

    char msg[CMD_BUF_SIZE+12]; int i=0;
    const char *pre="unknown: "; while(*pre) msg[i++]=*pre++;
    for (int j=0; j<cmd_len; j++) msg[i++]=cmd_buf[j];
    msg[i]='\0';
    shell_puts(msg, VGA_COLOR_LIGHT_RED);
}

void shell_init(void)
{
    shell_splash();
    draw_header();
    cmd_len=0; history_count=0; history_idx=-1;
    shell_fg=VGA_COLOR_WHITE; shell_bg=VGA_COLOR_BLACK;
    memset(cmd_buf,0,CMD_BUF_SIZE);
    memset(history,0,sizeof(history));
    shell_puts("baSic_ shell ready. type 'help' for commands.", VGA_COLOR_LIGHT_GREY);
    prompt_redraw();
}

void shell_run(void)
{
    u32 last_s=(u32)-1;
    for (;;) {
        u32 now_s=timer_ticks()/1000;
        if (now_s!=last_s) { update_clock(); last_s=now_s; }
        char c=keyboard_getchar();
        if (!c) { __asm__ volatile ("hlt"); continue; }
        if (c=='\n') {
            history_push(); dispatch();
            cmd_len=0; memset(cmd_buf,0,CMD_BUF_SIZE); prompt_redraw();
        } else if (c=='\b') {
            if (cmd_len>0) { cmd_len--; prompt_redraw(); }
        } else if (c=='u'-96) {
            cmd_len=0; memset(cmd_buf,0,CMD_BUF_SIZE); prompt_redraw();
        } else if (c=='p'-96) {
            history_up();
        } else if (cmd_len<CMD_BUF_SIZE-1) {
            cmd_buf[cmd_len++]=c; prompt_redraw();
        }
    }
}
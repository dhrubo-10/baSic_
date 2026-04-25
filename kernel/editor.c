/* baSic_ - kernel/editor.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * baSic_ text editor
 *
 * controls:
 *   arrows / WASD   — move cursor
 *   Enter           — new line
 *   Backspace       — delete char left
 *   Ctrl-S          — save
 *   Ctrl-Q          — quit
 *   Ctrl-K          — delete current line
 *   Ctrl-E          — jump to end of line
 *   Ctrl-A          — jump to start of line
 *
 * layout:
 *   row 0           — title bar (filename, modified flag, keybinds)
 *   rows 1-22       — text area
 *   row 23          — status bar (cursor position, mode)
 *   row 24          — message bar (save confirm, errors)
 */

#include "editor.h"
#include "vga.h"
#include "keyboard.h"
#include "timer.h"
#include "../fs/vfs.h"
#include "../fs/ramfs.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"
#include "../include/types.h"


#define ROW_TITLE   0
#define ROW_TEXT_S  1
#define ROW_TEXT_E  22
#define ROW_STATUS  23
#define ROW_MSG     24

#define VGA_BASE  ((volatile u16 *)0xB8000)

static inline void ed_put(int col, int row, char c, u8 fg, u8 bg)
{
    if (col < 0 || col >= VGA_W || row < 0 || row >= VGA_H) return;
    VGA_BASE[row * VGA_W + col] = (u16)(((bg << 4 | fg) << 8) | (u8)c);
}

static void ed_fill_row(int row, char c, u8 fg, u8 bg)
{
    for (int i = 0; i < VGA_W; i++)
        ed_put(i, row, c, fg, bg);
}

static void ed_str(int col, int row, const char *s, u8 fg, u8 bg)
{
    while (*s && col < VGA_W)
        ed_put(col++, row, *s++, fg, bg);
}

/* write a small decimal at col,row */
static void ed_num(int col, int row, u32 v, u8 fg, u8 bg)
{
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (v == 0) { ed_put(col, row, '0', fg, bg); return; }
    while (v > 0) { buf[i--] = '0' + v % 10; v /= 10; }
    ed_str(col, row, &buf[i + 1], fg, bg);
}


static char  buf[ED_MAX_LINES][ED_LINE_MAX + 1];
static int   line_len[ED_MAX_LINES];
static int   num_lines;
static int   cx, cy;          /* cursor col, line in buffer */
static int   scroll_y;        /* first visible line         */
static int   modified;
static char  filename[128];
static char  msg[80];
static u32   msg_time;

static void buf_init(void)
{
    memset(buf, 0, sizeof(buf));
    memset(line_len, 0, sizeof(line_len));
    num_lines = 1;
    cx = cy = scroll_y = modified = 0;
    msg[0] = '\0';
    msg_time = 0;
}

static void buf_insert_char(char c)
{
    if (cy >= ED_MAX_LINES) return;
    int len = line_len[cy];
    if (len >= ED_LINE_MAX) return;

    /* shift right from cursor */
    for (int i = len; i > cx; i--)
        buf[cy][i] = buf[cy][i - 1];
    buf[cy][cx] = c;
    line_len[cy]++;
    buf[cy][line_len[cy]] = '\0';
    cx++;
    modified = 1;
}

static void buf_delete_char(void)
{
    if (cx == 0) {
        /* merge with previous line */
        if (cy == 0) return;
        int prev_len = line_len[cy - 1];
        int cur_len  = line_len[cy];
        if (prev_len + cur_len <= ED_LINE_MAX) {
            memcpy(buf[cy - 1] + prev_len, buf[cy], cur_len);
            line_len[cy - 1] += cur_len;
            buf[cy - 1][line_len[cy - 1]] = '\0';
            /* shift lines up */
            for (int i = cy; i < num_lines - 1; i++) {
                memcpy(buf[i], buf[i + 1], ED_LINE_MAX + 1);
                line_len[i] = line_len[i + 1];
            }
            num_lines--;
            cy--;
            cx = prev_len;
        }
        modified = 1;
        return;
    }
    /* shift left from cursor */
    for (int i = cx - 1; i < line_len[cy] - 1; i++)
        buf[cy][i] = buf[cy][i + 1];
    line_len[cy]--;
    buf[cy][line_len[cy]] = '\0';
    cx--;
    modified = 1;
}

static void buf_newline(void)
{
    if (num_lines >= ED_MAX_LINES) return;

    /* shift lines down */
    for (int i = num_lines; i > cy + 1; i--) {
        memcpy(buf[i], buf[i - 1], ED_LINE_MAX + 1);
        line_len[i] = line_len[i - 1];
    }
    num_lines++;

    /* split current line at cx */
    int rest = line_len[cy] - cx;
    memcpy(buf[cy + 1], buf[cy] + cx, rest);
    buf[cy + 1][rest] = '\0';
    line_len[cy + 1]  = rest;

    buf[cy][cx] = '\0';
    line_len[cy] = cx;

    cy++;
    cx = 0;
    modified = 1;
}

static void buf_delete_line(void)
{
    if (num_lines == 1) {
        memset(buf[0], 0, ED_LINE_MAX + 1);
        line_len[0] = 0;
        cx = 0;
        modified = 1;
        return;
    }
    for (int i = cy; i < num_lines - 1; i++) {
        memcpy(buf[i], buf[i + 1], ED_LINE_MAX + 1);
        line_len[i] = line_len[i + 1];
    }
    num_lines--;
    if (cy >= num_lines) cy = num_lines - 1;
    if (cx > line_len[cy]) cx = line_len[cy];
    modified = 1;
}


static void load_file(const char *path)
{
    vfs_node_t *node = vfs_resolve(path);
    if (!node || !(node->flags & VFS_FILE)) return;

    u8  tmp[128];
    u32 off   = 0;
    int line  = 0;
    int col   = 0;
    u32 n;

    while ((n = vfs_read(node, off, sizeof(tmp), tmp)) > 0 && line < ED_MAX_LINES) {
        for (u32 i = 0; i < n; i++) {
            char c = (char)tmp[i];
            if (c == '\n') {
                buf[line][col] = '\0';
                line_len[line] = col;
                line++;
                col = 0;
                if (line >= ED_MAX_LINES) break;
            } else if (col < ED_LINE_MAX) {
                buf[line][col++] = c;
            }
        }
        off += n;
    }
    if (line_len[line] == 0 && line < ED_MAX_LINES)
        num_lines = (line == 0) ? 1 : line;
    else
        num_lines = line + 1;

    if (num_lines < 1) num_lines = 1;
}

static void save_file(void)
{
    vfs_node_t *node = vfs_resolve(filename);
    if (!node) {
        /* find parent dir and basename */
        char parent_path[128];
        strncpy(parent_path, filename, 127);
        parent_path[127] = '\0';

        /* split at last '/' */
        char *slash = parent_path;
        char *last  = slash;
        while (*slash) { if (*slash == '/') last = slash; slash++; }

        const char *base;
        vfs_node_t *parent;
        if (last == parent_path) {
            /* file directly under root */
            parent = vfs_root();
            base   = filename + 1;
        } else {
            *last  = '\0';
            parent = vfs_resolve(parent_path);
            base   = last + 1;
        }

        if (!parent) {
            strncpy(msg, "save failed: no parent dir", 79);
            msg_time = timer_ticks();
            return;
        }
        node = ramfs_mkfile(parent, base);
        if (!node) {
            strncpy(msg, "save failed: could not create file", 79);
            msg_time = timer_ticks();
            return;
        }
    }

    node->length = 0;

    u32 off = 0;
    for (int i = 0; i < num_lines; i++) {
        u32 n = vfs_write(node, off, (u32)line_len[i], (u8 *)buf[i]);
        off += n;
        u8 nl = '\n';
        vfs_write(node, off, 1, &nl);
        off++;
    }

    modified = 0;
    strncpy(msg, "file saved.", 79);
    msg_time = timer_ticks();
}


static void render_titlebar(void)
{
    ed_fill_row(ROW_TITLE, ' ', VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    ed_str(1, ROW_TITLE, "baSic_ editor", VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    ed_str(16, ROW_TITLE, "|", VGA_COLOR_DARK_GREY, VGA_COLOR_LIGHT_GREY);
    ed_str(18, ROW_TITLE, filename, VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    if (modified)
        ed_str(18 + (int)strlen(filename), ROW_TITLE,
               " [modified]", VGA_COLOR_RED, VGA_COLOR_LIGHT_GREY);
    ed_str(55, ROW_TITLE, "^S save  ^Q quit  ^K del line",
           VGA_COLOR_DARK_GREY, VGA_COLOR_LIGHT_GREY);
}

static void render_text(void)
{
    for (int r = 0; r < ED_ROWS; r++) {
        int line = scroll_y + r;
        int vrow = ROW_TEXT_S + r;

        /* clear row */
        for (int c = 0; c < VGA_W; c++)
            ed_put(c, vrow, ' ', VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        if (line >= num_lines) continue;

        /* line number gutter */
        u32 lnum = (u32)(line + 1);
        char lbuf[5];
        int li = 3;
        lbuf[4] = '\0';
        lbuf[0] = lbuf[1] = lbuf[2] = lbuf[3] = ' ';
        while (lnum > 0 && li >= 0) {
            lbuf[li--] = '0' + lnum % 10;
            lnum /= 10;
        }
        ed_str(0, vrow, lbuf, VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
        ed_put(4, vrow, ' ', VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);

        /* text */
        int len = line_len[line];
        for (int c = 0; c < len && c + 5 < VGA_W; c++) {
            char ch = buf[line][c];
            u8   fg = VGA_COLOR_WHITE;
            ed_put(c + 5, vrow, ch, fg, VGA_COLOR_BLACK);
        }

        /* highlight cursor line */
        if (line == cy) {
            for (int c = 0; c < VGA_W; c++) {
                u16 cell = VGA_BASE[vrow * VGA_W + c];
                u8  ch   = (u8)(cell & 0xFF);
                u8  attr = (u8)((cell >> 8) & 0xFF);
                u8  fg   = attr & 0x0F;
                ed_put(c, vrow, (char)ch, fg, VGA_COLOR_DARK_GREY);
            }
        }
    }
}

static void render_cursor(void)
{
    int vrow = ROW_TEXT_S + (cy - scroll_y);
    int vcol = cx + 5;
    if (vrow < ROW_TEXT_S || vrow > ROW_TEXT_E) return;
    if (vcol >= VGA_W) vcol = VGA_W - 1;
    /* draw cursor block */
    u16 cell = VGA_BASE[vrow * VGA_W + vcol];
    u8  ch   = (u8)(cell & 0xFF);
    ed_put(vcol, vrow, ch ? ch : ' ', VGA_COLOR_BLACK, VGA_COLOR_WHITE);
}

static void render_statusbar(void)
{
    ed_fill_row(ROW_STATUS, ' ', VGA_COLOR_BLACK, VGA_COLOR_CYAN);
    ed_str(1, ROW_STATUS, "Ln:", VGA_COLOR_BLACK, VGA_COLOR_CYAN);
    ed_num(4, ROW_STATUS, (u32)(cy + 1), VGA_COLOR_BLACK, VGA_COLOR_CYAN);
    ed_str(8, ROW_STATUS, "Col:", VGA_COLOR_BLACK, VGA_COLOR_CYAN);
    ed_num(12, ROW_STATUS, (u32)(cx + 1), VGA_COLOR_BLACK, VGA_COLOR_CYAN);
    ed_str(16, ROW_STATUS, "Lines:", VGA_COLOR_BLACK, VGA_COLOR_CYAN);
    ed_num(22, ROW_STATUS, (u32)num_lines, VGA_COLOR_BLACK, VGA_COLOR_CYAN);
}

static void render_msgbar(void)
{
    ed_fill_row(ROW_MSG, ' ', VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    if (msg[0] && timer_ticks() - msg_time < 3000)
        ed_str(1, ROW_MSG, msg, VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    else
        ed_str(1, ROW_MSG, "^S save  ^Q quit  ^K kill line  ^A home  ^E end",
               VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
}

static void render_all(void)
{
    render_titlebar();
    render_text();
    render_statusbar();
    render_msgbar();
    render_cursor();
}


static void adjust_scroll(void)
{
    if (cy < scroll_y)
        scroll_y = cy;
    if (cy >= scroll_y + ED_ROWS)
        scroll_y = cy - ED_ROWS + 1;
}


void editor_open(const char *path)
{
    buf_init();
    strncpy(filename, path, 127);
    filename[127] = '\0';
    load_file(path);

    /* clear screen */
    for (int r = 0; r < VGA_H; r++)
        for (int c = 0; c < VGA_W; c++)
            ed_put(c, r, ' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);

    render_all();

    for (;;) {
        char c = keyboard_getchar();
        if (!c) {
            __asm__ volatile ("hlt");
            continue;
        }

        /* Ctrl keys */
        if (c == 'q' - 96) break;                          /* Ctrl-Q: quit */

        if (c == 's' - 96) {                               /* Ctrl-S: save */
            save_file();
            render_all();
            continue;
        }

        if (c == 'k' - 96) { buf_delete_line(); goto redraw; } /* Ctrl-K */
        if (c == 'a' - 96) { cx = 0;            goto redraw; } /* Ctrl-A home */
        if (c == 'e' - 96) { cx = line_len[cy]; goto redraw; } /* Ctrl-E end  */

        if (c == '\n') { buf_newline(); goto redraw; }
        if (c == '\b') { buf_delete_char(); goto redraw; }

        /* printable ASCII */
        if (c >= 32 && c < 127) {
            buf_insert_char(c);
            goto redraw;
        }

        /* movement via WASD since we don't decode escape sequences yet */
        if (c == 'w' - 96 || c == 'W') {    /* up */
            if (cy > 0) {
                cy--;
                if (cx > line_len[cy]) cx = line_len[cy];
            }
            goto redraw;
        }
        if (c == 's' - 96) {                 /* down (Ctrl-S handled above) */
            if (cy < num_lines - 1) {
                cy++;
                if (cx > line_len[cy]) cx = line_len[cy];
            }
            goto redraw;
        }

        continue;

redraw:
        adjust_scroll();
        render_all();
    }

    /* if modified and not saved, prompt */
    if (modified) {
        ed_fill_row(ROW_MSG, ' ', VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        ed_str(1, ROW_MSG, "unsaved changes! save? (y/n)",
               VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        for (;;) {
            char c = keyboard_getchar();
            if (!c) { __asm__ volatile ("hlt"); continue; }
            if (c == 'y' || c == 'Y') { save_file(); break; }
            if (c == 'n' || c == 'N') break;
        }
    }
}
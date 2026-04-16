/* baSic_ - kernel/shooter.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * baSic_ game: SHOOTER 
 *
 * controls:  A/D or left/right arrows = move
 *            SPACE = fire
 *            Q = quit
 *
 * layout:
 *   row 0       HUD (score, lives, level)
 *   rows 1–22   play field
 *   row 23      player row
 *   row 24      bottom border / game over line
 */

#include "shooter.h"
#include "vga.h"
#include "keyboard.h"
#include "timer.h"
#include "../lib/string.h"
#include "../include/types.h"

#define FIELD_TOP    1
#define FIELD_BOT   22
#define PLAYER_ROW  23

#define MAX_BULLETS  8
#define MAX_ENEMIES  24
#define MAX_STARS    20

#define VGA_BASE  ((volatile u16 *)0xB8000)

static inline void put(int col, int row, char c, u8 fg, u8 bg)
{
    if (col < 0 || col >= VGA_W || row < 0 || row >= VGA_H) return;
    VGA_BASE[row * VGA_W + col] = (u16)(((bg << 4 | fg) << 8) | (u8)c);
}

static void clear_field(void)
{
    for (int r = 0; r < VGA_H; r++)
        for (int c = 0; c < VGA_W; c++)
            put(c, r, ' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
}

static void draw_char_row(int col, int row, const char *s, u8 fg)
{
    while (*s) put(col++, row, *s++, fg, VGA_COLOR_BLACK);
}

/* write a small decimal number at col,row */
static void draw_num(int col, int row, u32 val, u8 fg)
{
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (val == 0) { put(col, row, '0', fg, VGA_COLOR_BLACK); return; }
    while (val > 0 && i > 0) { buf[i--] = '0' + val % 10; val /= 10; }
    draw_char_row(col, row, &buf[i + 1], fg);
}


typedef struct {
    int x, y;
    int active;
} bullet_t;

typedef struct {
    int x, y;
    int active;
    int hp;
    char ch;
    u8   color;
} enemy_t;

typedef struct {
    int x, y;
    char ch;
} star_t;

static int      player_x;
static int      player_lives;
static u32      score;
static u32      level;
static bullet_t bullets[MAX_BULLETS];
static enemy_t  enemies[MAX_ENEMIES];
static star_t   stars[MAX_STARS];
static int      game_over;
static u32      enemy_move_timer;
static u32      enemy_move_interval;  /* ticks between enemy steps */
static int      enemies_alive;

/* simple deterministic "random"  good enough for a game */
static u32 rng_state = 12345;
static u32 rng(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void init_stars(void)
{
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x  = (int)(rng() % VGA_W);
        stars[i].y  = FIELD_TOP + (int)(rng() % (FIELD_BOT - FIELD_TOP));
        stars[i].ch = (rng() % 3 == 0) ? '*' : '.';
    }
}

static void spawn_wave(void)
{
    int cols = 8;
    int rows = 3;
    int start_x = 4;
    int idx = 0;

    static const char enemy_chars[] = { 'V', 'W', 'M' };
    static const u8   enemy_colors[] = {
        VGA_COLOR_LIGHT_RED,
        VGA_COLOR_LIGHT_MAGENTA,
        VGA_COLOR_YELLOW
    };

    for (int r = 0; r < rows && idx < MAX_ENEMIES; r++) {
        for (int c = 0; c < cols && idx < MAX_ENEMIES; c++) {
            enemies[idx].x      = start_x + c * 9;
            enemies[idx].y      = FIELD_TOP + r * 3;
            enemies[idx].active = 1;
            enemies[idx].hp     = 1 + (int)(level / 3);
            enemies[idx].ch     = enemy_chars[r % 3];
            enemies[idx].color  = enemy_colors[r % 3];
            idx++;
        }
    }
    enemies_alive = idx;
    for (int i = idx; i < MAX_ENEMIES; i++) enemies[i].active = 0;

    enemy_move_interval = (level < 5) ? (30 - level * 4) : 10;
    if (enemy_move_interval < 6) enemy_move_interval = 6;
}

static void init_game(void)
{
    player_x     = VGA_W / 2;
    player_lives = 3;
    score        = 0;
    level        = 1;
    game_over    = 0;
    enemy_move_timer = 0;
    rng_state    = timer_ticks() + 1;

    memset(bullets, 0, sizeof(bullets));
    memset(enemies, 0, sizeof(enemies));

    init_stars();
    spawn_wave();
}

static void fire_bullet(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x      = player_x;
            bullets[i].y      = PLAYER_ROW - 1;
            bullets[i].active = 1;
            return;
        }
    }
}

static void draw_hud(void)
{
    /* clear hud row */
    for (int c = 0; c < VGA_W; c++)
        put(c, 0, ' ', VGA_COLOR_BLACK, VGA_COLOR_DARK_GREY);

    draw_char_row(1,  0, "SCORE:", VGA_COLOR_YELLOW);
    draw_num(8,  0, score, VGA_COLOR_WHITE);
    draw_char_row(20, 0, "LIVES:", VGA_COLOR_LIGHT_RED);
    for (int i = 0; i < player_lives; i++)
        put(27 + i * 2, 0, '<', VGA_COLOR_LIGHT_RED, VGA_COLOR_DARK_GREY);
    draw_char_row(50, 0, "LEVEL:", VGA_COLOR_LIGHT_CYAN);
    draw_num(57, 0, level, VGA_COLOR_WHITE);
    draw_char_row(65, 0, "A/D:move SPACE:fire", VGA_COLOR_DARK_GREY);
}

static void draw_stars(void)
{
    for (int i = 0; i < MAX_STARS; i++)
        put(stars[i].x, stars[i].y, stars[i].ch, VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
}

static void draw_player(void)
{
    put(player_x - 1, PLAYER_ROW, '(', VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    put(player_x,     PLAYER_ROW, '^', VGA_COLOR_WHITE,       VGA_COLOR_BLACK);
    put(player_x + 1, PLAYER_ROW, ')', VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
}

static void erase_player(void)
{
    put(player_x - 1, PLAYER_ROW, ' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
    put(player_x,     PLAYER_ROW, ' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
    put(player_x + 1, PLAYER_ROW, ' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
}

static void draw_enemies(void)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        put(enemies[i].x, enemies[i].y, enemies[i].ch,
            enemies[i].color, VGA_COLOR_BLACK);
    }
}

static void erase_enemies(void)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        put(enemies[i].x, enemies[i].y, ' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
    }
}

static void draw_bullets(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        put(bullets[i].x, bullets[i].y, '|', VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    }
}

static void erase_bullets(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        put(bullets[i].x, bullets[i].y, ' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
    }
}

static void update_bullets(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        bullets[i].y--;
        if (bullets[i].y < FIELD_TOP) {
            bullets[i].active = 0;
            continue;
        }
        /* check collision with enemies */
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!enemies[j].active) continue;
            if (bullets[i].x == enemies[j].x && bullets[i].y == enemies[j].y) {
                enemies[j].hp--;
                bullets[i].active = 0;
                if (enemies[j].hp <= 0) {
                    enemies[j].active = 0;
                    enemies_alive--;
                    score += 10 * level;
                }
                break;
            }
        }
    }
}

static void update_enemies(void)
{
    enemy_move_timer++;
    if (enemy_move_timer < enemy_move_interval) return;
    enemy_move_timer = 0;

    /* find bounds */
    int min_x = VGA_W, max_x = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        if (enemies[i].x < min_x) min_x = enemies[i].x;
        if (enemies[i].x > max_x) max_x = enemies[i].x;
    }

    static int dir = 1;
    int shift_down = 0;

    if (max_x >= VGA_W - 2) { dir = -1; shift_down = 1; }
    if (min_x <= 1)          { dir =  1; shift_down = 1; }

    erase_enemies();
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        enemies[i].x += dir;
        if (shift_down) enemies[i].y++;
        /* enemy reached player row */
        if (enemies[i].y >= PLAYER_ROW) {
            player_lives--;
            enemies[i].active = 0;
            enemies_alive--;
            if (player_lives <= 0) game_over = 1;
        }
    }
}

static void draw_game_over(void)
{
    int cx = VGA_W / 2;
    int cy = VGA_H / 2;
    draw_char_row(cx - 5, cy - 1, "GAME  OVER",  VGA_COLOR_LIGHT_RED);
    draw_char_row(cx - 6, cy,     "SCORE:", VGA_COLOR_YELLOW);
    draw_num(cx + 1, cy, score, VGA_COLOR_WHITE);
    draw_char_row(cx - 8, cy + 2, "press Q to exit", VGA_COLOR_LIGHT_GREY);
}

static void draw_level_clear(void)
{
    int cx = VGA_W / 2 - 6;
    int cy = VGA_H / 2;
    draw_char_row(cx, cy, "LEVEL CLEAR!", VGA_COLOR_LIGHT_GREEN);
    draw_char_row(cx, cy + 2, "next wave...", VGA_COLOR_LIGHT_GREY);
}

void shooter_run(void)
{
    clear_field();
    init_game();
    draw_hud();
    draw_stars();
    draw_enemies();
    draw_player();

    u32 last_tick  = timer_ticks();
    u32 frame_ms   = 33;   /* ~30 fps */
    int space_held = 0;
    int level_clear_timer = 0;

    for (;;) {
        /* wait for next frame */
        while (timer_ticks() - last_tick < frame_ms)
            __asm__ volatile ("hlt");
        last_tick = timer_ticks();

        if (level_clear_timer > 0) {
            level_clear_timer--;
            if (level_clear_timer == 0) {
                level++;
                memset(bullets, 0, sizeof(bullets));
                clear_field();
                spawn_wave();
                draw_hud();
                draw_stars();
            }
            continue;
        }

        /* input */
        char c = keyboard_getchar();
        if (c == 'q' || c == 'Q') break;

        if (!game_over) {
            erase_player();
            erase_bullets();
            erase_enemies();

            if (c == 'a' || c == 'A') { if (player_x > 2)        player_x--; }
            if (c == 'd' || c == 'D') { if (player_x < VGA_W - 3) player_x++; }

            if (c == ' ') {
                if (!space_held) { fire_bullet(); space_held = 1; }
            } else {
                space_held = 0;
            }

            update_bullets();
            update_enemies();

            draw_stars();
            draw_enemies();
            draw_bullets();
            draw_player();
            draw_hud();

            /* check wave cleared */
            if (enemies_alive <= 0 && !game_over) {
                draw_level_clear();
                level_clear_timer = 60;   /* ~2 seconds */
            }
        } else {
            draw_game_over();
            /* wait for Q */
        }
    }


    clear_field();
}
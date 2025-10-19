#include "editor.h"
#include "terminal.h"
#include "string.h"
#include "tools.h"

#define MAX_LINES 256
#define MAX_LINE_LEN 128

static char buffer[MAX_LINES][MAX_LINE_LEN];
static int current_line = 0;
static int current_col = 0;
static int insert_mode = 0;
static char filename[64];

/* Clears screen and redraws editor view */
void editor_refresh() {
    terminal_clear();
    for (int i = 0; i < MAX_LINES; i++) {
        terminal_write(buffer[i]);
        terminal_write("\n");
    }
    terminal_write("\033[7m"); // reverse color for status bar
    terminal_write(insert_mode ? "-- INSERT -- " : "-- NORMAL -- ");
    terminal_write(filename);
    terminal_write("\033[0m");
}

/* Move cursor to (row, col) */
void editor_move_cursor(int row, int col) {
    char seq[32];
    sprintf(seq, "\033[%d;%dH", row + 1, col + 1);
    terminal_write(seq);
}

/* Save buffer to file (later integrate with filesystem) */
void editor_save() {
    terminal_write("\n[Saved: ");
    terminal_write(filename);
    terminal_write("]\n");
}

/* Opens the editor */
void editor_open(const char *fname) {
    str_copy(filename, fname);
    memset(buffer, 0, sizeof(buffer));
    current_line = 0;
    current_col = 0;
    insert_mode = 0;
    editor_refresh();
    editor_run();
}

/* Handles key input */
void editor_run() {
    char key;
    while (1) {
        key = terminal_getchar();

        if (insert_mode) {
            if (key == 27) { // ESC
                insert_mode = 0;
            } else if (key == '\n') {
                current_line++;
                current_col = 0;
            } else {
                buffer[current_line][current_col++] = key;
            }
        } else {
            if (key == 'i') {
                insert_mode = 1;
            } else if (key == ':') {
                terminal_write("\n:");
                char cmd[16];
                terminal_readline(cmd, sizeof(cmd));
                if (str_eq(cmd, "w")) {
                    editor_save();
                } else if (str_eq(cmd, "q")) {
                    terminal_write("\nExiting editor...\n");
                    break;
                }
            } else if (key == 'h') {
                if (current_col > 0) current_col--;
            } else if (key == 'l') {
                current_col++;
            } else if (key == 'j') {
                if (current_line < MAX_LINES - 1) current_line++;
            } else if (key == 'k') {
                if (current_line > 0) current_line--;
            }
        }
        editor_refresh();
        editor_move_cursor(current_line, current_col);
    }
}

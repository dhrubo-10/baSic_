#include "io.h"

void print_char(char c) {
    putc(c);
}

void print_word(int w) {
    putw(w);
}

void print_error(void) {
    error();
}

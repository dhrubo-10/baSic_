#include <stdio.h>
#include <dirent.h>
#include "tools.h"
#include "terminal.h"
#include "clock.h"

void tool_echo(const char *msg) {
    printf("%s\n", msg);
}

void tool_clear(void) {
    term_clear();
}

void tool_ls(void) {
    DIR *d = opendir(".");
    struct dirent *dir;
    if (d) {
        while ((dir = readdir(d)) != NULL)
            printf("%s  ", dir->d_name);
        printf("\n");
        closedir(d);
    }
}

void tool_uptime(void) {
    u64 ticks = clock_ticks();
    printf("Uptime: %llu ms\n", ticks);
}

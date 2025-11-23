#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include "clock.h"

void clock_init(void) {
    printf("Clock initialized.\n");
}

u64 clock_ticks(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void clock_sleep(u64 ms) {
    usleep(ms * 1000);
}

void clock_now(char *buf, size_t len) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm_info);
}

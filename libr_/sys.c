#include "lib/s_c.h"
#include "lib/str.h"
#include "lib/std.h"
#include "lib/stdlib.h"
#include "lib/errno.h"

/*
 * sys.c
 * 
 * A simple system statistics tool that queries the kernel for uptime and
 * block I/O statistics, then prints them in a human-readable format.
 */

int main(int argc, char *argv[])
{
    struct system_stats s = {0};

    /* Get system stats from the kernel */
    if (syscall_system_stats(&s)) {
        fprintf(stderr, "Error: unable to retrieve system stats (%s)\n", strerror(errno));
        return 1;
    }

    /* Print system uptime */
    unsigned int hours = s.time / 3600;
    unsigned int minutes = (s.time % 3600) / 60;
    unsigned int seconds = s.time % 60;

    printf("System uptime: %u:%02u:%02u\n", hours, minutes, seconds);

    /* Print per-disk stats */
    for (int i = 0; i < 4; i++) {
        printf("Disk %d: %d blocks read, %d blocks written\n",
               i, s.blocks_read[i], s.blocks_written[i]);
    }

    return 0;
}

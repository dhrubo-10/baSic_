/* init.c - initial user process (in freestanding compiled into kernel or hosted)
 * On freestanding this should be linked into the kernel and called as the init process.
 * On hosted this file is compiled into usr_tools and run.
 */

#include "userlib.h"

#ifdef HOSTED
int main_init(int argc, char **argv) {
    (void)argc; (void)argv;
    u_printf("usr init: launching shell...\n");
    u_run_init();
    return 0;
}
#else
void init_main(void) {
    u_printf("init: starting shell\n");
    u_run_init();
    for(;;); /* wait for cild in userland */
}
#endif

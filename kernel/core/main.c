/* 
 * Rn  keeping it toally a noobie project and bootable just to see if it is loading and working correctly.
 * will get back to it later when i get some time off from my linux project.
 */

#include <kernelb/stdio.h>
#include <kernelb/fs.h>
#include <arch/x86.h>

void kernel_main(void)
{
    arch_init();
    
    printk("Kernel booted successfully!\n");
    printk("Initializing system...\n");
    
    mm_init();
    
    fs_init();
    
    drivers_init();
    
    printk("System initialized\n");
    
    while (1) {
        // Handle tasks
    }
}
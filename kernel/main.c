/* 
 * Rn  keeping it toally a noobie project and bootable just to see if it is loading and working correctly.
 * will get back to it later when i get some time off from my linux project.
 */
#include <kernelb/stdio.h>

void kernel_main(void)
{
    printk("Kernel booted successfully!\n");
    printk("Initializing system...\n");
    
    printk("System initialized\n");
    
    // Hang FOORVERERR
    for(;;);
}
/**
 * power.c - Horizon kernel power management
 * 
 * This file contains the implementation of the power management subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/console.h>

/**
 * Shutdown the system
 * 
 * @return 0 on success, negative error code on failure
 */
int power_shutdown(void) {
    /* Print shutdown message */
    console_write("System is shutting down...\n");
    
    /* Halt the system */
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
    
    return 0;
}

/**
 * Reboot the system
 * 
 * @return 0 on success, negative error code on failure
 */
int power_reboot(void) {
    /* Print reboot message */
    console_write("System is rebooting...\n");
    
    /* Reset the system */
    __asm__ volatile("cli");
    
    /* Try to reset using the keyboard controller */
    __asm__ volatile("movb $0xFE, %al");
    __asm__ volatile("outb %al, $0x64");
    
    /* If that didn't work, halt the system */
    for (;;) {
        __asm__ volatile("hlt");
    }
    
    return 0;
}

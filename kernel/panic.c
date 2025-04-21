/**
 * panic.c - Horizon kernel panic handling
 *
 * This file contains the implementation of the kernel panic handling.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/console.h>

/**
 * Kernel panic implementation
 *
 * @param message Panic message
 */
void kernel_panic(const char *message) {
    /* Print panic message */
    console_write("\n\nKERNEL PANIC: ");
    console_write(message);
    console_write("\n\nSystem halted.\n");

    /* Halt the system */
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

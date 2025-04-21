/**
 * kernel_main.c - Horizon kernel main entry point
 *
 * This file contains the main entry point for the Horizon kernel.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/sched.h>
#include <horizon/console.h>

/* Kernel main function */
void kernel_main(void) {
    /* Initialize console */
    console_init();
    console_write("Console initialized\n");

    /* Initialize memory management */
    mm_init();
    console_write("Memory management initialized\n");

    /* Initialize scheduler */
    sched_init();
    console_write("Scheduler initialized\n");

    /* Boot complete */
    console_write("Boot complete\n");

    /* Never reached */
    while (1) {
        /* Halt the CPU */
        __asm__ volatile("hlt");
    }
}

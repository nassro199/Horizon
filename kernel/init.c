/**
 * init.c - Kernel initialization
 *
 * This file contains the kernel initialization code.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/vmm.h>
#include <horizon/fs.h>
#include <horizon/sched.h>
#include <horizon/console.h>

/* External functions */
extern void capability_init(void);
extern void uhci_driver_init(void);



/* Kernel panic implementation */
void kernel_panic(const char *message) {
    early_console_print("\nKERNEL PANIC: ");
    early_console_print(message);
    early_console_print("\nSystem halted.\n");

    /* Halt the system */
    for (;;) {
        /* Do nothing */
    }
}

/* Kernel initialization */
void kernel_init(void) {
    early_console_print("Initializing kernel...\n");

    /* Initialize memory management */
    early_console_print("Initializing memory management...\n");
    mm_init();

    /* Initialize file system */
    early_console_print("Initializing file system...\n");
    fs_init();

    /* Initialize scheduler */
    early_console_print("Initializing scheduler...\n");
    sched_init();

    /* Initialize virtual memory manager */
    early_console_print("Initializing virtual memory manager...\n");
    vmm_init();

    early_console_print("Kernel initialization complete.\n");
}

/* Kernel main entry point */
void kernel_main(void) {
    early_console_print("Kernel starting...\n");

    /* Initialize the kernel */
    kernel_init();

    /* Start the scheduler */
    early_console_print("Starting scheduler...\n");

    /* This function should never return */
    for (;;) {
        /* Idle loop */
    }
}

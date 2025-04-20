/**
 * main.c - Horizon kernel initialization
 *
 * This file contains the kernel initialization code.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/vmm.h>
#include <horizon/fs.h>
#include <horizon/device.h>
#include <horizon/sched.h>
#include <horizon/sched/sched.h>
#include <horizon/syscall.h>
#include <horizon/input.h>
#include <horizon/net.h>
#include <horizon/ipc.h>
#include <horizon/security.h>
#include <horizon/usb.h>
#include <horizon/block.h>
#include <horizon/crypto.h>

/* External functions */
extern void capability_init(void);
extern void uhci_driver_init(void);
extern void block_init(void);
extern void crypto_init(void);

/* Early console output function */
static void early_console_print(const char *str) {
    /* This would be implemented with direct hardware access */
    /* For now, it's just a placeholder */
}

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
    early_console_print("Initializing Horizon kernel...\n");

    /* Initialize memory management */
    early_console_print("Initializing memory management...\n");
    mm_init();

    /* Initialize device management */
    early_console_print("Initializing device management...\n");
    device_init();

    /* Initialize file system */
    early_console_print("Initializing file system...\n");
    fs_init();

    /* Initialize scheduler */
    early_console_print("Initializing scheduler...\n");
    sched_init();

    /* Initialize system calls */
    early_console_print("Initializing system calls...\n");
    syscall_init();

    /* Initialize input subsystem */
    early_console_print("Initializing input subsystem...\n");
    input_init();

    /* Initialize networking subsystem */
    early_console_print("Initializing networking subsystem...\n");
    net_init();

    /* Initialize IPC subsystem */
    early_console_print("Initializing IPC subsystem...\n");
    ipc_init();

    /* Initialize security subsystem */
    early_console_print("Initializing security subsystem...\n");
    security_init();
    capability_init();

    /* Initialize block device subsystem */
    early_console_print("Initializing block device subsystem...\n");
    block_init();

    /* Initialize cryptography subsystem */
    early_console_print("Initializing cryptography subsystem...\n");
    crypto_init();

    /* Initialize USB subsystem */
    early_console_print("Initializing USB subsystem...\n");
    usb_init();
    uhci_driver_init();

    /* Initialize virtual memory manager */
    early_console_print("Initializing virtual memory manager...\n");
    vmm_init();

    /* Initialize advanced scheduler */
    early_console_print("Initializing advanced scheduler...\n");
    sched_init_advanced();

    early_console_print("Horizon kernel initialization complete.\n");
}

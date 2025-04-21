/**
 * main.c - Horizon kernel main entry point
 *
 * This file contains the kernel main entry point.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/string.h>
#include <horizon/console.h>
#include <horizon/multiboot.h>
#include <horizon/mm/early.h>
#include <horizon/acpi.h>
#include <horizon/boot.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* External functions */
extern void arch_setup(void);
extern void interrupt_init(void);
extern void vga_init(void);
extern void keyboard_init(void);
extern void keyboard_handler_init(void);
extern void shell_init(void);
extern void capability_init(void);
extern void uhci_driver_init(void);
extern void kernel_init(void);

/* Kernel entry point */
void kernel_entry(u32 magic, multiboot_info_t *mbi) {
    /* Initialize early console */
    early_console_init();

    /* Initialize boot process */
    boot_init();

    /* Check multiboot magic number */
    if (magic != 0x2BADB002) {
        boot_error("Invalid multiboot magic number");
        kernel_panic("Invalid multiboot magic number");
    }

    /* Initialize multiboot */
    boot_message("Initializing multiboot...");
    multiboot_init(magic, mbi);
    multiboot_print_info();
    boot_set_stage(BOOT_STAGE_MEMORY);

    /* Initialize early memory management */
    boot_message("Initializing early memory management...");
    early_mm_init();
    multiboot_parse_mmap();
    boot_progress(20);

    /* Architecture-specific setup */
    boot_message("Initializing architecture-specific features...");
    arch_setup();
    boot_progress(30);

    /* Initialize ACPI */
    boot_set_stage(BOOT_STAGE_ACPI);
    boot_message("Initializing ACPI...");
    acpi_init();
    boot_progress(40);

    /* Initialize interrupts */
    boot_set_stage(BOOT_STAGE_INTERRUPTS);
    boot_message("Initializing interrupt system...");
    interrupt_init();
    boot_progress(50);

    /* Initialize the VGA console */
    boot_set_stage(BOOT_STAGE_CONSOLE);
    boot_message("Initializing VGA console...");
    vga_init();
    boot_progress(60);

    /* Initialize the timer */
    boot_set_stage(BOOT_STAGE_TIMER);
    boot_message("Initializing timer...");
    /* TODO: Initialize timer */
    boot_progress(70);

    /* Initialize the scheduler */
    boot_set_stage(BOOT_STAGE_SCHEDULER);
    boot_message("Initializing scheduler...");
    /* TODO: Initialize scheduler */
    boot_progress(80);

    /* Initialize the kernel */
    boot_set_stage(BOOT_STAGE_DRIVERS);
    boot_message("Initializing Horizon kernel subsystems...");
    kernel_init();
    boot_progress(90);

    /* Enable interrupts */
    boot_message("Enabling interrupts...");
    __asm__ volatile("sti");

    /* Boot complete */
    boot_set_stage(BOOT_STAGE_COMPLETE);
    boot_message("Horizon kernel initialization complete.");
    boot_print_time();
    boot_progress(100);

    /* Show welcome message */
    console_clear();
    console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    console_write_color("  _    _            _                   ____   _____ \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    console_write_color(" | |  | |          (_)                 / __ \\ / ____|\n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    console_write_color(" | |__| | ___  _ __ _ _______  _ __   | |  | | (___  \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    console_write_color(" |  __  |/ _ \\| '__| |_  / _ \\| '_ \\  | |  | |\\___ \\ \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    console_write_color(" | |  | | (_) | |  | |/ / (_) | | | | | |__| |____) |\n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    console_write_color(" |_|  |_|\\___/|_|  |_/___\\___/|_| |_|  \\____/|_____/ \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    console_write_color("                   Horizon OS v0.1                   \n", CONSOLE_YELLOW, CONSOLE_BLACK);
    console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    console_write_color("Welcome to Horizon OS!\n", CONSOLE_LIGHT_GREEN, CONSOLE_BLACK);
    console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);

    /* Main loop */
    for (;;) {
        /* Halt the CPU until an interrupt occurs */
        __asm__ volatile("hlt");
    }
}

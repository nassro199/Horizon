/**
 * kernel.h - Main kernel header file
 * 
 * This file contains the core definitions and structures for the
 * monolithic kernel architecture.
 */

#ifndef _KERNEL_H
#define _KERNEL_H

/* Kernel version information */
#define KERNEL_VERSION_MAJOR    0
#define KERNEL_VERSION_MINOR    1
#define KERNEL_VERSION_PATCH    0
#define KERNEL_NAME             "Horizon"

/* Kernel panic function */
void kernel_panic(const char *message);

/* Kernel initialization */
void kernel_init(void);

/* Kernel main entry point */
void kernel_main(void);

#endif /* _KERNEL_H */

/**
 * panic.h - Horizon kernel panic handling definitions
 * 
 * This file contains definitions for the kernel panic handling.
 */

#ifndef _HORIZON_PANIC_H
#define _HORIZON_PANIC_H

/* Kernel panic information */
struct panic_info;

/* Kernel panic implementation */
void __kernel_panic(const char *message, const char *file, int line, void *caller);

/* Kernel panic macro */
#define kernel_panic(message) __kernel_panic(message, __FILE__, __LINE__, __builtin_return_address(0))

/* Get the last panic information */
const struct panic_info *kernel_panic_info(void);

/* Check if panic is in progress */
int kernel_panic_in_progress(void);

#endif /* _HORIZON_PANIC_H */

/**
 * swap.h - Horizon kernel swap definitions
 * 
 * This file contains definitions for the swap subsystem.
 */

#ifndef _HORIZON_MM_SWAP_H
#define _HORIZON_MM_SWAP_H

#include <horizon/types.h>

/* Initialize the swap subsystem */
void swap_init(void);

/* Add a swap area */
int swap_add(const char *path, u32 size);

/* Remove a swap area */
int swap_remove(const char *path);

/* Allocate a swap entry */
u32 swap_alloc(void);

/* Free a swap entry */
int swap_free(u32 entry);

/* Write a page to swap */
int swap_write(u32 entry, void *data);

/* Read a page from swap */
int swap_read(u32 entry, void *data);

/* Swap out a page */
int swap_out_page(struct task_struct *task, u32 addr);

/* Swap in a page */
int swap_in_page(struct task_struct *task, u32 addr);

/* Print swap statistics */
void swap_print_stats(void);

#endif /* _HORIZON_MM_SWAP_H */

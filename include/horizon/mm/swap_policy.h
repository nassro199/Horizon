/**
 * swap_policy.h - Horizon kernel swap policy definitions
 * 
 * This file contains definitions for the swap policy subsystem.
 */

#ifndef _HORIZON_MM_SWAP_POLICY_H
#define _HORIZON_MM_SWAP_POLICY_H

#include <horizon/types.h>

/* Swap policies */
typedef enum swap_policy {
    SWAP_POLICY_NONE,      /* No swapping */
    SWAP_POLICY_LRU,       /* Least Recently Used */
    SWAP_POLICY_FIFO,      /* First In, First Out */
    SWAP_POLICY_CLOCK,     /* Clock algorithm */
    SWAP_POLICY_RANDOM     /* Random selection */
} swap_policy_t;

/* Initialize the swap policy subsystem */
void swap_policy_init(void);

/* Set the swap policy */
int swap_policy_set(swap_policy_t policy);

/* Get the swap policy */
swap_policy_t swap_policy_get(void);

/* Scan for pages to swap out */
int swap_policy_scan(struct task_struct *task, u32 count);

/* Scan for pages to swap out using the LRU policy */
int swap_policy_scan_lru(struct task_struct *task, u32 count);

/* Scan for pages to swap out using the FIFO policy */
int swap_policy_scan_fifo(struct task_struct *task, u32 count);

/* Scan for pages to swap out using the clock policy */
int swap_policy_scan_clock(struct task_struct *task, u32 count);

/* Scan for pages to swap out using the random policy */
int swap_policy_scan_random(struct task_struct *task, u32 count);

/* Add a page to the swap candidates */
int swap_policy_add_candidate(struct task_struct *task, u32 addr);

/* Prefetch pages from swap */
int swap_policy_prefetch(struct task_struct *task, u32 addr, u32 count);

/* Print swap policy statistics */
void swap_policy_print_stats(void);

#endif /* _HORIZON_MM_SWAP_POLICY_H */

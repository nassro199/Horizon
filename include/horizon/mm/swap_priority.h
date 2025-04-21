/**
 * swap_priority.h - Horizon kernel swap prioritization definitions
 * 
 * This file contains definitions for the swap prioritization subsystem.
 */

#ifndef _HORIZON_MM_SWAP_PRIORITY_H
#define _HORIZON_MM_SWAP_PRIORITY_H

#include <horizon/types.h>

/* Page priorities */
typedef enum swap_priority {
    SWAP_PRIORITY_LOW,     /* Low priority */
    SWAP_PRIORITY_MEDIUM,  /* Medium priority */
    SWAP_PRIORITY_HIGH     /* High priority */
} swap_priority_t;

/* Prioritization algorithms */
typedef enum swap_priority_algo {
    SWAP_PRIORITY_NONE,    /* No prioritization */
    SWAP_PRIORITY_ACCESS,  /* Prioritize based on access time */
    SWAP_PRIORITY_TYPE,    /* Prioritize based on page type */
    SWAP_PRIORITY_CUSTOM   /* Custom prioritization */
} swap_priority_algo_t;

/* Initialize the swap prioritization subsystem */
void swap_priority_init(void);

/* Set the prioritization algorithm */
int swap_priority_set_algo(swap_priority_algo_t algo);

/* Get the prioritization algorithm */
swap_priority_algo_t swap_priority_get_algo(void);

/* Get the priority of a page */
swap_priority_t swap_priority_get(struct task_struct *task, u32 addr);

/* Prioritize a page based on access time */
swap_priority_t swap_priority_access(struct task_struct *task, u32 addr, struct page *page);

/* Prioritize a page based on page type */
swap_priority_t swap_priority_type(struct task_struct *task, u32 addr, struct page *page);

/* Prioritize a page using a custom algorithm */
swap_priority_t swap_priority_custom(struct task_struct *task, u32 addr, struct page *page);

/* Scan for high priority pages */
int swap_priority_scan_high(struct task_struct *task, u32 count);

/* Scan for medium priority pages */
int swap_priority_scan_medium(struct task_struct *task, u32 count);

/* Scan for low priority pages */
int swap_priority_scan_low(struct task_struct *task, u32 count);

/* Print priority statistics */
void swap_priority_print_stats(void);

#endif /* _HORIZON_MM_SWAP_PRIORITY_H */

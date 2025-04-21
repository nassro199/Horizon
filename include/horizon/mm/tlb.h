/**
 * tlb.h - Horizon kernel TLB management definitions
 * 
 * This file contains definitions for the TLB (Translation Lookaside Buffer) management.
 */

#ifndef _HORIZON_MM_TLB_H
#define _HORIZON_MM_TLB_H

#include <horizon/types.h>

/* Initialize the TLB management */
void tlb_init(void);

/* Flush a single TLB entry */
void tlb_flush_single(u32 addr);

/* Flush the entire TLB */
void tlb_flush_all(void);

/* Flush a range of TLB entries */
void tlb_flush_range(u32 start, u32 end);

/* Flush the TLB for a specific task */
void tlb_flush_task(struct task_struct *task);

/* Flush the TLB for a specific address space */
void tlb_flush_mm(struct mm_struct *mm);

/* Flush the TLB for a specific virtual memory area */
void tlb_flush_vma(struct mm_struct *mm, struct vm_area_struct *vma);

/* Print TLB statistics */
void tlb_print_stats(void);

#endif /* _HORIZON_MM_TLB_H */

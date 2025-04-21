/**
 * tlb.c - Horizon kernel TLB management implementation
 * 
 * This file contains the implementation of the TLB (Translation Lookaside Buffer) management.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/vmm.h>
#include <horizon/mm/pmm.h>
#include <horizon/mm/page.h>
#include <horizon/mm/tlb.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/task.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* TLB statistics */
static u64 tlb_flush_count = 0;
static u64 tlb_flush_single_count = 0;
static u64 tlb_flush_all_count = 0;
static u64 tlb_flush_range_count = 0;

/* TLB lock */
static spinlock_t tlb_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize the TLB management
 */
void tlb_init(void) {
    /* Reset statistics */
    tlb_flush_count = 0;
    tlb_flush_single_count = 0;
    tlb_flush_all_count = 0;
    tlb_flush_range_count = 0;
    
    printk(KERN_INFO "TLB: Initialized TLB management\n");
}

/**
 * Flush a single TLB entry
 * 
 * @param addr Address to flush
 */
void tlb_flush_single(u32 addr) {
    /* Flush the TLB entry */
    __asm__ volatile("invlpg (%0)" : : "r" (addr) : "memory");
    
    /* Update statistics */
    spin_lock(&tlb_lock);
    tlb_flush_count++;
    tlb_flush_single_count++;
    spin_unlock(&tlb_lock);
}

/**
 * Flush the entire TLB
 */
void tlb_flush_all(void) {
    /* Get the current CR3 value */
    u32 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    
    /* Reload CR3 to flush the TLB */
    __asm__ volatile("mov %0, %%cr3" : : "r" (cr3) : "memory");
    
    /* Update statistics */
    spin_lock(&tlb_lock);
    tlb_flush_count++;
    tlb_flush_all_count++;
    spin_unlock(&tlb_lock);
}

/**
 * Flush a range of TLB entries
 * 
 * @param start Start address
 * @param end End address
 */
void tlb_flush_range(u32 start, u32 end) {
    /* Align the addresses to page boundaries */
    start &= ~(PAGE_SIZE - 1);
    end = (end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    /* Flush each page in the range */
    for (u32 addr = start; addr < end; addr += PAGE_SIZE) {
        tlb_flush_single(addr);
    }
    
    /* Update statistics */
    spin_lock(&tlb_lock);
    tlb_flush_range_count++;
    spin_unlock(&tlb_lock);
}

/**
 * Flush the TLB for a specific task
 * 
 * @param task Task to flush TLB for
 */
void tlb_flush_task(task_struct_t *task) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL) {
        return;
    }
    
    /* Check if the task is the current task */
    if (task == task_current()) {
        /* Flush the entire TLB */
        tlb_flush_all();
    } else {
        /* No need to flush the TLB for other tasks */
        /* This would be implemented with IPI for SMP systems */
    }
}

/**
 * Flush the TLB for a specific address space
 * 
 * @param mm Memory descriptor to flush TLB for
 */
void tlb_flush_mm(mm_struct_t *mm) {
    /* Check parameters */
    if (mm == NULL) {
        return;
    }
    
    /* Check if the address space is active */
    if (mm == task_current()->mm) {
        /* Flush the entire TLB */
        tlb_flush_all();
    } else {
        /* No need to flush the TLB for inactive address spaces */
        /* This would be implemented with IPI for SMP systems */
    }
}

/**
 * Flush the TLB for a specific virtual memory area
 * 
 * @param mm Memory descriptor
 * @param vma Virtual memory area
 */
void tlb_flush_vma(mm_struct_t *mm, vm_area_struct_t *vma) {
    /* Check parameters */
    if (mm == NULL || vma == NULL) {
        return;
    }
    
    /* Check if the address space is active */
    if (mm == task_current()->mm) {
        /* Flush the range of addresses */
        tlb_flush_range(vma->vm_start, vma->vm_end);
    } else {
        /* No need to flush the TLB for inactive address spaces */
        /* This would be implemented with IPI for SMP systems */
    }
}

/**
 * Print TLB statistics
 */
void tlb_print_stats(void) {
    /* Print the statistics */
    printk(KERN_INFO "TLB: Total flushes: %llu\n", tlb_flush_count);
    printk(KERN_INFO "TLB: Single entry flushes: %llu\n", tlb_flush_single_count);
    printk(KERN_INFO "TLB: Full flushes: %llu\n", tlb_flush_all_count);
    printk(KERN_INFO "TLB: Range flushes: %llu\n", tlb_flush_range_count);
}

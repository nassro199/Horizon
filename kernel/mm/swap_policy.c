/**
 * swap_policy.c - Horizon kernel swap policy implementation
 * 
 * This file contains the implementation of the swap policy subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/vmm.h>
#include <horizon/mm/pmm.h>
#include <horizon/mm/page.h>
#include <horizon/mm/swap.h>
#include <horizon/mm/swap_policy.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/task.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Swap policy statistics */
static u64 swap_policy_scan_count = 0;
static u64 swap_policy_scan_pages = 0;
static u64 swap_policy_swapout_count = 0;
static u64 swap_policy_swapout_pages = 0;
static u64 swap_policy_prefetch_count = 0;
static u64 swap_policy_prefetch_pages = 0;

/* Swap policy lock */
static spinlock_t swap_policy_lock = SPIN_LOCK_INITIALIZER;

/* Current swap policy */
static swap_policy_t current_policy = SWAP_POLICY_LRU;

/**
 * Initialize the swap policy subsystem
 */
void swap_policy_init(void) {
    /* Reset statistics */
    swap_policy_scan_count = 0;
    swap_policy_scan_pages = 0;
    swap_policy_swapout_count = 0;
    swap_policy_swapout_pages = 0;
    swap_policy_prefetch_count = 0;
    swap_policy_prefetch_pages = 0;
    
    /* Set the default policy */
    current_policy = SWAP_POLICY_LRU;
    
    printk(KERN_INFO "SWAP_POLICY: Initialized swap policy subsystem\n");
}

/**
 * Set the swap policy
 * 
 * @param policy Swap policy to set
 * @return 0 on success, negative error code on failure
 */
int swap_policy_set(swap_policy_t policy) {
    /* Check parameters */
    if (policy < SWAP_POLICY_NONE || policy > SWAP_POLICY_RANDOM) {
        return -EINVAL;
    }
    
    /* Lock the swap policy */
    spin_lock(&swap_policy_lock);
    
    /* Set the policy */
    current_policy = policy;
    
    /* Unlock the swap policy */
    spin_unlock(&swap_policy_lock);
    
    printk(KERN_INFO "SWAP_POLICY: Set swap policy to %d\n", policy);
    
    return 0;
}

/**
 * Get the swap policy
 * 
 * @return Current swap policy
 */
swap_policy_t swap_policy_get(void) {
    return current_policy;
}

/**
 * Scan for pages to swap out
 * 
 * @param task Task to scan
 * @param count Number of pages to scan
 * @return Number of pages found, or negative error code on failure
 */
int swap_policy_scan(task_struct_t *task, u32 count) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL || count == 0) {
        return -EINVAL;
    }
    
    /* Lock the swap policy */
    spin_lock(&swap_policy_lock);
    
    /* Increment the scan count */
    swap_policy_scan_count++;
    swap_policy_scan_pages += count;
    
    /* Unlock the swap policy */
    spin_unlock(&swap_policy_lock);
    
    /* Scan for pages based on the policy */
    switch (current_policy) {
        case SWAP_POLICY_NONE:
            /* No swapping */
            return 0;
        
        case SWAP_POLICY_LRU:
            /* Least Recently Used */
            return swap_policy_scan_lru(task, count);
        
        case SWAP_POLICY_FIFO:
            /* First In, First Out */
            return swap_policy_scan_fifo(task, count);
        
        case SWAP_POLICY_CLOCK:
            /* Clock algorithm */
            return swap_policy_scan_clock(task, count);
        
        case SWAP_POLICY_RANDOM:
            /* Random selection */
            return swap_policy_scan_random(task, count);
        
        default:
            /* Unknown policy */
            return -EINVAL;
    }
}

/**
 * Scan for pages to swap out using the LRU policy
 * 
 * @param task Task to scan
 * @param count Number of pages to scan
 * @return Number of pages found, or negative error code on failure
 */
int swap_policy_scan_lru(task_struct_t *task, u32 count) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL || count == 0) {
        return -EINVAL;
    }
    
    /* Get the memory descriptor */
    mm_struct_t *mm = task->mm;
    
    /* Initialize the page count */
    u32 found = 0;
    
    /* Scan the virtual memory areas */
    vm_area_struct_t *vma;
    for (vma = mm->mmap; vma != NULL && found < count; vma = vma->vm_list.next) {
        /* Skip non-swappable areas */
        if (vma->vm_flags & VM_LOCKED) {
            continue;
        }
        
        /* Scan the pages in the area */
        for (u32 addr = vma->vm_start; addr < vma->vm_end && found < count; addr += PAGE_SIZE) {
            /* Get the page */
            page_t *page = vmm_get_page(mm, addr);
            
            /* Check if the page exists and is swappable */
            if (page != NULL && !page_test_flags(page, (1 << PG_locked))) {
                /* Add the page to the swap candidates */
                swap_policy_add_candidate(task, addr);
                found++;
            }
        }
    }
    
    return found;
}

/**
 * Scan for pages to swap out using the FIFO policy
 * 
 * @param task Task to scan
 * @param count Number of pages to scan
 * @return Number of pages found, or negative error code on failure
 */
int swap_policy_scan_fifo(task_struct_t *task, u32 count) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL || count == 0) {
        return -EINVAL;
    }
    
    /* Get the memory descriptor */
    mm_struct_t *mm = task->mm;
    
    /* Initialize the page count */
    u32 found = 0;
    
    /* Scan the virtual memory areas */
    vm_area_struct_t *vma;
    for (vma = mm->mmap; vma != NULL && found < count; vma = vma->vm_list.next) {
        /* Skip non-swappable areas */
        if (vma->vm_flags & VM_LOCKED) {
            continue;
        }
        
        /* Scan the pages in the area */
        for (u32 addr = vma->vm_start; addr < vma->vm_end && found < count; addr += PAGE_SIZE) {
            /* Get the page */
            page_t *page = vmm_get_page(mm, addr);
            
            /* Check if the page exists and is swappable */
            if (page != NULL && !page_test_flags(page, (1 << PG_locked))) {
                /* Add the page to the swap candidates */
                swap_policy_add_candidate(task, addr);
                found++;
            }
        }
    }
    
    return found;
}

/**
 * Scan for pages to swap out using the clock policy
 * 
 * @param task Task to scan
 * @param count Number of pages to scan
 * @return Number of pages found, or negative error code on failure
 */
int swap_policy_scan_clock(task_struct_t *task, u32 count) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL || count == 0) {
        return -EINVAL;
    }
    
    /* Get the memory descriptor */
    mm_struct_t *mm = task->mm;
    
    /* Initialize the page count */
    u32 found = 0;
    
    /* Scan the virtual memory areas */
    vm_area_struct_t *vma;
    for (vma = mm->mmap; vma != NULL && found < count; vma = vma->vm_list.next) {
        /* Skip non-swappable areas */
        if (vma->vm_flags & VM_LOCKED) {
            continue;
        }
        
        /* Scan the pages in the area */
        for (u32 addr = vma->vm_start; addr < vma->vm_end && found < count; addr += PAGE_SIZE) {
            /* Get the page */
            page_t *page = vmm_get_page(mm, addr);
            
            /* Check if the page exists and is swappable */
            if (page != NULL && !page_test_flags(page, (1 << PG_locked))) {
                /* Check if the page has been accessed */
                if (page_test_flags(page, (1 << PG_accessed))) {
                    /* Clear the accessed flag */
                    page_clear_flags(page, (1 << PG_accessed));
                } else {
                    /* Add the page to the swap candidates */
                    swap_policy_add_candidate(task, addr);
                    found++;
                }
            }
        }
    }
    
    return found;
}

/**
 * Scan for pages to swap out using the random policy
 * 
 * @param task Task to scan
 * @param count Number of pages to scan
 * @return Number of pages found, or negative error code on failure
 */
int swap_policy_scan_random(task_struct_t *task, u32 count) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL || count == 0) {
        return -EINVAL;
    }
    
    /* Get the memory descriptor */
    mm_struct_t *mm = task->mm;
    
    /* Initialize the page count */
    u32 found = 0;
    
    /* Scan the virtual memory areas */
    vm_area_struct_t *vma;
    for (vma = mm->mmap; vma != NULL && found < count; vma = vma->vm_list.next) {
        /* Skip non-swappable areas */
        if (vma->vm_flags & VM_LOCKED) {
            continue;
        }
        
        /* Scan the pages in the area */
        for (u32 addr = vma->vm_start; addr < vma->vm_end && found < count; addr += PAGE_SIZE) {
            /* Get the page */
            page_t *page = vmm_get_page(mm, addr);
            
            /* Check if the page exists and is swappable */
            if (page != NULL && !page_test_flags(page, (1 << PG_locked))) {
                /* Add the page to the swap candidates with a random chance */
                if ((timer_get_ticks() % 100) < 10) {
                    swap_policy_add_candidate(task, addr);
                    found++;
                }
            }
        }
    }
    
    return found;
}

/**
 * Add a page to the swap candidates
 * 
 * @param task Task that owns the page
 * @param addr Address of the page
 * @return 0 on success, negative error code on failure
 */
int swap_policy_add_candidate(task_struct_t *task, u32 addr) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL) {
        return -EINVAL;
    }
    
    /* Align the address to a page boundary */
    addr &= ~(PAGE_SIZE - 1);
    
    /* Swap out the page */
    return swap_out_page(task, addr);
}

/**
 * Prefetch pages from swap
 * 
 * @param task Task to prefetch for
 * @param addr Base address to prefetch around
 * @param count Number of pages to prefetch
 * @return Number of pages prefetched, or negative error code on failure
 */
int swap_policy_prefetch(task_struct_t *task, u32 addr, u32 count) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL || count == 0) {
        return -EINVAL;
    }
    
    /* Align the address to a page boundary */
    addr &= ~(PAGE_SIZE - 1);
    
    /* Lock the swap policy */
    spin_lock(&swap_policy_lock);
    
    /* Increment the prefetch count */
    swap_policy_prefetch_count++;
    swap_policy_prefetch_pages += count;
    
    /* Unlock the swap policy */
    spin_unlock(&swap_policy_lock);
    
    /* Initialize the prefetch count */
    u32 prefetched = 0;
    
    /* Prefetch pages after the base address */
    for (u32 i = 1; i <= count / 2; i++) {
        u32 prefetch_addr = addr + (i * PAGE_SIZE);
        
        /* Check if the address is valid */
        if (prefetch_addr >= task->mm->start_stack) {
            break;
        }
        
        /* Check if the page is swapped out */
        if (task->mm->swap_map != NULL && task->mm->swap_map[prefetch_addr / PAGE_SIZE] != 0) {
            /* Swap in the page */
            if (swap_in_page(task, prefetch_addr) == 0) {
                prefetched++;
            }
        }
    }
    
    /* Prefetch pages before the base address */
    for (u32 i = 1; i <= count / 2; i++) {
        u32 prefetch_addr = addr - (i * PAGE_SIZE);
        
        /* Check if the address is valid */
        if (prefetch_addr < task->mm->start_code) {
            break;
        }
        
        /* Check if the page is swapped out */
        if (task->mm->swap_map != NULL && task->mm->swap_map[prefetch_addr / PAGE_SIZE] != 0) {
            /* Swap in the page */
            if (swap_in_page(task, prefetch_addr) == 0) {
                prefetched++;
            }
        }
    }
    
    return prefetched;
}

/**
 * Print swap policy statistics
 */
void swap_policy_print_stats(void) {
    /* Print the statistics */
    printk(KERN_INFO "SWAP_POLICY: Current policy: %d\n", current_policy);
    printk(KERN_INFO "SWAP_POLICY: Scan count: %llu\n", swap_policy_scan_count);
    printk(KERN_INFO "SWAP_POLICY: Scan pages: %llu\n", swap_policy_scan_pages);
    printk(KERN_INFO "SWAP_POLICY: Swapout count: %llu\n", swap_policy_swapout_count);
    printk(KERN_INFO "SWAP_POLICY: Swapout pages: %llu\n", swap_policy_swapout_pages);
    printk(KERN_INFO "SWAP_POLICY: Prefetch count: %llu\n", swap_policy_prefetch_count);
    printk(KERN_INFO "SWAP_POLICY: Prefetch pages: %llu\n", swap_policy_prefetch_pages);
}

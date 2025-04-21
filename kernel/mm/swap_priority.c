/**
 * swap_priority.c - Horizon kernel swap prioritization implementation
 * 
 * This file contains the implementation of swap prioritization.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/vmm.h>
#include <horizon/mm/pmm.h>
#include <horizon/mm/page.h>
#include <horizon/mm/swap.h>
#include <horizon/mm/swap_priority.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/task.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Priority statistics */
static u64 priority_scan_count = 0;
static u64 priority_scan_pages = 0;
static u64 priority_high_count = 0;
static u64 priority_medium_count = 0;
static u64 priority_low_count = 0;

/* Priority lock */
static spinlock_t priority_lock = SPIN_LOCK_INITIALIZER;

/* Current priority algorithm */
static swap_priority_algo_t current_algo = SWAP_PRIORITY_ACCESS;

/**
 * Initialize the swap prioritization subsystem
 */
void swap_priority_init(void) {
    /* Reset statistics */
    priority_scan_count = 0;
    priority_scan_pages = 0;
    priority_high_count = 0;
    priority_medium_count = 0;
    priority_low_count = 0;
    
    /* Set the default algorithm */
    current_algo = SWAP_PRIORITY_ACCESS;
    
    printk(KERN_INFO "SWAP_PRIORITY: Initialized swap prioritization subsystem\n");
}

/**
 * Set the prioritization algorithm
 * 
 * @param algo Algorithm to set
 * @return 0 on success, negative error code on failure
 */
int swap_priority_set_algo(swap_priority_algo_t algo) {
    /* Check parameters */
    if (algo < SWAP_PRIORITY_NONE || algo > SWAP_PRIORITY_CUSTOM) {
        return -EINVAL;
    }
    
    /* Lock the priority */
    spin_lock(&priority_lock);
    
    /* Set the algorithm */
    current_algo = algo;
    
    /* Unlock the priority */
    spin_unlock(&priority_lock);
    
    printk(KERN_INFO "SWAP_PRIORITY: Set prioritization algorithm to %d\n", algo);
    
    return 0;
}

/**
 * Get the prioritization algorithm
 * 
 * @return Current prioritization algorithm
 */
swap_priority_algo_t swap_priority_get_algo(void) {
    return current_algo;
}

/**
 * Get the priority of a page
 * 
 * @param task Task that owns the page
 * @param addr Address of the page
 * @return Priority of the page
 */
swap_priority_t swap_priority_get(task_struct_t *task, u32 addr) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL) {
        return SWAP_PRIORITY_LOW;
    }
    
    /* Align the address to a page boundary */
    addr &= ~(PAGE_SIZE - 1);
    
    /* Get the page */
    page_t *page = vmm_get_page(task->mm, addr);
    
    /* Check if the page exists */
    if (page == NULL) {
        return SWAP_PRIORITY_LOW;
    }
    
    /* Get the priority based on the algorithm */
    switch (current_algo) {
        case SWAP_PRIORITY_NONE:
            /* No prioritization */
            return SWAP_PRIORITY_MEDIUM;
        
        case SWAP_PRIORITY_ACCESS:
            /* Prioritize based on access time */
            return swap_priority_access(task, addr, page);
        
        case SWAP_PRIORITY_TYPE:
            /* Prioritize based on page type */
            return swap_priority_type(task, addr, page);
        
        case SWAP_PRIORITY_CUSTOM:
            /* Custom prioritization */
            return swap_priority_custom(task, addr, page);
        
        default:
            /* Unknown algorithm */
            return SWAP_PRIORITY_MEDIUM;
    }
}

/**
 * Prioritize a page based on access time
 * 
 * @param task Task that owns the page
 * @param addr Address of the page
 * @param page Page to prioritize
 * @return Priority of the page
 */
swap_priority_t swap_priority_access(task_struct_t *task, u32 addr, page_t *page) {
    /* Check if the page has been accessed recently */
    if (page_test_flags(page, (1 << PG_accessed))) {
        /* Page has been accessed recently */
        return SWAP_PRIORITY_HIGH;
    }
    
    /* Check if the page has been modified */
    if (page_test_flags(page, (1 << PG_dirty))) {
        /* Page has been modified */
        return SWAP_PRIORITY_MEDIUM;
    }
    
    /* Page has not been accessed or modified recently */
    return SWAP_PRIORITY_LOW;
}

/**
 * Prioritize a page based on page type
 * 
 * @param task Task that owns the page
 * @param addr Address of the page
 * @param page Page to prioritize
 * @return Priority of the page
 */
swap_priority_t swap_priority_type(task_struct_t *task, u32 addr, page_t *page) {
    /* Find the virtual memory area */
    vm_area_struct_t *vma = vmm_find_vma(task->mm, addr);
    
    /* Check if the virtual memory area was found */
    if (vma == NULL) {
        return SWAP_PRIORITY_LOW;
    }
    
    /* Check if the page is in the stack */
    if (addr >= task->mm->start_stack && addr < task->mm->start_stack + THREAD_STACK_SIZE) {
        /* Stack page */
        return SWAP_PRIORITY_HIGH;
    }
    
    /* Check if the page is in the heap */
    if (addr >= task->mm->start_brk && addr < task->mm->brk) {
        /* Heap page */
        return SWAP_PRIORITY_MEDIUM;
    }
    
    /* Check if the page is in the code segment */
    if (addr >= task->mm->start_code && addr < task->mm->end_code) {
        /* Code page */
        return SWAP_PRIORITY_LOW;
    }
    
    /* Check if the page is in the data segment */
    if (addr >= task->mm->start_data && addr < task->mm->end_data) {
        /* Data page */
        return SWAP_PRIORITY_MEDIUM;
    }
    
    /* Unknown page type */
    return SWAP_PRIORITY_LOW;
}

/**
 * Prioritize a page using a custom algorithm
 * 
 * @param task Task that owns the page
 * @param addr Address of the page
 * @param page Page to prioritize
 * @return Priority of the page
 */
swap_priority_t swap_priority_custom(task_struct_t *task, u32 addr, page_t *page) {
    /* This is a placeholder for a custom prioritization algorithm */
    /* In a real implementation, this would use a more sophisticated algorithm */
    
    /* Combine access time and page type */
    swap_priority_t access_priority = swap_priority_access(task, addr, page);
    swap_priority_t type_priority = swap_priority_type(task, addr, page);
    
    /* Return the higher priority */
    return (access_priority > type_priority) ? access_priority : type_priority;
}

/**
 * Scan for high priority pages
 * 
 * @param task Task to scan
 * @param count Number of pages to scan
 * @return Number of high priority pages found
 */
int swap_priority_scan_high(task_struct_t *task, u32 count) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL || count == 0) {
        return 0;
    }
    
    /* Lock the priority */
    spin_lock(&priority_lock);
    
    /* Increment the scan count */
    priority_scan_count++;
    priority_scan_pages += count;
    
    /* Unlock the priority */
    spin_unlock(&priority_lock);
    
    /* Initialize the page count */
    u32 found = 0;
    
    /* Scan the virtual memory areas */
    vm_area_struct_t *vma;
    for (vma = task->mm->mmap; vma != NULL && found < count; vma = vma->vm_list.next) {
        /* Skip non-swappable areas */
        if (vma->vm_flags & VM_LOCKED) {
            continue;
        }
        
        /* Scan the pages in the area */
        for (u32 addr = vma->vm_start; addr < vma->vm_end && found < count; addr += PAGE_SIZE) {
            /* Get the page */
            page_t *page = vmm_get_page(task->mm, addr);
            
            /* Check if the page exists and is swappable */
            if (page != NULL && !page_test_flags(page, (1 << PG_locked))) {
                /* Get the priority */
                swap_priority_t priority = swap_priority_get(task, addr);
                
                /* Check if the page is high priority */
                if (priority == SWAP_PRIORITY_HIGH) {
                    /* Increment the high priority count */
                    spin_lock(&priority_lock);
                    priority_high_count++;
                    spin_unlock(&priority_lock);
                    
                    found++;
                }
            }
        }
    }
    
    return found;
}

/**
 * Scan for medium priority pages
 * 
 * @param task Task to scan
 * @param count Number of pages to scan
 * @return Number of medium priority pages found
 */
int swap_priority_scan_medium(task_struct_t *task, u32 count) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL || count == 0) {
        return 0;
    }
    
    /* Lock the priority */
    spin_lock(&priority_lock);
    
    /* Increment the scan count */
    priority_scan_count++;
    priority_scan_pages += count;
    
    /* Unlock the priority */
    spin_unlock(&priority_lock);
    
    /* Initialize the page count */
    u32 found = 0;
    
    /* Scan the virtual memory areas */
    vm_area_struct_t *vma;
    for (vma = task->mm->mmap; vma != NULL && found < count; vma = vma->vm_list.next) {
        /* Skip non-swappable areas */
        if (vma->vm_flags & VM_LOCKED) {
            continue;
        }
        
        /* Scan the pages in the area */
        for (u32 addr = vma->vm_start; addr < vma->vm_end && found < count; addr += PAGE_SIZE) {
            /* Get the page */
            page_t *page = vmm_get_page(task->mm, addr);
            
            /* Check if the page exists and is swappable */
            if (page != NULL && !page_test_flags(page, (1 << PG_locked))) {
                /* Get the priority */
                swap_priority_t priority = swap_priority_get(task, addr);
                
                /* Check if the page is medium priority */
                if (priority == SWAP_PRIORITY_MEDIUM) {
                    /* Increment the medium priority count */
                    spin_lock(&priority_lock);
                    priority_medium_count++;
                    spin_unlock(&priority_lock);
                    
                    found++;
                }
            }
        }
    }
    
    return found;
}

/**
 * Scan for low priority pages
 * 
 * @param task Task to scan
 * @param count Number of pages to scan
 * @return Number of low priority pages found
 */
int swap_priority_scan_low(task_struct_t *task, u32 count) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL || count == 0) {
        return 0;
    }
    
    /* Lock the priority */
    spin_lock(&priority_lock);
    
    /* Increment the scan count */
    priority_scan_count++;
    priority_scan_pages += count;
    
    /* Unlock the priority */
    spin_unlock(&priority_lock);
    
    /* Initialize the page count */
    u32 found = 0;
    
    /* Scan the virtual memory areas */
    vm_area_struct_t *vma;
    for (vma = task->mm->mmap; vma != NULL && found < count; vma = vma->vm_list.next) {
        /* Skip non-swappable areas */
        if (vma->vm_flags & VM_LOCKED) {
            continue;
        }
        
        /* Scan the pages in the area */
        for (u32 addr = vma->vm_start; addr < vma->vm_end && found < count; addr += PAGE_SIZE) {
            /* Get the page */
            page_t *page = vmm_get_page(task->mm, addr);
            
            /* Check if the page exists and is swappable */
            if (page != NULL && !page_test_flags(page, (1 << PG_locked))) {
                /* Get the priority */
                swap_priority_t priority = swap_priority_get(task, addr);
                
                /* Check if the page is low priority */
                if (priority == SWAP_PRIORITY_LOW) {
                    /* Increment the low priority count */
                    spin_lock(&priority_lock);
                    priority_low_count++;
                    spin_unlock(&priority_lock);
                    
                    found++;
                }
            }
        }
    }
    
    return found;
}

/**
 * Print priority statistics
 */
void swap_priority_print_stats(void) {
    /* Print the statistics */
    printk(KERN_INFO "SWAP_PRIORITY: Current algorithm: %d\n", current_algo);
    printk(KERN_INFO "SWAP_PRIORITY: Scan count: %llu\n", priority_scan_count);
    printk(KERN_INFO "SWAP_PRIORITY: Scan pages: %llu\n", priority_scan_pages);
    printk(KERN_INFO "SWAP_PRIORITY: High priority pages: %llu\n", priority_high_count);
    printk(KERN_INFO "SWAP_PRIORITY: Medium priority pages: %llu\n", priority_medium_count);
    printk(KERN_INFO "SWAP_PRIORITY: Low priority pages: %llu\n", priority_low_count);
}

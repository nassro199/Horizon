/**
 * mm.c - Horizon kernel memory management implementation
 *
 * This file contains the implementation of the memory management subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/pmm.h>
#include <horizon/mm/vmm.h>
#include <horizon/mm/page.h>
#include <horizon/mm/swap.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Initialize the memory management subsystem
 */
void mm_init(void) {
    /* Initialize the physical memory manager */
    pmm_init();

    /* Initialize the virtual memory manager */
    vmm_init();

    /* Initialize the page fault handler */
    page_fault_init();

    /* Initialize the swap subsystem */
    swap_init();

    /* Initialize the swap policy subsystem */
    swap_policy_init();

    /* Initialize the swap compression subsystem */
    swap_compress_init();

    /* Initialize the swap prioritization subsystem */
    swap_priority_init();

    /* Initialize the swap monitoring subsystem */
    swap_monitor_init();

    /* Initialize the TLB management */
    tlb_init();

    /* Initialize the cache management */
    cache_init();

    /* Initialize the cache coherency subsystem */
    cache_coherency_init();

    /* Initialize the NUMA subsystem */
    numa_init();

    /* Initialize the memory migration subsystem */
    memory_migration_init();

    printk(KERN_INFO "MM: Initialized memory management subsystem\n");
}

/**
 * Allocate pages
 *
 * @param count Number of pages to allocate
 * @param flags Allocation flags
 * @return Pointer to the allocated pages, or NULL on failure
 */
void *mm_alloc_pages(u32 count, u32 flags) {
    /* Allocate pages */
    page_t *page = page_alloc(count);

    if (page == NULL) {
        return NULL;
    }

    /* Convert the page to a virtual address */
    void *addr = pmm_page_to_virt(page);

    /* Zero the memory if requested */
    if (flags & MEM_ZERO) {
        memset(addr, 0, count * PAGE_SIZE);
    }

    return addr;
}

/**
 * Free pages
 *
 * @param addr Address of the pages to free
 * @param count Number of pages to free
 */
void mm_free_pages(void *addr, u32 count) {
    /* Convert the virtual address to a page */
    page_t *page = pmm_virt_to_page(addr);

    /* Free the pages */
    page_free(page, count);
}

/**
 * Allocate kernel memory
 *
 * @param size Size to allocate
 * @param flags Allocation flags
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *kmalloc(size_t size, u32 flags) {
    /* Calculate the number of pages needed */
    u32 count = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Allocate pages */
    void *addr = mm_alloc_pages(count, flags);

    if (addr == NULL) {
        return NULL;
    }

    return addr;
}

/**
 * Free kernel memory
 *
 * @param addr Address to free
 */
void kfree(void *addr) {
    /* Free the pages */
    mm_free_pages(addr, 1);
}

/**
 * Allocate virtual memory
 *
 * @param size Size to allocate
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *vmalloc(size_t size) {
    /* Calculate the number of pages needed */
    u32 count = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Allocate pages */
    void *addr = mm_alloc_pages(count, MEM_KERNEL);

    if (addr == NULL) {
        return NULL;
    }

    return addr;
}

/**
 * Free virtual memory
 *
 * @param addr Address to free
 */
void vfree(void *addr) {
    /* Free the pages */
    mm_free_pages(addr, 1);
}

/**
 * memory.c - Memory management implementation
 * 
 * This file contains the implementation of the memory management subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>

/* Memory management state */
static void *mem_start;
static void *mem_end;
static u32 total_pages;
static u32 free_pages;

/* Page bitmap - one bit per page */
static u8 *page_bitmap;

/* Initialize memory management */
void mm_init(void) {
    /* This would be initialized with information from the bootloader */
    /* For now, we'll just use placeholder values */
    mem_start = (void *)0x100000;    /* 1MB */
    mem_end = (void *)0x1000000;     /* 16MB */
    
    total_pages = ((u32)mem_end - (u32)mem_start) / PAGE_SIZE;
    free_pages = total_pages;
    
    /* Allocate page bitmap */
    u32 bitmap_size = (total_pages + 7) / 8;
    page_bitmap = (u8 *)mem_start;
    
    /* Mark all pages as free */
    for (u32 i = 0; i < bitmap_size; i++) {
        page_bitmap[i] = 0;
    }
    
    /* Mark bitmap pages as used */
    u32 bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (u32 i = 0; i < bitmap_pages; i++) {
        page_bitmap[i / 8] |= (1 << (i % 8));
    }
    
    free_pages -= bitmap_pages;
}

/* Allocate pages */
void *mm_alloc_pages(u32 count, u32 flags) {
    if (count == 0) {
        return NULL;
    }
    
    if (count > free_pages) {
        return NULL;
    }
    
    /* Find a contiguous block of free pages */
    u32 start_page = 0;
    u32 found_pages = 0;
    
    for (u32 i = 0; i < total_pages; i++) {
        if (page_bitmap[i / 8] & (1 << (i % 8))) {
            /* Page is used */
            start_page = i + 1;
            found_pages = 0;
        } else {
            /* Page is free */
            found_pages++;
            
            if (found_pages == count) {
                /* Found enough contiguous pages */
                /* Mark pages as used */
                for (u32 j = 0; j < count; j++) {
                    u32 page = start_page + j;
                    page_bitmap[page / 8] |= (1 << (page % 8));
                }
                
                free_pages -= count;
                
                /* Return the start address */
                return (void *)((u32)mem_start + start_page * PAGE_SIZE);
            }
        }
    }
    
    /* Not enough contiguous pages */
    return NULL;
}

/* Free pages */
void mm_free_pages(void *addr, u32 count) {
    if (addr == NULL || count == 0) {
        return;
    }
    
    /* Calculate the page index */
    u32 page = ((u32)addr - (u32)mem_start) / PAGE_SIZE;
    
    /* Mark pages as free */
    for (u32 i = 0; i < count; i++) {
        u32 p = page + i;
        
        if (p < total_pages) {
            if (page_bitmap[p / 8] & (1 << (p % 8))) {
                /* Page was used */
                page_bitmap[p / 8] &= ~(1 << (p % 8));
                free_pages++;
            }
        }
    }
}

/* Simple kernel memory allocator */
void *kmalloc(size_t size, u32 flags) {
    if (size == 0) {
        return NULL;
    }
    
    /* Round up to page size */
    u32 pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    /* Allocate pages */
    void *addr = mm_alloc_pages(pages, flags);
    
    /* Zero memory if requested */
    if (addr != NULL && (flags & MEM_ZERO)) {
        u8 *p = (u8 *)addr;
        for (u32 i = 0; i < pages * PAGE_SIZE; i++) {
            p[i] = 0;
        }
    }
    
    return addr;
}

/* Free kernel memory */
void kfree(void *addr) {
    if (addr == NULL) {
        return;
    }
    
    /* For this simple implementation, we'll just free one page */
    /* A real implementation would keep track of allocation sizes */
    mm_free_pages(addr, 1);
}

/* Virtual memory allocator */
void *vmalloc(size_t size) {
    /* For this simple implementation, vmalloc is the same as kmalloc */
    return kmalloc(size, MEM_KERNEL);
}

/* Free virtual memory */
void vfree(void *addr) {
    /* For this simple implementation, vfree is the same as kfree */
    kfree(addr);
}

/**
 * pmm.c - Horizon kernel physical memory manager implementation
 * 
 * This file contains the implementation of the physical memory manager.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/pmm.h>
#include <horizon/mm/page.h>
#include <horizon/spinlock.h>
#include <horizon/list.h>
#include <horizon/string.h>
#include <horizon/printk.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Memory map entry structure */
typedef struct memory_map_entry {
    u64 base;       /* Base address */
    u64 length;     /* Length in bytes */
    u32 type;       /* Entry type */
    u32 reserved;   /* Reserved */
} memory_map_entry_t;

/* Memory map entry types */
#define MEMORY_MAP_AVAILABLE      1
#define MEMORY_MAP_RESERVED       2
#define MEMORY_MAP_ACPI_RECLAIM   3
#define MEMORY_MAP_ACPI_NVS       4
#define MEMORY_MAP_BAD_MEMORY     5

/* Memory map */
static memory_map_entry_t *memory_map = NULL;
static u32 memory_map_entries = 0;

/* Memory zones */
static zone_t zones[MAX_NR_ZONES];

/* Memory node */
static pglist_data_t pgdat;

/* Page frame array */
static page_t *page_frames = NULL;
static u32 page_frames_count = 0;

/* Buddy system free lists */
static list_head_t free_area[11];

/* Physical memory manager lock */
static spinlock_t pmm_lock = SPIN_LOCK_INITIALIZER;

/* Physical memory statistics */
static u64 total_pages = 0;
static u64 free_pages = 0;
static u64 reserved_pages = 0;

/**
 * Initialize the physical memory manager
 */
void pmm_init(void) {
    /* Initialize the memory zones */
    for (int i = 0; i < MAX_NR_ZONES; i++) {
        memset(&zones[i], 0, sizeof(zone_t));
        spin_lock_init(&zones[i].lock);
        
        for (int j = 0; j < 11; j++) {
            list_init(&zones[i].free_area[j]);
        }
    }
    
    /* Set zone names */
    zones[ZONE_DMA].name = "DMA";
    zones[ZONE_NORMAL].name = "Normal";
    zones[ZONE_HIGHMEM].name = "HighMem";
    
    /* Initialize the memory node */
    memset(&pgdat, 0, sizeof(pglist_data_t));
    pgdat.node_id = 0;
    pgdat.node_zones[ZONE_DMA] = zones[ZONE_DMA];
    pgdat.node_zones[ZONE_NORMAL] = zones[ZONE_NORMAL];
    pgdat.node_zones[ZONE_HIGHMEM] = zones[ZONE_HIGHMEM];
    
    /* Initialize the buddy system free lists */
    for (int i = 0; i < 11; i++) {
        list_init(&free_area[i]);
    }
    
    /* Get the memory map from the bootloader */
    /* This would be implemented with actual memory map getting */
    /* For now, just create a simple memory map */
    memory_map_entries = 3;
    memory_map = kmalloc(sizeof(memory_map_entry_t) * memory_map_entries, MEM_KERNEL | MEM_ZERO);
    
    if (memory_map == NULL) {
        printk(KERN_ERR "PMM: Failed to allocate memory map\n");
        return;
    }
    
    /* Create a simple memory map */
    memory_map[0].base = 0;
    memory_map[0].length = 0x100000; /* 1 MB */
    memory_map[0].type = MEMORY_MAP_RESERVED;
    
    memory_map[1].base = 0x100000;
    memory_map[1].length = 0x1F00000; /* 31 MB */
    memory_map[1].type = MEMORY_MAP_AVAILABLE;
    
    memory_map[2].base = 0x2000000;
    memory_map[2].length = 0x2000000; /* 32 MB */
    memory_map[2].type = MEMORY_MAP_AVAILABLE;
    
    /* Calculate the total memory */
    u64 total_memory = 0;
    
    for (u32 i = 0; i < memory_map_entries; i++) {
        if (memory_map[i].type == MEMORY_MAP_AVAILABLE) {
            total_memory += memory_map[i].length;
        }
    }
    
    /* Calculate the number of pages */
    page_frames_count = total_memory / PAGE_SIZE;
    
    /* Allocate the page frame array */
    page_frames = kmalloc(sizeof(page_t) * page_frames_count, MEM_KERNEL | MEM_ZERO);
    
    if (page_frames == NULL) {
        printk(KERN_ERR "PMM: Failed to allocate page frame array\n");
        return;
    }
    
    /* Initialize the page frame array */
    for (u32 i = 0; i < page_frames_count; i++) {
        page_frames[i].flags = 0;
        atomic_set(&page_frames[i].count, 0);
        atomic_set(&page_frames[i].mapcount, 0);
        page_frames[i].order = 0;
        list_init(&page_frames[i].list);
        page_frames[i].mapping = NULL;
        page_frames[i].index = 0;
        list_init(&page_frames[i].lru);
        page_frames[i].virtual = NULL;
        page_frames[i].private = NULL;
    }
    
    /* Initialize the memory map */
    for (u32 i = 0; i < memory_map_entries; i++) {
        if (memory_map[i].type == MEMORY_MAP_AVAILABLE) {
            pmm_init_memmap(memory_map[i].base / PAGE_SIZE, (memory_map[i].base + memory_map[i].length) / PAGE_SIZE);
        } else {
            pmm_reserve_range(memory_map[i].base / PAGE_SIZE, (memory_map[i].base + memory_map[i].length) / PAGE_SIZE);
        }
    }
    
    /* Print memory information */
    printk(KERN_INFO "PMM: Total memory: %llu MB\n", total_memory / (1024 * 1024));
    printk(KERN_INFO "PMM: Total pages: %llu\n", total_pages);
    printk(KERN_INFO "PMM: Free pages: %llu\n", free_pages);
    printk(KERN_INFO "PMM: Reserved pages: %llu\n", reserved_pages);
}

/**
 * Initialize the memory map
 * 
 * @param start_pfn Start page frame number
 * @param end_pfn End page frame number
 */
void pmm_init_memmap(unsigned long start_pfn, unsigned long end_pfn) {
    /* Check parameters */
    if (start_pfn >= end_pfn) {
        return;
    }
    
    /* Initialize the memory map */
    for (unsigned long pfn = start_pfn; pfn < end_pfn; pfn++) {
        /* Check if the page frame is valid */
        if (pfn >= page_frames_count) {
            break;
        }
        
        /* Initialize the page frame */
        page_t *page = &page_frames[pfn];
        
        /* Clear the page flags */
        page->flags = 0;
        
        /* Set the page as free */
        page_set_flags(page, (1 << PG_buddy));
        
        /* Add the page to the free list */
        list_add(&page->list, &free_area[0]);
        
        /* Increment the free pages counter */
        free_pages++;
        total_pages++;
    }
}

/**
 * Free a range of page frames
 * 
 * @param start_pfn Start page frame number
 * @param end_pfn End page frame number
 */
void pmm_free_range(unsigned long start_pfn, unsigned long end_pfn) {
    /* Check parameters */
    if (start_pfn >= end_pfn) {
        return;
    }
    
    /* Free the range */
    for (unsigned long pfn = start_pfn; pfn < end_pfn; pfn++) {
        /* Check if the page frame is valid */
        if (pfn >= page_frames_count) {
            break;
        }
        
        /* Get the page frame */
        page_t *page = &page_frames[pfn];
        
        /* Check if the page is reserved */
        if (page_test_flags(page, (1 << PG_reserved))) {
            /* Clear the reserved flag */
            page_clear_flags(page, (1 << PG_reserved));
            
            /* Decrement the reserved pages counter */
            reserved_pages--;
            
            /* Set the page as free */
            page_set_flags(page, (1 << PG_buddy));
            
            /* Add the page to the free list */
            list_add(&page->list, &free_area[0]);
            
            /* Increment the free pages counter */
            free_pages++;
        }
    }
}

/**
 * Reserve a range of page frames
 * 
 * @param start_pfn Start page frame number
 * @param end_pfn End page frame number
 */
void pmm_reserve_range(unsigned long start_pfn, unsigned long end_pfn) {
    /* Check parameters */
    if (start_pfn >= end_pfn) {
        return;
    }
    
    /* Reserve the range */
    for (unsigned long pfn = start_pfn; pfn < end_pfn; pfn++) {
        /* Check if the page frame is valid */
        if (pfn >= page_frames_count) {
            break;
        }
        
        /* Get the page frame */
        page_t *page = &page_frames[pfn];
        
        /* Check if the page is free */
        if (page_test_flags(page, (1 << PG_buddy))) {
            /* Clear the buddy flag */
            page_clear_flags(page, (1 << PG_buddy));
            
            /* Remove the page from the free list */
            list_del(&page->list);
            
            /* Decrement the free pages counter */
            free_pages--;
        }
        
        /* Set the page as reserved */
        page_set_flags(page, (1 << PG_reserved));
        
        /* Increment the reserved pages counter */
        reserved_pages++;
    }
}

/**
 * Allocate pages
 * 
 * @param order Page order
 * @param flags Allocation flags
 * @return Pointer to the allocated page, or NULL on failure
 */
page_t *pmm_alloc_pages(unsigned int order, unsigned int flags) {
    /* Check parameters */
    if (order > 10) {
        return NULL;
    }
    
    /* Lock the physical memory manager */
    spin_lock(&pmm_lock);
    
    /* Find a free block of the requested size */
    page_t *page = NULL;
    unsigned int current_order = order;
    
    while (current_order <= 10) {
        /* Check if there are free blocks of the current order */
        if (!list_empty(&free_area[current_order])) {
            /* Get the first free block */
            page = list_entry(free_area[current_order].next, page_t, list);
            
            /* Remove the block from the free list */
            list_del(&page->list);
            
            /* Clear the buddy flag */
            page_clear_flags(page, (1 << PG_buddy));
            
            /* Split the block if necessary */
            while (current_order > order) {
                /* Decrement the order */
                current_order--;
                
                /* Calculate the buddy page */
                page_t *buddy = &page_frames[pmm_page_to_pfn(page) + (1 << current_order)];
                
                /* Set the buddy as free */
                page_set_flags(buddy, (1 << PG_buddy));
                
                /* Add the buddy to the free list */
                list_add(&buddy->list, &free_area[current_order]);
            }
            
            /* Set the page order */
            page->order = order;
            
            /* Decrement the free pages counter */
            free_pages -= (1 << order);
            
            break;
        }
        
        /* Try the next order */
        current_order++;
    }
    
    /* Unlock the physical memory manager */
    spin_unlock(&pmm_lock);
    
    /* Return the allocated page */
    return page;
}

/**
 * Free pages
 * 
 * @param page Page to free
 * @param order Page order
 */
void pmm_free_pages(page_t *page, unsigned int order) {
    /* Check parameters */
    if (page == NULL || order > 10) {
        return;
    }
    
    /* Lock the physical memory manager */
    spin_lock(&pmm_lock);
    
    /* Get the page frame number */
    unsigned long pfn = pmm_page_to_pfn(page);
    
    /* Set the page as free */
    page_set_flags(page, (1 << PG_buddy));
    
    /* Set the page order */
    page->order = order;
    
    /* Increment the free pages counter */
    free_pages += (1 << order);
    
    /* Try to merge with buddies */
    while (order < 10) {
        /* Calculate the buddy page frame number */
        unsigned long buddy_pfn = pfn ^ (1 << order);
        
        /* Check if the buddy is valid */
        if (buddy_pfn >= page_frames_count) {
            break;
        }
        
        /* Get the buddy page */
        page_t *buddy = &page_frames[buddy_pfn];
        
        /* Check if the buddy is free and has the same order */
        if (!page_test_flags(buddy, (1 << PG_buddy)) || buddy->order != order) {
            break;
        }
        
        /* Remove the buddy from the free list */
        list_del(&buddy->list);
        
        /* Clear the buddy flag */
        page_clear_flags(buddy, (1 << PG_buddy));
        
        /* Merge with the buddy */
        if (buddy_pfn < pfn) {
            pfn = buddy_pfn;
            page = buddy;
        }
        
        /* Increment the order */
        order++;
        
        /* Set the new order */
        page->order = order;
    }
    
    /* Add the page to the free list */
    list_add(&page->list, &free_area[order]);
    
    /* Unlock the physical memory manager */
    spin_unlock(&pmm_lock);
}

/**
 * Get the number of free pages
 * 
 * @return Number of free pages
 */
unsigned long pmm_get_free_pages(void) {
    return free_pages;
}

/**
 * Get the total number of pages
 * 
 * @return Total number of pages
 */
unsigned long pmm_get_total_pages(void) {
    return total_pages;
}

/**
 * Get the number of reserved pages
 * 
 * @return Number of reserved pages
 */
unsigned long pmm_get_reserved_pages(void) {
    return reserved_pages;
}

/**
 * Get the number of used pages
 * 
 * @return Number of used pages
 */
unsigned long pmm_get_used_pages(void) {
    return total_pages - free_pages;
}

/**
 * Allocate a single page
 * 
 * @param flags Allocation flags
 * @return Pointer to the allocated page, or NULL on failure
 */
void *pmm_alloc_page(unsigned int flags) {
    /* Allocate a page */
    page_t *page = pmm_alloc_pages(0, flags);
    
    if (page == NULL) {
        return NULL;
    }
    
    /* Return the page address */
    return pmm_page_to_virt(page);
}

/**
 * Free a single page
 * 
 * @param page Page to free
 */
void pmm_free_page(void *page) {
    /* Check parameters */
    if (page == NULL) {
        return;
    }
    
    /* Free the page */
    pmm_free_pages(pmm_virt_to_page(page), 0);
}

/**
 * Convert a page to a virtual address
 * 
 * @param page Page to convert
 * @return Virtual address of the page
 */
void *pmm_page_to_virt(page_t *page) {
    /* Check parameters */
    if (page == NULL) {
        return NULL;
    }
    
    /* Calculate the virtual address */
    return (void *)(pmm_page_to_pfn(page) * PAGE_SIZE + 0xC0000000);
}

/**
 * Convert a virtual address to a page
 * 
 * @param addr Virtual address to convert
 * @return Page corresponding to the virtual address
 */
page_t *pmm_virt_to_page(void *addr) {
    /* Check parameters */
    if (addr == NULL) {
        return NULL;
    }
    
    /* Calculate the page frame number */
    unsigned long pfn = ((unsigned long)addr - 0xC0000000) / PAGE_SIZE;
    
    /* Check if the page frame number is valid */
    if (pfn >= page_frames_count) {
        return NULL;
    }
    
    /* Return the page */
    return &page_frames[pfn];
}

/**
 * Convert a virtual address to a physical address
 * 
 * @param addr Virtual address to convert
 * @return Physical address corresponding to the virtual address
 */
unsigned long pmm_virt_to_phys(void *addr) {
    /* Check parameters */
    if (addr == NULL) {
        return 0;
    }
    
    /* Calculate the physical address */
    return (unsigned long)addr - 0xC0000000;
}

/**
 * Convert a physical address to a virtual address
 * 
 * @param addr Physical address to convert
 * @return Virtual address corresponding to the physical address
 */
void *pmm_phys_to_virt(unsigned long addr) {
    /* Calculate the virtual address */
    return (void *)(addr + 0xC0000000);
}

/**
 * Check if an address is in low memory
 * 
 * @param addr Address to check
 * @return 1 if the address is in low memory, 0 otherwise
 */
int pmm_is_low_mem(void *addr) {
    /* Check parameters */
    if (addr == NULL) {
        return 0;
    }
    
    /* Check if the address is in low memory */
    return (unsigned long)addr < 0xC0000000;
}

/**
 * Check if an address is in high memory
 * 
 * @param addr Address to check
 * @return 1 if the address is in high memory, 0 otherwise
 */
int pmm_is_high_mem(void *addr) {
    /* Check parameters */
    if (addr == NULL) {
        return 0;
    }
    
    /* Check if the address is in high memory */
    return (unsigned long)addr >= 0xC0000000;
}

/**
 * Get the zone of a page
 * 
 * @param page Page to get the zone of
 * @return Zone of the page
 */
zone_t *pmm_page_zone(page_t *page) {
    /* Check parameters */
    if (page == NULL) {
        return NULL;
    }
    
    /* Get the page frame number */
    unsigned long pfn = pmm_page_to_pfn(page);
    
    /* Determine the zone */
    if (pfn < 16 * 1024 * 1024 / PAGE_SIZE) {
        return &zones[ZONE_DMA];
    } else if (pfn < 896 * 1024 * 1024 / PAGE_SIZE) {
        return &zones[ZONE_NORMAL];
    } else {
        return &zones[ZONE_HIGHMEM];
    }
}

/**
 * Get the node of a page
 * 
 * @param page Page to get the node of
 * @return Node of the page
 */
pglist_data_t *pmm_page_pgdat(page_t *page) {
    /* Check parameters */
    if (page == NULL) {
        return NULL;
    }
    
    /* Return the node */
    return &pgdat;
}

/**
 * Convert a page to a page frame number
 * 
 * @param page Page to convert
 * @return Page frame number of the page
 */
unsigned long pmm_page_to_pfn(page_t *page) {
    /* Check parameters */
    if (page == NULL) {
        return 0;
    }
    
    /* Calculate the page frame number */
    return page - page_frames;
}

/**
 * Convert a page frame number to a page
 * 
 * @param pfn Page frame number to convert
 * @return Page corresponding to the page frame number
 */
page_t *pmm_pfn_to_page(unsigned long pfn) {
    /* Check if the page frame number is valid */
    if (pfn >= page_frames_count) {
        return NULL;
    }
    
    /* Return the page */
    return &page_frames[pfn];
}

/**
 * pmm.h - Horizon kernel physical memory management definitions
 * 
 * This file contains definitions for the physical memory management subsystem.
 * The definitions are compatible with Linux.
 */

#ifndef _HORIZON_MM_PMM_H
#define _HORIZON_MM_PMM_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/mm/page.h>

/* Memory zone types */
#define ZONE_DMA        0
#define ZONE_NORMAL     1
#define ZONE_HIGHMEM    2
#define MAX_NR_ZONES    3

/* Memory zone flags */
#define ZONE_RECLAIM_LOCKED      (1 << 0)    /* Zone is locked for reclaim */
#define ZONE_OOM_LOCKED          (1 << 1)    /* Zone is locked for OOM */
#define ZONE_CONGESTED           (1 << 2)    /* Zone is congested */
#define ZONE_DIRTY               (1 << 3)    /* Zone has dirty pages */
#define ZONE_WRITEBACK           (1 << 4)    /* Zone has pages under writeback */
#define ZONE_RECLAIM_ACTIVE      (1 << 5)    /* Zone is being reclaimed */

/* Memory zone structure */
typedef struct zone {
    unsigned long flags;                  /* Zone flags */
    unsigned long watermark[3];           /* Zone watermarks */
    unsigned long nr_pages;               /* Number of pages in zone */
    unsigned long nr_free_pages;          /* Number of free pages in zone */
    unsigned long nr_active_pages;        /* Number of active pages in zone */
    unsigned long nr_inactive_pages;      /* Number of inactive pages in zone */
    unsigned long nr_dirty_pages;         /* Number of dirty pages in zone */
    unsigned long nr_writeback_pages;     /* Number of writeback pages in zone */
    unsigned long nr_slab_pages;          /* Number of slab pages in zone */
    unsigned long nr_isolated_pages;      /* Number of isolated pages in zone */
    unsigned long nr_unevictable_pages;   /* Number of unevictable pages in zone */
    unsigned long nr_mlock_pages;         /* Number of mlocked pages in zone */
    unsigned long nr_shmem_pages;         /* Number of shmem pages in zone */
    unsigned long nr_kernel_stack_pages;  /* Number of kernel stack pages in zone */
    unsigned long nr_pagetable_pages;     /* Number of page table pages in zone */
    unsigned long nr_bounce_pages;        /* Number of bounce pages in zone */
    unsigned long nr_free_cma_pages;      /* Number of free CMA pages in zone */
    unsigned long nr_reserved_pages;      /* Number of reserved pages in zone */
    unsigned long nr_unreclaimable_pages; /* Number of unreclaimable pages in zone */
    unsigned long start_pfn;              /* Start page frame number */
    unsigned long spanned_pages;          /* Number of pages spanned by zone */
    unsigned long present_pages;          /* Number of present pages in zone */
    unsigned long managed_pages;          /* Number of managed pages in zone */
    char *name;                           /* Zone name */
    struct list_head free_area[11];       /* Free areas (buddy system) */
    spinlock_t lock;                      /* Zone lock */
} zone_t;

/* Memory node structure */
typedef struct pglist_data {
    zone_t node_zones[MAX_NR_ZONES];      /* Zones for this node */
    unsigned long node_start_pfn;         /* Start page frame number */
    unsigned long node_present_pages;     /* Number of present pages */
    unsigned long node_spanned_pages;     /* Number of spanned pages */
    int node_id;                          /* Node ID */
    struct pglist_data *pgdat_next;       /* Next node in list */
} pglist_data_t;

/* Physical memory management functions */
void pmm_init(void);
void pmm_init_memmap(unsigned long start_pfn, unsigned long end_pfn);
void pmm_free_range(unsigned long start_pfn, unsigned long end_pfn);
void pmm_reserve_range(unsigned long start_pfn, unsigned long end_pfn);
page_t *pmm_alloc_pages(unsigned int order, unsigned int flags);
void pmm_free_pages(page_t *page, unsigned int order);
unsigned long pmm_get_free_pages(void);
unsigned long pmm_get_total_pages(void);
unsigned long pmm_get_reserved_pages(void);
unsigned long pmm_get_used_pages(void);
void *pmm_alloc_page(unsigned int flags);
void pmm_free_page(void *page);
void *pmm_page_to_virt(page_t *page);
page_t *pmm_virt_to_page(void *addr);
unsigned long pmm_virt_to_phys(void *addr);
void *pmm_phys_to_virt(unsigned long addr);
int pmm_is_low_mem(void *addr);
int pmm_is_high_mem(void *addr);
zone_t *pmm_page_zone(page_t *page);
pglist_data_t *pmm_page_pgdat(page_t *page);
unsigned long pmm_page_to_pfn(page_t *page);
page_t *pmm_pfn_to_page(unsigned long pfn);

#endif /* _HORIZON_MM_PMM_H */

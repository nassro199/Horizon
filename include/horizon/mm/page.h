/**
 * page.h - Horizon kernel page management definitions
 * 
 * This file contains definitions for the page management subsystem.
 * The definitions are compatible with Linux.
 */

#ifndef _KERNEL_MM_PAGE_H
#define _KERNEL_MM_PAGE_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/atomic.h>

/* Page flags */
#define PG_locked       0  /* Page is locked */
#define PG_error        1  /* Page I/O error occurred */
#define PG_referenced   2  /* Page has been referenced recently */
#define PG_uptodate     3  /* Page is up-to-date */
#define PG_dirty        4  /* Page has been written to */
#define PG_lru          5  /* Page is on the LRU list */
#define PG_active       6  /* Page is on the active list */
#define PG_slab         7  /* Page is in the slab allocator */
#define PG_owner_priv_1 8  /* Owner-defined private bit 1 */
#define PG_arch_1       9  /* Architecture-defined private bit */
#define PG_reserved     10 /* Page is reserved */
#define PG_private      11 /* Page has private data */
#define PG_writeback    12 /* Page is being written back */
#define PG_head         13 /* A head page */
#define PG_tail         14 /* A tail page */
#define PG_compound     15 /* A compound page */
#define PG_swapcache    16 /* Page is in the swap cache */
#define PG_mappedtodisk 17 /* Page has been mapped to disk */
#define PG_reclaim      18 /* Page will be reclaimed soon */
#define PG_buddy        19 /* Page is free and in the buddy system */
#define PG_swapbacked   20 /* Page is backed by swap */
#define PG_unevictable  21 /* Page is unevictable */
#define PG_mlocked      22 /* Page is memory locked */

/* Page structure */
typedef struct page {
    unsigned long flags;       /* Page flags */
    atomic_t count;            /* Reference count */
    atomic_t mapcount;         /* Count of page table entries */
    unsigned int order;        /* Order of allocation */
    struct list_head list;     /* List of pages */
    struct address_space *mapping; /* Address space */
    unsigned long index;       /* Page index in mapping */
    struct list_head lru;      /* LRU list */
    void *virtual;             /* Virtual address */
    void *private;             /* Private data */
} page_t;

/* Page table entry */
typedef struct pte {
    unsigned long pte;         /* Page table entry */
} pte_t;

/* Page middle directory entry */
typedef struct pmd {
    unsigned long pmd;         /* Page middle directory entry */
} pmd_t;

/* Page upper directory entry */
typedef struct pud {
    unsigned long pud;         /* Page upper directory entry */
} pud_t;

/* Page global directory entry */
typedef struct pgd {
    unsigned long pgd;         /* Page global directory entry */
} pgd_t;

/* Page table */
typedef struct page_table {
    pte_t *pte;                /* Page table entries */
    unsigned long size;        /* Page table size */
} page_table_t;

/* Page functions */
void page_init(void);
page_t *page_alloc(unsigned int order);
void page_free(page_t *page, unsigned int order);
void *page_address(const page_t *page);
page_t *virt_to_page(const void *addr);
void *page_to_virt(const page_t *page);
void page_set_flags(page_t *page, unsigned long flags);
void page_clear_flags(page_t *page, unsigned long flags);
int page_test_flags(const page_t *page, unsigned long flags);
void page_add_count(page_t *page, int count);
void page_remove_count(page_t *page, int count);
int page_count(const page_t *page);
void page_set_mapping(page_t *page, struct address_space *mapping, unsigned long index);
void page_clear_mapping(page_t *page);
struct address_space *page_mapping(const page_t *page);
unsigned long page_index(const page_t *page);
void page_set_private(page_t *page, void *private);
void *page_private(const page_t *page);

#endif /* _KERNEL_MM_PAGE_H */

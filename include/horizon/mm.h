/**
 * mm.h - Memory management definitions
 * 
 * This file contains definitions for the memory management subsystem.
 */

#ifndef _MM_H
#define _MM_H

#include <horizon/types.h>

/* Page size */
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE - 1))

/* Memory allocation flags */
#define MEM_KERNEL     0x01    /* Kernel memory */
#define MEM_USER       0x02    /* User memory */
#define MEM_DMA        0x04    /* DMA-capable memory */
#define MEM_ZERO       0x08    /* Zero memory */

/* Memory protection flags */
#define MEM_PROT_READ  0x01    /* Readable */
#define MEM_PROT_WRITE 0x02    /* Writable */
#define MEM_PROT_EXEC  0x04    /* Executable */

/* Memory map structure */
typedef struct mm_map {
    void *start;              /* Start address */
    void *end;                /* End address */
    u32 flags;                /* Flags */
    struct mm_map *next;      /* Next map in list */
} mm_map_t;

/* Memory management functions */
void mm_init(void);
void *mm_alloc_pages(u32 count, u32 flags);
void mm_free_pages(void *addr, u32 count);
void *kmalloc(size_t size, u32 flags);
void kfree(void *addr);
void *vmalloc(size_t size);
void vfree(void *addr);

#endif /* _MM_H */

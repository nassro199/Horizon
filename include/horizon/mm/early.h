/**
 * early.h - Horizon kernel early memory management definitions
 * 
 * This file contains definitions for early memory management.
 */

#ifndef _HORIZON_MM_EARLY_H
#define _HORIZON_MM_EARLY_H

#include <horizon/types.h>

/* Memory region types */
#define MEMORY_REGION_AVAILABLE   1  /* Available memory */
#define MEMORY_REGION_RESERVED    2  /* Reserved memory */
#define MEMORY_REGION_ACPI        3  /* ACPI reclaimable memory */
#define MEMORY_REGION_NVS         4  /* ACPI NVS memory */
#define MEMORY_REGION_BADRAM      5  /* Bad RAM */
#define MEMORY_REGION_KERNEL      6  /* Kernel memory */
#define MEMORY_REGION_MODULES     7  /* Module memory */
#define MEMORY_REGION_BOOTLOADER  8  /* Bootloader memory */

/* Memory region structure */
typedef struct memory_region {
    u64 start;                    /* Start address */
    u64 size;                     /* Size in bytes */
    u32 type;                     /* Region type */
    struct memory_region *next;   /* Next region */
} memory_region_t;

/* Early memory allocator structure */
typedef struct early_allocator {
    void *start;                  /* Start address */
    void *end;                    /* End address */
    void *current;                /* Current address */
} early_allocator_t;

/* Early memory functions */
void early_mm_init(void);
void early_mm_add_region(u64 start, u64 size, u32 type);
memory_region_t *early_mm_get_regions(void);
void early_mm_print_regions(void);
void *early_mm_alloc(u32 size, u32 align);
void early_mm_free(void *addr);
u64 early_mm_get_total_memory(void);
u64 early_mm_get_available_memory(void);
void early_mm_reserve_kernel(void *start, void *end);
void early_mm_reserve_modules(void);
void early_mm_init_allocator(void *start, void *end);
void *early_mm_allocator_alloc(u32 size, u32 align);
void early_mm_allocator_free(void *addr);

#endif /* _HORIZON_MM_EARLY_H */

/**
 * cache.h - Horizon kernel cache management definitions
 * 
 * This file contains definitions for the cache management subsystem.
 */

#ifndef _HORIZON_MM_CACHE_H
#define _HORIZON_MM_CACHE_H

#include <horizon/types.h>

/* Cache line size */
#define CACHE_LINE_SIZE 64

/* Initialize the cache management subsystem */
void cache_init(void);

/* Flush the data cache */
void cache_flush_data(void);

/* Flush the instruction cache */
void cache_flush_instruction(void);

/* Flush both caches */
void cache_flush_all(void);

/* Invalidate the data cache */
void cache_invalidate_data(void);

/* Invalidate the instruction cache */
void cache_invalidate_instruction(void);

/* Invalidate both caches */
void cache_invalidate_all(void);

/* Flush a range of memory from the data cache */
void cache_flush_range(void *addr, size_t size);

/* Prefetch a memory location into the data cache */
void cache_prefetch_data(void *addr);

/* Prefetch a memory location into the instruction cache */
void cache_prefetch_instruction(void *addr);

/* Get the cache line size */
u32 cache_get_line_size(void);

/* Print cache statistics */
void cache_print_stats(void);

#endif /* _HORIZON_MM_CACHE_H */

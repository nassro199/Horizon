/**
 * cache.c - Horizon kernel cache management implementation
 * 
 * This file contains the implementation of the cache management subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/cache.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Cache statistics */
static u64 cache_flush_count = 0;
static u64 cache_flush_data_count = 0;
static u64 cache_flush_instruction_count = 0;
static u64 cache_invalidate_count = 0;
static u64 cache_invalidate_data_count = 0;
static u64 cache_invalidate_instruction_count = 0;

/* Cache lock */
static spinlock_t cache_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize the cache management subsystem
 */
void cache_init(void) {
    /* Reset statistics */
    cache_flush_count = 0;
    cache_flush_data_count = 0;
    cache_flush_instruction_count = 0;
    cache_invalidate_count = 0;
    cache_invalidate_data_count = 0;
    cache_invalidate_instruction_count = 0;
    
    printk(KERN_INFO "CACHE: Initialized cache management subsystem\n");
}

/**
 * Flush the data cache
 */
void cache_flush_data(void) {
    /* Flush the data cache */
    __asm__ volatile("wbinvd" : : : "memory");
    
    /* Update statistics */
    spin_lock(&cache_lock);
    cache_flush_count++;
    cache_flush_data_count++;
    spin_unlock(&cache_lock);
}

/**
 * Flush the instruction cache
 */
void cache_flush_instruction(void) {
    /* Flush the instruction cache */
    __asm__ volatile("wbinvd" : : : "memory");
    
    /* Update statistics */
    spin_lock(&cache_lock);
    cache_flush_count++;
    cache_flush_instruction_count++;
    spin_unlock(&cache_lock);
}

/**
 * Flush both caches
 */
void cache_flush_all(void) {
    /* Flush both caches */
    __asm__ volatile("wbinvd" : : : "memory");
    
    /* Update statistics */
    spin_lock(&cache_lock);
    cache_flush_count++;
    cache_flush_data_count++;
    cache_flush_instruction_count++;
    spin_unlock(&cache_lock);
}

/**
 * Invalidate the data cache
 */
void cache_invalidate_data(void) {
    /* Invalidate the data cache */
    __asm__ volatile("invd" : : : "memory");
    
    /* Update statistics */
    spin_lock(&cache_lock);
    cache_invalidate_count++;
    cache_invalidate_data_count++;
    spin_unlock(&cache_lock);
}

/**
 * Invalidate the instruction cache
 */
void cache_invalidate_instruction(void) {
    /* Invalidate the instruction cache */
    __asm__ volatile("invd" : : : "memory");
    
    /* Update statistics */
    spin_lock(&cache_lock);
    cache_invalidate_count++;
    cache_invalidate_instruction_count++;
    spin_unlock(&cache_lock);
}

/**
 * Invalidate both caches
 */
void cache_invalidate_all(void) {
    /* Invalidate both caches */
    __asm__ volatile("invd" : : : "memory");
    
    /* Update statistics */
    spin_lock(&cache_lock);
    cache_invalidate_count++;
    cache_invalidate_data_count++;
    cache_invalidate_instruction_count++;
    spin_unlock(&cache_lock);
}

/**
 * Flush a range of memory from the data cache
 * 
 * @param addr Start address
 * @param size Size of the range
 */
void cache_flush_range(void *addr, size_t size) {
    /* Check parameters */
    if (addr == NULL || size == 0) {
        return;
    }
    
    /* Flush each cache line in the range */
    u8 *p = (u8 *)addr;
    u8 *end = p + size;
    
    for (; p < end; p += CACHE_LINE_SIZE) {
        __asm__ volatile("clflush (%0)" : : "r" (p) : "memory");
    }
    
    /* Memory barrier */
    __asm__ volatile("mfence" : : : "memory");
    
    /* Update statistics */
    spin_lock(&cache_lock);
    cache_flush_count++;
    cache_flush_data_count++;
    spin_unlock(&cache_lock);
}

/**
 * Prefetch a memory location into the data cache
 * 
 * @param addr Address to prefetch
 */
void cache_prefetch_data(void *addr) {
    /* Check parameters */
    if (addr == NULL) {
        return;
    }
    
    /* Prefetch the address */
    __asm__ volatile("prefetcht0 (%0)" : : "r" (addr));
}

/**
 * Prefetch a memory location into the instruction cache
 * 
 * @param addr Address to prefetch
 */
void cache_prefetch_instruction(void *addr) {
    /* Check parameters */
    if (addr == NULL) {
        return;
    }
    
    /* Prefetch the address */
    __asm__ volatile("prefetcht0 (%0)" : : "r" (addr));
}

/**
 * Get the cache line size
 * 
 * @return Cache line size in bytes
 */
u32 cache_get_line_size(void) {
    return CACHE_LINE_SIZE;
}

/**
 * Print cache statistics
 */
void cache_print_stats(void) {
    /* Print the statistics */
    printk(KERN_INFO "CACHE: Flush count: %llu\n", cache_flush_count);
    printk(KERN_INFO "CACHE: Flush data count: %llu\n", cache_flush_data_count);
    printk(KERN_INFO "CACHE: Flush instruction count: %llu\n", cache_flush_instruction_count);
    printk(KERN_INFO "CACHE: Invalidate count: %llu\n", cache_invalidate_count);
    printk(KERN_INFO "CACHE: Invalidate data count: %llu\n", cache_invalidate_data_count);
    printk(KERN_INFO "CACHE: Invalidate instruction count: %llu\n", cache_invalidate_instruction_count);
}

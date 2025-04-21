/**
 * cache_coherency.h - Horizon kernel cache coherency definitions
 * 
 * This file contains definitions for cache coherency protocols.
 */

#ifndef _HORIZON_MM_CACHE_COHERENCY_H
#define _HORIZON_MM_CACHE_COHERENCY_H

#include <horizon/types.h>

/* Cache coherency protocols */
typedef enum cache_coherency_protocol {
    CACHE_PROTOCOL_NONE,  /* No coherency protocol */
    CACHE_PROTOCOL_MSI,   /* Modified-Shared-Invalid protocol */
    CACHE_PROTOCOL_MESI,  /* Modified-Exclusive-Shared-Invalid protocol */
    CACHE_PROTOCOL_MOESI  /* Modified-Owned-Exclusive-Shared-Invalid protocol */
} cache_coherency_protocol_t;

/* Initialize the cache coherency subsystem */
void cache_coherency_init(void);

/* Set the cache coherency protocol */
int cache_coherency_set_protocol(cache_coherency_protocol_t protocol);

/* Get the cache coherency protocol */
cache_coherency_protocol_t cache_coherency_get_protocol(void);

/* Invalidate a cache line on all CPUs */
int cache_coherency_invalidate(u64 address);

/* Flush a cache line on all CPUs */
int cache_coherency_flush(u64 address);

/* Broadcast a cache line to all CPUs */
int cache_coherency_broadcast(u64 address);

/* Snoop a cache line */
int cache_coherency_snoop(u64 address, int cpu);

/* Upgrade a cache line state */
int cache_coherency_upgrade(u64 address, int cpu, int state);

/* Downgrade a cache line state */
int cache_coherency_downgrade(u64 address, int cpu, int state);

/* Handle a memory read */
int cache_coherency_read(u64 address, int cpu);

/* Handle a memory write */
int cache_coherency_write(u64 address, int cpu);

/* Print cache coherency statistics */
void cache_coherency_print_stats(void);

#endif /* _HORIZON_MM_CACHE_COHERENCY_H */

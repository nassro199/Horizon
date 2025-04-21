/**
 * numa.h - Horizon kernel NUMA definitions
 * 
 * This file contains definitions for NUMA (Non-Uniform Memory Access) support.
 */

#ifndef _HORIZON_MM_NUMA_H
#define _HORIZON_MM_NUMA_H

#include <horizon/types.h>

/* NUMA policies */
typedef enum numa_policy {
    NUMA_POLICY_LOCAL,      /* Allocate from the local node */
    NUMA_POLICY_INTERLEAVE, /* Interleave allocations across all nodes */
    NUMA_POLICY_PREFERRED   /* Allocate from the preferred node, fall back to any node */
} numa_policy_t;

/* Forward declarations */
struct numa_node;

/* Current preferred node for NUMA_POLICY_PREFERRED */
static int current_preferred_node = 0;

/* Initialize the NUMA subsystem */
void numa_init(void);

/* Detect NUMA nodes */
void numa_detect_nodes(void);

/* Get the number of NUMA nodes */
int numa_get_node_count(void);

/* Get a NUMA node by ID */
struct numa_node *numa_get_node(int id);

/* Get the local NUMA node */
struct numa_node *numa_get_local_node(void);

/* Get the node for a physical address */
struct numa_node *numa_get_node_for_addr(u64 phys_addr);

/* Set the NUMA policy */
int numa_set_policy(numa_policy_t policy, int preferred_node);

/* Get the NUMA policy */
numa_policy_t numa_get_policy(void);

/* Allocate pages from a specific NUMA node */
void *numa_alloc_pages(int node_id, u32 count, u32 flags);

/* Free pages allocated from a NUMA node */
void numa_free_pages(void *addr, u32 count);

/* Allocate pages according to the current NUMA policy */
void *numa_policy_alloc_pages(u32 count, u32 flags);

/* Migrate a page from one NUMA node to another */
int numa_migrate_page(void *addr, int target_node);

/* Print NUMA statistics */
void numa_print_stats(void);

#endif /* _HORIZON_MM_NUMA_H */

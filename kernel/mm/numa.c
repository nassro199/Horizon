/**
 * numa.c - Horizon kernel NUMA implementation
 * 
 * This file contains the implementation of NUMA (Non-Uniform Memory Access) support.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/numa.h>
#include <horizon/mm/pmm.h>
#include <horizon/mm/vmm.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of NUMA nodes */
#define MAX_NUMA_NODES 16

/* NUMA node structure */
typedef struct numa_node {
    int id;                     /* Node ID */
    u64 start_pfn;              /* Start page frame number */
    u64 end_pfn;                /* End page frame number */
    u64 free_pages;             /* Number of free pages */
    u64 total_pages;            /* Total number of pages */
    u32 distance[MAX_NUMA_NODES]; /* Distance to other nodes */
    spinlock_t lock;            /* Node lock */
} numa_node_t;

/* NUMA nodes */
static numa_node_t numa_nodes[MAX_NUMA_NODES];
static int numa_node_count = 0;

/* Current NUMA policy */
static numa_policy_t current_policy = NUMA_POLICY_LOCAL;

/* NUMA statistics */
static u64 numa_local_allocs = 0;
static u64 numa_remote_allocs = 0;
static u64 numa_interleave_allocs = 0;
static u64 numa_preferred_allocs = 0;
static u64 numa_migrations = 0;

/* NUMA lock */
static spinlock_t numa_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize the NUMA subsystem
 */
void numa_init(void) {
    /* Reset the NUMA nodes */
    memset(numa_nodes, 0, sizeof(numa_nodes));
    numa_node_count = 0;
    
    /* Reset statistics */
    numa_local_allocs = 0;
    numa_remote_allocs = 0;
    numa_interleave_allocs = 0;
    numa_preferred_allocs = 0;
    numa_migrations = 0;
    
    /* Set the default policy */
    current_policy = NUMA_POLICY_LOCAL;
    
    /* Detect NUMA nodes */
    numa_detect_nodes();
    
    printk(KERN_INFO "NUMA: Initialized NUMA subsystem with %d nodes\n", numa_node_count);
}

/**
 * Detect NUMA nodes
 * 
 * This function detects NUMA nodes by parsing the ACPI SRAT table.
 * For simplicity, we'll simulate a 2-node NUMA system.
 */
void numa_detect_nodes(void) {
    /* Lock the NUMA subsystem */
    spin_lock(&numa_lock);
    
    /* Simulate a 2-node NUMA system */
    if (numa_node_count == 0) {
        /* Create node 0 */
        numa_node_t *node0 = &numa_nodes[0];
        node0->id = 0;
        node0->start_pfn = 0;
        node0->end_pfn = pmm_get_total_pages() / 2;
        node0->free_pages = node0->end_pfn - node0->start_pfn;
        node0->total_pages = node0->free_pages;
        node0->lock = 0;
        
        /* Set distances */
        node0->distance[0] = 10; /* Local access */
        node0->distance[1] = 20; /* Remote access */
        
        /* Create node 1 */
        numa_node_t *node1 = &numa_nodes[1];
        node1->id = 1;
        node1->start_pfn = node0->end_pfn;
        node1->end_pfn = pmm_get_total_pages();
        node1->free_pages = node1->end_pfn - node1->start_pfn;
        node1->total_pages = node1->free_pages;
        node1->lock = 0;
        
        /* Set distances */
        node1->distance[0] = 20; /* Remote access */
        node1->distance[1] = 10; /* Local access */
        
        /* Set the node count */
        numa_node_count = 2;
    }
    
    /* Unlock the NUMA subsystem */
    spin_unlock(&numa_lock);
}

/**
 * Get the number of NUMA nodes
 * 
 * @return Number of NUMA nodes
 */
int numa_get_node_count(void) {
    return numa_node_count;
}

/**
 * Get a NUMA node by ID
 * 
 * @param id Node ID
 * @return Pointer to the node, or NULL if not found
 */
numa_node_t *numa_get_node(int id) {
    /* Check parameters */
    if (id < 0 || id >= numa_node_count) {
        return NULL;
    }
    
    return &numa_nodes[id];
}

/**
 * Get the local NUMA node
 * 
 * @return Pointer to the local node
 */
numa_node_t *numa_get_local_node(void) {
    /* Get the current CPU */
    int cpu = smp_processor_id();
    
    /* Get the node for this CPU */
    int node_id = cpu % numa_node_count;
    
    return &numa_nodes[node_id];
}

/**
 * Get the node for a physical address
 * 
 * @param phys_addr Physical address
 * @return Pointer to the node, or NULL if not found
 */
numa_node_t *numa_get_node_for_addr(u64 phys_addr) {
    /* Convert the address to a page frame number */
    u64 pfn = phys_addr / PAGE_SIZE;
    
    /* Find the node that contains this page */
    for (int i = 0; i < numa_node_count; i++) {
        numa_node_t *node = &numa_nodes[i];
        
        if (pfn >= node->start_pfn && pfn < node->end_pfn) {
            return node;
        }
    }
    
    return NULL;
}

/**
 * Set the NUMA policy
 * 
 * @param policy Policy to set
 * @param preferred_node Preferred node for NUMA_POLICY_PREFERRED
 * @return 0 on success, negative error code on failure
 */
int numa_set_policy(numa_policy_t policy, int preferred_node) {
    /* Check parameters */
    if (policy < NUMA_POLICY_LOCAL || policy > NUMA_POLICY_PREFERRED) {
        return -EINVAL;
    }
    
    if (policy == NUMA_POLICY_PREFERRED && (preferred_node < 0 || preferred_node >= numa_node_count)) {
        return -EINVAL;
    }
    
    /* Lock the NUMA subsystem */
    spin_lock(&numa_lock);
    
    /* Set the policy */
    current_policy = policy;
    
    /* Set the preferred node */
    if (policy == NUMA_POLICY_PREFERRED) {
        current_preferred_node = preferred_node;
    }
    
    /* Unlock the NUMA subsystem */
    spin_unlock(&numa_lock);
    
    printk(KERN_INFO "NUMA: Set policy to %d\n", policy);
    
    return 0;
}

/**
 * Get the NUMA policy
 * 
 * @return Current NUMA policy
 */
numa_policy_t numa_get_policy(void) {
    return current_policy;
}

/**
 * Allocate pages from a specific NUMA node
 * 
 * @param node_id Node ID to allocate from
 * @param count Number of pages to allocate
 * @param flags Allocation flags
 * @return Pointer to the allocated pages, or NULL on failure
 */
void *numa_alloc_pages(int node_id, u32 count, u32 flags) {
    /* Check parameters */
    if (node_id < 0 || node_id >= numa_node_count || count == 0) {
        return NULL;
    }
    
    /* Get the node */
    numa_node_t *node = &numa_nodes[node_id];
    
    /* Lock the node */
    spin_lock(&node->lock);
    
    /* Check if there are enough free pages */
    if (node->free_pages < count) {
        /* Not enough free pages */
        spin_unlock(&node->lock);
        return NULL;
    }
    
    /* Allocate pages from the node */
    void *addr = NULL;
    u64 start_pfn = node->start_pfn;
    u64 end_pfn = node->end_pfn;
    
    /* Find a contiguous range of free pages */
    for (u64 pfn = start_pfn; pfn <= end_pfn - count; pfn++) {
        int found = 1;
        
        /* Check if all pages are free */
        for (u64 i = 0; i < count; i++) {
            if (pmm_is_page_allocated(pfn + i)) {
                found = 0;
                break;
            }
        }
        
        if (found) {
            /* Allocate the pages */
            for (u64 i = 0; i < count; i++) {
                pmm_allocate_page(pfn + i);
            }
            
            /* Convert the PFN to a virtual address */
            addr = pmm_pfn_to_virt(pfn);
            
            /* Update the node's free pages */
            node->free_pages -= count;
            
            break;
        }
    }
    
    /* Unlock the node */
    spin_unlock(&node->lock);
    
    /* Update statistics */
    if (addr != NULL) {
        spin_lock(&numa_lock);
        
        if (node_id == numa_get_local_node()->id) {
            numa_local_allocs++;
        } else {
            numa_remote_allocs++;
        }
        
        spin_unlock(&numa_lock);
    }
    
    return addr;
}

/**
 * Free pages allocated from a NUMA node
 * 
 * @param addr Address to free
 * @param count Number of pages to free
 */
void numa_free_pages(void *addr, u32 count) {
    /* Check parameters */
    if (addr == NULL || count == 0) {
        return;
    }
    
    /* Convert the address to a page frame number */
    u64 pfn = pmm_virt_to_pfn(addr);
    
    /* Find the node that contains this page */
    numa_node_t *node = numa_get_node_for_addr(pfn * PAGE_SIZE);
    
    if (node == NULL) {
        /* Not a NUMA allocation */
        return;
    }
    
    /* Lock the node */
    spin_lock(&node->lock);
    
    /* Free the pages */
    for (u64 i = 0; i < count; i++) {
        pmm_free_page(pfn + i);
    }
    
    /* Update the node's free pages */
    node->free_pages += count;
    
    /* Unlock the node */
    spin_unlock(&node->lock);
}

/**
 * Allocate pages according to the current NUMA policy
 * 
 * @param count Number of pages to allocate
 * @param flags Allocation flags
 * @return Pointer to the allocated pages, or NULL on failure
 */
void *numa_policy_alloc_pages(u32 count, u32 flags) {
    /* Check parameters */
    if (count == 0) {
        return NULL;
    }
    
    /* Allocate according to the policy */
    switch (current_policy) {
        case NUMA_POLICY_LOCAL:
            /* Allocate from the local node */
            {
                numa_node_t *local_node = numa_get_local_node();
                void *addr = numa_alloc_pages(local_node->id, count, flags);
                
                if (addr != NULL) {
                    return addr;
                }
                
                /* Fall back to any node */
                for (int i = 0; i < numa_node_count; i++) {
                    if (i != local_node->id) {
                        addr = numa_alloc_pages(i, count, flags);
                        
                        if (addr != NULL) {
                            return addr;
                        }
                    }
                }
            }
            break;
        
        case NUMA_POLICY_INTERLEAVE:
            /* Interleave allocations across all nodes */
            {
                static int next_node = 0;
                
                /* Try each node in turn */
                for (int i = 0; i < numa_node_count; i++) {
                    int node_id = (next_node + i) % numa_node_count;
                    void *addr = numa_alloc_pages(node_id, count, flags);
                    
                    if (addr != NULL) {
                        /* Update the next node */
                        next_node = (node_id + 1) % numa_node_count;
                        
                        /* Update statistics */
                        spin_lock(&numa_lock);
                        numa_interleave_allocs++;
                        spin_unlock(&numa_lock);
                        
                        return addr;
                    }
                }
            }
            break;
        
        case NUMA_POLICY_PREFERRED:
            /* Allocate from the preferred node, fall back to any node */
            {
                void *addr = numa_alloc_pages(current_preferred_node, count, flags);
                
                if (addr != NULL) {
                    /* Update statistics */
                    spin_lock(&numa_lock);
                    numa_preferred_allocs++;
                    spin_unlock(&numa_lock);
                    
                    return addr;
                }
                
                /* Fall back to any node */
                for (int i = 0; i < numa_node_count; i++) {
                    if (i != current_preferred_node) {
                        addr = numa_alloc_pages(i, count, flags);
                        
                        if (addr != NULL) {
                            return addr;
                        }
                    }
                }
            }
            break;
        
        default:
            /* Unknown policy */
            return NULL;
    }
    
    /* Failed to allocate pages */
    return NULL;
}

/**
 * Migrate a page from one NUMA node to another
 * 
 * @param addr Address to migrate
 * @param target_node Target node ID
 * @return 0 on success, negative error code on failure
 */
int numa_migrate_page(void *addr, int target_node) {
    /* Check parameters */
    if (addr == NULL || target_node < 0 || target_node >= numa_node_count) {
        return -EINVAL;
    }
    
    /* Convert the address to a page frame number */
    u64 pfn = pmm_virt_to_pfn(addr);
    
    /* Find the source node */
    numa_node_t *source_node = numa_get_node_for_addr(pfn * PAGE_SIZE);
    
    if (source_node == NULL) {
        /* Not a NUMA allocation */
        return -EINVAL;
    }
    
    /* Check if the page is already on the target node */
    if (source_node->id == target_node) {
        return 0;
    }
    
    /* Get the target node */
    numa_node_t *target = &numa_nodes[target_node];
    
    /* Lock both nodes */
    if (source_node->id < target_node) {
        spin_lock(&source_node->lock);
        spin_lock(&target->lock);
    } else {
        spin_lock(&target->lock);
        spin_lock(&source_node->lock);
    }
    
    /* Check if there are enough free pages on the target node */
    if (target->free_pages < 1) {
        /* Not enough free pages */
        spin_unlock(&source_node->lock);
        spin_unlock(&target->lock);
        return -ENOMEM;
    }
    
    /* Find a free page on the target node */
    u64 target_pfn = 0;
    int found = 0;
    
    for (u64 pfn = target->start_pfn; pfn < target->end_pfn; pfn++) {
        if (!pmm_is_page_allocated(pfn)) {
            target_pfn = pfn;
            found = 1;
            break;
        }
    }
    
    if (!found) {
        /* No free page found */
        spin_unlock(&source_node->lock);
        spin_unlock(&target->lock);
        return -ENOMEM;
    }
    
    /* Allocate the target page */
    pmm_allocate_page(target_pfn);
    
    /* Copy the page contents */
    memcpy(pmm_pfn_to_virt(target_pfn), addr, PAGE_SIZE);
    
    /* Update the page tables */
    /* This would be implemented with actual page table updates */
    
    /* Free the source page */
    pmm_free_page(pfn);
    
    /* Update the nodes' free pages */
    source_node->free_pages++;
    target->free_pages--;
    
    /* Unlock both nodes */
    spin_unlock(&source_node->lock);
    spin_unlock(&target->lock);
    
    /* Update statistics */
    spin_lock(&numa_lock);
    numa_migrations++;
    spin_unlock(&numa_lock);
    
    return 0;
}

/**
 * Print NUMA statistics
 */
void numa_print_stats(void) {
    /* Lock the NUMA subsystem */
    spin_lock(&numa_lock);
    
    /* Print the statistics */
    printk(KERN_INFO "NUMA: Nodes: %d\n", numa_node_count);
    
    for (int i = 0; i < numa_node_count; i++) {
        numa_node_t *node = &numa_nodes[i];
        
        printk(KERN_INFO "NUMA: Node %d: %llu/%llu pages free\n",
               node->id, node->free_pages, node->total_pages);
        
        printk(KERN_INFO "NUMA: Node %d distances:", node->id);
        for (int j = 0; j < numa_node_count; j++) {
            printk(" %d", node->distance[j]);
        }
        printk("\n");
    }
    
    printk(KERN_INFO "NUMA: Local allocations: %llu\n", numa_local_allocs);
    printk(KERN_INFO "NUMA: Remote allocations: %llu\n", numa_remote_allocs);
    printk(KERN_INFO "NUMA: Interleave allocations: %llu\n", numa_interleave_allocs);
    printk(KERN_INFO "NUMA: Preferred allocations: %llu\n", numa_preferred_allocs);
    printk(KERN_INFO "NUMA: Migrations: %llu\n", numa_migrations);
    
    /* Unlock the NUMA subsystem */
    spin_unlock(&numa_lock);
}

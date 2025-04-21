/**
 * memory_migration.c - Horizon kernel memory migration implementation
 * 
 * This file contains the implementation of memory migration.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/pmm.h>
#include <horizon/mm/vmm.h>
#include <horizon/mm/numa.h>
#include <horizon/mm/memory_migration.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/task.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Migration statistics */
static u64 migration_count = 0;
static u64 migration_pages = 0;
static u64 migration_bytes = 0;
static u64 migration_success = 0;
static u64 migration_failed = 0;
static u64 migration_deferred = 0;

/* Migration lock */
static spinlock_t migration_lock = SPIN_LOCK_INITIALIZER;

/* Migration parameters */
static int migration_enabled = 1;
static u64 migration_interval = 1000; /* 1 second */
static u64 migration_threshold = 25; /* 25% imbalance */
static u64 migration_last_time = 0;

/**
 * Initialize the memory migration subsystem
 */
void memory_migration_init(void) {
    /* Reset statistics */
    migration_count = 0;
    migration_pages = 0;
    migration_bytes = 0;
    migration_success = 0;
    migration_failed = 0;
    migration_deferred = 0;
    
    /* Set parameters */
    migration_enabled = 1;
    migration_interval = 1000; /* 1 second */
    migration_threshold = 25; /* 25% imbalance */
    migration_last_time = 0;
    
    printk(KERN_INFO "MEMORY_MIGRATION: Initialized memory migration subsystem\n");
}

/**
 * Enable or disable memory migration
 * 
 * @param enable 1 to enable, 0 to disable
 * @return 0 on success, negative error code on failure
 */
int memory_migration_enable(int enable) {
    /* Lock the memory migration */
    spin_lock(&migration_lock);
    
    /* Set the state */
    migration_enabled = enable ? 1 : 0;
    
    /* Unlock the memory migration */
    spin_unlock(&migration_lock);
    
    printk(KERN_INFO "MEMORY_MIGRATION: %s memory migration\n", enable ? "Enabled" : "Disabled");
    
    return 0;
}

/**
 * Set the memory migration interval
 * 
 * @param interval Interval in milliseconds
 * @return 0 on success, negative error code on failure
 */
int memory_migration_set_interval(u64 interval) {
    /* Check parameters */
    if (interval == 0) {
        return -EINVAL;
    }
    
    /* Lock the memory migration */
    spin_lock(&migration_lock);
    
    /* Set the interval */
    migration_interval = interval;
    
    /* Unlock the memory migration */
    spin_unlock(&migration_lock);
    
    printk(KERN_INFO "MEMORY_MIGRATION: Set interval to %llu ms\n", interval);
    
    return 0;
}

/**
 * Set the memory migration threshold
 * 
 * @param threshold Threshold in percent
 * @return 0 on success, negative error code on failure
 */
int memory_migration_set_threshold(u64 threshold) {
    /* Check parameters */
    if (threshold > 100) {
        return -EINVAL;
    }
    
    /* Lock the memory migration */
    spin_lock(&migration_lock);
    
    /* Set the threshold */
    migration_threshold = threshold;
    
    /* Unlock the memory migration */
    spin_unlock(&migration_lock);
    
    printk(KERN_INFO "MEMORY_MIGRATION: Set threshold to %llu%%\n", threshold);
    
    return 0;
}

/**
 * Check if memory migration is needed
 * 
 * @return 1 if needed, 0 if not
 */
int memory_migration_needed(void) {
    /* Check if memory migration is enabled */
    if (!migration_enabled) {
        return 0;
    }
    
    /* Get the current time */
    u64 current_time = timer_get_ticks();
    
    /* Check if it's time to migrate */
    if (current_time - migration_last_time < migration_interval) {
        return 0;
    }
    
    /* Update the last time */
    migration_last_time = current_time;
    
    /* Check if there is an imbalance */
    return memory_migration_check_imbalance();
}

/**
 * Check if there is an imbalance between NUMA nodes
 * 
 * @return 1 if there is an imbalance, 0 if not
 */
int memory_migration_check_imbalance(void) {
    /* Get the number of NUMA nodes */
    int nr_nodes = numa_get_node_count();
    
    /* Check if there is more than one node */
    if (nr_nodes <= 1) {
        return 0;
    }
    
    /* Find the busiest and idlest nodes */
    int busiest_node = -1;
    int idlest_node = -1;
    u64 busiest_used = 0;
    u64 idlest_used = UINT64_MAX;
    
    for (int i = 0; i < nr_nodes; i++) {
        /* Get the node */
        numa_node_t *node = numa_get_node(i);
        
        /* Calculate the used pages */
        u64 used = node->total_pages - node->free_pages;
        
        /* Check if this is the busiest node */
        if (used > busiest_used) {
            busiest_used = used;
            busiest_node = i;
        }
        
        /* Check if this is the idlest node */
        if (used < idlest_used) {
            idlest_used = used;
            idlest_node = i;
        }
    }
    
    /* Check if there is an imbalance */
    if (busiest_node != -1 && idlest_node != -1 && busiest_node != idlest_node) {
        /* Calculate the imbalance */
        u64 imbalance = busiest_used - idlest_used;
        
        /* Check if the imbalance is above the threshold */
        if (imbalance > 0 && (imbalance * 100) / (busiest_used + 1) >= migration_threshold) {
            /* There is an imbalance */
            return 1;
        }
    }
    
    return 0;
}

/**
 * Migrate memory between NUMA nodes
 * 
 * @return Number of pages migrated, or negative error code on failure
 */
int memory_migration_run(void) {
    /* Check if memory migration is needed */
    if (!memory_migration_needed()) {
        return 0;
    }
    
    /* Lock the memory migration */
    spin_lock(&migration_lock);
    
    /* Increment the count */
    migration_count++;
    
    /* Get the number of NUMA nodes */
    int nr_nodes = numa_get_node_count();
    
    /* Find the busiest and idlest nodes */
    int busiest_node = -1;
    int idlest_node = -1;
    u64 busiest_used = 0;
    u64 idlest_used = UINT64_MAX;
    
    for (int i = 0; i < nr_nodes; i++) {
        /* Get the node */
        numa_node_t *node = numa_get_node(i);
        
        /* Calculate the used pages */
        u64 used = node->total_pages - node->free_pages;
        
        /* Check if this is the busiest node */
        if (used > busiest_used) {
            busiest_used = used;
            busiest_node = i;
        }
        
        /* Check if this is the idlest node */
        if (used < idlest_used) {
            idlest_used = used;
            idlest_node = i;
        }
    }
    
    /* Check if there is an imbalance */
    if (busiest_node == -1 || idlest_node == -1 || busiest_node == idlest_node) {
        /* No imbalance */
        spin_unlock(&migration_lock);
        return 0;
    }
    
    /* Calculate the imbalance */
    u64 imbalance = busiest_used - idlest_used;
    
    /* Check if the imbalance is above the threshold */
    if (imbalance == 0 || (imbalance * 100) / (busiest_used + 1) < migration_threshold) {
        /* No significant imbalance */
        spin_unlock(&migration_lock);
        return 0;
    }
    
    /* Calculate the number of pages to migrate */
    u64 nr_to_migrate = imbalance / 2;
    
    /* Make sure we migrate at least one page */
    if (nr_to_migrate == 0) {
        nr_to_migrate = 1;
    }
    
    /* Get the nodes */
    numa_node_t *busiest = numa_get_node(busiest_node);
    numa_node_t *idlest = numa_get_node(idlest_node);
    
    /* Migrate pages from the busiest to the idlest node */
    u64 nr_migrated = 0;
    
    /* Find pages to migrate */
    for (u64 pfn = busiest->start_pfn; pfn < busiest->end_pfn && nr_migrated < nr_to_migrate; pfn++) {
        /* Check if the page is allocated */
        if (pmm_is_page_allocated(pfn)) {
            /* Get the virtual address */
            void *addr = pmm_pfn_to_virt(pfn);
            
            /* Migrate the page */
            int ret = numa_migrate_page(addr, idlest_node);
            
            if (ret == 0) {
                /* Page was migrated */
                nr_migrated++;
            } else if (ret == -EAGAIN) {
                /* Page migration was deferred */
                migration_deferred++;
            } else {
                /* Page migration failed */
                migration_failed++;
            }
        }
    }
    
    /* Update statistics */
    migration_pages += nr_migrated;
    migration_bytes += nr_migrated * PAGE_SIZE;
    migration_success += nr_migrated;
    
    /* Unlock the memory migration */
    spin_unlock(&migration_lock);
    
    return nr_migrated;
}

/**
 * Migrate a specific memory range
 * 
 * @param addr Start address
 * @param size Size in bytes
 * @param target_node Target node ID
 * @return Number of pages migrated, or negative error code on failure
 */
int memory_migration_range(void *addr, size_t size, int target_node) {
    /* Check parameters */
    if (addr == NULL || size == 0 || target_node < 0 || target_node >= numa_get_node_count()) {
        return -EINVAL;
    }
    
    /* Align the address to a page boundary */
    u64 start = (u64)addr & ~(PAGE_SIZE - 1);
    u64 end = ((u64)addr + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    /* Calculate the number of pages */
    u64 nr_pages = (end - start) / PAGE_SIZE;
    
    /* Lock the memory migration */
    spin_lock(&migration_lock);
    
    /* Increment the count */
    migration_count++;
    
    /* Migrate the pages */
    u64 nr_migrated = 0;
    
    for (u64 page = start; page < end; page += PAGE_SIZE) {
        /* Migrate the page */
        int ret = numa_migrate_page((void *)page, target_node);
        
        if (ret == 0) {
            /* Page was migrated */
            nr_migrated++;
        } else if (ret == -EAGAIN) {
            /* Page migration was deferred */
            migration_deferred++;
        } else {
            /* Page migration failed */
            migration_failed++;
        }
    }
    
    /* Update statistics */
    migration_pages += nr_migrated;
    migration_bytes += nr_migrated * PAGE_SIZE;
    migration_success += nr_migrated;
    
    /* Unlock the memory migration */
    spin_unlock(&migration_lock);
    
    return nr_migrated;
}

/**
 * Migrate a task's memory
 * 
 * @param task Task to migrate
 * @param target_node Target node ID
 * @return Number of pages migrated, or negative error code on failure
 */
int memory_migration_task(task_struct_t *task, int target_node) {
    /* Check parameters */
    if (task == NULL || target_node < 0 || target_node >= numa_get_node_count()) {
        return -EINVAL;
    }
    
    /* Lock the memory migration */
    spin_lock(&migration_lock);
    
    /* Increment the count */
    migration_count++;
    
    /* Migrate the task's memory */
    u64 nr_migrated = 0;
    
    /* Iterate through the task's memory areas */
    vm_area_struct_t *vma;
    for (vma = task->mm->mmap; vma != NULL; vma = vma->vm_list.next) {
        /* Skip non-migratable areas */
        if (vma->vm_flags & VM_LOCKED) {
            continue;
        }
        
        /* Migrate the pages in the area */
        for (u64 addr = vma->vm_start; addr < vma->vm_end; addr += PAGE_SIZE) {
            /* Get the page */
            page_t *page = vmm_get_page(task->mm, addr);
            
            /* Check if the page exists */
            if (page != NULL) {
                /* Migrate the page */
                int ret = numa_migrate_page(pmm_page_to_virt(page), target_node);
                
                if (ret == 0) {
                    /* Page was migrated */
                    nr_migrated++;
                } else if (ret == -EAGAIN) {
                    /* Page migration was deferred */
                    migration_deferred++;
                } else {
                    /* Page migration failed */
                    migration_failed++;
                }
            }
        }
    }
    
    /* Update statistics */
    migration_pages += nr_migrated;
    migration_bytes += nr_migrated * PAGE_SIZE;
    migration_success += nr_migrated;
    
    /* Unlock the memory migration */
    spin_unlock(&migration_lock);
    
    return nr_migrated;
}

/**
 * Print memory migration statistics
 */
void memory_migration_print_stats(void) {
    /* Lock the memory migration */
    spin_lock(&migration_lock);
    
    /* Print the statistics */
    printk(KERN_INFO "MEMORY_MIGRATION: Enabled: %s\n", migration_enabled ? "Yes" : "No");
    printk(KERN_INFO "MEMORY_MIGRATION: Interval: %llu ms\n", migration_interval);
    printk(KERN_INFO "MEMORY_MIGRATION: Threshold: %llu%%\n", migration_threshold);
    printk(KERN_INFO "MEMORY_MIGRATION: Count: %llu\n", migration_count);
    printk(KERN_INFO "MEMORY_MIGRATION: Pages: %llu\n", migration_pages);
    printk(KERN_INFO "MEMORY_MIGRATION: Bytes: %llu\n", migration_bytes);
    printk(KERN_INFO "MEMORY_MIGRATION: Success: %llu\n", migration_success);
    printk(KERN_INFO "MEMORY_MIGRATION: Failed: %llu\n", migration_failed);
    printk(KERN_INFO "MEMORY_MIGRATION: Deferred: %llu\n", migration_deferred);
    
    /* Unlock the memory migration */
    spin_unlock(&migration_lock);
}

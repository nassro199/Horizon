/**
 * sched_domain.c - Horizon kernel scheduler domains implementation
 * 
 * This file contains the implementation of scheduler domains.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/sched.h>
#include <horizon/sched/sched_domain.h>
#include <horizon/thread.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of domains */
#define MAX_DOMAINS 8

/* Maximum number of groups per domain */
#define MAX_GROUPS 8

/* Scheduler domains */
static sched_domain_t domains[MAX_DOMAINS];
static int domain_count = 0;

/* Domain lock */
static spinlock_t domain_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize the scheduler domains
 */
void sched_domain_init(void) {
    /* Reset the domains */
    memset(domains, 0, sizeof(domains));
    domain_count = 0;
    
    /* Get the number of CPUs */
    int nr_cpus = CONFIG_NR_CPUS;
    
    /* Check if there is more than one CPU */
    if (nr_cpus <= 1) {
        /* No need for domains with only one CPU */
        return;
    }
    
    /* Create a single domain for all CPUs */
    sched_domain_t *domain = &domains[0];
    domain->id = 0;
    domain->parent = NULL;
    domain->flags = SD_LOAD_BALANCE | SD_BALANCE_NEWIDLE | SD_WAKE_AFFINE;
    domain->min_interval = 1;
    domain->max_interval = 100;
    domain->busy_factor = 32;
    domain->imbalance_pct = 125;
    domain->cache_nice_tries = 1;
    
    /* Create a single group for all CPUs */
    sched_group_t *group = &domain->groups[0];
    group->id = 0;
    group->cpu_mask = 0;
    
    /* Add all CPUs to the group */
    for (int i = 0; i < nr_cpus; i++) {
        group->cpu_mask |= (1ULL << i);
    }
    
    /* Set the group count */
    domain->group_count = 1;
    
    /* Set the domain count */
    domain_count = 1;
    
    printk(KERN_INFO "SCHED_DOMAIN: Initialized scheduler domains\n");
}

/**
 * Create a scheduler domain
 * 
 * @param parent Parent domain
 * @param flags Domain flags
 * @return Pointer to the domain, or NULL on failure
 */
sched_domain_t *sched_domain_create(sched_domain_t *parent, u32 flags) {
    /* Lock the domains */
    spin_lock(&domain_lock);
    
    /* Check if there is space for a new domain */
    if (domain_count >= MAX_DOMAINS) {
        /* No space for a new domain */
        spin_unlock(&domain_lock);
        return NULL;
    }
    
    /* Create the domain */
    sched_domain_t *domain = &domains[domain_count];
    domain->id = domain_count;
    domain->parent = parent;
    domain->flags = flags;
    domain->min_interval = 1;
    domain->max_interval = 100;
    domain->busy_factor = 32;
    domain->imbalance_pct = 125;
    domain->cache_nice_tries = 1;
    domain->group_count = 0;
    
    /* Increment the domain count */
    domain_count++;
    
    /* Unlock the domains */
    spin_unlock(&domain_lock);
    
    return domain;
}

/**
 * Add a group to a domain
 * 
 * @param domain Domain to add the group to
 * @param cpu_mask CPU mask for the group
 * @return Pointer to the group, or NULL on failure
 */
sched_group_t *sched_domain_add_group(sched_domain_t *domain, u64 cpu_mask) {
    /* Check parameters */
    if (domain == NULL || cpu_mask == 0) {
        return NULL;
    }
    
    /* Lock the domains */
    spin_lock(&domain_lock);
    
    /* Check if there is space for a new group */
    if (domain->group_count >= MAX_GROUPS) {
        /* No space for a new group */
        spin_unlock(&domain_lock);
        return NULL;
    }
    
    /* Create the group */
    sched_group_t *group = &domain->groups[domain->group_count];
    group->id = domain->group_count;
    group->cpu_mask = cpu_mask;
    
    /* Increment the group count */
    domain->group_count++;
    
    /* Unlock the domains */
    spin_unlock(&domain_lock);
    
    return group;
}

/**
 * Find a domain by ID
 * 
 * @param id Domain ID
 * @return Pointer to the domain, or NULL if not found
 */
sched_domain_t *sched_domain_find(int id) {
    /* Check parameters */
    if (id < 0 || id >= domain_count) {
        return NULL;
    }
    
    return &domains[id];
}

/**
 * Find a group by ID in a domain
 * 
 * @param domain Domain to search in
 * @param id Group ID
 * @return Pointer to the group, or NULL if not found
 */
sched_group_t *sched_domain_find_group(sched_domain_t *domain, int id) {
    /* Check parameters */
    if (domain == NULL || id < 0 || id >= domain->group_count) {
        return NULL;
    }
    
    return &domain->groups[id];
}

/**
 * Find a domain for a CPU
 * 
 * @param cpu CPU to find domain for
 * @return Pointer to the domain, or NULL if not found
 */
sched_domain_t *sched_domain_find_for_cpu(int cpu) {
    /* Check parameters */
    if (cpu < 0 || cpu >= CONFIG_NR_CPUS) {
        return NULL;
    }
    
    /* Find a domain that contains the CPU */
    for (int i = 0; i < domain_count; i++) {
        sched_domain_t *domain = &domains[i];
        
        /* Check if the domain contains the CPU */
        for (int j = 0; j < domain->group_count; j++) {
            sched_group_t *group = &domain->groups[j];
            
            /* Check if the group contains the CPU */
            if (group->cpu_mask & (1ULL << cpu)) {
                return domain;
            }
        }
    }
    
    return NULL;
}

/**
 * Find a group for a CPU in a domain
 * 
 * @param domain Domain to search in
 * @param cpu CPU to find group for
 * @return Pointer to the group, or NULL if not found
 */
sched_group_t *sched_domain_find_group_for_cpu(sched_domain_t *domain, int cpu) {
    /* Check parameters */
    if (domain == NULL || cpu < 0 || cpu >= CONFIG_NR_CPUS) {
        return NULL;
    }
    
    /* Find a group that contains the CPU */
    for (int i = 0; i < domain->group_count; i++) {
        sched_group_t *group = &domain->groups[i];
        
        /* Check if the group contains the CPU */
        if (group->cpu_mask & (1ULL << cpu)) {
            return group;
        }
    }
    
    return NULL;
}

/**
 * Check if a domain contains a CPU
 * 
 * @param domain Domain to check
 * @param cpu CPU to check for
 * @return 1 if the domain contains the CPU, 0 if not
 */
int sched_domain_contains_cpu(sched_domain_t *domain, int cpu) {
    /* Check parameters */
    if (domain == NULL || cpu < 0 || cpu >= CONFIG_NR_CPUS) {
        return 0;
    }
    
    /* Check if the domain contains the CPU */
    for (int i = 0; i < domain->group_count; i++) {
        sched_group_t *group = &domain->groups[i];
        
        /* Check if the group contains the CPU */
        if (group->cpu_mask & (1ULL << cpu)) {
            return 1;
        }
    }
    
    return 0;
}

/**
 * Get the CPU mask for a domain
 * 
 * @param domain Domain to get CPU mask for
 * @return CPU mask for the domain
 */
u64 sched_domain_get_cpu_mask(sched_domain_t *domain) {
    /* Check parameters */
    if (domain == NULL) {
        return 0;
    }
    
    /* Calculate the CPU mask */
    u64 cpu_mask = 0;
    
    for (int i = 0; i < domain->group_count; i++) {
        sched_group_t *group = &domain->groups[i];
        cpu_mask |= group->cpu_mask;
    }
    
    return cpu_mask;
}

/**
 * Print scheduler domain information
 */
void sched_domain_print(void) {
    /* Print the domain count */
    printk(KERN_INFO "SCHED_DOMAIN: Domains: %d\n", domain_count);
    
    /* Print each domain */
    for (int i = 0; i < domain_count; i++) {
        sched_domain_t *domain = &domains[i];
        
        printk(KERN_INFO "SCHED_DOMAIN: Domain %d: Groups: %d, Flags: 0x%08x\n",
               domain->id, domain->group_count, domain->flags);
        
        /* Print each group */
        for (int j = 0; j < domain->group_count; j++) {
            sched_group_t *group = &domain->groups[j];
            
            printk(KERN_INFO "SCHED_DOMAIN: Domain %d, Group %d: CPU Mask: 0x%016llx\n",
                   domain->id, group->id, group->cpu_mask);
        }
    }
}

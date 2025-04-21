/**
 * sched_domain.h - Horizon kernel scheduler domains definitions
 * 
 * This file contains definitions for scheduler domains.
 */

#ifndef _HORIZON_SCHED_DOMAIN_H
#define _HORIZON_SCHED_DOMAIN_H

#include <horizon/types.h>

/* Scheduler domain flags */
#define SD_LOAD_BALANCE      0x00000001 /* Do load balancing on this domain */
#define SD_BALANCE_NEWIDLE   0x00000002 /* Balance when about to become idle */
#define SD_BALANCE_EXEC      0x00000004 /* Balance on exec */
#define SD_BALANCE_FORK      0x00000008 /* Balance on fork */
#define SD_BALANCE_WAKE      0x00000010 /* Balance on wakeup */
#define SD_WAKE_AFFINE       0x00000020 /* Wake tasks to CPU where they ran last */
#define SD_SHARE_CPUPOWER    0x00000040 /* Domain members share CPU power */
#define SD_SHARE_PKG_RESOURCES 0x00000080 /* Domain members share package resources */
#define SD_SERIALIZE         0x00000100 /* Only one load balancing instance */
#define SD_ASYM_PACKING      0x00000200 /* Pack tasks tightly on lower capacity CPUs */
#define SD_PREFER_SIBLING    0x00000400 /* Prefer to place tasks in a sibling domain */
#define SD_OVERLAP           0x00000800 /* Domains overlap */
#define SD_NUMA              0x00001000 /* Domain is NUMA */

/* Forward declarations */
struct sched_domain;
struct sched_group;

/* Scheduler group structure */
typedef struct sched_group {
    int id;                 /* Group ID */
    u64 cpu_mask;           /* CPU mask */
} sched_group_t;

/* Scheduler domain structure */
typedef struct sched_domain {
    int id;                 /* Domain ID */
    struct sched_domain *parent; /* Parent domain */
    u32 flags;              /* Domain flags */
    u32 min_interval;       /* Minimum balance interval */
    u32 max_interval;       /* Maximum balance interval */
    u32 busy_factor;        /* Busy factor */
    u32 imbalance_pct;      /* Imbalance percentage */
    u32 cache_nice_tries;   /* Cache nice tries */
    int group_count;        /* Number of groups */
    struct sched_group groups[8]; /* Groups in this domain */
} sched_domain_t;

/* Initialize the scheduler domains */
void sched_domain_init(void);

/* Create a scheduler domain */
sched_domain_t *sched_domain_create(sched_domain_t *parent, u32 flags);

/* Add a group to a domain */
sched_group_t *sched_domain_add_group(sched_domain_t *domain, u64 cpu_mask);

/* Find a domain by ID */
sched_domain_t *sched_domain_find(int id);

/* Find a group by ID in a domain */
sched_group_t *sched_domain_find_group(sched_domain_t *domain, int id);

/* Find a domain for a CPU */
sched_domain_t *sched_domain_find_for_cpu(int cpu);

/* Find a group for a CPU in a domain */
sched_group_t *sched_domain_find_group_for_cpu(sched_domain_t *domain, int cpu);

/* Check if a domain contains a CPU */
int sched_domain_contains_cpu(sched_domain_t *domain, int cpu);

/* Get the CPU mask for a domain */
u64 sched_domain_get_cpu_mask(sched_domain_t *domain);

/* Print scheduler domain information */
void sched_domain_print(void);

#endif /* _HORIZON_SCHED_DOMAIN_H */

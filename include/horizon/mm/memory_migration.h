/**
 * memory_migration.h - Horizon kernel memory migration definitions
 * 
 * This file contains definitions for memory migration.
 */

#ifndef _HORIZON_MM_MEMORY_MIGRATION_H
#define _HORIZON_MM_MEMORY_MIGRATION_H

#include <horizon/types.h>

/* Forward declarations */
struct task_struct;

/* Initialize the memory migration subsystem */
void memory_migration_init(void);

/* Enable or disable memory migration */
int memory_migration_enable(int enable);

/* Set the memory migration interval */
int memory_migration_set_interval(u64 interval);

/* Set the memory migration threshold */
int memory_migration_set_threshold(u64 threshold);

/* Check if memory migration is needed */
int memory_migration_needed(void);

/* Check if there is an imbalance between NUMA nodes */
int memory_migration_check_imbalance(void);

/* Migrate memory between NUMA nodes */
int memory_migration_run(void);

/* Migrate a specific memory range */
int memory_migration_range(void *addr, size_t size, int target_node);

/* Migrate a task's memory */
int memory_migration_task(struct task_struct *task, int target_node);

/* Print memory migration statistics */
void memory_migration_print_stats(void);

#endif /* _HORIZON_MM_MEMORY_MIGRATION_H */

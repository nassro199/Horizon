/**
 * cpumask.h - CPU mask definitions
 *
 * This file contains the CPU mask definitions.
 */

#ifndef _HORIZON_CPUMASK_H
#define _HORIZON_CPUMASK_H

#include <horizon/types.h>
#include <horizon/config.h>

/* CPU mask structure */
typedef struct cpumask {
    u32 bits[CONFIG_NR_CPUS / 32 + 1];
} cpumask_t;

/* CPU mask functions */
void cpumask_clear(cpumask_t *mask);
void cpumask_set_cpu(u32 cpu, cpumask_t *mask);
void cpumask_clear_cpu(u32 cpu, cpumask_t *mask);
int cpumask_test_cpu(u32 cpu, const cpumask_t *mask);
int cpumask_empty(const cpumask_t *mask);
int cpumask_full(const cpumask_t *mask);
int cpumask_weight(const cpumask_t *mask);
void cpumask_copy(cpumask_t *dst, const cpumask_t *src);
void cpumask_and(cpumask_t *dst, const cpumask_t *src1, const cpumask_t *src2);
void cpumask_or(cpumask_t *dst, const cpumask_t *src1, const cpumask_t *src2);
void cpumask_xor(cpumask_t *dst, const cpumask_t *src1, const cpumask_t *src2);
void cpumask_complement(cpumask_t *dst, const cpumask_t *src);
int cpumask_equal(const cpumask_t *src1, const cpumask_t *src2);
int cpumask_subset(const cpumask_t *src1, const cpumask_t *src2);
int cpumask_intersects(const cpumask_t *src1, const cpumask_t *src2);
int cpumask_any(const cpumask_t *mask);
int cpumask_first(const cpumask_t *mask);
int cpumask_next(int cpu, const cpumask_t *mask);
int cpumask_last(const cpumask_t *mask);

/* CPU mask macros */
#define for_each_cpu(cpu, mask) \
    for ((cpu) = cpumask_first(mask); \
         (cpu) < CONFIG_NR_CPUS; \
         (cpu) = cpumask_next((cpu), (mask)))

#define for_each_possible_cpu(cpu) \
    for ((cpu) = 0; (cpu) < CONFIG_NR_CPUS; (cpu)++)

/* CPU mask variables */
extern cpumask_t cpu_possible_mask;
extern cpumask_t cpu_online_mask;
extern cpumask_t cpu_present_mask;
extern cpumask_t cpu_active_mask;

#endif /* _HORIZON_CPUMASK_H */

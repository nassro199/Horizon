/**
 * smp.h - Horizon kernel SMP definitions
 * 
 * This file contains definitions for symmetric multiprocessing.
 */

#ifndef _HORIZON_SMP_H
#define _HORIZON_SMP_H

#include <horizon/types.h>
#include <horizon/spinlock.h>

/* Maximum number of CPUs */
#define NR_CPUS 32

/* CPU states */
#define CPU_OFFLINE     0  /* CPU is offline */
#define CPU_ONLINE      1  /* CPU is online */
#define CPU_DEAD        2  /* CPU is dead */
#define CPU_DYING       3  /* CPU is dying */

/* CPU mask */
typedef struct cpumask {
    unsigned long bits[NR_CPUS / (8 * sizeof(unsigned long))];
} cpumask_t;

/* Per-CPU data */
struct percpu_data {
    int cpu_id;                    /* CPU ID */
    int cpu_state;                 /* CPU state */
    struct task_struct *current;   /* Current task */
    struct task_struct *idle;      /* Idle task */
    void *stack;                   /* CPU stack */
    unsigned long flags;           /* CPU flags */
    unsigned long irq_count;       /* IRQ count */
    unsigned long softirq_count;   /* Soft IRQ count */
    unsigned long timer_count;     /* Timer count */
    unsigned long syscall_count;   /* System call count */
    unsigned long context_switches; /* Context switches */
    unsigned long ticks;           /* CPU ticks */
    unsigned long idle_ticks;      /* Idle ticks */
    unsigned long user_ticks;      /* User ticks */
    unsigned long system_ticks;    /* System ticks */
    unsigned long irq_ticks;       /* IRQ ticks */
    unsigned long softirq_ticks;   /* Soft IRQ ticks */
    unsigned long steal_ticks;     /* Steal ticks */
    unsigned long guest_ticks;     /* Guest ticks */
    unsigned long guest_nice_ticks; /* Guest nice ticks */
    unsigned long iowait_ticks;    /* I/O wait ticks */
    unsigned long nice_ticks;      /* Nice ticks */
};

/* CPU mask operations */
#define cpu_set(cpu, mask) ((mask)->bits[(cpu) / (8 * sizeof(unsigned long))] |= (1UL << ((cpu) % (8 * sizeof(unsigned long)))))
#define cpu_clear(cpu, mask) ((mask)->bits[(cpu) / (8 * sizeof(unsigned long))] &= ~(1UL << ((cpu) % (8 * sizeof(unsigned long)))))
#define cpu_isset(cpu, mask) ((mask)->bits[(cpu) / (8 * sizeof(unsigned long))] & (1UL << ((cpu) % (8 * sizeof(unsigned long)))))
#define cpu_test_and_set(cpu, mask) ({ \
    int __ret = cpu_isset(cpu, mask); \
    cpu_set(cpu, mask); \
    __ret; \
})
#define cpu_test_and_clear(cpu, mask) ({ \
    int __ret = cpu_isset(cpu, mask); \
    cpu_clear(cpu, mask); \
    __ret; \
})
#define cpus_clear(mask) memset(&(mask), 0, sizeof(cpumask_t))
#define cpus_setall(mask) memset(&(mask), 0xFF, sizeof(cpumask_t))
#define cpus_equal(mask1, mask2) (memcmp(&(mask1), &(mask2), sizeof(cpumask_t)) == 0)
#define cpus_empty(mask) ({ \
    int __ret = 1; \
    int __i; \
    for (__i = 0; __i < NR_CPUS / (8 * sizeof(unsigned long)); __i++) { \
        if ((mask).bits[__i] != 0) { \
            __ret = 0; \
            break; \
        } \
    } \
    __ret; \
})
#define cpus_weight(mask) ({ \
    int __ret = 0; \
    int __i; \
    for (__i = 0; __i < NR_CPUS; __i++) { \
        if (cpu_isset(__i, &(mask))) { \
            __ret++; \
        } \
    } \
    __ret; \
})
#define cpus_complement(dst, src) ({ \
    int __i; \
    for (__i = 0; __i < NR_CPUS / (8 * sizeof(unsigned long)); __i++) { \
        (dst).bits[__i] = ~(src).bits[__i]; \
    } \
})
#define cpus_and(dst, src1, src2) ({ \
    int __i; \
    for (__i = 0; __i < NR_CPUS / (8 * sizeof(unsigned long)); __i++) { \
        (dst).bits[__i] = (src1).bits[__i] & (src2).bits[__i]; \
    } \
})
#define cpus_or(dst, src1, src2) ({ \
    int __i; \
    for (__i = 0; __i < NR_CPUS / (8 * sizeof(unsigned long)); __i++) { \
        (dst).bits[__i] = (src1).bits[__i] | (src2).bits[__i]; \
    } \
})
#define cpus_xor(dst, src1, src2) ({ \
    int __i; \
    for (__i = 0; __i < NR_CPUS / (8 * sizeof(unsigned long)); __i++) { \
        (dst).bits[__i] = (src1).bits[__i] ^ (src2).bits[__i]; \
    } \
})

/* SMP functions */
void smp_init(void);
void smp_boot_cpus(void);
void smp_prepare_cpus(unsigned int max_cpus);
int smp_processor_id(void);
int smp_num_cpus(void);
int smp_call_function(void (*func)(void *), void *info, int wait);
int smp_call_function_single(int cpu, void (*func)(void *), void *info, int wait);
void smp_send_reschedule(int cpu);
void smp_send_stop(void);
void smp_prepare_boot_cpu(void);
void smp_setup_processor_id(void);
int smp_cpu_online(int cpu);
int smp_cpu_offline(int cpu);
int smp_cpu_present(int cpu);
int smp_cpu_possible(int cpu);

/* Per-CPU data access */
extern struct percpu_data *percpu_data[NR_CPUS];
#define get_cpu_var(var) (percpu_data[smp_processor_id()]->var)
#define put_cpu_var(var) do { } while (0)
#define this_cpu_read(var) get_cpu_var(var)
#define this_cpu_write(var, val) do { get_cpu_var(var) = (val); } while (0)
#define this_cpu_add(var, val) do { get_cpu_var(var) += (val); } while (0)
#define this_cpu_sub(var, val) do { get_cpu_var(var) -= (val); } while (0)
#define this_cpu_inc(var) do { get_cpu_var(var)++; } while (0)
#define this_cpu_dec(var) do { get_cpu_var(var)--; } while (0)

/* CPU masks */
extern cpumask_t cpu_online_mask;
extern cpumask_t cpu_possible_mask;
extern cpumask_t cpu_present_mask;
extern cpumask_t cpu_active_mask;

#endif /* _HORIZON_SMP_H */

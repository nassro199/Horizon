/**
 * smp.c - Horizon kernel SMP implementation
 *
 * This file contains the implementation of symmetric multiprocessing.
 */

#define NULL ((void *)0)

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/stddef.h>
#include <horizon/smp.h>
#include <horizon/spinlock.h>
#include <horizon/mm.h>
#include <horizon/sched.h>
#include <horizon/task.h>
#include <horizon/interrupt.h>
#include <horizon/errno.h>

/* IPI types */
#define IPI_CALL_FUNC   0
#define IPI_RESCHEDULE  1
#define IPI_STOP        2

/* Per-CPU data */
struct percpu_data *percpu_data[NR_CPUS];

/* CPU masks */
cpumask_t cpu_online_mask;
cpumask_t cpu_possible_mask;
cpumask_t cpu_present_mask;
cpumask_t cpu_active_mask;

/* SMP lock */
static spinlock_t smp_lock = { { 0 } };

/* Boot CPU ID */
static int boot_cpu_id = 0;

/* Number of CPUs */
static int num_cpus = 1;

/* Maximum number of CPUs */
static int max_cpus = NR_CPUS;

/* CPU states */
static int cpu_states[NR_CPUS];

/* CPU call function */
typedef struct {
    void (*func)(void *);          /* Function to call */
    void *info;                    /* Function argument */
    int wait;                      /* Wait flag */
    int done;                      /* Done flag */
    cpumask_t cpus;                /* CPUs to call */
} smp_call_t;

/* CPU call queue */
static smp_call_t *cpu_call_queue[NR_CPUS];

/* CPU call lock */
static spinlock_t cpu_call_lock = { { 0 } };

/**
 * Initialize SMP
 */
void smp_init(void) {
    int i;

    /* Initialize CPU masks */
    cpus_clear(cpu_online_mask);
    cpus_clear(cpu_possible_mask);
    cpus_clear(cpu_present_mask);
    cpus_clear(cpu_active_mask);

    /* Set boot CPU */
    cpu_set(boot_cpu_id, &cpu_online_mask);
    cpu_set(boot_cpu_id, &cpu_possible_mask);
    cpu_set(boot_cpu_id, &cpu_present_mask);
    cpu_set(boot_cpu_id, &cpu_active_mask);

    /* Initialize CPU states */
    for (i = 0; i < NR_CPUS; i++) {
        cpu_states[i] = CPU_OFFLINE;
    }
    cpu_states[boot_cpu_id] = CPU_ONLINE;

    /* Initialize per-CPU data */
    for (i = 0; i < NR_CPUS; i++) {
        percpu_data[i] = NULL;
    }

    /* Allocate per-CPU data for boot CPU */
    percpu_data[boot_cpu_id] = kmalloc(sizeof(struct percpu_data), MEM_KERNEL | MEM_ZERO);
    if (percpu_data[boot_cpu_id] == NULL) {
        kernel_panic("Failed to allocate per-CPU data for boot CPU");
    }

    /* Initialize per-CPU data for boot CPU */
    percpu_data[boot_cpu_id]->cpu_id = boot_cpu_id;
    percpu_data[boot_cpu_id]->cpu_state = CPU_ONLINE;
    percpu_data[boot_cpu_id]->current = current;
    percpu_data[boot_cpu_id]->idle = NULL; /* Will be set by scheduler */

    /* Initialize CPU call queue */
    for (i = 0; i < NR_CPUS; i++) {
        cpu_call_queue[i] = NULL;
    }
}

/**
 * Prepare CPUs for boot
 *
 * @param max_cpus Maximum number of CPUs
 */
void smp_prepare_cpus(unsigned int max_cpus) {
    /* Set maximum number of CPUs */
    if (max_cpus > NR_CPUS) {
        max_cpus = NR_CPUS;
    }
    if (max_cpus < 1) {
        max_cpus = 1;
    }

    /* Set maximum number of CPUs */
    max_cpus = max_cpus;

    /* Prepare CPUs */
    int i;
    for (i = 0; i < max_cpus; i++) {
        if (i != boot_cpu_id) {
            /* Set CPU as possible */
            cpu_set(i, &cpu_possible_mask);
        }
    }
}

/**
 * Boot secondary CPUs
 */
void smp_boot_cpus(void) {
    int i;

    /* Boot secondary CPUs */
    for (i = 0; i < max_cpus; i++) {
        if (i != boot_cpu_id && cpu_isset(i, &cpu_possible_mask)) {
            /* Boot CPU */
            if (arch_smp_boot_cpu(i) == 0) {
                /* CPU booted successfully */
                cpu_set(i, &cpu_present_mask);
                cpu_states[i] = CPU_ONLINE;
                num_cpus++;
            }
        }
    }
}

/**
 * Get current processor ID
 *
 * @return Processor ID
 */
int smp_processor_id(void) {
    /* This would be implemented with architecture-specific code */
    /* For now, just return boot CPU ID */
    return boot_cpu_id;
}

/**
 * Get number of CPUs
 *
 * @return Number of CPUs
 */
int smp_num_cpus(void) {
    return num_cpus;
}

/**
 * Call a function on all CPUs
 *
 * @param func Function to call
 * @param info Function argument
 * @param wait Wait flag
 * @return 0 on success, negative error code on failure
 */
int smp_call_function(void (*func)(void *), void *info, int wait) {
    int i;
    smp_call_t call;

    /* Check parameters */
    if (func == NULL) {
        return -EINVAL;
    }

    /* Initialize call */
    call.func = func;
    call.info = info;
    call.wait = wait;
    call.done = 0;
    cpus_clear(call.cpus);

    /* Set CPUs to call */
    for (i = 0; i < NR_CPUS; i++) {
        if (i != smp_processor_id() && cpu_isset(i, &cpu_online_mask)) {
            cpu_set(i, &call.cpus);
        }
    }

    /* Check if there are any CPUs to call */
    if (cpus_empty(call.cpus)) {
        return 0;
    }

    /* Add call to queue */
    spin_lock(&cpu_call_lock);
    for (i = 0; i < NR_CPUS; i++) {
        if (cpu_isset(i, &call.cpus)) {
            cpu_call_queue[i] = &call;
        }
    }
    spin_unlock(&cpu_call_lock);

    /* Send IPI to CPUs */
    for (i = 0; i < NR_CPUS; i++) {
        if (cpu_isset(i, &call.cpus)) {
            arch_smp_send_ipi(i, IPI_CALL_FUNC);
        }
    }

    /* Wait for call to complete */
    if (wait) {
        while (call.done < cpus_weight(call.cpus)) {
            /* Wait */
            arch_cpu_relax();
        }
    }

    return 0;
}

/**
 * Call a function on a single CPU
 *
 * @param cpu CPU to call
 * @param func Function to call
 * @param info Function argument
 * @param wait Wait flag
 * @return 0 on success, negative error code on failure
 */
int smp_call_function_single(int cpu, void (*func)(void *), void *info, int wait) {
    smp_call_t call;

    /* Check parameters */
    if (func == NULL || cpu < 0 || cpu >= NR_CPUS) {
        return -EINVAL;
    }

    /* Check if CPU is online */
    if (!cpu_isset(cpu, &cpu_online_mask)) {
        return -ENODEV;
    }

    /* Check if CPU is current */
    if (cpu == smp_processor_id()) {
        func(info);
        return 0;
    }

    /* Initialize call */
    call.func = func;
    call.info = info;
    call.wait = wait;
    call.done = 0;
    cpus_clear(call.cpus);
    cpu_set(cpu, &call.cpus);

    /* Add call to queue */
    spin_lock(&cpu_call_lock);
    cpu_call_queue[cpu] = &call;
    spin_unlock(&cpu_call_lock);

    /* Send IPI to CPU */
    arch_smp_send_ipi(cpu, IPI_CALL_FUNC);

    /* Wait for call to complete */
    if (wait) {
        while (call.done < 1) {
            /* Wait */
            arch_cpu_relax();
        }
    }

    return 0;
}

/**
 * Send reschedule IPI to a CPU
 *
 * @param cpu CPU to send to
 */
void smp_send_reschedule(int cpu) {
    /* Check parameters */
    if (cpu < 0 || cpu >= NR_CPUS) {
        return;
    }

    /* Check if CPU is online */
    if (!cpu_isset(cpu, &cpu_online_mask)) {
        return;
    }

    /* Send IPI */
    arch_smp_send_ipi(cpu, IPI_RESCHEDULE);
}

/**
 * Send stop IPI to all CPUs
 */
void smp_send_stop(void) {
    int i;

    /* Send IPI to all CPUs */
    for (i = 0; i < NR_CPUS; i++) {
        if (i != smp_processor_id() && cpu_isset(i, &cpu_online_mask)) {
            arch_smp_send_ipi(i, IPI_STOP);
        }
    }
}

/**
 * Prepare boot CPU
 */
void smp_prepare_boot_cpu(void) {
    /* Set boot CPU ID */
    boot_cpu_id = arch_smp_processor_id();

    /* Set boot CPU as online */
    cpu_set(boot_cpu_id, &cpu_online_mask);
    cpu_set(boot_cpu_id, &cpu_possible_mask);
    cpu_set(boot_cpu_id, &cpu_present_mask);
    cpu_set(boot_cpu_id, &cpu_active_mask);

    /* Set boot CPU state */
    cpu_states[boot_cpu_id] = CPU_ONLINE;
}

/**
 * Setup processor ID
 */
void smp_setup_processor_id(void) {
    /* This would be implemented with architecture-specific code */
    /* For now, just set boot CPU ID */
    boot_cpu_id = 0;
}

/**
 * Check if CPU is online
 *
 * @param cpu CPU to check
 * @return 1 if online, 0 if not
 */
int smp_cpu_online(int cpu) {
    /* Check parameters */
    if (cpu < 0 || cpu >= NR_CPUS) {
        return 0;
    }

    return cpu_isset(cpu, &cpu_online_mask);
}

/**
 * Check if CPU is offline
 *
 * @param cpu CPU to check
 * @return 1 if offline, 0 if not
 */
int smp_cpu_offline(int cpu) {
    /* Check parameters */
    if (cpu < 0 || cpu >= NR_CPUS) {
        return 1;
    }

    return !cpu_isset(cpu, &cpu_online_mask);
}

/**
 * Check if CPU is present
 *
 * @param cpu CPU to check
 * @return 1 if present, 0 if not
 */
int smp_cpu_present(int cpu) {
    /* Check parameters */
    if (cpu < 0 || cpu >= NR_CPUS) {
        return 0;
    }

    return cpu_isset(cpu, &cpu_present_mask);
}

/**
 * Check if CPU is possible
 *
 * @param cpu CPU to check
 * @return 1 if possible, 0 if not
 */
int smp_cpu_possible(int cpu) {
    /* Check parameters */
    if (cpu < 0 || cpu >= NR_CPUS) {
        return 0;
    }

    return cpu_isset(cpu, &cpu_possible_mask);
}

/**
 * Handle call function IPI
 */
void smp_handle_call_function_ipi(void) {
    int cpu = smp_processor_id();
    smp_call_t *call;

    /* Get call */
    spin_lock(&cpu_call_lock);
    call = cpu_call_queue[cpu];
    cpu_call_queue[cpu] = NULL;
    spin_unlock(&cpu_call_lock);

    /* Check if there is a call */
    if (call == NULL) {
        return;
    }

    /* Call function */
    call->func(call->info);

    /* Mark as done */
    if (call->wait) {
        call->done++;
    }
}

/**
 * Handle reschedule IPI
 */
void smp_handle_reschedule_ipi(void) {
    /* Reschedule */
    schedule();
}

/**
 * Handle stop IPI
 */
void smp_handle_stop_ipi(void) {
    /* Stop CPU */
    int cpu = smp_processor_id();

    /* Set CPU state */
    cpu_states[cpu] = CPU_OFFLINE;

    /* Clear CPU from masks */
    cpu_clear(cpu, &cpu_online_mask);
    cpu_clear(cpu, &cpu_active_mask);

    /* Halt CPU */
    for (;;) {
        arch_cpu_halt();
    }
}

/**
 * sched.c - Horizon kernel process scheduling implementation
 * 
 * This file contains the implementation of the process scheduling subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/process.h>
#include <horizon/sched.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Process scheduling priorities */
#define PRIO_PROCESS    0       /* Process ID */
#define PRIO_PGRP       1       /* Process group ID */
#define PRIO_USER       2       /* User ID */

/* Process scheduling priority limits */
#define PRIO_MIN        -20     /* Minimum priority (highest) */
#define PRIO_MAX        20      /* Maximum priority (lowest) */

/* Process scheduling policies */
#define SCHED_OTHER     0       /* Default scheduling policy */
#define SCHED_FIFO      1       /* First-in, first-out scheduling policy */
#define SCHED_RR        2       /* Round-robin scheduling policy */
#define SCHED_BATCH     3       /* Batch scheduling policy */
#define SCHED_IDLE      5       /* Idle scheduling policy */
#define SCHED_DEADLINE  6       /* Deadline scheduling policy */

/* Initialize the process scheduling subsystem */
void process_sched_init(void) {
    /* Initialize the process scheduling subsystem */
}

/* Change process priority */
int process_nice(int inc) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Get the current nice value */
    int nice = task->static_prio - 120;
    
    /* Calculate the new nice value */
    int new_nice = nice + inc;
    
    /* Check if the new nice value is valid */
    if (new_nice < PRIO_MIN) {
        new_nice = PRIO_MIN;
    } else if (new_nice > PRIO_MAX) {
        new_nice = PRIO_MAX;
    }
    
    /* Set the new priority */
    task->static_prio = 120 + new_nice;
    
    /* Recalculate the dynamic priority */
    task->prio = task->static_prio;
    
    return new_nice;
}

/* Get program scheduling priority */
int process_getpriority(int which, int who) {
    /* Check if the which parameter is valid */
    if (which < PRIO_PROCESS || which > PRIO_USER) {
        return -1;
    }
    
    /* Get the task */
    task_struct_t *task = NULL;
    
    switch (which) {
        case PRIO_PROCESS:
            /* Get the task by process ID */
            if (who == 0) {
                /* Get the current task */
                task = task_current();
            } else {
                /* Get the task by process ID */
                task = task_get(who);
            }
            break;
        
        case PRIO_PGRP:
            /* Get the task by process group ID */
            if (who == 0) {
                /* Get the current task's process group */
                task = task_current();
                who = task->pgid;
            }
            
            /* Find the task with the lowest priority in the process group */
            task = task_get_by_pgid(who);
            break;
        
        case PRIO_USER:
            /* Get the task by user ID */
            if (who == 0) {
                /* Get the current task's user ID */
                task = task_current();
                who = task->uid;
            }
            
            /* Find the task with the lowest priority for the user */
            task = task_get_by_uid(who);
            break;
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Return the nice value */
    return task->static_prio - 120;
}

/* Set program scheduling priority */
int process_setpriority(int which, int who, int prio) {
    /* Check if the which parameter is valid */
    if (which < PRIO_PROCESS || which > PRIO_USER) {
        return -1;
    }
    
    /* Check if the priority is valid */
    if (prio < PRIO_MIN) {
        prio = PRIO_MIN;
    } else if (prio > PRIO_MAX) {
        prio = PRIO_MAX;
    }
    
    /* Get the task */
    task_struct_t *task = NULL;
    
    switch (which) {
        case PRIO_PROCESS:
            /* Get the task by process ID */
            if (who == 0) {
                /* Get the current task */
                task = task_current();
            } else {
                /* Get the task by process ID */
                task = task_get(who);
            }
            
            /* Check if the task was found */
            if (task == NULL) {
                return -1;
            }
            
            /* Set the priority */
            task->static_prio = 120 + prio;
            
            /* Recalculate the dynamic priority */
            task->prio = task->static_prio;
            break;
        
        case PRIO_PGRP:
            /* Get the task by process group ID */
            if (who == 0) {
                /* Get the current task's process group */
                task = task_current();
                who = task->pgid;
            }
            
            /* Set the priority for all tasks in the process group */
            task_set_prio_by_pgid(who, 120 + prio);
            break;
        
        case PRIO_USER:
            /* Get the task by user ID */
            if (who == 0) {
                /* Get the current task's user ID */
                task = task_current();
                who = task->uid;
            }
            
            /* Set the priority for all tasks for the user */
            task_set_prio_by_uid(who, 120 + prio);
            break;
    }
    
    return 0;
}

/* Get scheduling policy */
int process_sched_getscheduler(pid_t pid) {
    /* Get the task */
    task_struct_t *task = NULL;
    
    if (pid == 0) {
        /* Get the current task */
        task = task_current();
    } else {
        /* Get the task by process ID */
        task = task_get(pid);
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Return the scheduling policy */
    return task->policy;
}

/* Set scheduling policy and parameters */
int process_sched_setscheduler(pid_t pid, int policy, const struct sched_param *param) {
    /* Check if the policy is valid */
    if (policy < 0 || policy > SCHED_DEADLINE) {
        return -1;
    }
    
    /* Check if the parameters are valid */
    if (param == NULL) {
        return -1;
    }
    
    /* Get the task */
    task_struct_t *task = NULL;
    
    if (pid == 0) {
        /* Get the current task */
        task = task_current();
    } else {
        /* Get the task by process ID */
        task = task_get(pid);
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Set the scheduling policy */
    task->policy = policy;
    
    /* Set the scheduling parameters */
    task->rt_priority = param->sched_priority;
    
    return 0;
}

/* Get scheduling parameters */
int process_sched_getparam(pid_t pid, struct sched_param *param) {
    /* Check if the parameters are valid */
    if (param == NULL) {
        return -1;
    }
    
    /* Get the task */
    task_struct_t *task = NULL;
    
    if (pid == 0) {
        /* Get the current task */
        task = task_current();
    } else {
        /* Get the task by process ID */
        task = task_get(pid);
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Get the scheduling parameters */
    param->sched_priority = task->rt_priority;
    
    return 0;
}

/* Set scheduling parameters */
int process_sched_setparam(pid_t pid, const struct sched_param *param) {
    /* Check if the parameters are valid */
    if (param == NULL) {
        return -1;
    }
    
    /* Get the task */
    task_struct_t *task = NULL;
    
    if (pid == 0) {
        /* Get the current task */
        task = task_current();
    } else {
        /* Get the task by process ID */
        task = task_get(pid);
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Set the scheduling parameters */
    task->rt_priority = param->sched_priority;
    
    return 0;
}

/* Get maximum scheduling priority */
int process_sched_get_priority_max(int policy) {
    /* Check if the policy is valid */
    if (policy < 0 || policy > SCHED_DEADLINE) {
        return -1;
    }
    
    /* Return the maximum priority */
    switch (policy) {
        case SCHED_OTHER:
        case SCHED_BATCH:
        case SCHED_IDLE:
            return 0;
        
        case SCHED_FIFO:
        case SCHED_RR:
            return 99;
        
        case SCHED_DEADLINE:
            return 0;
        
        default:
            return -1;
    }
}

/* Get minimum scheduling priority */
int process_sched_get_priority_min(int policy) {
    /* Check if the policy is valid */
    if (policy < 0 || policy > SCHED_DEADLINE) {
        return -1;
    }
    
    /* Return the minimum priority */
    switch (policy) {
        case SCHED_OTHER:
        case SCHED_BATCH:
        case SCHED_IDLE:
            return 0;
        
        case SCHED_FIFO:
        case SCHED_RR:
            return 1;
        
        case SCHED_DEADLINE:
            return 0;
        
        default:
            return -1;
    }
}

/* Get round-robin time quantum */
int process_sched_rr_get_interval(pid_t pid, struct timespec *tp) {
    /* Check if the parameters are valid */
    if (tp == NULL) {
        return -1;
    }
    
    /* Get the task */
    task_struct_t *task = NULL;
    
    if (pid == 0) {
        /* Get the current task */
        task = task_current();
    } else {
        /* Get the task by process ID */
        task = task_get(pid);
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Check if the task uses round-robin scheduling */
    if (task->policy != SCHED_RR) {
        return -1;
    }
    
    /* Set the time quantum */
    tp->tv_sec = 0;
    tp->tv_nsec = 100000000; /* 100 ms */
    
    return 0;
}

/* Yield the processor */
int process_sched_yield(void) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Yield the processor */
    task_yield(task);
    
    return 0;
}

/* Get CPU affinity */
int process_sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask) {
    /* Check if the parameters are valid */
    if (mask == NULL || cpusetsize < sizeof(cpu_set_t)) {
        return -1;
    }
    
    /* Get the task */
    task_struct_t *task = NULL;
    
    if (pid == 0) {
        /* Get the current task */
        task = task_current();
    } else {
        /* Get the task by process ID */
        task = task_get(pid);
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Get the CPU affinity */
    memcpy(mask, &task->cpus_allowed, sizeof(cpu_set_t));
    
    return 0;
}

/* Set CPU affinity */
int process_sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask) {
    /* Check if the parameters are valid */
    if (mask == NULL || cpusetsize < sizeof(cpu_set_t)) {
        return -1;
    }
    
    /* Get the task */
    task_struct_t *task = NULL;
    
    if (pid == 0) {
        /* Get the current task */
        task = task_current();
    } else {
        /* Get the task by process ID */
        task = task_get(pid);
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Set the CPU affinity */
    memcpy(&task->cpus_allowed, mask, sizeof(cpu_set_t));
    
    return 0;
}

/**
 * resource.c - Horizon kernel process resource management implementation
 * 
 * This file contains the implementation of the process resource management subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/process.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Resource limits */
#define RLIMIT_CPU      0       /* CPU time in seconds */
#define RLIMIT_FSIZE    1       /* Maximum file size */
#define RLIMIT_DATA     2       /* Maximum data size */
#define RLIMIT_STACK    3       /* Maximum stack size */
#define RLIMIT_CORE     4       /* Maximum core file size */
#define RLIMIT_RSS      5       /* Maximum resident set size */
#define RLIMIT_NPROC    6       /* Maximum number of processes */
#define RLIMIT_NOFILE   7       /* Maximum number of open files */
#define RLIMIT_MEMLOCK  8       /* Maximum locked-in-memory address space */
#define RLIMIT_AS       9       /* Address space limit */
#define RLIMIT_LOCKS    10      /* Maximum file locks */
#define RLIMIT_SIGPENDING 11    /* Maximum number of pending signals */
#define RLIMIT_MSGQUEUE 12      /* Maximum bytes in POSIX message queues */
#define RLIMIT_NICE     13      /* Maximum nice priority */
#define RLIMIT_RTPRIO   14      /* Maximum real-time priority */
#define RLIMIT_RTTIME   15      /* Maximum real-time timeout */
#define RLIMIT_NLIMITS  16      /* Number of resource limits */

/* Initialize the process resource management subsystem */
void process_resource_init(void) {
    /* Initialize the process resource management subsystem */
}

/* Get resource limits */
int process_getrlimit(int resource, struct rlimit *rlim) {
    /* Check if the parameters are valid */
    if (rlim == NULL) {
        return -1;
    }
    
    /* Check if the resource is valid */
    if (resource < 0 || resource >= RLIMIT_NLIMITS) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Get the resource limit */
    memcpy(rlim, &task->rlim[resource], sizeof(struct rlimit));
    
    return 0;
}

/* Set resource limits */
int process_setrlimit(int resource, const struct rlimit *rlim) {
    /* Check if the parameters are valid */
    if (rlim == NULL) {
        return -1;
    }
    
    /* Check if the resource is valid */
    if (resource < 0 || resource >= RLIMIT_NLIMITS) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Check if the limit is valid */
    if (rlim->rlim_cur > rlim->rlim_max) {
        return -1;
    }
    
    /* Set the resource limit */
    memcpy(&task->rlim[resource], rlim, sizeof(struct rlimit));
    
    return 0;
}

/* Get/set resource limits */
int process_prlimit64(pid_t pid, int resource, const struct rlimit64 *new_limit, struct rlimit64 *old_limit) {
    /* Check if the resource is valid */
    if (resource < 0 || resource >= RLIMIT_NLIMITS) {
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
    
    /* Get the old limit */
    if (old_limit != NULL) {
        old_limit->rlim_cur = task->rlim[resource].rlim_cur;
        old_limit->rlim_max = task->rlim[resource].rlim_max;
    }
    
    /* Set the new limit */
    if (new_limit != NULL) {
        /* Check if the limit is valid */
        if (new_limit->rlim_cur > new_limit->rlim_max) {
            return -1;
        }
        
        /* Set the resource limit */
        task->rlim[resource].rlim_cur = new_limit->rlim_cur;
        task->rlim[resource].rlim_max = new_limit->rlim_max;
    }
    
    return 0;
}

/* Get resource usage */
int process_getrusage(int who, struct rusage *usage) {
    /* Check if the parameters are valid */
    if (usage == NULL) {
        return -1;
    }
    
    /* Check if the who parameter is valid */
    if (who != RUSAGE_SELF && who != RUSAGE_CHILDREN && who != RUSAGE_THREAD) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Get the resource usage */
    switch (who) {
        case RUSAGE_SELF:
            /* Get the resource usage for the current process */
            memcpy(usage, &task->rusage, sizeof(struct rusage));
            break;
        
        case RUSAGE_CHILDREN:
            /* Get the resource usage for the children */
            memcpy(usage, &task->rusage_children, sizeof(struct rusage));
            break;
        
        case RUSAGE_THREAD:
            /* Get the resource usage for the current thread */
            memcpy(usage, &task->rusage, sizeof(struct rusage));
            break;
    }
    
    return 0;
}

/* Get process times */
clock_t process_times(struct tms *buf) {
    /* Check if the parameters are valid */
    if (buf == NULL) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Get the process times */
    buf->tms_utime = task->utime;
    buf->tms_stime = task->stime;
    buf->tms_cutime = task->cutime;
    buf->tms_cstime = task->cstime;
    
    /* Return the clock ticks since system boot */
    return task->start_time;
}

/* Set the process execution domain */
int process_personality(unsigned long persona) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Get the old personality */
    unsigned long old_persona = task->personality;
    
    /* Set the new personality */
    task->personality = persona;
    
    /* Return the old personality */
    return old_persona;
}

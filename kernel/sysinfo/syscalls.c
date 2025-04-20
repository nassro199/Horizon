/**
 * syscalls.c - Horizon kernel system information system calls
 * 
 * This file contains the implementation of the system information system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/sysinfo.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: uname */
long sys_uname(long buf, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get system information */
    struct utsname *name = (struct utsname *)buf;
    
    if (name == NULL) {
        return -1;
    }
    
    /* Set the system information */
    strcpy(name->sysname, "Horizon");
    strcpy(name->nodename, "horizon");
    strcpy(name->release, "1.0.0");
    strcpy(name->version, "1.0.0");
    strcpy(name->machine, "x86_64");
    strcpy(name->domainname, "");
    
    return 0;
}

/* System call: sysinfo */
long sys_sysinfo(long info, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get system information */
    struct sysinfo *i = (struct sysinfo *)info;
    
    if (i == NULL) {
        return -1;
    }
    
    /* Get the system information */
    return sysinfo_get(i);
}

/* System call: getcpu */
long sys_getcpu(long cpu, long node, long tcache, long unused1, long unused2, long unused3) {
    /* Get CPU and node information */
    unsigned int *cpup = (unsigned int *)cpu;
    unsigned int *nodep = (unsigned int *)node;
    
    /* Get the CPU and node */
    return sysinfo_getcpu(cpup, nodep, tcache);
}

/* System call: sysctl */
long sys_sysctl(long args, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Read/write system parameters */
    struct __sysctl_args *a = (struct __sysctl_args *)args;
    
    if (a == NULL) {
        return -1;
    }
    
    /* Perform the sysctl operation */
    return sysinfo_sysctl(a);
}

/* System call: syslog */
long sys_syslog(long type, long buf, long len, long unused1, long unused2, long unused3) {
    /* Read and/or clear the kernel message ring buffer */
    return sysinfo_syslog(type, (char *)buf, len);
}

/* System call: getrusage */
long sys_getrusage(long who, long usage, long unused1, long unused2, long unused3, long unused4) {
    /* Get resource usage */
    struct rusage *u = (struct rusage *)usage;
    
    if (u == NULL) {
        return -1;
    }
    
    /* Get the resource usage */
    return sysinfo_getrusage(who, u);
}

/* System call: times */
long sys_times(long buf, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get process times */
    struct tms *t = (struct tms *)buf;
    
    if (t == NULL) {
        return -1;
    }
    
    /* Get the process times */
    return sysinfo_times(t);
}

/* System call: getloadavg */
long sys_getloadavg(long loadavg, long nelem, long unused1, long unused2, long unused3, long unused4) {
    /* Get system load averages */
    double *l = (double *)loadavg;
    
    if (l == NULL) {
        return -1;
    }
    
    /* Get the load averages */
    return sysinfo_getloadavg(l, nelem);
}

/* System call: sched_getaffinity */
long sys_sched_getaffinity(long pid, long len, long mask, long unused1, long unused2, long unused3) {
    /* Get CPU affinity mask */
    return sysinfo_sched_getaffinity(pid, len, (cpu_set_t *)mask);
}

/* System call: sched_setaffinity */
long sys_sched_setaffinity(long pid, long len, long mask, long unused1, long unused2, long unused3) {
    /* Set CPU affinity mask */
    return sysinfo_sched_setaffinity(pid, len, (const cpu_set_t *)mask);
}

/* System call: sched_getparam */
long sys_sched_getparam(long pid, long param, long unused1, long unused2, long unused3, long unused4) {
    /* Get scheduling parameters */
    struct sched_param *p = (struct sched_param *)param;
    
    if (p == NULL) {
        return -1;
    }
    
    /* Get the scheduling parameters */
    return sysinfo_sched_getparam(pid, p);
}

/* System call: sched_setparam */
long sys_sched_setparam(long pid, long param, long unused1, long unused2, long unused3, long unused4) {
    /* Set scheduling parameters */
    const struct sched_param *p = (const struct sched_param *)param;
    
    if (p == NULL) {
        return -1;
    }
    
    /* Set the scheduling parameters */
    return sysinfo_sched_setparam(pid, p);
}

/* System call: sched_getscheduler */
long sys_sched_getscheduler(long pid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get scheduling policy */
    return sysinfo_sched_getscheduler(pid);
}

/* System call: sched_setscheduler */
long sys_sched_setscheduler(long pid, long policy, long param, long unused1, long unused2, long unused3) {
    /* Set scheduling policy and parameters */
    const struct sched_param *p = (const struct sched_param *)param;
    
    if (p == NULL) {
        return -1;
    }
    
    /* Set the scheduling policy and parameters */
    return sysinfo_sched_setscheduler(pid, policy, p);
}

/* System call: sched_get_priority_max */
long sys_sched_get_priority_max(long policy, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get maximum scheduling priority */
    return sysinfo_sched_get_priority_max(policy);
}

/* System call: sched_get_priority_min */
long sys_sched_get_priority_min(long policy, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get minimum scheduling priority */
    return sysinfo_sched_get_priority_min(policy);
}

/* System call: sched_rr_get_interval */
long sys_sched_rr_get_interval(long pid, long interval, long unused1, long unused2, long unused3, long unused4) {
    /* Get round-robin time quantum */
    struct timespec *t = (struct timespec *)interval;
    
    if (t == NULL) {
        return -1;
    }
    
    /* Get the round-robin time quantum */
    return sysinfo_sched_rr_get_interval(pid, t);
}

/* System call: sched_yield */
long sys_sched_yield(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Yield the processor */
    return sysinfo_sched_yield();
}

/* Register system information system calls */
void sysinfo_syscalls_init(void) {
    /* Register system information system calls */
    syscall_register(SYS_UNAME, sys_uname);
    syscall_register(SYS_SYSINFO, sys_sysinfo);
    syscall_register(SYS_GETCPU, sys_getcpu);
    syscall_register(SYS_SYSCTL, sys_sysctl);
    syscall_register(SYS_SYSLOG, sys_syslog);
    syscall_register(SYS_GETRUSAGE, sys_getrusage);
    syscall_register(SYS_TIMES, sys_times);
    syscall_register(SYS_SCHED_GETAFFINITY, sys_sched_getaffinity);
    syscall_register(SYS_SCHED_SETAFFINITY, sys_sched_setaffinity);
    syscall_register(SYS_SCHED_GETPARAM, sys_sched_getparam);
    syscall_register(SYS_SCHED_SETPARAM, sys_sched_setparam);
    syscall_register(SYS_SCHED_GETSCHEDULER, sys_sched_getscheduler);
    syscall_register(SYS_SCHED_SETSCHEDULER, sys_sched_setscheduler);
    syscall_register(SYS_SCHED_GET_PRIORITY_MAX, sys_sched_get_priority_max);
    syscall_register(SYS_SCHED_GET_PRIORITY_MIN, sys_sched_get_priority_min);
    syscall_register(SYS_SCHED_RR_GET_INTERVAL, sys_sched_rr_get_interval);
    syscall_register(SYS_SCHED_YIELD, sys_sched_yield);
}

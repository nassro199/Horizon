/**
 * sysinfo.h - Horizon kernel system information definitions
 * 
 * This file contains definitions for the system information subsystem.
 */

#ifndef _KERNEL_SYSINFO_H
#define _KERNEL_SYSINFO_H

#include <horizon/types.h>

/* System information structure */
struct sysinfo {
    long uptime;                 /* Seconds since boot */
    unsigned long loads[3];      /* 1, 5, and 15 minute load averages */
    unsigned long totalram;      /* Total usable main memory size */
    unsigned long freeram;       /* Available memory size */
    unsigned long sharedram;     /* Amount of shared memory */
    unsigned long bufferram;     /* Memory used by buffers */
    unsigned long totalswap;     /* Total swap space size */
    unsigned long freeswap;      /* Swap space still available */
    unsigned short procs;        /* Number of current processes */
    unsigned long totalhigh;     /* Total high memory size */
    unsigned long freehigh;      /* Available high memory size */
    unsigned int mem_unit;       /* Memory unit size in bytes */
    char _f[20-2*sizeof(long)-sizeof(int)]; /* Padding */
};

/* System control structure */
struct __sysctl_args {
    int *name;                   /* Integer vector describing variable */
    int nlen;                    /* Length of this vector */
    void *oldval;                /* 0 or address where to store old value */
    size_t *oldlenp;             /* Available room for old value, overwritten by actual size of old value */
    void *newval;                /* 0 or address of new value */
    size_t newlen;               /* Size of new value */
};

/* Resource usage structure */
struct rusage {
    struct timeval ru_utime;     /* User time used */
    struct timeval ru_stime;     /* System time used */
    long ru_maxrss;              /* Maximum resident set size */
    long ru_ixrss;               /* Integral shared memory size */
    long ru_idrss;               /* Integral unshared data size */
    long ru_isrss;               /* Integral unshared stack size */
    long ru_minflt;              /* Page reclaims */
    long ru_majflt;              /* Page faults */
    long ru_nswap;               /* Swaps */
    long ru_inblock;             /* Block input operations */
    long ru_oublock;             /* Block output operations */
    long ru_msgsnd;              /* Messages sent */
    long ru_msgrcv;              /* Messages received */
    long ru_nsignals;            /* Signals received */
    long ru_nvcsw;               /* Voluntary context switches */
    long ru_nivcsw;              /* Involuntary context switches */
};

/* Process times structure */
struct tms {
    clock_t tms_utime;           /* User time */
    clock_t tms_stime;           /* System time */
    clock_t tms_cutime;          /* User time of children */
    clock_t tms_cstime;          /* System time of children */
};

/* CPU set structure */
typedef struct {
    unsigned long bits[1024 / sizeof(unsigned long)];
} cpu_set_t;

/* Scheduling parameter structure */
struct sched_param {
    int sched_priority;          /* Scheduling priority */
};

/* System information functions */
int sysinfo_get(struct sysinfo *info);
int sysinfo_getcpu(unsigned int *cpu, unsigned int *node, void *tcache);
int sysinfo_sysctl(struct __sysctl_args *args);
int sysinfo_syslog(int type, char *buf, int len);
int sysinfo_getrusage(int who, struct rusage *usage);
int sysinfo_times(struct tms *buf);
int sysinfo_getloadavg(double *loadavg, int nelem);
int sysinfo_sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
int sysinfo_sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask);
int sysinfo_sched_getparam(pid_t pid, struct sched_param *param);
int sysinfo_sched_setparam(pid_t pid, const struct sched_param *param);
int sysinfo_sched_getscheduler(pid_t pid);
int sysinfo_sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
int sysinfo_sched_get_priority_max(int policy);
int sysinfo_sched_get_priority_min(int policy);
int sysinfo_sched_rr_get_interval(pid_t pid, struct timespec *tp);
int sysinfo_sched_yield(void);

#endif /* _KERNEL_SYSINFO_H */

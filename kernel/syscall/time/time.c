/**
 * time.c - Horizon kernel time-related system calls
 *
 * This file contains the implementation of time-related system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/time.h>
#include <horizon/sched.h>
#include <horizon/task.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Get time of day system call */
long sys_gettimeofday(long tv, long tz, long arg3, long arg4, long arg5, long arg6) {
    /* Get the current time */
    struct timeval *tp = (struct timeval *)tv;
    struct timezone *tzp = (struct timezone *)tz;

    /* Set the time */
    if (tp != NULL) {
        tp->tv_sec = time_get_seconds();
        tp->tv_usec = time_get_microseconds();
    }

    /* Set the timezone */
    if (tzp != NULL) {
        tzp->tz_minuteswest = 0;
        tzp->tz_dsttime = 0;
    }

    return 0;
}

/* Set time of day system call */
long sys_settimeofday(long tv, long tz, long arg3, long arg4, long arg5, long arg6) {
    /* Set the current time */
    struct timeval *tp = (struct timeval *)tv;
    struct timezone *tzp = (struct timezone *)tz;

    /* Check if the user has permission to set the time */
    if (task_current()->uid != 0) {
        return -EPERM;
    }

    /* Set the time */
    if (tp != NULL) {
        time_set_seconds(tp->tv_sec);
        time_set_microseconds(tp->tv_usec);
    }

    /* Set the timezone */
    if (tzp != NULL) {
        /* Timezone is not used */
    }

    return 0;
}

/* Nanosleep system call */
long sys_nanosleep(long req, long rem, long arg3, long arg4, long arg5, long arg6) {
    /* Sleep for a specified time */
    struct timespec *tp = (struct timespec *)req;
    struct timespec *rmtp = (struct timespec *)rem;

    /* Check if the timespec is valid */
    if (tp == NULL) {
        return -EINVAL;
    }

    /* Sleep */
    unsigned long timeout = tp->tv_sec * 1000 + tp->tv_nsec / 1000000;
    schedule_timeout_interruptible(timeout);

    /* Set the remaining time */
    if (rmtp != NULL) {
        rmtp->tv_sec = 0;
        rmtp->tv_nsec = 0;
    }

    return 0;
}

/* Time system call */
long sys_time(long tloc, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Get the current time */
    time_t t = time_get_seconds();

    /* Set the time */
    if (tloc != 0) {
        *(time_t *)tloc = t;
    }

    return t;
}

/* Clock gettime system call */
long sys_clock_gettime(long clockid, long tp, long arg3, long arg4, long arg5, long arg6) {
    /* Get the time of the specified clock */
    struct timespec *tsp = (struct timespec *)tp;

    /* Check if the timespec is valid */
    if (tsp == NULL) {
        return -EINVAL;
    }

    /* Get the time */
    switch (clockid) {
        case CLOCK_REALTIME:
            tsp->tv_sec = time_get_seconds();
            tsp->tv_nsec = time_get_nanoseconds();
            break;
        case CLOCK_MONOTONIC:
            tsp->tv_sec = time_get_monotonic_seconds();
            tsp->tv_nsec = time_get_monotonic_nanoseconds();
            break;
        case CLOCK_PROCESS_CPUTIME_ID:
            tsp->tv_sec = task_current()->utime / 1000000000;
            tsp->tv_nsec = task_current()->utime % 1000000000;
            break;
        case CLOCK_THREAD_CPUTIME_ID:
            tsp->tv_sec = thread_self()->utime / 1000000000;
            tsp->tv_nsec = thread_self()->utime % 1000000000;
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

/* Clock settime system call */
long sys_clock_settime(long clockid, long tp, long arg3, long arg4, long arg5, long arg6) {
    /* Set the time of the specified clock */
    struct timespec *tsp = (struct timespec *)tp;

    /* Check if the timespec is valid */
    if (tsp == NULL) {
        return -EINVAL;
    }

    /* Check if the user has permission to set the time */
    if (task_current()->uid != 0) {
        return -EPERM;
    }

    /* Set the time */
    switch (clockid) {
        case CLOCK_REALTIME:
            time_set_seconds(tsp->tv_sec);
            time_set_nanoseconds(tsp->tv_nsec);
            break;
        case CLOCK_MONOTONIC:
            /* Cannot set monotonic clock */
            return -EPERM;
        case CLOCK_PROCESS_CPUTIME_ID:
            /* Cannot set process CPU time */
            return -EPERM;
        case CLOCK_THREAD_CPUTIME_ID:
            /* Cannot set thread CPU time */
            return -EPERM;
        default:
            return -EINVAL;
    }

    return 0;
}

/* Clock getres system call */
long sys_clock_getres(long clockid, long res, long arg3, long arg4, long arg5, long arg6) {
    /* Get the resolution of the specified clock */
    struct timespec *resp = (struct timespec *)res;

    /* Check if the timespec is valid */
    if (resp == NULL) {
        return -EINVAL;
    }

    /* Get the resolution */
    switch (clockid) {
        case CLOCK_REALTIME:
        case CLOCK_MONOTONIC:
            resp->tv_sec = 0;
            resp->tv_nsec = 1; /* 1 nanosecond resolution */
            break;
        case CLOCK_PROCESS_CPUTIME_ID:
        case CLOCK_THREAD_CPUTIME_ID:
            resp->tv_sec = 0;
            resp->tv_nsec = 1000; /* 1 microsecond resolution */
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

/* Clock nanosleep system call */
long sys_clock_nanosleep(long clockid, long flags, long req, long rem, long arg5, long arg6) {
    /* Sleep for a specified time */
    struct timespec *tp = (struct timespec *)req;
    struct timespec *rmtp = (struct timespec *)rem;

    /* Check if the timespec is valid */
    if (tp == NULL) {
        return -EINVAL;
    }

    /* Sleep */
    unsigned long timeout = tp->tv_sec * 1000 + tp->tv_nsec / 1000000;
    schedule_timeout_interruptible(timeout);

    /* Set the remaining time */
    if (rmtp != NULL) {
        rmtp->tv_sec = 0;
        rmtp->tv_nsec = 0;
    }

    return 0;
}

/* Initialize time-related system calls */
void time_syscalls_init(void) {
    /* Register time-related system calls */
    syscall_register(SYS_GETTIMEOFDAY, sys_gettimeofday);
    syscall_register(SYS_SETTIMEOFDAY, sys_settimeofday);
    syscall_register(SYS_NANOSLEEP, sys_nanosleep);
    syscall_register(SYS_TIME, sys_time);
    syscall_register(SYS_CLOCK_GETTIME, sys_clock_gettime);
    syscall_register(SYS_CLOCK_SETTIME, sys_clock_settime);
    syscall_register(SYS_CLOCK_GETRES, sys_clock_getres);
    syscall_register(SYS_CLOCK_NANOSLEEP, sys_clock_nanosleep);
}

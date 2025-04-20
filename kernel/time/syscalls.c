/**
 * syscalls.c - Horizon kernel time system calls
 *
 * This file contains the implementation of the time system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/time.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: time */
long sys_time(long tloc, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the current time */
    time_t t = time_get();

    /* Set the time if requested */
    if (tloc != 0) {
        *((time_t *)tloc) = t;
    }

    return t;
}

/* System call: stime */
long sys_stime(long tptr, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set the system time */
    time_t t = *((time_t *)tptr);

    return time_set(t);
}

/* System call: gettimeofday */
long sys_gettimeofday(long tv, long tz, long unused1, long unused2, long unused3, long unused4) {
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

/* System call: settimeofday */
long sys_settimeofday(long tv, long tz, long unused1, long unused2, long unused3, long unused4) {
    /* Set the current time */
    struct timeval *tp = (struct timeval *)tv;
    struct timezone *tzp = (struct timezone *)tz;

    /* Set the time */
    if (tp != NULL) {
        time_set_seconds(tp->tv_sec);
        time_set_microseconds(tp->tv_usec);
    }

    /* Set the timezone */
    if (tzp != NULL) {
        /* This would be implemented with actual timezone setting */
    }

    return 0;
}

/* System call: clock_gettime */
long sys_clock_gettime(long clockid, long tp, long unused1, long unused2, long unused3, long unused4) {
    /* Get the time of the specified clock */
    struct timespec *tsp = (struct timespec *)tp;

    if (tsp == NULL) {
        return -1;
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
            /* This would be implemented with actual process CPU time */
            tsp->tv_sec = 0;
            tsp->tv_nsec = 0;
            break;

        case CLOCK_THREAD_CPUTIME_ID:
            /* This would be implemented with actual thread CPU time */
            tsp->tv_sec = 0;
            tsp->tv_nsec = 0;
            break;

        default:
            return -1;
    }

    return 0;
}

/* System call: clock_settime */
long sys_clock_settime(long clockid, long tp, long unused1, long unused2, long unused3, long unused4) {
    /* Set the time of the specified clock */
    struct timespec *tsp = (struct timespec *)tp;

    if (tsp == NULL) {
        return -1;
    }

    /* Set the time */
    switch (clockid) {
        case CLOCK_REALTIME:
            time_set_seconds(tsp->tv_sec);
            time_set_nanoseconds(tsp->tv_nsec);
            break;

        case CLOCK_MONOTONIC:
            /* Cannot set the monotonic clock */
            return -1;

        case CLOCK_PROCESS_CPUTIME_ID:
            /* Cannot set the process CPU time */
            return -1;

        case CLOCK_THREAD_CPUTIME_ID:
            /* Cannot set the thread CPU time */
            return -1;

        default:
            return -1;
    }

    return 0;
}

/* System call: clock_getres */
long sys_clock_getres(long clockid, long res, long unused1, long unused2, long unused3, long unused4) {
    /* Get the resolution of the specified clock */
    struct timespec *resp = (struct timespec *)res;

    if (resp == NULL) {
        return -1;
    }

    /* Get the resolution */
    switch (clockid) {
        case CLOCK_REALTIME:
            resp->tv_sec = 0;
            resp->tv_nsec = 1000; /* 1 microsecond */
            break;

        case CLOCK_MONOTONIC:
            resp->tv_sec = 0;
            resp->tv_nsec = 1000; /* 1 microsecond */
            break;

        case CLOCK_PROCESS_CPUTIME_ID:
            resp->tv_sec = 0;
            resp->tv_nsec = 1000000; /* 1 millisecond */
            break;

        case CLOCK_THREAD_CPUTIME_ID:
            resp->tv_sec = 0;
            resp->tv_nsec = 1000000; /* 1 millisecond */
            break;

        default:
            return -1;
    }

    return 0;
}

/* System call: clock_nanosleep */
long sys_clock_nanosleep(long clockid, long flags, long req, long rem, long unused1, long unused2) {
    /* Sleep for the specified time */
    struct timespec *rqtp = (struct timespec *)req;
    struct timespec *rmtp = (struct timespec *)rem;

    if (rqtp == NULL) {
        return -1;
    }

    /* Check the clock ID */
    if (clockid != CLOCK_REALTIME && clockid != CLOCK_MONOTONIC) {
        return -1;
    }

    /* Sleep */
    unsigned long timeout = rqtp->tv_sec * 1000 + rqtp->tv_nsec / 1000000;

    if (flags & TIMER_ABSTIME) {
        /* Absolute time */
        unsigned long now;

        if (clockid == CLOCK_REALTIME) {
            now = time_get_seconds() * 1000 + time_get_microseconds() / 1000;
        } else {
            now = time_get_monotonic_seconds() * 1000 + time_get_monotonic_nanoseconds() / 1000000;
        }

        if (now >= timeout) {
            return 0;
        }

        timeout -= now;
    }

    /* Sleep */
    schedule_timeout_interruptible(timeout);

    /* Set the remaining time */
    if (rmtp != NULL) {
        rmtp->tv_sec = 0;
        rmtp->tv_nsec = 0;
    }

    return 0;
}

/* System call: nanosleep */
long sys_nanosleep(long req, long rem, long unused1, long unused2, long unused3, long unused4) {
    /* Sleep for the specified time */
    return sys_clock_nanosleep(CLOCK_REALTIME, 0, req, rem, 0, 0);
}

/* System call: alarm */
long sys_alarm(long seconds, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set an alarm */
    return time_alarm(seconds);
}

/* System call: getitimer */
long sys_getitimer(long which, long value, long unused1, long unused2, long unused3, long unused4) {
    /* Get the value of an interval timer */
    struct itimerval *curr_value = (struct itimerval *)value;

    if (curr_value == NULL) {
        return -1;
    }

    return time_getitimer(which, curr_value);
}

/* System call: setitimer */
long sys_setitimer(long which, long value, long ovalue, long unused1, long unused2, long unused3) {
    /* Set the value of an interval timer */
    struct itimerval *new_value = (struct itimerval *)value;
    struct itimerval *old_value = (struct itimerval *)ovalue;

    return time_setitimer(which, new_value, old_value);
}

/* System call: timer_create */
long sys_timer_create(long clockid, long evp, long timerid, long unused1, long unused2, long unused3) {
    /* Create a POSIX timer */
    struct sigevent *sevp = (struct sigevent *)evp;
    timer_t *tid = (timer_t *)timerid;

    return time_timer_create(clockid, sevp, tid);
}

/* System call: timer_delete */
long sys_timer_delete(long timerid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Delete a POSIX timer */
    return time_timer_delete((timer_t)timerid);
}

/* System call: timer_gettime */
long sys_timer_gettime(long timerid, long value, long unused1, long unused2, long unused3, long unused4) {
    /* Get the time remaining on a POSIX timer */
    struct itimerspec *curr_value = (struct itimerspec *)value;

    if (curr_value == NULL) {
        return -1;
    }

    return time_timer_gettime((timer_t)timerid, curr_value);
}

/* System call: timer_settime */
long sys_timer_settime(long timerid, long flags, long value, long ovalue, long unused1, long unused2) {
    /* Set the time on a POSIX timer */
    struct itimerspec *new_value = (struct itimerspec *)value;
    struct itimerspec *old_value = (struct itimerspec *)ovalue;

    if (new_value == NULL) {
        return -1;
    }

    return time_timer_settime((timer_t)timerid, flags, new_value, old_value);
}

/* System call: timer_getoverrun */
long sys_timer_getoverrun(long timerid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the overrun count for a POSIX timer */
    return time_timer_getoverrun((timer_t)timerid);
}

/* System call: adjtimex */
long sys_adjtimex(long buf, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Adjust the system time */
    struct timex *txp = (struct timex *)buf;

    if (txp == NULL) {
        return -1;
    }

    return time_adjtimex(txp);
}

/* System call: clock_adjtime */
long sys_clock_adjtime(long clockid, long tx, long unused1, long unused2, long unused3, long unused4) {
    /* Adjust the time of a specific clock */
    struct timex *txp = (struct timex *)tx;

    if (txp == NULL) {
        return -1;
    }

    /* Check the clock ID */
    if (clockid != CLOCK_REALTIME) {
        return -1;
    }

    return time_adjtimex(txp);
}

/* System call: timerfd_create */
long sys_timerfd_create(long clockid, long flags, long unused1, long unused2, long unused3, long unused4) {
    /* Create a timer file descriptor */
    return time_timerfd_create(clockid, flags);
}

/* System call: timerfd_settime */
long sys_timerfd_settime(long fd, long flags, long new_value, long old_value, long unused1, long unused2) {
    /* Set the time on a timer file descriptor */
    struct itimerspec *new_value_p = (struct itimerspec *)new_value;
    struct itimerspec *old_value_p = (struct itimerspec *)old_value;

    if (new_value_p == NULL) {
        return -1;
    }

    return time_timerfd_settime(fd, flags, new_value_p, old_value_p);
}

/* System call: timerfd_gettime */
long sys_timerfd_gettime(long fd, long curr_value, long unused1, long unused2, long unused3, long unused4) {
    /* Get the time remaining on a timer file descriptor */
    struct itimerspec *curr_value_p = (struct itimerspec *)curr_value;

    if (curr_value_p == NULL) {
        return -1;
    }

    return time_timerfd_gettime(fd, curr_value_p);
}

/* Register time system calls */
void time_syscalls_init(void) {
    /* Register time system calls */
    syscall_register(SYS_TIME, sys_time);
    syscall_register(SYS_STIME, sys_stime);
    syscall_register(SYS_GETTIMEOFDAY, sys_gettimeofday);
    syscall_register(SYS_SETTIMEOFDAY, sys_settimeofday);
    syscall_register(SYS_ADJTIMEX, sys_adjtimex);
    syscall_register(SYS_CLOCK_GETTIME, sys_clock_gettime);
    syscall_register(SYS_CLOCK_SETTIME, sys_clock_settime);
    syscall_register(SYS_CLOCK_GETRES, sys_clock_getres);
    syscall_register(SYS_CLOCK_NANOSLEEP, sys_clock_nanosleep);
    syscall_register(SYS_NANOSLEEP, sys_nanosleep);
    syscall_register(SYS_ALARM, sys_alarm);
    syscall_register(SYS_GETITIMER, sys_getitimer);
    syscall_register(SYS_SETITIMER, sys_setitimer);
    syscall_register(SYS_TIMER_CREATE, sys_timer_create);
    syscall_register(SYS_TIMER_DELETE, sys_timer_delete);
    syscall_register(SYS_TIMER_GETTIME, sys_timer_gettime);
    syscall_register(SYS_TIMER_SETTIME, sys_timer_settime);
    syscall_register(SYS_TIMER_GETOVERRUN, sys_timer_getoverrun);
    syscall_register(SYS_CLOCK_ADJTIME, sys_clock_adjtime);
    syscall_register(SYS_TIMERFD_CREATE, sys_timerfd_create);
    syscall_register(SYS_TIMERFD_SETTIME, sys_timerfd_settime);
    syscall_register(SYS_TIMERFD_GETTIME, sys_timerfd_gettime);
}

/**
 * time.h - Horizon kernel time definitions
 * 
 * This file contains definitions for the time subsystem.
 * The definitions are compatible with Linux.
 */

#ifndef _KERNEL_TIME_H
#define _KERNEL_TIME_H

#include <horizon/types.h>

/* Clock IDs */
#define CLOCK_REALTIME           0
#define CLOCK_MONOTONIC          1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID  3
#define CLOCK_MONOTONIC_RAW      4
#define CLOCK_REALTIME_COARSE    5
#define CLOCK_MONOTONIC_COARSE   6
#define CLOCK_BOOTTIME           7
#define CLOCK_REALTIME_ALARM     8
#define CLOCK_BOOTTIME_ALARM     9
#define CLOCK_SGI_CYCLE         10
#define CLOCK_TAI               11

/* Timer IDs */
#define TIMER_ABSTIME           0x01

/* Interval timer IDs */
#define ITIMER_REAL             0
#define ITIMER_VIRTUAL          1
#define ITIMER_PROF             2

/* Time structures */
struct timespec {
    time_t tv_sec;              /* Seconds */
    long   tv_nsec;             /* Nanoseconds */
};

struct timeval {
    time_t      tv_sec;         /* Seconds */
    suseconds_t tv_usec;        /* Microseconds */
};

struct timezone {
    int tz_minuteswest;         /* Minutes west of Greenwich */
    int tz_dsttime;             /* Type of DST correction */
};

struct itimerval {
    struct timeval it_interval; /* Timer interval */
    struct timeval it_value;    /* Current value */
};

struct itimerspec {
    struct timespec it_interval; /* Timer interval */
    struct timespec it_value;    /* Current value */
};

struct timex {
    unsigned int modes;         /* Mode selector */
    long offset;                /* Time offset */
    long freq;                  /* Frequency offset */
    long maxerror;              /* Maximum error */
    long esterror;              /* Estimated error */
    int status;                 /* Clock status */
    long constant;              /* PLL time constant */
    long precision;             /* Clock precision */
    long tolerance;             /* Clock frequency tolerance */
    struct timeval time;        /* Current time */
    long tick;                  /* Microseconds between clock ticks */
    long ppsfreq;               /* PPS frequency */
    long jitter;                /* PPS jitter */
    int shift;                  /* PPS interval duration */
    long stabil;                /* PPS stability */
    long jitcnt;                /* PPS count of jitter limit exceeded */
    long calcnt;                /* PPS count of calibration intervals */
    long errcnt;                /* PPS count of calibration errors */
    long stbcnt;                /* PPS count of stability limit exceeded */
    int tai;                    /* TAI offset */
    int:32; int:32; int:32; int:32;
    int:32; int:32; int:32; int:32;
    int:32; int:32; int:32;
};

/* Time functions */
time_t time_get(void);
int time_set(time_t t);
time_t time_get_seconds(void);
int time_set_seconds(time_t t);
long time_get_microseconds(void);
int time_set_microseconds(long us);
long time_get_nanoseconds(void);
int time_set_nanoseconds(long ns);
time_t time_get_monotonic_seconds(void);
long time_get_monotonic_nanoseconds(void);
unsigned long time_alarm(unsigned long seconds);
int time_getitimer(int which, struct itimerval *curr_value);
int time_setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
int time_adjtimex(struct timex *buf);
int time_clock_gettime(clockid_t clk_id, struct timespec *tp);
int time_clock_settime(clockid_t clk_id, const struct timespec *tp);
int time_clock_getres(clockid_t clk_id, struct timespec *res);
int time_clock_nanosleep(clockid_t clockid, int flags, const struct timespec *request, struct timespec *remain);
int time_nanosleep(const struct timespec *req, struct timespec *rem);
int time_clock_adjtime(clockid_t clk_id, struct timex *tx);
int time_timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
int time_timer_delete(timer_t timerid);
int time_timer_gettime(timer_t timerid, struct itimerspec *curr_value);
int time_timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);
int time_timer_getoverrun(timer_t timerid);
int time_timerfd_create(int clockid, int flags);
int time_timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);
int time_timerfd_gettime(int fd, struct itimerspec *curr_value);

/* Time initialization */
void time_init(void);
void timerfd_init(void);

#endif /* _KERNEL_TIME_H */

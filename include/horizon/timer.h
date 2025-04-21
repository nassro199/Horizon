/**
 * timer.h - Horizon kernel timer definitions
 *
 * This file contains definitions for the timer subsystem.
 * The definitions are compatible with Linux.
 */

#ifndef _HORIZON_TIMER_H
#define _HORIZON_TIMER_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/time.h>

/* Timer ID type */
typedef u32 timer_id_t;

/* Timer callback function type */
typedef void (*timer_callback_t)(timer_id_t id, void *data);

/* Timer flags */
#define TIMER_FLAG_ACTIVE      0x00000001  /* Timer is active */
#define TIMER_FLAG_PERIODIC    0x00000002  /* Timer is periodic */
#define TIMER_FLAG_ONESHOT     0x00000004  /* Timer is one-shot */
#define TIMER_FLAG_EXPIRED     0x00000008  /* Timer has expired */
#define TIMER_FLAG_PENDING     0x00000010  /* Timer is pending */
#define TIMER_FLAG_DEFERRABLE  0x00000020  /* Timer is deferrable */
#define TIMER_FLAG_PINNED      0x00000040  /* Timer is pinned */
#define TIMER_FLAG_MIGRATING   0x00000080  /* Timer is migrating */
#define TIMER_FLAG_HIGH_RES    0x00000100  /* Timer is high resolution */
#define TIMER_FLAG_NO_REQUEUE  0x00000200  /* Timer should not be requeued */

/* Timer list */
typedef struct timer_list {
    struct list_head entry;        /* List entry */
    unsigned long expires;         /* Expiration time */
    void (*function)(unsigned long);  /* Timer function */
    unsigned long data;            /* Timer data */
    unsigned int flags;            /* Timer flags */
    struct timer_base *base;       /* Timer base */
} timer_list_t;

/* Timer base */
typedef struct timer_base {
    spinlock_t lock;               /* Base lock */
    struct list_head timers;       /* List of timers */
    unsigned long next_expiry;     /* Next expiry */
    unsigned int active_timers;    /* Number of active timers */
    unsigned int shutdown;         /* Shutdown flag */
} timer_base_t;

/* High resolution timer */
typedef struct hrtimer {
    struct rb_node node;           /* Red-black tree node */
    ktime_t expires;               /* Expiration time */
    enum hrtimer_restart (*function)(struct hrtimer *);  /* Timer function */
    struct hrtimer_clock_base *base;  /* Timer base */
    unsigned long state;           /* Timer state */
    unsigned int is_rel;           /* Is relative */
    unsigned int is_soft;          /* Is soft */
    unsigned int is_hard;          /* Is hard */
} hrtimer_t;

/* High resolution timer clock base */
typedef struct hrtimer_clock_base {
    struct hrtimer_cpu_base *cpu_base;  /* CPU base */
    int index;                     /* Base index */
    clockid_t clockid;             /* Clock ID */
    struct rb_root active;         /* Active timers */
    struct rb_node *first;         /* First timer */
    ktime_t resolution;            /* Base resolution */
    ktime_t softirq_time;          /* Softirq time */
    ktime_t offset;                /* Base offset */
} hrtimer_clock_base_t;

/* High resolution timer CPU base */
typedef struct hrtimer_cpu_base {
    raw_spinlock_t lock;           /* Base lock */
    unsigned int active_bases;     /* Active bases */
    unsigned int clock_was_set;    /* Clock was set */
    unsigned int hres_active;      /* High resolution active */
    unsigned int hang_detected;    /* Hang detected */
    unsigned int nr_events;        /* Number of events */
    unsigned int nr_retries;       /* Number of retries */
    unsigned int nr_hangs;         /* Number of hangs */
    unsigned int max_hang_time;    /* Maximum hang time */
    ktime_t expires_next;          /* Next expiration */
    struct hrtimer_clock_base clock_base[HRTIMER_MAX_CLOCK_BASES];  /* Clock bases */
} hrtimer_cpu_base_t;

/* Timer information structure */
typedef struct timer_info {
    timer_id_t id;              /* Timer ID */
    u64 expires;                /* Expiration time in milliseconds */
    u64 period;                 /* Period in milliseconds (0 for one-shot) */
    u32 flags;                  /* Timer flags */
} timer_info_t;

/* High resolution timer restart values */
enum hrtimer_restart {
    HRTIMER_NORESTART,             /* Timer is not restarted */
    HRTIMER_RESTART,               /* Timer is restarted */
};

/* Timer functions */
void timer_init(void);
void init_timer(struct timer_list *timer);
void add_timer(struct timer_list *timer);
int del_timer(struct timer_list *timer);
int mod_timer(struct timer_list *timer, unsigned long expires);
int timer_pending(const struct timer_list *timer);
void setup_timer(struct timer_list *timer, void (*function)(unsigned long), unsigned long data);
void setup_timer_on_stack(struct timer_list *timer, void (*function)(unsigned long), unsigned long data);
void destroy_timer_on_stack(struct timer_list *timer);
void timer_stats_timer_set_start_info(struct timer_list *timer);
void timer_stats_timer_clear_start_info(struct timer_list *timer);
void msleep(unsigned int msecs);
unsigned long msleep_interruptible(unsigned int msecs);
void usleep_range(unsigned long min, unsigned long max);
void ssleep(unsigned int seconds);

/* New timer API */
timer_id_t timer_create(timer_callback_t callback, void *data);
int timer_delete(timer_id_t id);
int timer_start(timer_id_t id, u64 expires, u64 period, u32 flags);
int timer_stop(timer_id_t id);
int timer_get_info(timer_id_t id, timer_info_t *info);
void timer_process(void);
u64 timer_get_jiffies(void);
u32 timer_get_frequency(void);
u64 timer_get_tick_period(void);
u64 timer_msecs_to_jiffies(u64 msec);
u64 timer_jiffies_to_msecs(u64 j);
u64 timer_nsecs_to_jiffies(u64 nsec);
u64 timer_jiffies_to_nsecs(u64 j);
void timer_msleep(u64 msec);
void timer_usleep(u64 usec);
void timer_nsleep(u64 nsec);
void timer_sleep_until(u64 timeout);

/* High resolution timer functions */
void hrtimer_init(struct hrtimer *timer, clockid_t clock_id, enum hrtimer_mode mode);
int hrtimer_start(struct hrtimer *timer, ktime_t tim, const enum hrtimer_mode mode);
int hrtimer_cancel(struct hrtimer *timer);
int hrtimer_try_to_cancel(struct hrtimer *timer);
int hrtimer_restart(struct hrtimer *timer);
ktime_t hrtimer_get_remaining(const struct hrtimer *timer);
int hrtimer_get_res(const clockid_t which_clock, struct timespec *tp);
int hrtimer_is_queued(struct hrtimer *timer);
int hrtimer_active(const struct hrtimer *timer);
int hrtimer_is_hres_active(struct hrtimer *timer);
int hrtimer_callback_running(struct hrtimer *timer);
int hrtimer_start_range_ns(struct hrtimer *timer, ktime_t tim, u64 range_ns, const enum hrtimer_mode mode);
void hrtimer_init_sleeper(struct hrtimer_sleeper *sl, struct task_struct *task);
void hrtimer_set_expires(struct hrtimer *timer, ktime_t time);
void hrtimer_set_expires_range_ns(struct hrtimer *timer, ktime_t time, u64 range_ns);
void hrtimer_set_expires_tv64(struct hrtimer *timer, s64 time);
void hrtimer_add_expires(struct hrtimer *timer, ktime_t time);
void hrtimer_add_expires_ns(struct hrtimer *timer, u64 ns);
void hrtimer_forward(struct hrtimer *timer, ktime_t now, ktime_t interval);
u64 hrtimer_forward_now(struct hrtimer *timer, ktime_t interval);
ktime_t hrtimer_expires_remaining(const struct hrtimer *timer);
ktime_t hrtimer_cb_get_time(struct hrtimer *timer);
int schedule_hrtimeout(ktime_t *expires, const enum hrtimer_mode mode);
int schedule_hrtimeout_range(ktime_t *expires, u64 delta, const enum hrtimer_mode mode);
long schedule_timeout(long timeout);
long schedule_timeout_interruptible(long timeout);
long schedule_timeout_killable(long timeout);
long schedule_timeout_uninterruptible(long timeout);

/* Timer bases */
extern struct timer_base timer_bases[NR_CPUS];

/* High resolution timer bases */
extern struct hrtimer_cpu_base hrtimer_bases[NR_CPUS];

/* System timer */
extern struct timer_list system_timer;

/* Timer interrupt */
extern void timer_interrupt(struct interrupt_frame *frame);

/* Timer tick */
extern void timer_tick(void);

/* Timer frequency */
extern unsigned int timer_frequency;

/* Timer jiffies */
extern unsigned long jiffies;

/* Timer ticks per second */
extern unsigned int HZ;

#endif /* _HORIZON_TIMER_H */

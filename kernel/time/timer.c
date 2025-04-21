/**
 * timer.c - Horizon kernel timer implementation
 *
 * This file contains the implementation of the timer subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/time.h>
#include <horizon/timer.h>
#include <horizon/list.h>
#include <horizon/spinlock.h>
#include <horizon/sched.h>
#include <horizon/mm.h>
#include <horizon/errno.h>

/* Timer list */
static list_head_t timer_list;

/* Timer lock */
static spinlock_t timer_lock = SPIN_LOCK_INITIALIZER;

/* Timer ID counter */
static u32 timer_id_counter = 0;

/* Timer jiffies */
volatile u64 jiffies = 0;

/* Timer frequency in Hz */
static u32 timer_frequency = 0;

/* Timer tick period in nanoseconds */
static u64 timer_tick_period = 0;

/**
 * Initialize the timer subsystem
 */
void timer_init(void) {
    /* Initialize the timer list */
    list_init(&timer_list);

    /* Set the timer frequency */
    timer_frequency = 1000; /* 1000 Hz (1ms) */
    timer_tick_period = 1000000000ULL / timer_frequency; /* nanoseconds */

    /* Initialize the architecture-specific timer */
    arch_timer_init(timer_frequency);
}

/**
 * Create a timer
 * 
 * @param callback Timer callback function
 * @param data Timer callback data
 * @return Timer ID on success, 0 on failure
 */
timer_id_t timer_create(timer_callback_t callback, void *data) {
    timer_t *timer;

    /* Check parameters */
    if (callback == NULL) {
        return 0;
    }

    /* Allocate timer */
    timer = kmalloc(sizeof(timer_t), 0);
    if (timer == NULL) {
        return 0;
    }

    /* Initialize timer */
    timer->id = ++timer_id_counter;
    timer->callback = callback;
    timer->data = data;
    timer->expires = 0;
    timer->period = 0;
    timer->flags = 0;
    list_init(&timer->list);

    return timer->id;
}

/**
 * Delete a timer
 * 
 * @param id Timer ID
 * @return 0 on success, negative error code on failure
 */
int timer_delete(timer_id_t id) {
    timer_t *timer;
    list_head_t *pos;

    /* Check parameters */
    if (id == 0) {
        return -EINVAL;
    }

    /* Find the timer */
    spin_lock(&timer_lock);
    list_for_each(pos, &timer_list) {
        timer = list_entry(pos, timer_t, list);
        if (timer->id == id) {
            /* Remove the timer from the list */
            list_del(&timer->list);
            spin_unlock(&timer_lock);

            /* Free the timer */
            kfree(timer);
            return 0;
        }
    }
    spin_unlock(&timer_lock);

    /* Timer not found */
    return -ENOENT;
}

/**
 * Start a timer
 * 
 * @param id Timer ID
 * @param expires Expiration time in milliseconds
 * @param period Period in milliseconds (0 for one-shot)
 * @param flags Timer flags
 * @return 0 on success, negative error code on failure
 */
int timer_start(timer_id_t id, u64 expires, u64 period, u32 flags) {
    timer_t *timer;
    list_head_t *pos;

    /* Check parameters */
    if (id == 0) {
        return -EINVAL;
    }

    /* Find the timer */
    spin_lock(&timer_lock);
    list_for_each(pos, &timer_list) {
        timer = list_entry(pos, timer_t, list);
        if (timer->id == id) {
            /* Remove the timer from the list */
            list_del(&timer->list);
            break;
        }
    }
    if (pos == &timer_list) {
        /* Timer not found */
        spin_unlock(&timer_lock);
        return -ENOENT;
    }

    /* Set the timer parameters */
    timer->expires = jiffies + (expires * timer_frequency) / 1000;
    timer->period = (period * timer_frequency) / 1000;
    timer->flags = flags;

    /* Add the timer to the list */
    list_add_tail(&timer->list, &timer_list);
    spin_unlock(&timer_lock);

    return 0;
}

/**
 * Stop a timer
 * 
 * @param id Timer ID
 * @return 0 on success, negative error code on failure
 */
int timer_stop(timer_id_t id) {
    timer_t *timer;
    list_head_t *pos;

    /* Check parameters */
    if (id == 0) {
        return -EINVAL;
    }

    /* Find the timer */
    spin_lock(&timer_lock);
    list_for_each(pos, &timer_list) {
        timer = list_entry(pos, timer_t, list);
        if (timer->id == id) {
            /* Remove the timer from the list */
            list_del(&timer->list);
            spin_unlock(&timer_lock);
            return 0;
        }
    }
    spin_unlock(&timer_lock);

    /* Timer not found */
    return -ENOENT;
}

/**
 * Get timer information
 * 
 * @param id Timer ID
 * @param info Timer information
 * @return 0 on success, negative error code on failure
 */
int timer_get_info(timer_id_t id, timer_info_t *info) {
    timer_t *timer;
    list_head_t *pos;

    /* Check parameters */
    if (id == 0 || info == NULL) {
        return -EINVAL;
    }

    /* Find the timer */
    spin_lock(&timer_lock);
    list_for_each(pos, &timer_list) {
        timer = list_entry(pos, timer_t, list);
        if (timer->id == id) {
            /* Get the timer information */
            info->id = timer->id;
            info->expires = (timer->expires > jiffies) ? ((timer->expires - jiffies) * 1000) / timer_frequency : 0;
            info->period = (timer->period * 1000) / timer_frequency;
            info->flags = timer->flags;
            spin_unlock(&timer_lock);
            return 0;
        }
    }
    spin_unlock(&timer_lock);

    /* Timer not found */
    return -ENOENT;
}

/**
 * Process timers
 * 
 * This function is called by the timer interrupt handler to process expired timers.
 */
void timer_process(void) {
    timer_t *timer;
    list_head_t *pos, *n;
    list_head_t expired_timers;

    /* Initialize the expired timers list */
    list_init(&expired_timers);

    /* Find expired timers */
    spin_lock(&timer_lock);
    list_for_each_safe(pos, n, &timer_list) {
        timer = list_entry(pos, timer_t, list);
        if (timer->expires <= jiffies) {
            /* Remove the timer from the list */
            list_del(&timer->list);

            /* Add the timer to the expired timers list */
            list_add_tail(&timer->list, &expired_timers);
        }
    }
    spin_unlock(&timer_lock);

    /* Process expired timers */
    list_for_each_safe(pos, n, &expired_timers) {
        timer = list_entry(pos, timer_t, list);

        /* Remove the timer from the expired timers list */
        list_del(&timer->list);

        /* Call the timer callback */
        if (timer->callback != NULL) {
            timer->callback(timer->id, timer->data);
        }

        /* Check if the timer is periodic */
        if (timer->period > 0) {
            /* Restart the timer */
            spin_lock(&timer_lock);
            timer->expires = jiffies + timer->period;
            list_add_tail(&timer->list, &timer_list);
            spin_unlock(&timer_lock);
        } else {
            /* Free the timer */
            kfree(timer);
        }
    }
}

/**
 * Timer tick handler
 * 
 * This function is called by the timer interrupt handler on each timer tick.
 */
void timer_tick(void) {
    /* Increment jiffies */
    jiffies++;

    /* Update the time */
    time_update(jiffies / timer_frequency, (jiffies % timer_frequency) * (1000000 / timer_frequency));

    /* Process timers */
    timer_process();

    /* Schedule */
    schedule();
}

/**
 * Get the current jiffies value
 * 
 * @return Current jiffies value
 */
u64 timer_get_jiffies(void) {
    return jiffies;
}

/**
 * Get the timer frequency
 * 
 * @return Timer frequency in Hz
 */
u32 timer_get_frequency(void) {
    return timer_frequency;
}

/**
 * Get the timer tick period
 * 
 * @return Timer tick period in nanoseconds
 */
u64 timer_get_tick_period(void) {
    return timer_tick_period;
}

/**
 * Convert milliseconds to jiffies
 * 
 * @param msec Milliseconds
 * @return Jiffies
 */
u64 timer_msecs_to_jiffies(u64 msec) {
    return (msec * timer_frequency) / 1000;
}

/**
 * Convert jiffies to milliseconds
 * 
 * @param j Jiffies
 * @return Milliseconds
 */
u64 timer_jiffies_to_msecs(u64 j) {
    return (j * 1000) / timer_frequency;
}

/**
 * Convert nanoseconds to jiffies
 * 
 * @param nsec Nanoseconds
 * @return Jiffies
 */
u64 timer_nsecs_to_jiffies(u64 nsec) {
    return (nsec * timer_frequency) / 1000000000ULL;
}

/**
 * Convert jiffies to nanoseconds
 * 
 * @param j Jiffies
 * @return Nanoseconds
 */
u64 timer_jiffies_to_nsecs(u64 j) {
    return (j * 1000000000ULL) / timer_frequency;
}

/**
 * Sleep for a specified number of milliseconds
 * 
 * @param msec Milliseconds to sleep
 */
void timer_msleep(u64 msec) {
    u64 start = jiffies;
    u64 end = start + timer_msecs_to_jiffies(msec);

    while (jiffies < end) {
        schedule();
    }
}

/**
 * Sleep for a specified number of microseconds
 * 
 * @param usec Microseconds to sleep
 */
void timer_usleep(u64 usec) {
    timer_msleep(usec / 1000);
}

/**
 * Sleep for a specified number of nanoseconds
 * 
 * @param nsec Nanoseconds to sleep
 */
void timer_nsleep(u64 nsec) {
    timer_msleep(nsec / 1000000);
}

/**
 * Sleep until a specified absolute time
 * 
 * @param timeout Absolute time in jiffies
 */
void timer_sleep_until(u64 timeout) {
    while (jiffies < timeout) {
        schedule();
    }
}

/**
 * Schedule a timeout
 * 
 * @param timeout Timeout in jiffies
 * @return 0 on success, negative error code on failure
 */
int timer_schedule_timeout(u64 timeout) {
    u64 expire = jiffies + timeout;

    while (jiffies < expire) {
        schedule();
    }

    return 0;
}

/**
 * Schedule a timeout (interruptible)
 * 
 * @param timeout Timeout in jiffies
 * @return 0 on success, negative error code on failure
 */
int timer_schedule_timeout_interruptible(u64 timeout) {
    return timer_schedule_timeout(timeout);
}

/**
 * Schedule a timeout (uninterruptible)
 * 
 * @param timeout Timeout in jiffies
 * @return 0 on success, negative error code on failure
 */
int timer_schedule_timeout_uninterruptible(u64 timeout) {
    return timer_schedule_timeout(timeout);
}

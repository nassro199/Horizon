/**
 * timerfd.c - Horizon kernel timer file descriptor implementation
 * 
 * This file contains the implementation of the timer file descriptor.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/time.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Timer file descriptor flags */
#define TFD_CLOEXEC       O_CLOEXEC
#define TFD_NONBLOCK      O_NONBLOCK
#define TFD_SHARED_FCNTL_FLAGS (TFD_CLOEXEC | TFD_NONBLOCK)
#define TFD_CREATE_FLAGS  (TFD_SHARED_FCNTL_FLAGS | TFD_TIMER_ABSTIME)
#define TFD_TIMER_ABSTIME (1 << 0)

/* Timer file descriptor structure */
typedef struct timerfd {
    int clockid;                /* Clock ID */
    int flags;                  /* Flags */
    struct itimerspec value;    /* Timer value */
    struct wait_queue_head wait; /* Wait queue */
    uint64_t ticks;             /* Number of expirations */
    struct timer_list timer;    /* Timer */
    spinlock_t lock;            /* Lock */
} timerfd_t;

/* Maximum number of timer file descriptors */
#define MAX_TIMERFD 1024

/* Timer file descriptors */
static timerfd_t *timerfd_table[MAX_TIMERFD];

/* Timer file descriptor mutex */
static struct mutex timerfd_mutex;

/**
 * Initialize the timer file descriptor subsystem
 */
void timerfd_init(void) {
    /* Initialize the mutex */
    mutex_init(&timerfd_mutex);
    
    /* Initialize the timer file descriptors */
    for (int i = 0; i < MAX_TIMERFD; i++) {
        timerfd_table[i] = NULL;
    }
}

/**
 * Timer file descriptor timer callback
 * 
 * @param timer The timer
 */
static void timerfd_timer_callback(struct timer_list *timer) {
    /* Get the timer file descriptor */
    timerfd_t *tfd = container_of(timer, timerfd_t, timer);
    
    /* Lock the timer file descriptor */
    spin_lock(&tfd->lock);
    
    /* Increment the number of expirations */
    tfd->ticks++;
    
    /* Check if the timer is periodic */
    if (tfd->value.it_interval.tv_sec > 0 || tfd->value.it_interval.tv_nsec > 0) {
        /* Calculate the next expiration */
        unsigned long expires = jiffies;
        
        if (tfd->value.it_interval.tv_sec > 0) {
            expires += tfd->value.it_interval.tv_sec * HZ;
        }
        
        if (tfd->value.it_interval.tv_nsec > 0) {
            expires += tfd->value.it_interval.tv_nsec / (1000000000 / HZ);
        }
        
        /* Set the timer */
        mod_timer(&tfd->timer, expires);
    }
    
    /* Unlock the timer file descriptor */
    spin_unlock(&tfd->lock);
    
    /* Wake up any waiting processes */
    wake_up_interruptible(&tfd->wait);
}

/**
 * Create a timer file descriptor
 * 
 * @param clockid The clock ID
 * @param flags The flags
 * @return The file descriptor, or a negative error code
 */
int time_timerfd_create(int clockid, int flags) {
    /* Check the clock ID */
    if (clockid != CLOCK_REALTIME && clockid != CLOCK_MONOTONIC) {
        return -1;
    }
    
    /* Check the flags */
    if (flags & ~TFD_CREATE_FLAGS) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&timerfd_mutex);
    
    /* Find a free timer file descriptor */
    int id = -1;
    
    for (int i = 0; i < MAX_TIMERFD; i++) {
        if (timerfd_table[i] == NULL) {
            id = i;
            break;
        }
    }
    
    /* Check if we found a free timer file descriptor */
    if (id == -1) {
        mutex_unlock(&timerfd_mutex);
        return -1;
    }
    
    /* Allocate the timer file descriptor */
    timerfd_t *tfd = kmalloc(sizeof(timerfd_t), MEM_KERNEL | MEM_ZERO);
    
    if (tfd == NULL) {
        mutex_unlock(&timerfd_mutex);
        return -1;
    }
    
    /* Initialize the timer file descriptor */
    tfd->clockid = clockid;
    tfd->flags = flags & TFD_SHARED_FCNTL_FLAGS;
    memset(&tfd->value, 0, sizeof(struct itimerspec));
    init_waitqueue_head(&tfd->wait);
    tfd->ticks = 0;
    timer_setup(&tfd->timer, timerfd_timer_callback, 0);
    spin_lock_init(&tfd->lock);
    
    /* Set the timer file descriptor */
    timerfd_table[id] = tfd;
    
    /* Unlock the mutex */
    mutex_unlock(&timerfd_mutex);
    
    /* Create a file descriptor */
    file_t *file;
    int fd = file_anon_fd(tfd, &file);
    
    if (fd < 0) {
        /* Lock the mutex */
        mutex_lock(&timerfd_mutex);
        
        /* Free the timer file descriptor */
        timerfd_table[id] = NULL;
        kfree(tfd);
        
        /* Unlock the mutex */
        mutex_unlock(&timerfd_mutex);
        
        return -1;
    }
    
    /* Set the file flags */
    if (flags & TFD_NONBLOCK) {
        file->f_flags |= O_NONBLOCK;
    }
    
    if (flags & TFD_CLOEXEC) {
        file->f_flags |= O_CLOEXEC;
    }
    
    return fd;
}

/**
 * Set the time on a timer file descriptor
 * 
 * @param fd The file descriptor
 * @param flags The flags
 * @param new_value The new value
 * @param old_value The old value
 * @return 0 on success, or a negative error code
 */
int time_timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the timer file descriptor */
    timerfd_t *tfd = file->private_data;
    
    if (tfd == NULL) {
        return -1;
    }
    
    /* Check the flags */
    if (flags & ~TFD_TIMER_ABSTIME) {
        return -1;
    }
    
    /* Check the new value */
    if (new_value == NULL) {
        return -1;
    }
    
    /* Check the timer values */
    if (new_value->it_value.tv_sec < 0 || new_value->it_value.tv_nsec < 0 || new_value->it_value.tv_nsec >= 1000000000 ||
        new_value->it_interval.tv_sec < 0 || new_value->it_interval.tv_nsec < 0 || new_value->it_interval.tv_nsec >= 1000000000) {
        return -1;
    }
    
    /* Lock the timer file descriptor */
    spin_lock(&tfd->lock);
    
    /* Save the old value */
    if (old_value != NULL) {
        memcpy(old_value, &tfd->value, sizeof(struct itimerspec));
    }
    
    /* Cancel the timer */
    del_timer(&tfd->timer);
    
    /* Set the new value */
    memcpy(&tfd->value, new_value, sizeof(struct itimerspec));
    
    /* Check if the timer is enabled */
    if (new_value->it_value.tv_sec > 0 || new_value->it_value.tv_nsec > 0) {
        /* Calculate the expiration */
        unsigned long expires = jiffies;
        
        if (flags & TFD_TIMER_ABSTIME) {
            /* Absolute time */
            struct timespec now;
            
            if (tfd->clockid == CLOCK_REALTIME) {
                now.tv_sec = time_get_seconds();
                now.tv_nsec = time_get_nanoseconds();
            } else {
                now.tv_sec = time_get_monotonic_seconds();
                now.tv_nsec = time_get_monotonic_nanoseconds();
            }
            
            /* Calculate the relative time */
            time_t sec = new_value->it_value.tv_sec - now.tv_sec;
            long nsec = new_value->it_value.tv_nsec - now.tv_nsec;
            
            if (nsec < 0) {
                sec--;
                nsec += 1000000000;
            }
            
            if (sec < 0) {
                sec = 0;
                nsec = 0;
            }
            
            /* Calculate the expiration */
            if (sec > 0) {
                expires += sec * HZ;
            }
            
            if (nsec > 0) {
                expires += nsec / (1000000000 / HZ);
            }
        } else {
            /* Relative time */
            if (new_value->it_value.tv_sec > 0) {
                expires += new_value->it_value.tv_sec * HZ;
            }
            
            if (new_value->it_value.tv_nsec > 0) {
                expires += new_value->it_value.tv_nsec / (1000000000 / HZ);
            }
        }
        
        /* Set the timer */
        mod_timer(&tfd->timer, expires);
    }
    
    /* Unlock the timer file descriptor */
    spin_unlock(&tfd->lock);
    
    return 0;
}

/**
 * Get the time remaining on a timer file descriptor
 * 
 * @param fd The file descriptor
 * @param curr_value The current value
 * @return 0 on success, or a negative error code
 */
int time_timerfd_gettime(int fd, struct itimerspec *curr_value) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the timer file descriptor */
    timerfd_t *tfd = file->private_data;
    
    if (tfd == NULL) {
        return -1;
    }
    
    /* Check the current value */
    if (curr_value == NULL) {
        return -1;
    }
    
    /* Lock the timer file descriptor */
    spin_lock(&tfd->lock);
    
    /* Set the interval */
    curr_value->it_interval = tfd->value.it_interval;
    
    /* Check if the timer is enabled */
    if (timer_pending(&tfd->timer)) {
        /* Calculate the remaining time */
        unsigned long remaining = tfd->timer.expires - jiffies;
        
        /* Set the value */
        curr_value->it_value.tv_sec = remaining / HZ;
        curr_value->it_value.tv_nsec = (remaining % HZ) * (1000000000 / HZ);
    } else {
        /* Timer is not enabled */
        curr_value->it_value.tv_sec = 0;
        curr_value->it_value.tv_nsec = 0;
    }
    
    /* Unlock the timer file descriptor */
    spin_unlock(&tfd->lock);
    
    return 0;
}

/**
 * Read from a timer file descriptor
 * 
 * @param file The file
 * @param buf The buffer
 * @param count The count
 * @param ppos The position
 * @return The number of bytes read, or a negative error code
 */
static ssize_t timerfd_read(file_t *file, char *buf, size_t count, loff_t *ppos) {
    /* Get the timer file descriptor */
    timerfd_t *tfd = file->private_data;
    
    if (tfd == NULL) {
        return -1;
    }
    
    /* Check the count */
    if (count < sizeof(uint64_t)) {
        return -1;
    }
    
    /* Lock the timer file descriptor */
    spin_lock(&tfd->lock);
    
    /* Check if there are any expirations */
    if (tfd->ticks == 0) {
        /* Check if the file is non-blocking */
        if (file->f_flags & O_NONBLOCK) {
            spin_unlock(&tfd->lock);
            return -1;
        }
        
        /* Unlock the timer file descriptor */
        spin_unlock(&tfd->lock);
        
        /* Wait for expirations */
        int ret = wait_event_interruptible(tfd->wait, tfd->ticks > 0);
        
        if (ret) {
            return -1;
        }
        
        /* Lock the timer file descriptor */
        spin_lock(&tfd->lock);
    }
    
    /* Get the number of expirations */
    uint64_t ticks = tfd->ticks;
    
    /* Reset the number of expirations */
    tfd->ticks = 0;
    
    /* Unlock the timer file descriptor */
    spin_unlock(&tfd->lock);
    
    /* Copy the number of expirations to the buffer */
    memcpy(buf, &ticks, sizeof(uint64_t));
    
    return sizeof(uint64_t);
}

/**
 * Poll a timer file descriptor
 * 
 * @param file The file
 * @param wait The wait table
 * @return The poll mask
 */
static unsigned int timerfd_poll(file_t *file, struct poll_table_struct *wait) {
    /* Get the timer file descriptor */
    timerfd_t *tfd = file->private_data;
    
    if (tfd == NULL) {
        return 0;
    }
    
    /* Add the wait queue to the poll table */
    poll_wait(file, &tfd->wait, wait);
    
    /* Check if there are any expirations */
    if (tfd->ticks > 0) {
        return POLLIN | POLLRDNORM;
    }
    
    return 0;
}

/**
 * Release a timer file descriptor
 * 
 * @param inode The inode
 * @param file The file
 * @return 0 on success, or a negative error code
 */
static int timerfd_release(inode_t *inode, file_t *file) {
    /* Get the timer file descriptor */
    timerfd_t *tfd = file->private_data;
    
    if (tfd == NULL) {
        return 0;
    }
    
    /* Lock the mutex */
    mutex_lock(&timerfd_mutex);
    
    /* Find the timer file descriptor */
    int id = -1;
    
    for (int i = 0; i < MAX_TIMERFD; i++) {
        if (timerfd_table[i] == tfd) {
            id = i;
            break;
        }
    }
    
    /* Check if we found the timer file descriptor */
    if (id != -1) {
        /* Free the timer file descriptor */
        timerfd_table[id] = NULL;
    }
    
    /* Unlock the mutex */
    mutex_unlock(&timerfd_mutex);
    
    /* Cancel the timer */
    del_timer_sync(&tfd->timer);
    
    /* Free the timer file descriptor */
    kfree(tfd);
    
    return 0;
}

/* Timer file descriptor operations */
static const struct file_operations timerfd_fops = {
    .read = timerfd_read,
    .poll = timerfd_poll,
    .release = timerfd_release,
};

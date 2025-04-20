/**
 * eventfd.c - Horizon kernel eventfd implementation
 * 
 * This file contains the implementation of the eventfd subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/io.h>
#include <horizon/fs/vfs.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Eventfd flags */
#define EFD_SEMAPHORE  (1 << 0)
#define EFD_CLOEXEC    (1 << 1)
#define EFD_NONBLOCK   (1 << 2)

/* Eventfd structure */
typedef struct {
    uint64_t count;             /* Counter */
    unsigned int flags;         /* Flags */
    struct wait_queue_head wait_read; /* Wait queue for readers */
    struct wait_queue_head wait_write; /* Wait queue for writers */
    struct mutex mutex;         /* Mutex */
} eventfd_t;

/* Maximum number of eventfds */
#define MAX_EVENTFD 1024

/* Eventfds */
static eventfd_t *eventfd_table[MAX_EVENTFD];

/* Eventfd mutex */
static struct mutex eventfd_mutex;

/**
 * Initialize the eventfd subsystem
 */
void eventfd_init(void) {
    /* Initialize the mutex */
    mutex_init(&eventfd_mutex);
    
    /* Initialize the eventfds */
    for (int i = 0; i < MAX_EVENTFD; i++) {
        eventfd_table[i] = NULL;
    }
}

/**
 * Create an eventfd
 * 
 * @param initval The initial value
 * @param flags The flags
 * @return The file descriptor, or a negative error code
 */
int eventfd_create(unsigned int initval, int flags) {
    /* Check the flags */
    if (flags & ~(EFD_SEMAPHORE | EFD_CLOEXEC | EFD_NONBLOCK)) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&eventfd_mutex);
    
    /* Find a free eventfd */
    int id = -1;
    
    for (int i = 0; i < MAX_EVENTFD; i++) {
        if (eventfd_table[i] == NULL) {
            id = i;
            break;
        }
    }
    
    /* Check if we found a free eventfd */
    if (id == -1) {
        mutex_unlock(&eventfd_mutex);
        return -1;
    }
    
    /* Allocate a new eventfd */
    eventfd_t *efd = kmalloc(sizeof(eventfd_t), MEM_KERNEL | MEM_ZERO);
    
    if (efd == NULL) {
        mutex_unlock(&eventfd_mutex);
        return -1;
    }
    
    /* Initialize the eventfd */
    efd->count = initval;
    efd->flags = flags;
    init_waitqueue_head(&efd->wait_read);
    init_waitqueue_head(&efd->wait_write);
    mutex_init(&efd->mutex);
    
    /* Set the eventfd */
    eventfd_table[id] = efd;
    
    /* Unlock the mutex */
    mutex_unlock(&eventfd_mutex);
    
    /* Create a file descriptor */
    file_t *file;
    int fd = file_anon_fd(efd, &file);
    
    if (fd < 0) {
        /* Lock the mutex */
        mutex_lock(&eventfd_mutex);
        
        /* Free the eventfd */
        eventfd_table[id] = NULL;
        kfree(efd);
        
        /* Unlock the mutex */
        mutex_unlock(&eventfd_mutex);
        
        return -1;
    }
    
    /* Set the file flags */
    if (flags & EFD_NONBLOCK) {
        file->f_flags |= O_NONBLOCK;
    }
    
    if (flags & EFD_CLOEXEC) {
        file->f_flags |= O_CLOEXEC;
    }
    
    return fd;
}

/**
 * Read from an eventfd
 * 
 * @param file The file
 * @param buf The buffer
 * @param count The count
 * @param ppos The position
 * @return The number of bytes read, or a negative error code
 */
static ssize_t eventfd_read(file_t *file, char *buf, size_t count, loff_t *ppos) {
    /* Get the eventfd */
    eventfd_t *efd = file->private_data;
    
    if (efd == NULL) {
        return -1;
    }
    
    /* Check the count */
    if (count < sizeof(uint64_t)) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&efd->mutex);
    
    /* Check if the eventfd is empty */
    if (efd->count == 0) {
        /* Check if the file is non-blocking */
        if (file->f_flags & O_NONBLOCK) {
            mutex_unlock(&efd->mutex);
            return -1;
        }
        
        /* Wait for the eventfd to be non-empty */
        mutex_unlock(&efd->mutex);
        int ret = wait_event_interruptible(efd->wait_read, efd->count > 0);
        
        if (ret) {
            return -1;
        }
        
        mutex_lock(&efd->mutex);
    }
    
    /* Get the value */
    uint64_t val;
    
    if (efd->flags & EFD_SEMAPHORE) {
        val = 1;
        efd->count--;
    } else {
        val = efd->count;
        efd->count = 0;
    }
    
    /* Wake up writers */
    wake_up_interruptible(&efd->wait_write);
    
    /* Unlock the mutex */
    mutex_unlock(&efd->mutex);
    
    /* Copy the value to the buffer */
    memcpy(buf, &val, sizeof(uint64_t));
    
    return sizeof(uint64_t);
}

/**
 * Write to an eventfd
 * 
 * @param file The file
 * @param buf The buffer
 * @param count The count
 * @param ppos The position
 * @return The number of bytes written, or a negative error code
 */
static ssize_t eventfd_write(file_t *file, const char *buf, size_t count, loff_t *ppos) {
    /* Get the eventfd */
    eventfd_t *efd = file->private_data;
    
    if (efd == NULL) {
        return -1;
    }
    
    /* Check the count */
    if (count < sizeof(uint64_t)) {
        return -1;
    }
    
    /* Get the value */
    uint64_t val;
    memcpy(&val, buf, sizeof(uint64_t));
    
    /* Check the value */
    if (val == UINT64_MAX) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&efd->mutex);
    
    /* Check if the eventfd is full */
    if (UINT64_MAX - efd->count < val) {
        /* Check if the file is non-blocking */
        if (file->f_flags & O_NONBLOCK) {
            mutex_unlock(&efd->mutex);
            return -1;
        }
        
        /* Wait for the eventfd to be non-full */
        mutex_unlock(&efd->mutex);
        int ret = wait_event_interruptible(efd->wait_write, UINT64_MAX - efd->count >= val);
        
        if (ret) {
            return -1;
        }
        
        mutex_lock(&efd->mutex);
    }
    
    /* Add the value */
    efd->count += val;
    
    /* Wake up readers */
    wake_up_interruptible(&efd->wait_read);
    
    /* Unlock the mutex */
    mutex_unlock(&efd->mutex);
    
    return sizeof(uint64_t);
}

/**
 * Poll an eventfd
 * 
 * @param file The file
 * @param wait The wait table
 * @return The poll mask
 */
static unsigned int eventfd_poll(file_t *file, struct poll_table_struct *wait) {
    /* Get the eventfd */
    eventfd_t *efd = file->private_data;
    
    if (efd == NULL) {
        return POLLERR;
    }
    
    /* Add the wait queues to the poll table */
    poll_wait(file, &efd->wait_read, wait);
    poll_wait(file, &efd->wait_write, wait);
    
    /* Get the poll mask */
    unsigned int mask = 0;
    
    /* Lock the mutex */
    mutex_lock(&efd->mutex);
    
    /* Check if the eventfd is readable */
    if (efd->count > 0) {
        mask |= POLLIN | POLLRDNORM;
    }
    
    /* Check if the eventfd is writable */
    if (UINT64_MAX - efd->count > 0) {
        mask |= POLLOUT | POLLWRNORM;
    }
    
    /* Unlock the mutex */
    mutex_unlock(&efd->mutex);
    
    return mask;
}

/**
 * Release an eventfd
 * 
 * @param inode The inode
 * @param file The file
 * @return 0 on success, or a negative error code
 */
static int eventfd_release(inode_t *inode, file_t *file) {
    /* Get the eventfd */
    eventfd_t *efd = file->private_data;
    
    if (efd == NULL) {
        return 0;
    }
    
    /* Lock the mutex */
    mutex_lock(&eventfd_mutex);
    
    /* Find the eventfd */
    int id = -1;
    
    for (int i = 0; i < MAX_EVENTFD; i++) {
        if (eventfd_table[i] == efd) {
            id = i;
            break;
        }
    }
    
    /* Check if we found the eventfd */
    if (id != -1) {
        /* Free the eventfd */
        eventfd_table[id] = NULL;
    }
    
    /* Unlock the mutex */
    mutex_unlock(&eventfd_mutex);
    
    /* Free the eventfd */
    kfree(efd);
    
    return 0;
}

/* Eventfd operations */
static const struct file_operations eventfd_fops = {
    .read = eventfd_read,
    .write = eventfd_write,
    .poll = eventfd_poll,
    .release = eventfd_release,
};

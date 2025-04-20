/**
 * epoll.c - Horizon kernel epoll implementation
 * 
 * This file contains the implementation of the epoll system call.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/net.h>
#include <horizon/fs/vfs.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Epoll events */
#define EPOLLIN         0x0001  /* The associated file is available for read operations */
#define EPOLLPRI        0x0002  /* There is urgent data available for read operations */
#define EPOLLOUT        0x0004  /* The associated file is available for write operations */
#define EPOLLERR        0x0008  /* Error condition happened on the associated file descriptor */
#define EPOLLHUP        0x0010  /* Hang up happened on the associated file descriptor */
#define EPOLLNVAL       0x0020  /* Invalid request: fd not open */
#define EPOLLRDNORM     0x0040  /* Normal data may be read */
#define EPOLLRDBAND     0x0080  /* Priority data may be read */
#define EPOLLWRNORM     0x0100  /* Writing now will not block */
#define EPOLLWRBAND     0x0200  /* Priority data may be written */
#define EPOLLMSG        0x0400  /* A message is available */
#define EPOLLRDHUP      0x2000  /* Stream socket peer closed connection */
#define EPOLLWAKEUP     (1 << 29) /* Epoll event: disable system suspend */
#define EPOLLONESHOT    (1 << 30) /* Epoll event: one shot */
#define EPOLLET         (1 << 31) /* Epoll event: edge triggered */

/* Epoll operation */
#define EPOLL_CTL_ADD   1       /* Add a file descriptor to the interface */
#define EPOLL_CTL_DEL   2       /* Remove a file descriptor from the interface */
#define EPOLL_CTL_MOD   3       /* Change file descriptor epoll_event structure */

/* Epoll event structure */
struct epoll_event {
    uint32_t events;    /* Epoll events */
    epoll_data_t data;  /* User data variable */
};

/* Epoll data structure */
typedef union epoll_data {
    void *ptr;          /* Pointer to user data */
    int fd;             /* File descriptor */
    uint32_t u32;       /* 32-bit integer */
    uint64_t u64;       /* 64-bit integer */
} epoll_data_t;

/* Epoll item structure */
struct epoll_item {
    struct list_head list;      /* List of epoll items */
    int fd;                     /* File descriptor */
    file_t *file;               /* File */
    struct epoll_event event;   /* Epoll event */
};

/* Epoll structure */
struct epoll {
    struct list_head items;     /* List of epoll items */
    struct wait_queue_head wait; /* Wait queue */
    struct mutex mutex;         /* Mutex */
};

/* Maximum number of epoll instances */
#define MAX_EPOLL 1024

/* Epoll instances */
static struct epoll *epoll_table[MAX_EPOLL];

/* Epoll mutex */
static struct mutex epoll_mutex;

/**
 * Initialize the epoll subsystem
 */
void epoll_init(void) {
    /* Initialize the mutex */
    mutex_init(&epoll_mutex);
    
    /* Initialize the epoll instances */
    for (int i = 0; i < MAX_EPOLL; i++) {
        epoll_table[i] = NULL;
    }
}

/**
 * Create an epoll instance
 * 
 * @param size The size hint
 * @return The file descriptor, or a negative error code
 */
int epoll_create(int size) {
    /* Check parameters */
    if (size <= 0) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&epoll_mutex);
    
    /* Find a free epoll instance */
    int id = -1;
    
    for (int i = 0; i < MAX_EPOLL; i++) {
        if (epoll_table[i] == NULL) {
            id = i;
            break;
        }
    }
    
    /* Check if we found a free epoll instance */
    if (id == -1) {
        mutex_unlock(&epoll_mutex);
        return -1;
    }
    
    /* Allocate a new epoll instance */
    struct epoll *ep = kmalloc(sizeof(struct epoll), MEM_KERNEL | MEM_ZERO);
    
    if (ep == NULL) {
        mutex_unlock(&epoll_mutex);
        return -1;
    }
    
    /* Initialize the epoll instance */
    INIT_LIST_HEAD(&ep->items);
    init_waitqueue_head(&ep->wait);
    mutex_init(&ep->mutex);
    
    /* Set the epoll instance */
    epoll_table[id] = ep;
    
    /* Unlock the mutex */
    mutex_unlock(&epoll_mutex);
    
    /* Create a file descriptor */
    file_t *file;
    int fd = file_anon_fd(ep, &file);
    
    if (fd < 0) {
        /* Lock the mutex */
        mutex_lock(&epoll_mutex);
        
        /* Free the epoll instance */
        epoll_table[id] = NULL;
        kfree(ep);
        
        /* Unlock the mutex */
        mutex_unlock(&epoll_mutex);
        
        return -1;
    }
    
    return fd;
}

/**
 * Create an epoll instance
 * 
 * @param flags The flags
 * @return The file descriptor, or a negative error code
 */
int epoll_create1(int flags) {
    /* Check parameters */
    if (flags & ~(O_CLOEXEC)) {
        return -1;
    }
    
    /* Create an epoll instance */
    int fd = epoll_create(1);
    
    if (fd < 0) {
        return fd;
    }
    
    /* Set the flags */
    if (flags & O_CLOEXEC) {
        /* Get the file */
        file_t *file = process_get_file(task_current(), fd);
        
        if (file == NULL) {
            return -1;
        }
        
        /* Set the close-on-exec flag */
        file->f_flags |= O_CLOEXEC;
    }
    
    return fd;
}

/**
 * Control an epoll instance
 * 
 * @param epfd The epoll file descriptor
 * @param op The operation
 * @param fd The file descriptor
 * @param event The event
 * @return 0 on success, or a negative error code
 */
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    /* Check parameters */
    if (op != EPOLL_CTL_ADD && op != EPOLL_CTL_DEL && op != EPOLL_CTL_MOD) {
        return -1;
    }
    
    if (op != EPOLL_CTL_DEL && event == NULL) {
        return -1;
    }
    
    /* Get the epoll file */
    file_t *epfile = process_get_file(task_current(), epfd);
    
    if (epfile == NULL) {
        return -1;
    }
    
    /* Get the epoll instance */
    struct epoll *ep = epfile->private_data;
    
    if (ep == NULL) {
        return -1;
    }
    
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&ep->mutex);
    
    /* Find the epoll item */
    struct epoll_item *item;
    int found = 0;
    
    list_for_each_entry(item, &ep->items, list) {
        if (item->fd == fd) {
            found = 1;
            break;
        }
    }
    
    /* Perform the operation */
    int ret = 0;
    
    switch (op) {
        case EPOLL_CTL_ADD:
            /* Check if the item already exists */
            if (found) {
                ret = -1;
                break;
            }
            
            /* Allocate a new epoll item */
            item = kmalloc(sizeof(struct epoll_item), MEM_KERNEL | MEM_ZERO);
            
            if (item == NULL) {
                ret = -1;
                break;
            }
            
            /* Initialize the epoll item */
            item->fd = fd;
            item->file = file;
            memcpy(&item->event, event, sizeof(struct epoll_event));
            
            /* Add the epoll item to the list */
            list_add_tail(&item->list, &ep->items);
            break;
        
        case EPOLL_CTL_DEL:
            /* Check if the item exists */
            if (!found) {
                ret = -1;
                break;
            }
            
            /* Remove the epoll item from the list */
            list_del(&item->list);
            
            /* Free the epoll item */
            kfree(item);
            break;
        
        case EPOLL_CTL_MOD:
            /* Check if the item exists */
            if (!found) {
                ret = -1;
                break;
            }
            
            /* Update the epoll item */
            memcpy(&item->event, event, sizeof(struct epoll_event));
            break;
    }
    
    /* Unlock the mutex */
    mutex_unlock(&ep->mutex);
    
    return ret;
}

/**
 * Wait for events on an epoll instance
 * 
 * @param epfd The epoll file descriptor
 * @param events The events
 * @param maxevents The maximum number of events
 * @param timeout The timeout
 * @return The number of ready file descriptors, or a negative error code
 */
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    /* Check parameters */
    if (events == NULL || maxevents <= 0) {
        return -1;
    }
    
    /* Get the epoll file */
    file_t *epfile = process_get_file(task_current(), epfd);
    
    if (epfile == NULL) {
        return -1;
    }
    
    /* Get the epoll instance */
    struct epoll *ep = epfile->private_data;
    
    if (ep == NULL) {
        return -1;
    }
    
    /* Initialize the poll table */
    struct poll_table_struct table;
    poll_init_table(&table);
    
    /* Lock the mutex */
    mutex_lock(&ep->mutex);
    
    /* Poll all files */
    int count = 0;
    struct epoll_item *item;
    
    list_for_each_entry(item, &ep->items, list) {
        /* Poll the file */
        unsigned int mask = file_poll(item->file, &table);
        
        /* Check if the file is ready */
        if (mask & item->event.events) {
            /* Set the events */
            events[count].events = mask & item->event.events;
            events[count].data = item->event.data;
            
            /* Increment the count */
            count++;
            
            /* Check if we have reached the maximum number of events */
            if (count >= maxevents) {
                break;
            }
        }
    }
    
    /* Check if any files are ready */
    if (count > 0 || timeout == 0) {
        /* Free the poll table */
        poll_free_table(&table);
        
        /* Unlock the mutex */
        mutex_unlock(&ep->mutex);
        
        return count;
    }
    
    /* Unlock the mutex */
    mutex_unlock(&ep->mutex);
    
    /* Wait for events */
    if (timeout < 0) {
        /* Wait indefinitely */
        schedule();
    } else {
        /* Wait for the timeout */
        schedule_timeout(timeout);
    }
    
    /* Free the poll table */
    poll_free_table(&table);
    
    /* Lock the mutex */
    mutex_lock(&ep->mutex);
    
    /* Poll all files again */
    count = 0;
    
    list_for_each_entry(item, &ep->items, list) {
        /* Poll the file */
        unsigned int mask = file_poll(item->file, NULL);
        
        /* Check if the file is ready */
        if (mask & item->event.events) {
            /* Set the events */
            events[count].events = mask & item->event.events;
            events[count].data = item->event.data;
            
            /* Increment the count */
            count++;
            
            /* Check if we have reached the maximum number of events */
            if (count >= maxevents) {
                break;
            }
        }
    }
    
    /* Unlock the mutex */
    mutex_unlock(&ep->mutex);
    
    return count;
}

/**
 * Wait for events on an epoll instance with a timeout
 * 
 * @param epfd The epoll file descriptor
 * @param events The events
 * @param maxevents The maximum number of events
 * @param timeout The timeout
 * @param sigmask The signal mask
 * @return The number of ready file descriptors, or a negative error code
 */
int epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask) {
    /* Wait for events */
    return epoll_wait(epfd, events, maxevents, timeout);
}

/**
 * Close an epoll instance
 * 
 * @param epfd The epoll file descriptor
 * @return 0 on success, or a negative error code
 */
int epoll_close(int epfd) {
    /* Get the epoll file */
    file_t *epfile = process_get_file(task_current(), epfd);
    
    if (epfile == NULL) {
        return -1;
    }
    
    /* Get the epoll instance */
    struct epoll *ep = epfile->private_data;
    
    if (ep == NULL) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&epoll_mutex);
    
    /* Find the epoll instance */
    int id = -1;
    
    for (int i = 0; i < MAX_EPOLL; i++) {
        if (epoll_table[i] == ep) {
            id = i;
            break;
        }
    }
    
    /* Check if we found the epoll instance */
    if (id != -1) {
        /* Free the epoll instance */
        epoll_table[id] = NULL;
    }
    
    /* Unlock the mutex */
    mutex_unlock(&epoll_mutex);
    
    /* Lock the epoll mutex */
    mutex_lock(&ep->mutex);
    
    /* Free all epoll items */
    struct epoll_item *item, *tmp;
    
    list_for_each_entry_safe(item, tmp, &ep->items, list) {
        /* Remove the epoll item from the list */
        list_del(&item->list);
        
        /* Free the epoll item */
        kfree(item);
    }
    
    /* Unlock the epoll mutex */
    mutex_unlock(&ep->mutex);
    
    /* Free the epoll instance */
    kfree(ep);
    
    /* Close the file */
    file_close(epfile);
    
    return 0;
}

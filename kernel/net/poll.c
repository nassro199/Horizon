/**
 * poll.c - Horizon kernel poll implementation
 * 
 * This file contains the implementation of the poll system call.
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

/* Poll events */
#define POLLIN          0x0001  /* There is data to read */
#define POLLPRI         0x0002  /* There is urgent data to read */
#define POLLOUT         0x0004  /* Writing now will not block */
#define POLLERR         0x0008  /* Error condition */
#define POLLHUP         0x0010  /* Hung up */
#define POLLNVAL        0x0020  /* Invalid request: fd not open */
#define POLLRDNORM      0x0040  /* Normal data may be read */
#define POLLRDBAND      0x0080  /* Priority data may be read */
#define POLLWRNORM      0x0100  /* Writing now will not block */
#define POLLWRBAND      0x0200  /* Priority data may be written */
#define POLLMSG         0x0400  /* A message is available */
#define POLLREMOVE      0x1000  /* Remove from poll set */
#define POLLRDHUP       0x2000  /* Stream socket peer closed connection */

/* Poll structure */
struct pollfd {
    int fd;             /* File descriptor */
    short events;       /* Requested events */
    short revents;      /* Returned events */
};

/* Poll table structure */
struct poll_table_struct {
    struct list_head wait_list;  /* List of wait queues */
};

/* Poll wait structure */
struct poll_wait {
    struct list_head list;       /* List of poll waits */
    struct wait_queue_head *wait; /* Wait queue */
};

/**
 * Initialize a poll table
 * 
 * @param table The poll table
 */
void poll_init_table(struct poll_table_struct *table) {
    /* Initialize the wait list */
    INIT_LIST_HEAD(&table->wait_list);
}

/**
 * Add a wait queue to a poll table
 * 
 * @param file The file
 * @param wait The wait queue
 * @param table The poll table
 */
void poll_wait(file_t *file, struct wait_queue_head *wait, struct poll_table_struct *table) {
    /* Check parameters */
    if (file == NULL || wait == NULL || table == NULL) {
        return;
    }
    
    /* Allocate a new poll wait */
    struct poll_wait *pw = kmalloc(sizeof(struct poll_wait), MEM_KERNEL | MEM_ZERO);
    
    if (pw == NULL) {
        return;
    }
    
    /* Initialize the poll wait */
    pw->wait = wait;
    
    /* Add the poll wait to the table */
    list_add_tail(&pw->list, &table->wait_list);
    
    /* Add the current task to the wait queue */
    add_wait_queue(wait, task_current());
}

/**
 * Free a poll table
 * 
 * @param table The poll table
 */
void poll_free_table(struct poll_table_struct *table) {
    /* Check parameters */
    if (table == NULL) {
        return;
    }
    
    /* Remove the current task from all wait queues */
    struct poll_wait *pw, *tmp;
    
    list_for_each_entry_safe(pw, tmp, &table->wait_list, list) {
        /* Remove the current task from the wait queue */
        remove_wait_queue(pw->wait, task_current());
        
        /* Remove the poll wait from the table */
        list_del(&pw->list);
        
        /* Free the poll wait */
        kfree(pw);
    }
}

/**
 * Poll a file
 * 
 * @param file The file
 * @param wait The poll table
 * @return The poll mask
 */
unsigned int file_poll(file_t *file, struct poll_table_struct *wait) {
    /* Check parameters */
    if (file == NULL) {
        return POLLNVAL;
    }
    
    /* Check if the file has a poll operation */
    if (file->f_op == NULL || file->f_op->poll == NULL) {
        return POLLERR;
    }
    
    /* Poll the file */
    return file->f_op->poll(file, wait);
}

/**
 * Poll files
 * 
 * @param fds The file descriptors
 * @param nfds The number of file descriptors
 * @param timeout The timeout
 * @return The number of ready file descriptors, or a negative error code
 */
int do_poll(struct pollfd *fds, unsigned int nfds, int timeout) {
    /* Check parameters */
    if (fds == NULL || nfds == 0) {
        return -1;
    }
    
    /* Initialize the poll table */
    struct poll_table_struct table;
    poll_init_table(&table);
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Poll all files */
    int count = 0;
    unsigned int i;
    
    for (i = 0; i < nfds; i++) {
        /* Get the file */
        file_t *file = process_get_file(task, fds[i].fd);
        
        /* Check if the file is valid */
        if (file == NULL) {
            fds[i].revents = POLLNVAL;
            count++;
            continue;
        }
        
        /* Poll the file */
        fds[i].revents = file_poll(file, &table);
        
        /* Check if the file is ready */
        if (fds[i].revents & fds[i].events) {
            count++;
        }
    }
    
    /* Check if any files are ready */
    if (count > 0 || timeout == 0) {
        /* Free the poll table */
        poll_free_table(&table);
        
        return count;
    }
    
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
    
    /* Poll all files again */
    count = 0;
    
    for (i = 0; i < nfds; i++) {
        /* Get the file */
        file_t *file = process_get_file(task, fds[i].fd);
        
        /* Check if the file is valid */
        if (file == NULL) {
            fds[i].revents = POLLNVAL;
            count++;
            continue;
        }
        
        /* Poll the file */
        fds[i].revents = file_poll(file, NULL);
        
        /* Check if the file is ready */
        if (fds[i].revents & fds[i].events) {
            count++;
        }
    }
    
    return count;
}

/**
 * Poll files
 * 
 * @param fds The file descriptors
 * @param nfds The number of file descriptors
 * @param timeout The timeout
 * @return The number of ready file descriptors, or a negative error code
 */
int sys_poll(struct pollfd *fds, unsigned int nfds, int timeout) {
    /* Poll the files */
    return do_poll(fds, nfds, timeout);
}

/**
 * Poll files with a timeout
 * 
 * @param fds The file descriptors
 * @param nfds The number of file descriptors
 * @param timeout The timeout
 * @param sigmask The signal mask
 * @return The number of ready file descriptors, or a negative error code
 */
int sys_ppoll(struct pollfd *fds, unsigned int nfds, const struct timespec *timeout, const sigset_t *sigmask) {
    /* Check parameters */
    if (timeout == NULL) {
        return do_poll(fds, nfds, -1);
    }
    
    /* Calculate the timeout in milliseconds */
    int timeout_ms = timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000;
    
    /* Poll the files */
    return do_poll(fds, nfds, timeout_ms);
}

/**
 * Select file descriptors
 * 
 * @param nfds The number of file descriptors
 * @param readfds The read file descriptors
 * @param writefds The write file descriptors
 * @param exceptfds The exception file descriptors
 * @param timeout The timeout
 * @return The number of ready file descriptors, or a negative error code
 */
int sys_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    /* Check parameters */
    if (nfds < 0 || nfds > FD_SETSIZE) {
        return -1;
    }
    
    /* Calculate the timeout in milliseconds */
    int timeout_ms = -1;
    
    if (timeout != NULL) {
        timeout_ms = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    }
    
    /* Allocate the poll file descriptors */
    struct pollfd *fds = kmalloc(nfds * sizeof(struct pollfd), MEM_KERNEL | MEM_ZERO);
    
    if (fds == NULL) {
        return -1;
    }
    
    /* Initialize the poll file descriptors */
    int i, count = 0;
    
    for (i = 0; i < nfds; i++) {
        /* Check if the file descriptor is in any set */
        if ((readfds != NULL && FD_ISSET(i, readfds)) ||
            (writefds != NULL && FD_ISSET(i, writefds)) ||
            (exceptfds != NULL && FD_ISSET(i, exceptfds))) {
            /* Set the file descriptor */
            fds[count].fd = i;
            
            /* Set the events */
            if (readfds != NULL && FD_ISSET(i, readfds)) {
                fds[count].events |= POLLIN;
            }
            
            if (writefds != NULL && FD_ISSET(i, writefds)) {
                fds[count].events |= POLLOUT;
            }
            
            if (exceptfds != NULL && FD_ISSET(i, exceptfds)) {
                fds[count].events |= POLLPRI;
            }
            
            count++;
        }
    }
    
    /* Poll the files */
    int ret = do_poll(fds, count, timeout_ms);
    
    if (ret < 0) {
        kfree(fds);
        return ret;
    }
    
    /* Clear the file descriptor sets */
    if (readfds != NULL) {
        FD_ZERO(readfds);
    }
    
    if (writefds != NULL) {
        FD_ZERO(writefds);
    }
    
    if (exceptfds != NULL) {
        FD_ZERO(exceptfds);
    }
    
    /* Set the file descriptor sets */
    for (i = 0; i < count; i++) {
        /* Check if the file descriptor is ready */
        if (fds[i].revents & POLLIN) {
            if (readfds != NULL) {
                FD_SET(fds[i].fd, readfds);
            }
        }
        
        if (fds[i].revents & POLLOUT) {
            if (writefds != NULL) {
                FD_SET(fds[i].fd, writefds);
            }
        }
        
        if (fds[i].revents & POLLPRI) {
            if (exceptfds != NULL) {
                FD_SET(fds[i].fd, exceptfds);
            }
        }
    }
    
    /* Free the poll file descriptors */
    kfree(fds);
    
    return ret;
}

/**
 * Select file descriptors with a timeout
 * 
 * @param nfds The number of file descriptors
 * @param readfds The read file descriptors
 * @param writefds The write file descriptors
 * @param exceptfds The exception file descriptors
 * @param timeout The timeout
 * @param sigmask The signal mask
 * @return The number of ready file descriptors, or a negative error code
 */
int sys_pselect6(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask) {
    /* Check parameters */
    if (timeout == NULL) {
        return sys_select(nfds, readfds, writefds, exceptfds, NULL);
    }
    
    /* Convert the timeout */
    struct timeval tv;
    tv.tv_sec = timeout->tv_sec;
    tv.tv_usec = timeout->tv_nsec / 1000;
    
    /* Select the file descriptors */
    return sys_select(nfds, readfds, writefds, exceptfds, &tv);
}

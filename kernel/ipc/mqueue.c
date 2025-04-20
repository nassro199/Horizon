/**
 * mqueue.c - Horizon kernel POSIX message queue implementation
 * 
 * This file contains the implementation of POSIX message queues.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/ipc.h>
#include <horizon/fs/vfs.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Message queue attributes */
struct mq_attr {
    long mq_flags;      /* Message queue flags */
    long mq_maxmsg;     /* Maximum number of messages */
    long mq_msgsize;    /* Maximum message size */
    long mq_curmsgs;    /* Number of messages currently queued */
    long __reserved[4]; /* Reserved for future use */
};

/* Message queue message */
struct mq_msg {
    struct list_head list;      /* List of messages */
    size_t msg_len;             /* Message length */
    unsigned int msg_prio;      /* Message priority */
    char msg_data[0];           /* Message data */
};

/* Message queue */
struct mqueue {
    struct list_head list;      /* List of message queues */
    char *name;                 /* Message queue name */
    struct list_head msg_list;  /* List of messages */
    struct mq_attr attr;        /* Message queue attributes */
    struct wait_queue_head wait_read;  /* Wait queue for readers */
    struct wait_queue_head wait_write; /* Wait queue for writers */
    struct mutex mutex;         /* Mutex */
    int refcount;               /* Reference count */
};

/* List of message queues */
static LIST_HEAD(mqueue_list);

/* Message queue mutex */
static struct mutex mqueue_mutex;

/**
 * Initialize the message queue subsystem
 */
void mqueue_init(void) {
    /* Initialize the mutex */
    mutex_init(&mqueue_mutex);
}

/**
 * Find a message queue by name
 * 
 * @param name The message queue name
 * @return The message queue, or NULL if not found
 */
static struct mqueue *mqueue_find(const char *name) {
    /* Check parameters */
    if (name == NULL) {
        return NULL;
    }
    
    /* Find the message queue */
    struct mqueue *mq;
    
    list_for_each_entry(mq, &mqueue_list, list) {
        if (strcmp(mq->name, name) == 0) {
            return mq;
        }
    }
    
    return NULL;
}

/**
 * Create or open a message queue
 * 
 * @param name The message queue name
 * @param oflag The open flags
 * @param mode The file mode
 * @param attr The message queue attributes
 * @return The message queue descriptor, or a negative error code
 */
int mqueue_open(const char *name, int oflag, mode_t mode, struct mq_attr *attr) {
    /* Check parameters */
    if (name == NULL) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&mqueue_mutex);
    
    /* Find the message queue */
    struct mqueue *mq = mqueue_find(name);
    
    /* Check if the message queue exists */
    if (mq == NULL) {
        /* Check if we should create the message queue */
        if (!(oflag & O_CREAT)) {
            mutex_unlock(&mqueue_mutex);
            return -1;
        }
        
        /* Allocate a new message queue */
        mq = kmalloc(sizeof(struct mqueue), MEM_KERNEL | MEM_ZERO);
        
        if (mq == NULL) {
            mutex_unlock(&mqueue_mutex);
            return -1;
        }
        
        /* Allocate the name */
        mq->name = kmalloc(strlen(name) + 1, MEM_KERNEL);
        
        if (mq->name == NULL) {
            kfree(mq);
            mutex_unlock(&mqueue_mutex);
            return -1;
        }
        
        /* Copy the name */
        strcpy(mq->name, name);
        
        /* Initialize the message queue */
        INIT_LIST_HEAD(&mq->msg_list);
        
        /* Set the attributes */
        if (attr != NULL) {
            mq->attr.mq_flags = attr->mq_flags;
            mq->attr.mq_maxmsg = attr->mq_maxmsg;
            mq->attr.mq_msgsize = attr->mq_msgsize;
        } else {
            mq->attr.mq_flags = 0;
            mq->attr.mq_maxmsg = 10;
            mq->attr.mq_msgsize = 8192;
        }
        
        mq->attr.mq_curmsgs = 0;
        
        /* Initialize the wait queues */
        init_waitqueue_head(&mq->wait_read);
        init_waitqueue_head(&mq->wait_write);
        
        /* Initialize the mutex */
        mutex_init(&mq->mutex);
        
        /* Set the reference count */
        mq->refcount = 1;
        
        /* Add the message queue to the list */
        list_add(&mq->list, &mqueue_list);
    } else {
        /* Check if we should exclusively create the message queue */
        if ((oflag & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL)) {
            mutex_unlock(&mqueue_mutex);
            return -1;
        }
        
        /* Increment the reference count */
        mq->refcount++;
    }
    
    /* Unlock the mutex */
    mutex_unlock(&mqueue_mutex);
    
    /* Create a file descriptor */
    file_t *file;
    int fd = file_anon_fd(mq, &file);
    
    if (fd < 0) {
        /* Lock the mutex */
        mutex_lock(&mqueue_mutex);
        
        /* Decrement the reference count */
        mq->refcount--;
        
        /* Check if the message queue should be freed */
        if (mq->refcount == 0) {
            /* Remove the message queue from the list */
            list_del(&mq->list);
            
            /* Free the name */
            kfree(mq->name);
            
            /* Free the message queue */
            kfree(mq);
        }
        
        /* Unlock the mutex */
        mutex_unlock(&mqueue_mutex);
        
        return -1;
    }
    
    /* Set the file flags */
    if (oflag & O_NONBLOCK) {
        file->f_flags |= O_NONBLOCK;
    }
    
    return fd;
}

/**
 * Close a message queue
 * 
 * @param mqdes The message queue descriptor
 * @return 0 on success, or a negative error code
 */
int mqueue_close(int mqdes) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), mqdes);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the message queue */
    struct mqueue *mq = file->private_data;
    
    if (mq == NULL) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&mqueue_mutex);
    
    /* Decrement the reference count */
    mq->refcount--;
    
    /* Check if the message queue should be freed */
    if (mq->refcount == 0) {
        /* Remove the message queue from the list */
        list_del(&mq->list);
        
        /* Free all messages */
        struct mq_msg *msg, *tmp;
        
        list_for_each_entry_safe(msg, tmp, &mq->msg_list, list) {
            /* Remove the message from the list */
            list_del(&msg->list);
            
            /* Free the message */
            kfree(msg);
        }
        
        /* Free the name */
        kfree(mq->name);
        
        /* Free the message queue */
        kfree(mq);
    }
    
    /* Unlock the mutex */
    mutex_unlock(&mqueue_mutex);
    
    /* Close the file */
    file_close(file);
    
    return 0;
}

/**
 * Remove a message queue
 * 
 * @param name The message queue name
 * @return 0 on success, or a negative error code
 */
int mqueue_unlink(const char *name) {
    /* Check parameters */
    if (name == NULL) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&mqueue_mutex);
    
    /* Find the message queue */
    struct mqueue *mq = mqueue_find(name);
    
    /* Check if the message queue exists */
    if (mq == NULL) {
        mutex_unlock(&mqueue_mutex);
        return -1;
    }
    
    /* Remove the message queue from the list */
    list_del(&mq->list);
    
    /* Check if the message queue is still in use */
    if (mq->refcount > 0) {
        /* Add the message queue to the list */
        list_add(&mq->list, &mqueue_list);
        
        /* Unlock the mutex */
        mutex_unlock(&mqueue_mutex);
        
        return 0;
    }
    
    /* Free all messages */
    struct mq_msg *msg, *tmp;
    
    list_for_each_entry_safe(msg, tmp, &mq->msg_list, list) {
        /* Remove the message from the list */
        list_del(&msg->list);
        
        /* Free the message */
        kfree(msg);
    }
    
    /* Free the name */
    kfree(mq->name);
    
    /* Free the message queue */
    kfree(mq);
    
    /* Unlock the mutex */
    mutex_unlock(&mqueue_mutex);
    
    return 0;
}

/**
 * Send a message to a message queue
 * 
 * @param mqdes The message queue descriptor
 * @param msg_ptr The message
 * @param msg_len The message length
 * @param msg_prio The message priority
 * @return 0 on success, or a negative error code
 */
int mqueue_send(int mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), mqdes);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the message queue */
    struct mqueue *mq = file->private_data;
    
    if (mq == NULL) {
        return -1;
    }
    
    /* Check the message length */
    if (msg_len > mq->attr.mq_msgsize) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&mq->mutex);
    
    /* Check if the message queue is full */
    if (mq->attr.mq_curmsgs >= mq->attr.mq_maxmsg) {
        /* Check if the file is non-blocking */
        if (file->f_flags & O_NONBLOCK) {
            mutex_unlock(&mq->mutex);
            return -1;
        }
        
        /* Wait for space */
        mutex_unlock(&mq->mutex);
        int ret = wait_event_interruptible(mq->wait_write, mq->attr.mq_curmsgs < mq->attr.mq_maxmsg);
        
        if (ret) {
            return -1;
        }
        
        mutex_lock(&mq->mutex);
    }
    
    /* Allocate a new message */
    struct mq_msg *msg = kmalloc(sizeof(struct mq_msg) + msg_len, MEM_KERNEL);
    
    if (msg == NULL) {
        mutex_unlock(&mq->mutex);
        return -1;
    }
    
    /* Initialize the message */
    msg->msg_len = msg_len;
    msg->msg_prio = msg_prio;
    
    /* Copy the message */
    memcpy(msg->msg_data, msg_ptr, msg_len);
    
    /* Add the message to the list */
    struct mq_msg *pos;
    struct list_head *p = &mq->msg_list;
    
    list_for_each_entry(pos, &mq->msg_list, list) {
        if (msg_prio > pos->msg_prio) {
            p = &pos->list;
            break;
        }
    }
    
    list_add_tail(&msg->list, p);
    
    /* Increment the message count */
    mq->attr.mq_curmsgs++;
    
    /* Wake up readers */
    wake_up_interruptible(&mq->wait_read);
    
    /* Unlock the mutex */
    mutex_unlock(&mq->mutex);
    
    return 0;
}

/**
 * Receive a message from a message queue
 * 
 * @param mqdes The message queue descriptor
 * @param msg_ptr The message buffer
 * @param msg_len The message buffer length
 * @param msg_prio The message priority
 * @return The message length, or a negative error code
 */
ssize_t mqueue_receive(int mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), mqdes);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the message queue */
    struct mqueue *mq = file->private_data;
    
    if (mq == NULL) {
        return -1;
    }
    
    /* Check the message buffer length */
    if (msg_len < mq->attr.mq_msgsize) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&mq->mutex);
    
    /* Check if the message queue is empty */
    if (mq->attr.mq_curmsgs == 0) {
        /* Check if the file is non-blocking */
        if (file->f_flags & O_NONBLOCK) {
            mutex_unlock(&mq->mutex);
            return -1;
        }
        
        /* Wait for a message */
        mutex_unlock(&mq->mutex);
        int ret = wait_event_interruptible(mq->wait_read, mq->attr.mq_curmsgs > 0);
        
        if (ret) {
            return -1;
        }
        
        mutex_lock(&mq->mutex);
    }
    
    /* Get the first message */
    struct mq_msg *msg = list_first_entry(&mq->msg_list, struct mq_msg, list);
    
    /* Remove the message from the list */
    list_del(&msg->list);
    
    /* Decrement the message count */
    mq->attr.mq_curmsgs--;
    
    /* Wake up writers */
    wake_up_interruptible(&mq->wait_write);
    
    /* Copy the message */
    size_t len = MIN(msg_len, msg->msg_len);
    memcpy(msg_ptr, msg->msg_data, len);
    
    /* Set the message priority */
    if (msg_prio != NULL) {
        *msg_prio = msg->msg_prio;
    }
    
    /* Get the message length */
    len = msg->msg_len;
    
    /* Free the message */
    kfree(msg);
    
    /* Unlock the mutex */
    mutex_unlock(&mq->mutex);
    
    return len;
}

/**
 * Get message queue attributes
 * 
 * @param mqdes The message queue descriptor
 * @param attr The message queue attributes
 * @return 0 on success, or a negative error code
 */
int mqueue_getattr(int mqdes, struct mq_attr *attr) {
    /* Check parameters */
    if (attr == NULL) {
        return -1;
    }
    
    /* Get the file */
    file_t *file = process_get_file(task_current(), mqdes);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the message queue */
    struct mqueue *mq = file->private_data;
    
    if (mq == NULL) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&mq->mutex);
    
    /* Copy the attributes */
    memcpy(attr, &mq->attr, sizeof(struct mq_attr));
    
    /* Set the flags */
    attr->mq_flags = file->f_flags & O_NONBLOCK;
    
    /* Unlock the mutex */
    mutex_unlock(&mq->mutex);
    
    return 0;
}

/**
 * Set message queue attributes
 * 
 * @param mqdes The message queue descriptor
 * @param attr The message queue attributes
 * @param oattr The old message queue attributes
 * @return 0 on success, or a negative error code
 */
int mqueue_setattr(int mqdes, const struct mq_attr *attr, struct mq_attr *oattr) {
    /* Check parameters */
    if (attr == NULL) {
        return -1;
    }
    
    /* Get the file */
    file_t *file = process_get_file(task_current(), mqdes);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the message queue */
    struct mqueue *mq = file->private_data;
    
    if (mq == NULL) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&mq->mutex);
    
    /* Save the old attributes */
    if (oattr != NULL) {
        memcpy(oattr, &mq->attr, sizeof(struct mq_attr));
        oattr->mq_flags = file->f_flags & O_NONBLOCK;
    }
    
    /* Set the flags */
    if (attr->mq_flags & O_NONBLOCK) {
        file->f_flags |= O_NONBLOCK;
    } else {
        file->f_flags &= ~O_NONBLOCK;
    }
    
    /* Unlock the mutex */
    mutex_unlock(&mq->mutex);
    
    return 0;
}

/**
 * Notify when a message is available
 * 
 * @param mqdes The message queue descriptor
 * @param sevp The notification event
 * @return 0 on success, or a negative error code
 */
int mqueue_notify(int mqdes, const struct sigevent *sevp) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), mqdes);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the message queue */
    struct mqueue *mq = file->private_data;
    
    if (mq == NULL) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&mq->mutex);
    
    /* Set the notification */
    /* This would be implemented with actual notification */
    
    /* Unlock the mutex */
    mutex_unlock(&mq->mutex);
    
    return 0;
}

/**
 * Send a message to a message queue with a timeout
 * 
 * @param mqdes The message queue descriptor
 * @param msg_ptr The message
 * @param msg_len The message length
 * @param msg_prio The message priority
 * @param abs_timeout The timeout
 * @return 0 on success, or a negative error code
 */
int mqueue_timedsend(int mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abs_timeout) {
    /* Check parameters */
    if (abs_timeout == NULL) {
        return mqueue_send(mqdes, msg_ptr, msg_len, msg_prio);
    }
    
    /* Get the file */
    file_t *file = process_get_file(task_current(), mqdes);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the message queue */
    struct mqueue *mq = file->private_data;
    
    if (mq == NULL) {
        return -1;
    }
    
    /* Check the message length */
    if (msg_len > mq->attr.mq_msgsize) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&mq->mutex);
    
    /* Check if the message queue is full */
    if (mq->attr.mq_curmsgs >= mq->attr.mq_maxmsg) {
        /* Check if the file is non-blocking */
        if (file->f_flags & O_NONBLOCK) {
            mutex_unlock(&mq->mutex);
            return -1;
        }
        
        /* Wait for space */
        mutex_unlock(&mq->mutex);
        int ret = wait_event_interruptible_timeout(mq->wait_write, mq->attr.mq_curmsgs < mq->attr.mq_maxmsg, timespec_to_jiffies(abs_timeout));
        
        if (ret <= 0) {
            return -1;
        }
        
        mutex_lock(&mq->mutex);
    }
    
    /* Allocate a new message */
    struct mq_msg *msg = kmalloc(sizeof(struct mq_msg) + msg_len, MEM_KERNEL);
    
    if (msg == NULL) {
        mutex_unlock(&mq->mutex);
        return -1;
    }
    
    /* Initialize the message */
    msg->msg_len = msg_len;
    msg->msg_prio = msg_prio;
    
    /* Copy the message */
    memcpy(msg->msg_data, msg_ptr, msg_len);
    
    /* Add the message to the list */
    struct mq_msg *pos;
    struct list_head *p = &mq->msg_list;
    
    list_for_each_entry(pos, &mq->msg_list, list) {
        if (msg_prio > pos->msg_prio) {
            p = &pos->list;
            break;
        }
    }
    
    list_add_tail(&msg->list, p);
    
    /* Increment the message count */
    mq->attr.mq_curmsgs++;
    
    /* Wake up readers */
    wake_up_interruptible(&mq->wait_read);
    
    /* Unlock the mutex */
    mutex_unlock(&mq->mutex);
    
    return 0;
}

/**
 * Receive a message from a message queue with a timeout
 * 
 * @param mqdes The message queue descriptor
 * @param msg_ptr The message buffer
 * @param msg_len The message buffer length
 * @param msg_prio The message priority
 * @param abs_timeout The timeout
 * @return The message length, or a negative error code
 */
ssize_t mqueue_timedreceive(int mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio, const struct timespec *abs_timeout) {
    /* Check parameters */
    if (abs_timeout == NULL) {
        return mqueue_receive(mqdes, msg_ptr, msg_len, msg_prio);
    }
    
    /* Get the file */
    file_t *file = process_get_file(task_current(), mqdes);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the message queue */
    struct mqueue *mq = file->private_data;
    
    if (mq == NULL) {
        return -1;
    }
    
    /* Check the message buffer length */
    if (msg_len < mq->attr.mq_msgsize) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&mq->mutex);
    
    /* Check if the message queue is empty */
    if (mq->attr.mq_curmsgs == 0) {
        /* Check if the file is non-blocking */
        if (file->f_flags & O_NONBLOCK) {
            mutex_unlock(&mq->mutex);
            return -1;
        }
        
        /* Wait for a message */
        mutex_unlock(&mq->mutex);
        int ret = wait_event_interruptible_timeout(mq->wait_read, mq->attr.mq_curmsgs > 0, timespec_to_jiffies(abs_timeout));
        
        if (ret <= 0) {
            return -1;
        }
        
        mutex_lock(&mq->mutex);
    }
    
    /* Get the first message */
    struct mq_msg *msg = list_first_entry(&mq->msg_list, struct mq_msg, list);
    
    /* Remove the message from the list */
    list_del(&msg->list);
    
    /* Decrement the message count */
    mq->attr.mq_curmsgs--;
    
    /* Wake up writers */
    wake_up_interruptible(&mq->wait_write);
    
    /* Copy the message */
    size_t len = MIN(msg_len, msg->msg_len);
    memcpy(msg_ptr, msg->msg_data, len);
    
    /* Set the message priority */
    if (msg_prio != NULL) {
        *msg_prio = msg->msg_prio;
    }
    
    /* Get the message length */
    len = msg->msg_len;
    
    /* Free the message */
    kfree(msg);
    
    /* Unlock the mutex */
    mutex_unlock(&mq->mutex);
    
    return len;
}

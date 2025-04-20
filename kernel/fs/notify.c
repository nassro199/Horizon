/**
 * notify.c - Horizon kernel file notification operations
 * 
 * This file contains the implementation of file notification operations.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* inotify event structure */
struct inotify_event {
    int wd;             /* Watch descriptor */
    uint32_t mask;      /* Watch mask */
    uint32_t cookie;    /* Cookie to synchronize two events */
    uint32_t len;       /* Length of name field */
    char name[];        /* Optional name */
};

/* inotify watch flags */
#define IN_ACCESS        0x00000001  /* File was accessed */
#define IN_MODIFY        0x00000002  /* File was modified */
#define IN_ATTRIB        0x00000004  /* Metadata changed */
#define IN_CLOSE_WRITE   0x00000008  /* Writable file was closed */
#define IN_CLOSE_NOWRITE 0x00000010  /* Unwritable file closed */
#define IN_OPEN          0x00000020  /* File was opened */
#define IN_MOVED_FROM    0x00000040  /* File was moved from X */
#define IN_MOVED_TO      0x00000080  /* File was moved to Y */
#define IN_CREATE        0x00000100  /* Subfile was created */
#define IN_DELETE        0x00000200  /* Subfile was deleted */
#define IN_DELETE_SELF   0x00000400  /* Self was deleted */
#define IN_MOVE_SELF     0x00000800  /* Self was moved */

/* inotify special flags */
#define IN_ONLYDIR       0x01000000  /* Only watch the path if it is a directory */
#define IN_DONT_FOLLOW   0x02000000  /* Don't follow a symlink */
#define IN_EXCL_UNLINK   0x04000000  /* Exclude events on unlinked objects */
#define IN_MASK_ADD      0x20000000  /* Add to the mask of an existing watch */
#define IN_ISDIR         0x40000000  /* Event occurred against dir */
#define IN_ONESHOT       0x80000000  /* Only send event once */

/* inotify init flags */
#define IN_CLOEXEC       0x00080000  /* Set close-on-exec flag */
#define IN_NONBLOCK      0x00000800  /* Set O_NONBLOCK flag */

/* Maximum number of inotify instances */
#define MAX_INOTIFY_INSTANCES 128

/* Maximum number of watches per instance */
#define MAX_INOTIFY_WATCHES 8192

/* Maximum number of events per read */
#define MAX_INOTIFY_EVENTS 16384

/* Maximum event queue size */
#define MAX_INOTIFY_QUEUESIZE 16384

/* inotify watch structure */
typedef struct inotify_watch {
    int wd;                     /* Watch descriptor */
    struct dentry *dentry;      /* Watched dentry */
    struct mount *mnt;          /* Watched mount */
    uint32_t mask;              /* Watch mask */
    struct inotify_instance *instance; /* Parent instance */
    struct list_head list;      /* List of watches */
} inotify_watch_t;

/* inotify event queue entry */
typedef struct inotify_event_entry {
    struct inotify_event *event; /* Event */
    struct list_head list;      /* List of events */
} inotify_event_entry_t;

/* inotify instance structure */
typedef struct inotify_instance {
    int id;                     /* Instance ID */
    struct list_head watches;   /* List of watches */
    struct list_head events;    /* Event queue */
    size_t event_count;         /* Number of events in queue */
    size_t event_size;          /* Size of events in queue */
    struct mutex mutex;         /* Mutex */
    struct wait_queue_head wait; /* Wait queue */
    uint32_t last_wd;           /* Last watch descriptor */
    int flags;                  /* Flags */
    int user_count;             /* User count */
} inotify_instance_t;

/* inotify instances */
static inotify_instance_t *inotify_instances[MAX_INOTIFY_INSTANCES];

/* inotify mutex */
static struct mutex inotify_mutex;

/**
 * Initialize the inotify subsystem
 */
void inotify_init_module(void) {
    /* Initialize the mutex */
    mutex_init(&inotify_mutex);
    
    /* Initialize the instances */
    for (int i = 0; i < MAX_INOTIFY_INSTANCES; i++) {
        inotify_instances[i] = NULL;
    }
}

/**
 * Create a new inotify instance
 * 
 * @param flags The flags
 * @return The file descriptor, or a negative error code
 */
int inotify_init1(int flags) {
    /* Check flags */
    if (flags & ~(IN_CLOEXEC | IN_NONBLOCK)) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&inotify_mutex);
    
    /* Find a free instance */
    int id = -1;
    
    for (int i = 0; i < MAX_INOTIFY_INSTANCES; i++) {
        if (inotify_instances[i] == NULL) {
            id = i;
            break;
        }
    }
    
    /* Check if we found a free instance */
    if (id == -1) {
        mutex_unlock(&inotify_mutex);
        return -1;
    }
    
    /* Allocate the instance */
    inotify_instance_t *instance = kmalloc(sizeof(inotify_instance_t), MEM_KERNEL | MEM_ZERO);
    
    if (instance == NULL) {
        mutex_unlock(&inotify_mutex);
        return -1;
    }
    
    /* Initialize the instance */
    instance->id = id;
    INIT_LIST_HEAD(&instance->watches);
    INIT_LIST_HEAD(&instance->events);
    instance->event_count = 0;
    instance->event_size = 0;
    mutex_init(&instance->mutex);
    init_waitqueue_head(&instance->wait);
    instance->last_wd = 0;
    instance->flags = flags;
    instance->user_count = 1;
    
    /* Set the instance */
    inotify_instances[id] = instance;
    
    /* Unlock the mutex */
    mutex_unlock(&inotify_mutex);
    
    /* Create a file descriptor */
    file_t *file;
    int fd = file_anon_fd(instance, &file);
    
    if (fd < 0) {
        /* Lock the mutex */
        mutex_lock(&inotify_mutex);
        
        /* Free the instance */
        inotify_instances[id] = NULL;
        kfree(instance);
        
        /* Unlock the mutex */
        mutex_unlock(&inotify_mutex);
        
        return -1;
    }
    
    /* Set the file flags */
    if (flags & IN_NONBLOCK) {
        file->f_flags |= O_NONBLOCK;
    }
    
    if (flags & IN_CLOEXEC) {
        file->f_flags |= O_CLOEXEC;
    }
    
    return fd;
}

/**
 * Create a new inotify instance
 * 
 * @return The file descriptor, or a negative error code
 */
int inotify_init(void) {
    /* Call inotify_init1 with no flags */
    return inotify_init1(0);
}

/**
 * Add a watch to an inotify instance
 * 
 * @param fd The file descriptor
 * @param pathname The path to watch
 * @param mask The watch mask
 * @return The watch descriptor, or a negative error code
 */
int inotify_add_watch(int fd, const char *pathname, uint32_t mask) {
    /* Check parameters */
    if (pathname == NULL) {
        return -1;
    }
    
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the instance */
    inotify_instance_t *instance = file->private_data;
    
    if (instance == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path p;
    int flags = 0;
    
    if (mask & IN_DONT_FOLLOW) {
        flags |= LOOKUP_NOFOLLOW;
    }
    
    int error = vfs_kern_path(pathname, flags, &p);
    
    if (error) {
        return error;
    }
    
    /* Check if it's a directory */
    if ((mask & IN_ONLYDIR) && !S_ISDIR(p.dentry->d_inode->i_mode)) {
        vfs_path_release(&p);
        return -1;
    }
    
    /* Lock the instance mutex */
    mutex_lock(&instance->mutex);
    
    /* Check if we already have a watch for this path */
    inotify_watch_t *watch;
    int found = 0;
    
    list_for_each_entry(watch, &instance->watches, list) {
        if (watch->dentry == p.dentry && watch->mnt == p.mnt) {
            found = 1;
            break;
        }
    }
    
    /* Check if we found a watch */
    if (found) {
        /* Update the mask */
        if (mask & IN_MASK_ADD) {
            watch->mask |= mask & ~IN_MASK_ADD;
        } else {
            watch->mask = mask;
        }
        
        /* Unlock the instance mutex */
        mutex_unlock(&instance->mutex);
        
        /* Release the path */
        vfs_path_release(&p);
        
        return watch->wd;
    }
    
    /* Check if we have too many watches */
    if (instance->last_wd >= MAX_INOTIFY_WATCHES) {
        mutex_unlock(&instance->mutex);
        vfs_path_release(&p);
        return -1;
    }
    
    /* Allocate a new watch */
    watch = kmalloc(sizeof(inotify_watch_t), MEM_KERNEL | MEM_ZERO);
    
    if (watch == NULL) {
        mutex_unlock(&instance->mutex);
        vfs_path_release(&p);
        return -1;
    }
    
    /* Initialize the watch */
    watch->wd = ++instance->last_wd;
    watch->dentry = p.dentry;
    watch->mnt = p.mnt;
    watch->mask = mask;
    watch->instance = instance;
    
    /* Add the watch to the instance */
    list_add(&watch->list, &instance->watches);
    
    /* Unlock the instance mutex */
    mutex_unlock(&instance->mutex);
    
    return watch->wd;
}

/**
 * Remove a watch from an inotify instance
 * 
 * @param fd The file descriptor
 * @param wd The watch descriptor
 * @return 0 on success, or a negative error code
 */
int inotify_rm_watch(int fd, int wd) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the instance */
    inotify_instance_t *instance = file->private_data;
    
    if (instance == NULL) {
        return -1;
    }
    
    /* Lock the instance mutex */
    mutex_lock(&instance->mutex);
    
    /* Find the watch */
    inotify_watch_t *watch;
    int found = 0;
    
    list_for_each_entry(watch, &instance->watches, list) {
        if (watch->wd == wd) {
            found = 1;
            break;
        }
    }
    
    /* Check if we found the watch */
    if (!found) {
        mutex_unlock(&instance->mutex);
        return -1;
    }
    
    /* Remove the watch from the instance */
    list_del(&watch->list);
    
    /* Release the path */
    vfs_path_release(&(struct path){watch->mnt, watch->dentry});
    
    /* Free the watch */
    kfree(watch);
    
    /* Unlock the instance mutex */
    mutex_unlock(&instance->mutex);
    
    return 0;
}

/**
 * Close an inotify instance
 * 
 * @param instance The instance
 */
void inotify_close(inotify_instance_t *instance) {
    /* Lock the mutex */
    mutex_lock(&inotify_mutex);
    
    /* Decrement the user count */
    instance->user_count--;
    
    /* Check if we should free the instance */
    if (instance->user_count == 0) {
        /* Lock the instance mutex */
        mutex_lock(&instance->mutex);
        
        /* Free all watches */
        inotify_watch_t *watch, *tmp;
        
        list_for_each_entry_safe(watch, tmp, &instance->watches, list) {
            /* Remove the watch from the instance */
            list_del(&watch->list);
            
            /* Release the path */
            vfs_path_release(&(struct path){watch->mnt, watch->dentry});
            
            /* Free the watch */
            kfree(watch);
        }
        
        /* Free all events */
        inotify_event_entry_t *entry, *tmp2;
        
        list_for_each_entry_safe(entry, tmp2, &instance->events, list) {
            /* Remove the entry from the instance */
            list_del(&entry->list);
            
            /* Free the event */
            kfree(entry->event);
            
            /* Free the entry */
            kfree(entry);
        }
        
        /* Unlock the instance mutex */
        mutex_unlock(&instance->mutex);
        
        /* Free the instance */
        inotify_instances[instance->id] = NULL;
        kfree(instance);
    }
    
    /* Unlock the mutex */
    mutex_unlock(&inotify_mutex);
}

/**
 * Read events from an inotify instance
 * 
 * @param instance The instance
 * @param buffer The buffer to store the events
 * @param count The size of the buffer
 * @return The number of bytes read, or a negative error code
 */
ssize_t inotify_read(inotify_instance_t *instance, char *buffer, size_t count) {
    /* Check parameters */
    if (buffer == NULL) {
        return -1;
    }
    
    /* Check if the buffer is too small */
    if (count < sizeof(struct inotify_event)) {
        return -1;
    }
    
    /* Lock the instance mutex */
    mutex_lock(&instance->mutex);
    
    /* Check if there are any events */
    if (list_empty(&instance->events)) {
        /* Check if the file is non-blocking */
        if (instance->flags & IN_NONBLOCK) {
            mutex_unlock(&instance->mutex);
            return -1;
        }
        
        /* Wait for events */
        mutex_unlock(&instance->mutex);
        int ret = wait_event_interruptible(instance->wait, !list_empty(&instance->events));
        
        if (ret) {
            return -1;
        }
        
        mutex_lock(&instance->mutex);
    }
    
    /* Read events */
    ssize_t total = 0;
    
    while (!list_empty(&instance->events) && total + sizeof(struct inotify_event) <= count) {
        /* Get the first event */
        inotify_event_entry_t *entry = list_first_entry(&instance->events, inotify_event_entry_t, list);
        struct inotify_event *event = entry->event;
        
        /* Calculate the event size */
        size_t event_size = sizeof(struct inotify_event) + event->len;
        
        /* Check if the buffer is too small */
        if (total + event_size > count) {
            break;
        }
        
        /* Copy the event to the buffer */
        memcpy(buffer + total, event, event_size);
        
        /* Update the total */
        total += event_size;
        
        /* Remove the event from the instance */
        list_del(&entry->list);
        
        /* Update the event count and size */
        instance->event_count--;
        instance->event_size -= event_size;
        
        /* Free the event */
        kfree(event);
        
        /* Free the entry */
        kfree(entry);
    }
    
    /* Unlock the instance mutex */
    mutex_unlock(&instance->mutex);
    
    return total;
}

/**
 * Add an event to an inotify instance
 * 
 * @param instance The instance
 * @param wd The watch descriptor
 * @param mask The event mask
 * @param cookie The cookie
 * @param name The name
 * @param name_len The name length
 * @return 0 on success, or a negative error code
 */
int inotify_add_event(inotify_instance_t *instance, int wd, uint32_t mask, uint32_t cookie, const char *name, size_t name_len) {
    /* Check parameters */
    if (instance == NULL) {
        return -1;
    }
    
    /* Lock the instance mutex */
    mutex_lock(&instance->mutex);
    
    /* Check if the queue is full */
    if (instance->event_count >= MAX_INOTIFY_EVENTS || instance->event_size >= MAX_INOTIFY_QUEUESIZE) {
        mutex_unlock(&instance->mutex);
        return -1;
    }
    
    /* Calculate the event size */
    size_t event_size = sizeof(struct inotify_event) + name_len;
    
    /* Allocate the event */
    struct inotify_event *event = kmalloc(event_size, MEM_KERNEL | MEM_ZERO);
    
    if (event == NULL) {
        mutex_unlock(&instance->mutex);
        return -1;
    }
    
    /* Initialize the event */
    event->wd = wd;
    event->mask = mask;
    event->cookie = cookie;
    event->len = name_len;
    
    if (name != NULL && name_len > 0) {
        memcpy(event->name, name, name_len);
    }
    
    /* Allocate the entry */
    inotify_event_entry_t *entry = kmalloc(sizeof(inotify_event_entry_t), MEM_KERNEL | MEM_ZERO);
    
    if (entry == NULL) {
        kfree(event);
        mutex_unlock(&instance->mutex);
        return -1;
    }
    
    /* Initialize the entry */
    entry->event = event;
    
    /* Add the entry to the instance */
    list_add_tail(&entry->list, &instance->events);
    
    /* Update the event count and size */
    instance->event_count++;
    instance->event_size += event_size;
    
    /* Wake up any waiting processes */
    wake_up_interruptible(&instance->wait);
    
    /* Unlock the instance mutex */
    mutex_unlock(&instance->mutex);
    
    return 0;
}

/**
 * Notify an inotify event
 * 
 * @param path The path
 * @param mask The event mask
 * @param cookie The cookie
 * @param name The name
 * @param name_len The name length
 */
void inotify_notify_event(struct path *path, uint32_t mask, uint32_t cookie, const char *name, size_t name_len) {
    /* Check parameters */
    if (path == NULL) {
        return;
    }
    
    /* Lock the mutex */
    mutex_lock(&inotify_mutex);
    
    /* Iterate over all instances */
    for (int i = 0; i < MAX_INOTIFY_INSTANCES; i++) {
        inotify_instance_t *instance = inotify_instances[i];
        
        if (instance == NULL) {
            continue;
        }
        
        /* Lock the instance mutex */
        mutex_lock(&instance->mutex);
        
        /* Find watches for this path */
        inotify_watch_t *watch;
        
        list_for_each_entry(watch, &instance->watches, list) {
            if (watch->dentry == path->dentry && watch->mnt == path->mnt) {
                /* Check if the watch is interested in this event */
                if (watch->mask & mask) {
                    /* Add the event to the instance */
                    inotify_add_event(instance, watch->wd, mask, cookie, name, name_len);
                    
                    /* Check if this is a one-shot watch */
                    if (watch->mask & IN_ONESHOT) {
                        /* Remove the watch from the instance */
                        list_del(&watch->list);
                        
                        /* Release the path */
                        vfs_path_release(&(struct path){watch->mnt, watch->dentry});
                        
                        /* Free the watch */
                        kfree(watch);
                    }
                }
            }
        }
        
        /* Unlock the instance mutex */
        mutex_unlock(&instance->mutex);
    }
    
    /* Unlock the mutex */
    mutex_unlock(&inotify_mutex);
}

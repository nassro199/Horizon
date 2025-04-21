/**
 * signalfd.c - Horizon kernel signal file descriptor implementation
 *
 * This file contains the implementation of the signal file descriptor.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/signal.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/task.h>
#include <horizon/errno.h>

/* Forward declarations */
struct poll_table_struct;
int poll_wait(file_t *file, struct wait_queue_head *wait, struct poll_table_struct *wait_table);

/* Poll flags */
#define POLLIN          0x0001
#define POLLRDNORM      0x0040

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Signal file descriptor flags */
#define SFD_CLOEXEC       O_CLOEXEC
#define SFD_NONBLOCK      O_NONBLOCK

/* Signal file descriptor structure */
typedef struct signalfd {
    sigset_t mask;              /* Signal mask */
    struct wait_queue_head wait; /* Wait queue */
    spinlock_t lock;            /* Lock */
} signalfd_t;

/* Maximum number of signal file descriptors */
#define MAX_SIGNALFD 1024

/* Signal file descriptors */
static signalfd_t *signalfd_table[MAX_SIGNALFD];

/* Signal file descriptor lock */
static u32 signalfd_lock;

/**
 * Initialize the signal file descriptor subsystem
 */
void signalfd_init(void) {
    /* Initialize the lock */
    signalfd_lock = 0;

    /* Initialize the signal file descriptors */
    for (int i = 0; i < MAX_SIGNALFD; i++) {
        signalfd_table[i] = NULL;
    }
}

/**
 * Read from a signal file descriptor
 *
 * @param file The file
 * @param buf The buffer
 * @param count The count
 * @param ppos The position
 * @return The number of bytes read, or a negative error code
 */
static ssize_t signalfd_read(file_t *file, char *buf, size_t count, loff_t *ppos) {
    /* Get the signal file descriptor */
    signalfd_t *sfd = file->private_data;

    if (sfd == NULL) {
        return -1;
    }

    /* Check the count */
    if (count < sizeof(struct signalfd_siginfo)) {
        return -1;
    }

    /* Lock the signal file descriptor */
    spin_lock(&sfd->lock);

    /* Check if there are any signals */
    sigset_t pending;
    signal_get_pending(&pending);

    sigset_t ready = pending & sfd->mask;

    /* Check if there are any signals */
    if (ready == 0) {
        /* Check if the file is non-blocking */
        if (file->f_flags & O_NONBLOCK) {
            spin_unlock(&sfd->lock);
            return -1;
        }

        /* Unlock the signal file descriptor */
        spin_unlock(&sfd->lock);

        /* Wait for signals - this would be implemented with a proper wait queue */
        /* For now, just return an error */
        return -EAGAIN;

        /* Lock the signal file descriptor */
        spin_lock(&sfd->lock);
    }

    /* Get the first signal */
    int sig = 0;

    for (int i = 1; i < SIGRTMAX; i++) {
        if (ready & (1ULL << (i - 1))) {
            sig = i;
            break;
        }
    }

    /* Check if we found a signal */
    if (sig == 0) {
        spin_unlock(&sfd->lock);
        return -1;
    }

    /* Clear the signal */
    signal_clear_pending(sig);

    /* Unlock the signal file descriptor */
    spin_unlock(&sfd->lock);

    /* Fill the signal info */
    struct signalfd_siginfo info;
    memset(&info, 0, sizeof(struct signalfd_siginfo));

    info.ssi_signo = sig;
    info.ssi_errno = 0;
    info.ssi_code = SI_USER;
    info.ssi_pid = 0;
    info.ssi_uid = 0;
    info.ssi_fd = 0;
    info.ssi_tid = 0;
    info.ssi_band = 0;
    info.ssi_overrun = 0;
    info.ssi_trapno = 0;
    info.ssi_status = 0;
    info.ssi_int = 0;
    info.ssi_ptr = 0;
    info.ssi_utime = 0;
    info.ssi_stime = 0;
    info.ssi_addr = 0;

    /* Copy the signal info to the buffer */
    memcpy(buf, &info, sizeof(struct signalfd_siginfo));

    return sizeof(struct signalfd_siginfo);
}

/**
 * Poll a signal file descriptor
 *
 * @param file The file
 * @param wait The wait table
 * @return The poll mask
 */
static unsigned int signalfd_poll(file_t *file, struct poll_table_struct *wait) {
    /* Get the signal file descriptor */
    signalfd_t *sfd = file->private_data;

    if (sfd == NULL) {
        return 0;
    }

    /* Add the wait queue to the poll table - this would be implemented with a proper wait queue */
    /* For now, just check for signals */

    /* Check if there are any signals */
    sigset_t pending;
    signal_get_pending(&pending);

    sigset_t ready = pending & sfd->mask;

    if (ready != 0) {
        return POLLIN | POLLRDNORM;
    }

    return 0;
}

/**
 * Release a signal file descriptor
 *
 * @param inode The inode
 * @param file The file
 * @return 0 on success, or a negative error code
 */
static int signalfd_release(inode_t *inode, file_t *file) {
    /* Get the signal file descriptor */
    signalfd_t *sfd = file->private_data;

    if (sfd == NULL) {
        return 0;
    }

    /* Lock the signal file descriptor table */
    spin_lock(&signalfd_lock);

    /* Find the signal file descriptor */
    int id = -1;

    for (int i = 0; i < MAX_SIGNALFD; i++) {
        if (signalfd_table[i] == sfd) {
            id = i;
            break;
        }
    }

    /* Check if we found the signal file descriptor */
    if (id != -1) {
        /* Free the signal file descriptor */
        signalfd_table[id] = NULL;
    }

    /* Unlock the signal file descriptor table */
    spin_unlock(&signalfd_lock);

    /* Free the signal file descriptor */
    kfree(sfd);

    return 0;
}

/* Signal file descriptor operations */
static const struct file_operations signalfd_fops = {
    .read = signalfd_read,
    .poll = signalfd_poll,
    .release = signalfd_release,
};

/**
 * Create a signal file descriptor
 *
 * @param ufd The file descriptor
 * @param user_mask The signal mask
 * @param sigsetsize The size of the signal mask
 * @return The file descriptor, or a negative error code
 */
int signal_signalfd(int ufd, const sigset_t *user_mask, size_t sigsetsize) {
    /* Check the parameters */
    if (user_mask == NULL) {
        return -1;
    }

    /* Check the signal set size */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Check if we're updating an existing file descriptor */
    if (ufd >= 0) {
        /* Get the signal file descriptor - this would be implemented with a proper file system */
        /* For now, just check if the file descriptor is valid */
        int id = ufd - 1000;

        if (id < 0 || id >= MAX_SIGNALFD) {
            return -EBADF;
        }

        /* Get the signal file descriptor */
        signalfd_t *sfd = signalfd_table[id];

        if (sfd == NULL) {
            return -EBADF;
        }

        /* Lock the signal file descriptor */
        spin_lock(&sfd->lock);

        /* Update the signal mask */
        memcpy(&sfd->mask, user_mask, sizeof(sigset_t));

        /* Unlock the signal file descriptor */
        spin_unlock(&sfd->lock);

        return ufd;
    }

    /* Lock the signal file descriptor table */
    spin_lock(&signalfd_lock);

    /* Find a free signal file descriptor */
    int id = -1;

    for (int i = 0; i < MAX_SIGNALFD; i++) {
        if (signalfd_table[i] == NULL) {
            id = i;
            break;
        }
    }

    /* Check if we found a free signal file descriptor */
    if (id == -1) {
        spin_unlock(&signalfd_lock);
        return -EMFILE;
    }

    /* Allocate the signal file descriptor */
    signalfd_t *sfd = kmalloc(sizeof(signalfd_t), 0);

    if (sfd == NULL) {
        spin_unlock(&signalfd_lock);
        return -ENOMEM;
    }

    /* Initialize the signal file descriptor */
    memset(sfd, 0, sizeof(signalfd_t));
    memcpy(&sfd->mask, user_mask, sizeof(sigset_t));
    sfd->lock = 0;

    /* Set the signal file descriptor */
    signalfd_table[id] = sfd;

    /* Unlock the signal file descriptor table */
    spin_unlock(&signalfd_lock);

    /* Create a file descriptor - this would be implemented with a proper file system */
    /* For now, just return a dummy file descriptor */
    int fd = id + 1000; /* Use a high number to avoid conflicts */

    if (fd < 0) {
        /* Lock the signal file descriptor table */
        spin_lock(&signalfd_lock);

        /* Free the signal file descriptor */
        signalfd_table[id] = NULL;
        kfree(sfd);

        /* Unlock the signal file descriptor table */
        spin_unlock(&signalfd_lock);

        return -EMFILE;
    }

    return fd;
}

/**
 * Create a signal file descriptor
 *
 * @param ufd The file descriptor
 * @param user_mask The signal mask
 * @param sigsetsize The size of the signal mask
 * @param flags The flags
 * @return The file descriptor, or a negative error code
 */
int signal_signalfd4(int ufd, const sigset_t *user_mask, size_t sigsetsize, int flags) {
    /* Check the flags */
    if (flags & ~(SFD_CLOEXEC | SFD_NONBLOCK)) {
        return -1;
    }

    /* Create the signal file descriptor */
    int fd = signal_signalfd(ufd, user_mask, sigsetsize);

    if (fd < 0) {
        return fd;
    }

    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Set the file flags */
    if (flags & SFD_NONBLOCK) {
        file->f_flags |= O_NONBLOCK;
    }

    if (flags & SFD_CLOEXEC) {
        file->f_flags |= O_CLOEXEC;
    }

    return fd;
}

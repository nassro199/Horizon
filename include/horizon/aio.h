/**
 * aio.h - Horizon kernel asynchronous I/O definitions
 * 
 * This file contains definitions for the asynchronous I/O subsystem.
 */

#ifndef _KERNEL_AIO_H
#define _KERNEL_AIO_H

#include <horizon/types.h>

/* AIO context type */
typedef unsigned long aio_context_t;

/* AIO operation structure */
struct iocb {
    void *data;                  /* User-defined data */
    unsigned int key;            /* Key for use in identifying this operation */
    unsigned short aio_lio_opcode; /* Operation code */
    short aio_reqprio;           /* Request priority offset */
    int aio_fildes;              /* File descriptor */
    void *aio_buf;               /* I/O buffer */
    size_t aio_nbytes;           /* Number of bytes */
    long long aio_offset;        /* File offset */
    unsigned long aio_reserved1; /* Reserved */
    unsigned long aio_reserved2; /* Reserved */
    unsigned long aio_reserved3; /* Reserved */
};

/* AIO event structure */
struct io_event {
    void *data;                  /* User-defined data */
    struct iocb *obj;            /* AIO operation */
    long res;                    /* Result code */
    long res2;                   /* Secondary result */
};

/* Poll structure */
struct pollfd {
    int fd;                      /* File descriptor */
    short events;                /* Requested events */
    short revents;               /* Returned events */
};

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

/* Epoll events */
#define EPOLLIN         0x00000001 /* Available for read */
#define EPOLLPRI        0x00000002 /* Urgent data available */
#define EPOLLOUT        0x00000004 /* Available for write */
#define EPOLLERR        0x00000008 /* Error condition */
#define EPOLLHUP        0x00000010 /* Hang up */
#define EPOLLNVAL       0x00000020 /* Invalid request, fd not open */
#define EPOLLRDNORM     0x00000040 /* Normal data available for read */
#define EPOLLRDBAND     0x00000080 /* Priority data available for read */
#define EPOLLWRNORM     0x00000100 /* Normal data available for write */
#define EPOLLWRBAND     0x00000200 /* Priority data available for write */
#define EPOLLMSG        0x00000400 /* Message available */
#define EPOLLRDHUP      0x00002000 /* Stream socket peer closed connection */
#define EPOLLEXCLUSIVE  0x10000000 /* Exclusive wakeup */
#define EPOLLWAKEUP     0x20000000 /* Wakeup the calling process */
#define EPOLLONESHOT    0x40000000 /* One shot event */
#define EPOLLET         0x80000000 /* Edge-triggered event */

/* Epoll operations */
#define EPOLL_CTL_ADD   1       /* Add a file descriptor to the interface */
#define EPOLL_CTL_DEL   2       /* Remove a file descriptor from the interface */
#define EPOLL_CTL_MOD   3       /* Change file descriptor epoll_event structure */

/* Epoll event structure */
struct epoll_event {
    uint32_t events;             /* Epoll events */
    epoll_data_t data;           /* User data variable */
};

/* Epoll data union */
typedef union epoll_data {
    void *ptr;                   /* Pointer to user-defined data */
    int fd;                      /* File descriptor */
    uint32_t u32;                /* 32-bit integer */
    uint64_t u64;                /* 64-bit integer */
} epoll_data_t;

/* AIO functions */
int aio_io_setup(unsigned nr_events, aio_context_t *ctxp);
int aio_io_destroy(aio_context_t ctx);
int aio_io_submit(aio_context_t ctx, long nr, struct iocb **iocbpp);
int aio_io_cancel(aio_context_t ctx, struct iocb *iocb, struct io_event *result);
int aio_io_getevents(aio_context_t ctx, long min_nr, long nr, struct io_event *events, struct timespec *timeout);
int aio_poll(struct pollfd *fds, nfds_t nfds, int timeout);
int aio_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *tmo_p, const sigset_t *sigmask, size_t sigsetsize);
int aio_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int aio_pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask);
int aio_epoll_create(int size);
int aio_epoll_create1(int flags);
int aio_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int aio_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
int aio_epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask, size_t sigsetsize);
int aio_eventfd(unsigned int initval);
int aio_eventfd2(unsigned int initval, int flags);
int aio_signalfd(int ufd, const sigset_t *mask, size_t sizemask);
int aio_signalfd4(int ufd, const sigset_t *mask, size_t sizemask, int flags);
int aio_timerfd_create(int clockid, int flags);
int aio_timerfd_settime(int ufd, int flags, const struct itimerspec *utmr, struct itimerspec *otmr);
int aio_timerfd_gettime(int ufd, struct itimerspec *otmr);
int aio_inotify_init(void);
int aio_inotify_init1(int flags);
int aio_inotify_add_watch(int fd, const char *pathname, uint32_t mask);
int aio_inotify_rm_watch(int fd, int wd);
int aio_fanotify_init(unsigned int flags, unsigned int event_f_flags);
int aio_fanotify_mark(int fanotify_fd, unsigned int flags, uint64_t mask, int dirfd, const char *pathname);

#endif /* _KERNEL_AIO_H */

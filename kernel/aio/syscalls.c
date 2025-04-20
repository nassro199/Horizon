/**
 * syscalls.c - Horizon kernel asynchronous I/O system calls
 * 
 * This file contains the implementation of the asynchronous I/O system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/aio.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: io_setup */
long sys_io_setup(long nr_events, long ctxp, long unused1, long unused2, long unused3, long unused4) {
    /* Create an asynchronous I/O context */
    return aio_io_setup(nr_events, (aio_context_t *)ctxp);
}

/* System call: io_destroy */
long sys_io_destroy(long ctx, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Destroy an asynchronous I/O context */
    return aio_io_destroy(ctx);
}

/* System call: io_submit */
long sys_io_submit(long ctx, long nr, long iocbpp, long unused1, long unused2, long unused3) {
    /* Submit asynchronous I/O blocks for processing */
    return aio_io_submit(ctx, nr, (struct iocb **)iocbpp);
}

/* System call: io_cancel */
long sys_io_cancel(long ctx, long iocb, long result, long unused1, long unused2, long unused3) {
    /* Cancel an outstanding asynchronous I/O operation */
    return aio_io_cancel(ctx, (struct iocb *)iocb, (struct io_event *)result);
}

/* System call: io_getevents */
long sys_io_getevents(long ctx, long min_nr, long nr, long events, long timeout, long unused1) {
    /* Read asynchronous I/O events from the completion queue */
    return aio_io_getevents(ctx, min_nr, nr, (struct io_event *)events, (struct timespec *)timeout);
}

/* System call: poll */
long sys_poll(long fds, long nfds, long timeout, long unused1, long unused2, long unused3) {
    /* Wait for some event on a file descriptor */
    return aio_poll((struct pollfd *)fds, nfds, timeout);
}

/* System call: ppoll */
long sys_ppoll(long fds, long nfds, long tsp, long sigmask, long sigsetsize, long unused1) {
    /* Wait for some event on a file descriptor (with timeout and signal mask) */
    return aio_ppoll((struct pollfd *)fds, nfds, (const struct timespec *)tsp, (const sigset_t *)sigmask, sigsetsize);
}

/* System call: select */
long sys_select(long nfds, long readfds, long writefds, long exceptfds, long timeout, long unused1) {
    /* Synchronous I/O multiplexing */
    return aio_select(nfds, (fd_set *)readfds, (fd_set *)writefds, (fd_set *)exceptfds, (struct timeval *)timeout);
}

/* System call: pselect6 */
long sys_pselect6(long nfds, long readfds, long writefds, long exceptfds, long timeout, long sigmask) {
    /* Synchronous I/O multiplexing (with timeout and signal mask) */
    struct timespec *ts = (struct timespec *)timeout;
    sigset_t *mask = NULL;
    
    if (sigmask != 0) {
        struct {
            sigset_t *ss;
            size_t ss_len;
        } *ssp = (void *)sigmask;
        
        mask = ssp->ss;
    }
    
    return aio_pselect(nfds, (fd_set *)readfds, (fd_set *)writefds, (fd_set *)exceptfds, ts, mask);
}

/* System call: epoll_create */
long sys_epoll_create(long size, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Open an epoll file descriptor */
    return aio_epoll_create(size);
}

/* System call: epoll_create1 */
long sys_epoll_create1(long flags, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Open an epoll file descriptor (with flags) */
    return aio_epoll_create1(flags);
}

/* System call: epoll_ctl */
long sys_epoll_ctl(long epfd, long op, long fd, long event, long unused1, long unused2) {
    /* Control interface for an epoll file descriptor */
    return aio_epoll_ctl(epfd, op, fd, (struct epoll_event *)event);
}

/* System call: epoll_wait */
long sys_epoll_wait(long epfd, long events, long maxevents, long timeout, long unused1, long unused2) {
    /* Wait for an I/O event on an epoll file descriptor */
    return aio_epoll_wait(epfd, (struct epoll_event *)events, maxevents, timeout);
}

/* System call: epoll_pwait */
long sys_epoll_pwait(long epfd, long events, long maxevents, long timeout, long sigmask, long sigsetsize) {
    /* Wait for an I/O event on an epoll file descriptor (with signal mask) */
    return aio_epoll_pwait(epfd, (struct epoll_event *)events, maxevents, timeout, (const sigset_t *)sigmask, sigsetsize);
}

/* System call: eventfd */
long sys_eventfd(long count, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Create a file descriptor for event notification */
    return aio_eventfd(count);
}

/* System call: eventfd2 */
long sys_eventfd2(long count, long flags, long unused1, long unused2, long unused3, long unused4) {
    /* Create a file descriptor for event notification (with flags) */
    return aio_eventfd2(count, flags);
}

/* System call: signalfd */
long sys_signalfd(long ufd, long user_mask, long sizemask, long unused1, long unused2, long unused3) {
    /* Create a file descriptor for accepting signals */
    return aio_signalfd(ufd, (const sigset_t *)user_mask, sizemask);
}

/* System call: signalfd4 */
long sys_signalfd4(long ufd, long user_mask, long sizemask, long flags, long unused1, long unused2) {
    /* Create a file descriptor for accepting signals (with flags) */
    return aio_signalfd4(ufd, (const sigset_t *)user_mask, sizemask, flags);
}

/* System call: timerfd_create */
long sys_timerfd_create(long clockid, long flags, long unused1, long unused2, long unused3, long unused4) {
    /* Create a file descriptor for timer notification */
    return aio_timerfd_create(clockid, flags);
}

/* System call: timerfd_settime */
long sys_timerfd_settime(long ufd, long flags, long utmr, long otmr, long unused1, long unused2) {
    /* Arm/disarm a timerfd */
    return aio_timerfd_settime(ufd, flags, (const struct itimerspec *)utmr, (struct itimerspec *)otmr);
}

/* System call: timerfd_gettime */
long sys_timerfd_gettime(long ufd, long otmr, long unused1, long unused2, long unused3, long unused4) {
    /* Get the time remaining on a timerfd */
    return aio_timerfd_gettime(ufd, (struct itimerspec *)otmr);
}

/* System call: inotify_init */
long sys_inotify_init(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Initialize inotify instance */
    return aio_inotify_init();
}

/* System call: inotify_init1 */
long sys_inotify_init1(long flags, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Initialize inotify instance (with flags) */
    return aio_inotify_init1(flags);
}

/* System call: inotify_add_watch */
long sys_inotify_add_watch(long fd, long pathname, long mask, long unused1, long unused2, long unused3) {
    /* Add a watch to an initialized inotify instance */
    return aio_inotify_add_watch(fd, (const char *)pathname, mask);
}

/* System call: inotify_rm_watch */
long sys_inotify_rm_watch(long fd, long wd, long unused1, long unused2, long unused3, long unused4) {
    /* Remove an existing watch from an inotify instance */
    return aio_inotify_rm_watch(fd, wd);
}

/* System call: fanotify_init */
long sys_fanotify_init(long flags, long event_f_flags, long unused1, long unused2, long unused3, long unused4) {
    /* Initialize fanotify instance */
    return aio_fanotify_init(flags, event_f_flags);
}

/* System call: fanotify_mark */
long sys_fanotify_mark(long fanotify_fd, long flags, long mask, long dirfd, long pathname, long unused1) {
    /* Add, remove, or modify an fanotify mark on a filesystem object */
    return aio_fanotify_mark(fanotify_fd, flags, mask, dirfd, (const char *)pathname);
}

/* Register asynchronous I/O system calls */
void aio_syscalls_init(void) {
    /* Register asynchronous I/O system calls */
    syscall_register(SYS_IO_SETUP, sys_io_setup);
    syscall_register(SYS_IO_DESTROY, sys_io_destroy);
    syscall_register(SYS_IO_SUBMIT, sys_io_submit);
    syscall_register(SYS_IO_CANCEL, sys_io_cancel);
    syscall_register(SYS_IO_GETEVENTS, sys_io_getevents);
    syscall_register(SYS_POLL, sys_poll);
    syscall_register(SYS_PPOLL, sys_ppoll);
    syscall_register(SYS_SELECT, sys_select);
    syscall_register(SYS_PSELECT6, sys_pselect6);
    syscall_register(SYS_EPOLL_CREATE, sys_epoll_create);
    syscall_register(SYS_EPOLL_CREATE1, sys_epoll_create1);
    syscall_register(SYS_EPOLL_CTL, sys_epoll_ctl);
    syscall_register(SYS_EPOLL_WAIT, sys_epoll_wait);
    syscall_register(SYS_EPOLL_PWAIT, sys_epoll_pwait);
    syscall_register(SYS_EVENTFD, sys_eventfd);
    syscall_register(SYS_EVENTFD2, sys_eventfd2);
    syscall_register(SYS_SIGNALFD, sys_signalfd);
    syscall_register(SYS_SIGNALFD4, sys_signalfd4);
    syscall_register(SYS_TIMERFD_CREATE, sys_timerfd_create);
    syscall_register(SYS_TIMERFD_SETTIME, sys_timerfd_settime);
    syscall_register(SYS_TIMERFD_GETTIME, sys_timerfd_gettime);
    syscall_register(SYS_INOTIFY_INIT, sys_inotify_init);
    syscall_register(SYS_INOTIFY_INIT1, sys_inotify_init1);
    syscall_register(SYS_INOTIFY_ADD_WATCH, sys_inotify_add_watch);
    syscall_register(SYS_INOTIFY_RM_WATCH, sys_inotify_rm_watch);
    syscall_register(SYS_FANOTIFY_INIT, sys_fanotify_init);
    syscall_register(SYS_FANOTIFY_MARK, sys_fanotify_mark);
}

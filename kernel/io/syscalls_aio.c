/**
 * syscalls_aio.c - Horizon kernel asynchronous I/O system calls
 * 
 * This file contains the implementation of the asynchronous I/O system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/io.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: io_setup */
long sys_io_setup(long nr_events, long ctxp, long unused1, long unused2, long unused3, long unused4) {
    /* Create an AIO context */
    return io_setup(nr_events, (aio_context_t **)ctxp);
}

/* System call: io_destroy */
long sys_io_destroy(long ctx, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Destroy an AIO context */
    return io_destroy((aio_context_t *)ctx);
}

/* System call: io_submit */
long sys_io_submit(long ctx, long nr, long iocbpp, long unused1, long unused2, long unused3) {
    /* Submit an AIO request */
    return io_submit((aio_context_t *)ctx, nr, (struct iocb **)iocbpp);
}

/* System call: io_cancel */
long sys_io_cancel(long ctx, long iocb, long result, long unused1, long unused2, long unused3) {
    /* Cancel an AIO request */
    return io_cancel((aio_context_t *)ctx, (struct iocb *)iocb, (struct io_event *)result);
}

/* System call: io_getevents */
long sys_io_getevents(long ctx, long min_nr, long nr, long events, long timeout, long unused1) {
    /* Get AIO events */
    return io_getevents((aio_context_t *)ctx, min_nr, nr, (struct io_event *)events, (struct timespec *)timeout);
}

/* System call: eventfd */
long sys_eventfd(long initval, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Create an eventfd */
    return eventfd_create(initval, 0);
}

/* System call: eventfd2 */
long sys_eventfd2(long initval, long flags, long unused1, long unused2, long unused3, long unused4) {
    /* Create an eventfd */
    return eventfd_create(initval, flags);
}

/* Register asynchronous I/O system calls */
void io_syscalls_init(void) {
    /* Register asynchronous I/O system calls */
    syscall_register(SYS_IO_SETUP, sys_io_setup);
    syscall_register(SYS_IO_DESTROY, sys_io_destroy);
    syscall_register(SYS_IO_SUBMIT, sys_io_submit);
    syscall_register(SYS_IO_CANCEL, sys_io_cancel);
    syscall_register(SYS_IO_GETEVENTS, sys_io_getevents);
    syscall_register(SYS_EVENTFD, sys_eventfd);
    syscall_register(SYS_EVENTFD2, sys_eventfd2);
}

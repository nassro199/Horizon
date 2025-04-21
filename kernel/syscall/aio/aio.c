/**
 * aio.c - Horizon kernel asynchronous I/O-related system calls
 *
 * This file contains the implementation of asynchronous I/O-related system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/aio.h>
#include <horizon/task.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* IO setup system call */
long sys_io_setup(long nr_events, long ctxp, long arg3, long arg4, long arg5, long arg6) {
    /* Set up an asynchronous I/O context */
    return aio_setup(nr_events, (aio_context_t *)ctxp);
}

/* IO destroy system call */
long sys_io_destroy(long ctx, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Destroy an asynchronous I/O context */
    return aio_destroy(ctx);
}

/* IO submit system call */
long sys_io_submit(long ctx, long nr, long iocbpp, long arg4, long arg5, long arg6) {
    /* Submit asynchronous I/O blocks for processing */
    return aio_submit(ctx, nr, (struct iocb **)iocbpp);
}

/* IO cancel system call */
long sys_io_cancel(long ctx, long iocb, long result, long arg4, long arg5, long arg6) {
    /* Cancel an outstanding asynchronous I/O operation */
    return aio_cancel(ctx, (struct iocb *)iocb, (struct io_event *)result);
}

/* IO getevents system call */
long sys_io_getevents(long ctx, long min_nr, long nr, long events, long timeout, long arg6) {
    /* Read asynchronous I/O events from the completion queue */
    return aio_getevents(ctx, min_nr, nr, (struct io_event *)events, (struct timespec *)timeout);
}

/* Initialize asynchronous I/O-related system calls */
void aio_syscalls_init(void) {
    /* Register asynchronous I/O-related system calls */
    syscall_register(SYS_IO_SETUP, sys_io_setup);
    syscall_register(SYS_IO_DESTROY, sys_io_destroy);
    syscall_register(SYS_IO_SUBMIT, sys_io_submit);
    syscall_register(SYS_IO_CANCEL, sys_io_cancel);
    syscall_register(SYS_IO_GETEVENTS, sys_io_getevents);
}

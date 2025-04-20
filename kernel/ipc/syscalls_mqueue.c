/**
 * syscalls_mqueue.c - Horizon kernel POSIX message queue system calls
 * 
 * This file contains the implementation of the POSIX message queue system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/ipc.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: mq_open */
long sys_mq_open(long name, long oflag, long mode, long attr, long unused1, long unused2) {
    /* Open a message queue */
    return mqueue_open((const char *)name, oflag, mode, (struct mq_attr *)attr);
}

/* System call: mq_close */
long sys_mq_close(long mqdes, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Close a message queue */
    return mqueue_close(mqdes);
}

/* System call: mq_unlink */
long sys_mq_unlink(long name, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Remove a message queue */
    return mqueue_unlink((const char *)name);
}

/* System call: mq_send */
long sys_mq_send(long mqdes, long msg_ptr, long msg_len, long msg_prio, long unused1, long unused2) {
    /* Send a message to a message queue */
    return mqueue_send(mqdes, (const char *)msg_ptr, msg_len, msg_prio);
}

/* System call: mq_receive */
long sys_mq_receive(long mqdes, long msg_ptr, long msg_len, long msg_prio, long unused1, long unused2) {
    /* Receive a message from a message queue */
    return mqueue_receive(mqdes, (char *)msg_ptr, msg_len, (unsigned int *)msg_prio);
}

/* System call: mq_getattr */
long sys_mq_getattr(long mqdes, long attr, long unused1, long unused2, long unused3, long unused4) {
    /* Get message queue attributes */
    return mqueue_getattr(mqdes, (struct mq_attr *)attr);
}

/* System call: mq_setattr */
long sys_mq_setattr(long mqdes, long attr, long oattr, long unused1, long unused2, long unused3) {
    /* Set message queue attributes */
    return mqueue_setattr(mqdes, (const struct mq_attr *)attr, (struct mq_attr *)oattr);
}

/* System call: mq_notify */
long sys_mq_notify(long mqdes, long sevp, long unused1, long unused2, long unused3, long unused4) {
    /* Notify when a message is available */
    return mqueue_notify(mqdes, (const struct sigevent *)sevp);
}

/* System call: mq_timedsend */
long sys_mq_timedsend(long mqdes, long msg_ptr, long msg_len, long msg_prio, long abs_timeout, long unused1) {
    /* Send a message to a message queue with a timeout */
    return mqueue_timedsend(mqdes, (const char *)msg_ptr, msg_len, msg_prio, (const struct timespec *)abs_timeout);
}

/* System call: mq_timedreceive */
long sys_mq_timedreceive(long mqdes, long msg_ptr, long msg_len, long msg_prio, long abs_timeout, long unused1) {
    /* Receive a message from a message queue with a timeout */
    return mqueue_timedreceive(mqdes, (char *)msg_ptr, msg_len, (unsigned int *)msg_prio, (const struct timespec *)abs_timeout);
}

/* Register POSIX message queue system calls */
void ipc_mqueue_syscalls_init(void) {
    /* Register POSIX message queue system calls */
    syscall_register(SYS_MQ_OPEN, sys_mq_open);
    syscall_register(SYS_MQ_CLOSE, sys_mq_close);
    syscall_register(SYS_MQ_UNLINK, sys_mq_unlink);
    syscall_register(SYS_MQ_SEND, sys_mq_send);
    syscall_register(SYS_MQ_RECEIVE, sys_mq_receive);
    syscall_register(SYS_MQ_GETATTR, sys_mq_getattr);
    syscall_register(SYS_MQ_SETATTR, sys_mq_setattr);
    syscall_register(SYS_MQ_NOTIFY, sys_mq_notify);
    syscall_register(SYS_MQ_TIMEDSEND, sys_mq_timedsend);
    syscall_register(SYS_MQ_TIMEDRECEIVE, sys_mq_timedreceive);
}

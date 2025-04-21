/**
 * ipc.c - Horizon kernel IPC-related system calls
 *
 * This file contains the implementation of IPC-related system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/ipc.h>
#include <horizon/task.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* IPC system call */
long sys_ipc(long call, long first, long second, long third, long ptr, long fifth) {
    /* Multiplexed IPC system call */
    switch (call) {
        case 1: /* SEMOP */
            return sys_semop(first, (struct sembuf *)ptr, second);
        case 2: /* SEMGET */
            return sys_semget(first, second, third);
        case 3: /* SEMCTL */
            return sys_semctl(first, second, third, (void *)ptr);
        case 4: /* SEMTIMEDOP */
            return -ENOSYS; /* Not implemented */
        case 11: /* MSGSND */
            return sys_msgsnd(first, (const void *)ptr, second, third);
        case 12: /* MSGRCV */
            return sys_msgrcv(first, (void *)ptr, second, fifth, third);
        case 13: /* MSGGET */
            return sys_msgget(first, second);
        case 14: /* MSGCTL */
            return sys_msgctl(first, second, (struct msqid_ds *)ptr);
        case 21: /* SHMAT */
            return (long)sys_shmat(first, (const void *)ptr, second);
        case 22: /* SHMDT */
            return sys_shmdt((const void *)ptr);
        case 23: /* SHMGET */
            return sys_shmget(first, second, third);
        case 24: /* SHMCTL */
            return sys_shmctl(first, second, (struct shmid_ds *)ptr);
        default:
            return -ENOSYS;
    }
}

/* Semaphore get system call */
long sys_semget(long key, long nsems, long semflg, long arg4, long arg5, long arg6) {
    /* Create or get a semaphore set */
    return semget(key, nsems, semflg);
}

/* Semaphore operation system call */
long sys_semop(long semid, long sops, long nsops, long arg4, long arg5, long arg6) {
    /* Perform semaphore operations */
    return semop(semid, (struct sembuf *)sops, nsops);
}

/* Semaphore control system call */
long sys_semctl(long semid, long semnum, long cmd, long arg, long arg5, long arg6) {
    /* Control semaphores */
    return semctl(semid, semnum, cmd, (void *)arg);
}

/* Message queue get system call */
long sys_msgget(long key, long msgflg, long arg3, long arg4, long arg5, long arg6) {
    /* Create or get a message queue */
    return msgget(key, msgflg);
}

/* Message send system call */
long sys_msgsnd(long msqid, long msgp, long msgsz, long msgflg, long arg5, long arg6) {
    /* Send a message to a message queue */
    return msgsnd(msqid, (const void *)msgp, msgsz, msgflg);
}

/* Message receive system call */
long sys_msgrcv(long msqid, long msgp, long msgsz, long msgtyp, long msgflg, long arg6) {
    /* Receive a message from a message queue */
    return msgrcv(msqid, (void *)msgp, msgsz, msgtyp, msgflg);
}

/* Message control system call */
long sys_msgctl(long msqid, long cmd, long buf, long arg4, long arg5, long arg6) {
    /* Control message queues */
    return msgctl(msqid, cmd, (struct msqid_ds *)buf);
}

/* Shared memory get system call */
long sys_shmget(long key, long size, long shmflg, long arg4, long arg5, long arg6) {
    /* Create or get a shared memory segment */
    return shmget(key, size, shmflg);
}

/* Shared memory attach system call */
long sys_shmat(long shmid, long shmaddr, long shmflg, long arg4, long arg5, long arg6) {
    /* Attach a shared memory segment */
    return (long)shmat(shmid, (const void *)shmaddr, shmflg);
}

/* Shared memory detach system call */
long sys_shmdt(long shmaddr, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Detach a shared memory segment */
    return shmdt((const void *)shmaddr);
}

/* Shared memory control system call */
long sys_shmctl(long shmid, long cmd, long buf, long arg4, long arg5, long arg6) {
    /* Control shared memory */
    return shmctl(shmid, cmd, (struct shmid_ds *)buf);
}

/* POSIX message queue open system call */
long sys_mq_open(long name, long oflag, long mode, long attr, long arg5, long arg6) {
    /* Open a message queue */
    return mq_open((const char *)name, oflag, mode, (struct mq_attr *)attr);
}

/* POSIX message queue unlink system call */
long sys_mq_unlink(long name, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Remove a message queue */
    return mq_unlink((const char *)name);
}

/* POSIX message queue timed send system call */
long sys_mq_timedsend(long mqdes, long msg_ptr, long msg_len, long msg_prio, long abs_timeout, long arg6) {
    /* Send a message to a message queue with timeout */
    return mq_send(mqdes, (const char *)msg_ptr, msg_len, msg_prio);
}

/* POSIX message queue timed receive system call */
long sys_mq_timedreceive(long mqdes, long msg_ptr, long msg_len, long msg_prio, long abs_timeout, long arg6) {
    /* Receive a message from a message queue with timeout */
    return mq_receive(mqdes, (char *)msg_ptr, msg_len, (unsigned int *)msg_prio);
}

/* POSIX message queue notify system call */
long sys_mq_notify(long mqdes, long notification, long arg3, long arg4, long arg5, long arg6) {
    /* Register for notification when a message is available */
    return mq_notify(mqdes, (const struct sigevent *)notification);
}

/* POSIX message queue get/set attributes system call */
long sys_mq_getsetattr(long mqdes, long mqstat, long omqstat, long arg4, long arg5, long arg6) {
    /* Get/set message queue attributes */
    return mq_setattr(mqdes, (const struct mq_attr *)mqstat, (struct mq_attr *)omqstat);
}

/* Initialize IPC-related system calls */
void ipc_syscalls_init(void) {
    /* Register IPC-related system calls */
    syscall_register(SYS_IPC, sys_ipc);
    syscall_register(SYS_SEMGET, sys_semget);
    syscall_register(SYS_SEMOP, sys_semop);
    syscall_register(SYS_SEMCTL, sys_semctl);
    syscall_register(SYS_MSGGET, sys_msgget);
    syscall_register(SYS_MSGSND, sys_msgsnd);
    syscall_register(SYS_MSGRCV, sys_msgrcv);
    syscall_register(SYS_MSGCTL, sys_msgctl);
    syscall_register(SYS_SHMGET, sys_shmget);
    syscall_register(SYS_SHMAT, sys_shmat);
    syscall_register(SYS_SHMDT, sys_shmdt);
    syscall_register(SYS_SHMCTL, sys_shmctl);
    syscall_register(SYS_MQ_OPEN, sys_mq_open);
    syscall_register(SYS_MQ_UNLINK, sys_mq_unlink);
    syscall_register(SYS_MQ_TIMEDSEND, sys_mq_timedsend);
    syscall_register(SYS_MQ_TIMEDRECEIVE, sys_mq_timedreceive);
    syscall_register(SYS_MQ_NOTIFY, sys_mq_notify);
    syscall_register(SYS_MQ_GETSETATTR, sys_mq_getsetattr);
}

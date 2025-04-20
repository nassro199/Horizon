/**
 * syscalls.c - Horizon kernel IPC system calls
 * 
 * This file contains the implementation of the IPC system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/ipc.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: pipe */
long sys_pipe(long fildes, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Create a pipe */
    return ipc_pipe((int *)fildes);
}

/* System call: pipe2 */
long sys_pipe2(long fildes, long flags, long unused1, long unused2, long unused3, long unused4) {
    /* Create a pipe with flags */
    return ipc_pipe2((int *)fildes, flags);
}

/* System call: shmget */
long sys_shmget(long key, long size, long shmflg, long unused1, long unused2, long unused3) {
    /* Get a shared memory segment */
    return ipc_shmget(key, size, shmflg);
}

/* System call: shmat */
long sys_shmat(long shmid, long shmaddr, long shmflg, long unused1, long unused2, long unused3) {
    /* Attach a shared memory segment */
    void *raddr;
    int error = ipc_shmat(shmid, (const void *)shmaddr, shmflg, &raddr);
    
    if (error) {
        return error;
    }
    
    return (long)raddr;
}

/* System call: shmdt */
long sys_shmdt(long shmaddr, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Detach a shared memory segment */
    return ipc_shmdt((const void *)shmaddr);
}

/* System call: shmctl */
long sys_shmctl(long shmid, long cmd, long buf, long unused1, long unused2, long unused3) {
    /* Shared memory control */
    return ipc_shmctl(shmid, cmd, (struct shmid_ds *)buf);
}

/* System call: semget */
long sys_semget(long key, long nsems, long semflg, long unused1, long unused2, long unused3) {
    /* Get a semaphore set */
    return ipc_semget(key, nsems, semflg);
}

/* System call: semop */
long sys_semop(long semid, long sops, long nsops, long unused1, long unused2, long unused3) {
    /* Semaphore operations */
    return ipc_semop(semid, (struct sembuf *)sops, nsops);
}

/* System call: semtimedop */
long sys_semtimedop(long semid, long sops, long nsops, long timeout, long unused1, long unused2) {
    /* Semaphore operations with timeout */
    return ipc_semtimedop(semid, (struct sembuf *)sops, nsops, (const struct timespec *)timeout);
}

/* System call: semctl */
long sys_semctl(long semid, long semnum, long cmd, long arg, long unused1, long unused2) {
    /* Semaphore control */
    return ipc_semctl(semid, semnum, cmd, (union semun)arg);
}

/* System call: msgget */
long sys_msgget(long key, long msgflg, long unused1, long unused2, long unused3, long unused4) {
    /* Get a message queue */
    return ipc_msgget(key, msgflg);
}

/* System call: msgsnd */
long sys_msgsnd(long msqid, long msgp, long msgsz, long msgflg, long unused1, long unused2) {
    /* Send a message to a message queue */
    return ipc_msgsnd(msqid, (const void *)msgp, msgsz, msgflg);
}

/* System call: msgrcv */
long sys_msgrcv(long msqid, long msgp, long msgsz, long msgtyp, long msgflg, long unused1) {
    /* Receive a message from a message queue */
    return ipc_msgrcv(msqid, (void *)msgp, msgsz, msgtyp, msgflg);
}

/* System call: msgctl */
long sys_msgctl(long msqid, long cmd, long buf, long unused1, long unused2, long unused3) {
    /* Message queue control */
    return ipc_msgctl(msqid, cmd, (struct msqid_ds *)buf);
}

/* System call: mq_open */
long sys_mq_open(long name, long oflag, long mode, long attr, long unused1, long unused2) {
    /* Open a message queue */
    return ipc_mq_open((const char *)name, oflag, mode, (struct mq_attr *)attr);
}

/* System call: mq_unlink */
long sys_mq_unlink(long name, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Remove a message queue */
    return ipc_mq_unlink((const char *)name);
}

/* System call: mq_timedsend */
long sys_mq_timedsend(long mqdes, long msg_ptr, long msg_len, long msg_prio, long abs_timeout, long unused1) {
    /* Send a message to a message queue with timeout */
    return ipc_mq_timedsend(mqdes, (const char *)msg_ptr, msg_len, msg_prio, (const struct timespec *)abs_timeout);
}

/* System call: mq_timedreceive */
long sys_mq_timedreceive(long mqdes, long msg_ptr, long msg_len, long msg_prio, long abs_timeout, long unused1) {
    /* Receive a message from a message queue with timeout */
    return ipc_mq_timedreceive(mqdes, (char *)msg_ptr, msg_len, (unsigned int *)msg_prio, (const struct timespec *)abs_timeout);
}

/* System call: mq_notify */
long sys_mq_notify(long mqdes, long notification, long unused1, long unused2, long unused3, long unused4) {
    /* Notify process when message is available */
    return ipc_mq_notify(mqdes, (const struct sigevent *)notification);
}

/* System call: mq_getsetattr */
long sys_mq_getsetattr(long mqdes, long mqstat, long omqstat, long unused1, long unused2, long unused3) {
    /* Get/set message queue attributes */
    return ipc_mq_getsetattr(mqdes, (const struct mq_attr *)mqstat, (struct mq_attr *)omqstat);
}

/* System call: ipc */
long sys_ipc(long call, long first, long second, long third, long ptr, long fifth) {
    /* System V IPC system call multiplexer */
    switch (call) {
        case SEMOP:
            return sys_semop(first, ptr, second, 0, 0, 0);
        case SEMGET:
            return sys_semget(first, second, third, 0, 0, 0);
        case SEMCTL:
            return sys_semctl(first, second, third, ptr, 0, 0);
        case SEMTIMEDOP:
            return sys_semtimedop(first, ptr, second, fifth, 0, 0);
        case MSGSND:
            return sys_msgsnd(first, ptr, second, third, 0, 0);
        case MSGRCV:
            return sys_msgrcv(first, ptr, second, fifth, third, 0);
        case MSGGET:
            return sys_msgget(first, second, 0, 0, 0, 0);
        case MSGCTL:
            return sys_msgctl(first, second, ptr, 0, 0, 0);
        case SHMAT:
            return sys_shmat(first, ptr, second, NULL, 0, 0);
        case SHMDT:
            return sys_shmdt(ptr, 0, 0, 0, 0, 0);
        case SHMGET:
            return sys_shmget(first, second, third, 0, 0, 0);
        case SHMCTL:
            return sys_shmctl(first, second, ptr, 0, 0, 0);
        default:
            return -1;
    }
}

/* Register IPC system calls */
void ipc_syscalls_init(void) {
    /* Register IPC system calls */
    syscall_register(SYS_PIPE, sys_pipe);
    syscall_register(SYS_PIPE2, sys_pipe2);
    syscall_register(SYS_SHMGET, sys_shmget);
    syscall_register(SYS_SHMAT, sys_shmat);
    syscall_register(SYS_SHMDT, sys_shmdt);
    syscall_register(SYS_SHMCTL, sys_shmctl);
    syscall_register(SYS_SEMGET, sys_semget);
    syscall_register(SYS_SEMOP, sys_semop);
    syscall_register(SYS_SEMTIMEDOP, sys_semtimedop);
    syscall_register(SYS_SEMCTL, sys_semctl);
    syscall_register(SYS_MSGGET, sys_msgget);
    syscall_register(SYS_MSGSND, sys_msgsnd);
    syscall_register(SYS_MSGRCV, sys_msgrcv);
    syscall_register(SYS_MSGCTL, sys_msgctl);
    syscall_register(SYS_MQ_OPEN, sys_mq_open);
    syscall_register(SYS_MQ_UNLINK, sys_mq_unlink);
    syscall_register(SYS_MQ_TIMEDSEND, sys_mq_timedsend);
    syscall_register(SYS_MQ_TIMEDRECEIVE, sys_mq_timedreceive);
    syscall_register(SYS_MQ_NOTIFY, sys_mq_notify);
    syscall_register(SYS_MQ_GETSETATTR, sys_mq_getsetattr);
    syscall_register(SYS_IPC, sys_ipc);
}

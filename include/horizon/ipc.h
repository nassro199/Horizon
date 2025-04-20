/**
 * ipc.h - Inter-Process Communication definitions
 * 
 * This file contains definitions for the inter-process communication subsystem.
 */

#ifndef _KERNEL_IPC_H
#define _KERNEL_IPC_H

#include <horizon/types.h>

/* IPC message types */
#define MSG_NORMAL      0   /* Normal message */
#define MSG_URGENT      1   /* Urgent message */
#define MSG_BROADCAST   2   /* Broadcast message */
#define MSG_SIGNAL      3   /* Signal message */
#define MSG_SYSTEM      4   /* System message */

/* IPC message flags */
#define MSG_NOBLOCK     0x01    /* Non-blocking operation */
#define MSG_NOWAIT      0x02    /* Don't wait for completion */
#define MSG_EXCEPT      0x04    /* Skip the sending process */
#define MSG_COPY        0x08    /* Copy the message */
#define MSG_DONTWAIT    0x10    /* Don't wait for message */

/* IPC message structure */
typedef struct ipc_message {
    u32 type;               /* Message type */
    u32 sender;             /* Sender process ID */
    u32 receiver;           /* Receiver process ID */
    u32 size;               /* Message size */
    void *data;             /* Message data */
} ipc_message_t;

/* IPC queue structure */
typedef struct ipc_queue {
    u32 id;                 /* Queue ID */
    u32 owner;              /* Owner process ID */
    u32 permissions;        /* Queue permissions */
    u32 max_size;           /* Maximum message size */
    u32 max_msgs;           /* Maximum number of messages */
    u32 num_msgs;           /* Current number of messages */
    ipc_message_t *msgs;    /* Messages */
    struct ipc_queue *next; /* Next queue in list */
} ipc_queue_t;

/* Semaphore structure */
typedef struct semaphore {
    u32 id;                 /* Semaphore ID */
    u32 owner;              /* Owner process ID */
    u32 permissions;        /* Semaphore permissions */
    u32 value;              /* Semaphore value */
    u32 waiters;            /* Number of waiting processes */
    struct semaphore *next; /* Next semaphore in list */
} semaphore_t;

/* Shared memory structure */
typedef struct shm_segment {
    u32 id;                 /* Segment ID */
    u32 owner;              /* Owner process ID */
    u32 permissions;        /* Segment permissions */
    u32 size;               /* Segment size */
    void *addr;             /* Segment address */
    u32 attachments;        /* Number of attachments */
    struct shm_segment *next; /* Next segment in list */
} shm_segment_t;

/* IPC functions */
void ipc_init(void);

/* Message queue functions */
int msgget(u32 key, int msgflg);
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
int msgctl(int msqid, int cmd, void *buf);

/* Semaphore functions */
int semget(u32 key, int nsems, int semflg);
int semop(int semid, void *sops, size_t nsops);
int semctl(int semid, int semnum, int cmd, void *arg);

/* Shared memory functions */
int shmget(u32 key, size_t size, int shmflg);
void *shmat(int shmid, const void *shmaddr, int shmflg);
int shmdt(const void *shmaddr);
int shmctl(int shmid, int cmd, void *buf);

#endif /* _KERNEL_IPC_H */

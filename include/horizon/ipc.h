/**
 * ipc.h - Horizon kernel IPC definitions
 *
 * This file contains definitions for Inter-Process Communication.
 */

#ifndef _KERNEL_IPC_H
#define _KERNEL_IPC_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/task.h>
#include <horizon/thread.h>

/* IPC types */
#define IPC_TYPE_NONE       0  /* No IPC */
#define IPC_TYPE_SYSV       1  /* System V IPC */
#define IPC_TYPE_POSIX      2  /* POSIX IPC */

/* IPC mechanisms */
#define IPC_MECH_NONE       0  /* No mechanism */
#define IPC_MECH_SHM        1  /* Shared memory */
#define IPC_MECH_SEM        2  /* Semaphores */
#define IPC_MECH_MSG        3  /* Message queues */
#define IPC_MECH_PIPE       4  /* Pipes */
#define IPC_MECH_FIFO       5  /* FIFOs */
#define IPC_MECH_SOCKET     6  /* Sockets */
#define IPC_MECH_SIGNAL     7  /* Signals */

/* IPC flags */
#define IPC_CREAT       0x0001  /* Create if key doesn't exist */
#define IPC_EXCL        0x0002  /* Fail if key exists */
#define IPC_NOWAIT      0x0004  /* Return error on wait */
#define IPC_RMID        0x0008  /* Remove resource */
#define IPC_SET         0x0010  /* Set resource */
#define IPC_STAT        0x0020  /* Get resource */
#define IPC_INFO        0x0040  /* Get info */
#define IPC_PRIVATE     0       /* Private key */

/* IPC permissions */
#define IPC_PERM_READ   0x0100  /* Read permission */
#define IPC_PERM_WRITE  0x0200  /* Write permission */
#define IPC_PERM_EXEC   0x0400  /* Execute permission */
#define IPC_PERM_ALL    0x0700  /* All permissions */

/* IPC message flags */
#define MSG_NOBLOCK     0x01    /* Non-blocking operation */
#define MSG_NOWAIT      0x02    /* Don't wait for completion */
#define MSG_EXCEPT      0x04    /* Skip the sending process */
#define MSG_COPY        0x08    /* Copy the message */
#define MSG_DONTWAIT    0x10    /* Don't wait for message */

/* IPC key */
typedef u32 ipc_key_t;

/* IPC ID */
typedef u32 ipc_id_t;

/* IPC permission structure */
typedef struct ipc_perm {
    u32 key;                /* Key */
    u32 uid;                /* Owner's user ID */
    u32 gid;                /* Owner's group ID */
    u32 cuid;               /* Creator's user ID */
    u32 cgid;               /* Creator's group ID */
    u16 mode;               /* Permissions */
    u16 seq;                /* Sequence number */
} ipc_perm_t;

/* IPC shared memory structure */
typedef struct shmid_ds {
    ipc_perm_t shm_perm;    /* Permissions */
    size_t shm_segsz;       /* Size of segment in bytes */
    time_t shm_atime;       /* Last attach time */
    time_t shm_dtime;       /* Last detach time */
    time_t shm_ctime;       /* Last change time */
    u32 shm_cpid;           /* PID of creator */
    u32 shm_lpid;           /* PID of last operation */
    u16 shm_nattch;         /* Number of current attaches */
    u16 shm_flags;          /* Flags */
    void *shm_addr;         /* Segment address */
} shmid_ds_t;

/* IPC semaphore structure */
typedef struct semid_ds {
    ipc_perm_t sem_perm;    /* Permissions */
    time_t sem_otime;       /* Last semop time */
    time_t sem_ctime;       /* Last change time */
    u16 sem_nsems;          /* Number of semaphores in set */
    u16 sem_flags;          /* Flags */
} semid_ds_t;

/* IPC semaphore structure */
typedef struct sem {
    u16 semval;             /* Semaphore value */
    u16 sempid;             /* PID of last operation */
    u16 semncnt;            /* Number of processes waiting for semval to increase */
    u16 semzcnt;            /* Number of processes waiting for semval to become 0 */
} sem_t;

/* IPC semaphore operations */
typedef struct sembuf {
    u16 sem_num;            /* Semaphore number */
    s16 sem_op;             /* Semaphore operation */
    s16 sem_flg;            /* Operation flags */
} sembuf_t;

/* IPC message queue structure */
typedef struct msqid_ds {
    ipc_perm_t msg_perm;    /* Permissions */
    time_t msg_stime;       /* Last msgsnd time */
    time_t msg_rtime;       /* Last msgrcv time */
    time_t msg_ctime;       /* Last change time */
    u32 msg_cbytes;         /* Current number of bytes in queue */
    u32 msg_qnum;           /* Current number of messages in queue */
    u32 msg_qbytes;         /* Maximum number of bytes in queue */
    u32 msg_lspid;          /* PID of last msgsnd */
    u32 msg_lrpid;          /* PID of last msgrcv */
} msqid_ds_t;

/* IPC message structure */
typedef struct msgbuf {
    long mtype;             /* Message type */
    char mtext[1];          /* Message text */
} msgbuf_t;

/* IPC message queue */
typedef struct msg_queue {
    msqid_ds_t q_ds;        /* Message queue descriptor */
    struct list_head q_messages; /* Message list */
    struct list_head q_receivers; /* Receiver list */
    struct list_head q_senders; /* Sender list */
} msg_queue_t;

/* IPC message */
typedef struct msg {
    struct list_head m_list; /* Message list */
    long m_type;            /* Message type */
    size_t m_size;          /* Message size */
    char m_data[0];         /* Message data */
} msg_t;

/* IPC receiver */
typedef struct msg_receiver {
    struct list_head r_list; /* Receiver list */
    task_struct_t *r_task;  /* Receiver task */
    thread_t *r_thread;     /* Receiver thread */
    long r_type;            /* Message type */
    long r_mode;            /* Receive mode */
    void *r_buf;            /* Receive buffer */
    size_t r_size;          /* Receive size */
    size_t r_received;      /* Received size */
} msg_receiver_t;

/* IPC sender */
typedef struct msg_sender {
    struct list_head s_list; /* Sender list */
    task_struct_t *s_task;  /* Sender task */
    thread_t *s_thread;     /* Sender thread */
    long s_type;            /* Message type */
    const void *s_buf;      /* Send buffer */
    size_t s_size;          /* Send size */
    int s_flags;            /* Send flags */
} msg_sender_t;

/* IPC functions */
void ipc_init(void);

/* IPC shared memory functions */
int shmget(ipc_key_t key, size_t size, int shmflg);
void *shmat(int shmid, const void *shmaddr, int shmflg);
int shmdt(const void *shmaddr);
int shmctl(int shmid, int cmd, struct shmid_ds *buf);

/* IPC semaphore functions */
int semget(ipc_key_t key, int nsems, int semflg);
int semop(int semid, struct sembuf *sops, size_t nsops);
int semctl(int semid, int semnum, int cmd, ...);

/* IPC message queue functions */
int msgget(ipc_key_t key, int msgflg);
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
int msgctl(int msqid, int cmd, struct msqid_ds *buf);

/* POSIX shared memory functions */
int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);

/* POSIX semaphore functions */
sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
int sem_close(sem_t *sem);
int sem_unlink(const char *name);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);

/* POSIX message queue functions */
mqd_t mq_open(const char *name, int oflag, mode_t mode, struct mq_attr *attr);
int mq_close(mqd_t mqdes);
int mq_unlink(const char *name);
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio);
ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio);
int mq_setattr(mqd_t mqdes, const struct mq_attr *newattr, struct mq_attr *oldattr);
int mq_getattr(mqd_t mqdes, struct mq_attr *attr);
int mq_notify(mqd_t mqdes, const struct sigevent *notification);

/* Pipe functions */
int pipe(int pipefd[2]);
int pipe2(int pipefd[2], int flags);

/* FIFO functions */
int mkfifo(const char *pathname, mode_t mode);

#endif /* _KERNEL_IPC_H */

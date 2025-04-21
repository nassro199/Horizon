/**
 * msg.c - Horizon kernel IPC message queue implementation
 *
 * This file contains the implementation of IPC message queues.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/ipc.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/task.h>
#include <horizon/thread.h>
#include <horizon/sched.h>
#include <horizon/time.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of message queues */
#define MSG_QUEUES_MAX 128

/* Maximum message size */
#define MSG_MAX_SIZE 8192

/* Maximum number of messages per queue */
#define MSG_MAX_MSGS 1024

/* Message flags */
#define MSG_NOERROR    0x10    /* Don't return error on large message */

/* Message queue array */
static msg_queue_t *msg_queues[MSG_QUEUES_MAX];

/* Message queue IDs */
static ipc_id_t msg_ids[MSG_QUEUES_MAX];

/* Next message queue ID */
static ipc_id_t next_msg_id = 1;

/* Message queue lock */
static u32 msg_lock = 0;

/**
 * Initialize message queues
 */
void msg_init(void) {
    /* Initialize message queues */
    memset(msg_queues, 0, sizeof(msg_queues));
    memset(msg_ids, 0, sizeof(msg_ids));
    next_msg_id = 1;
}

/**
 * Create or get a message queue
 *
 * @param key Key
 * @param msgflg Flags
 * @return Message queue ID on success, negative error code on failure
 */
int msgget(ipc_key_t key, int msgflg) {
    /* Lock message queues */
    spin_lock(&msg_lock);

    /* Check if message queue exists */
    for (int i = 0; i < MSG_QUEUES_MAX; i++) {
        if (msg_queues[i] != NULL && msg_queues[i]->q_ds.msg_perm.key == key) {
            /* Message queue exists */

            /* Check if IPC_EXCL is set */
            if (msgflg & IPC_EXCL) {
                /* Fail if message queue exists */
                spin_unlock(&msg_lock);
                return -EEXIST;
            }

            /* Return message queue ID */
            spin_unlock(&msg_lock);
            return msg_ids[i];
        }
    }

    /* Check if IPC_CREAT is set */
    if (!(msgflg & IPC_CREAT)) {
        /* Fail if message queue doesn't exist */
        spin_unlock(&msg_lock);
        return -ENOENT;
    }

    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MSG_QUEUES_MAX; i++) {
        if (msg_queues[i] == NULL) {
            slot = i;
            break;
        }
    }

    /* Check if we found a free slot */
    if (slot == -1) {
        /* No free slots */
        spin_unlock(&msg_lock);
        return -ENOSPC;
    }

    /* Allocate message queue */
    msg_queue_t *queue = kmalloc(sizeof(msg_queue_t), 0);
    if (queue == NULL) {
        spin_unlock(&msg_lock);
        return -ENOMEM;
    }

    /* Initialize message queue */
    memset(queue, 0, sizeof(msg_queue_t));

    /* Initialize message queue descriptor */
    queue->q_ds.msg_perm.key = key;
    queue->q_ds.msg_perm.uid = task_current()->uid;
    queue->q_ds.msg_perm.gid = task_current()->gid;
    queue->q_ds.msg_perm.cuid = task_current()->uid;
    queue->q_ds.msg_perm.cgid = task_current()->gid;
    queue->q_ds.msg_perm.mode = msgflg & 0777;
    queue->q_ds.msg_perm.seq = 0;
    queue->q_ds.msg_qnum = 0;
    queue->q_ds.msg_qbytes = MSG_MAX_SIZE;
    queue->q_ds.msg_lspid = 0;
    queue->q_ds.msg_lrpid = 0;
    queue->q_ds.msg_stime = 0;
    queue->q_ds.msg_rtime = 0;
    queue->q_ds.msg_ctime = get_timestamp();

    /* Initialize message queue lists */
    INIT_LIST_HEAD(&queue->q_messages);
    INIT_LIST_HEAD(&queue->q_receivers);
    INIT_LIST_HEAD(&queue->q_senders);

    /* Add message queue to array */
    msg_queues[slot] = queue;
    msg_ids[slot] = next_msg_id++;

    /* Unlock message queues */
    spin_unlock(&msg_lock);

    /* Return message queue ID */
    return msg_ids[slot];
}

/**
 * Send a message to a message queue
 *
 * @param msqid Message queue ID
 * @param msgp Message
 * @param msgsz Message size
 * @param msgflg Flags
 * @return 0 on success, negative error code on failure
 */
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg) {
    /* Check parameters */
    if (msgp == NULL) {
        return -EINVAL;
    }

    /* Check message size */
    if (msgsz > MSG_MAX_SIZE) {
        return -EINVAL;
    }

    /* Get message type */
    long mtype = *(long *)msgp;

    /* Check message type */
    if (mtype <= 0) {
        return -EINVAL;
    }

    /* Lock message queues */
    spin_lock(&msg_lock);

    /* Find message queue */
    int slot = -1;
    for (int i = 0; i < MSG_QUEUES_MAX; i++) {
        if (msg_queues[i] != NULL && msg_ids[i] == msqid) {
            slot = i;
            break;
        }
    }

    /* Check if we found the message queue */
    if (slot == -1) {
        /* Message queue not found */
        spin_unlock(&msg_lock);
        return -EINVAL;
    }

    /* Get message queue */
    msg_queue_t *queue = msg_queues[slot];

    /* Check if message queue is full */
    if (queue->q_ds.msg_qnum >= MSG_MAX_MSGS) {
        /* Check if IPC_NOWAIT is set */
        if (msgflg & IPC_NOWAIT) {
            /* Return error */
            spin_unlock(&msg_lock);
            return -EAGAIN;
        }

        /* Create sender */
        msg_sender_t *sender = kmalloc(sizeof(msg_sender_t), 0);
        if (sender == NULL) {
            spin_unlock(&msg_lock);
            return -ENOMEM;
        }

        /* Initialize sender */
        memset(sender, 0, sizeof(msg_sender_t));
        INIT_LIST_HEAD(&sender->s_list);
        sender->s_task = task_current();
        sender->s_thread = thread_self();
        sender->s_type = mtype;
        sender->s_buf = msgp;
        sender->s_size = msgsz;
        sender->s_flags = msgflg;

        /* Add sender to queue */
        list_add_tail(&sender->s_list, &queue->q_senders);

        /* Unlock message queues */
        spin_unlock(&msg_lock);

        /* Block thread */
        sched_block_thread(sender->s_thread);

        /* Thread has been woken up */

        /* Lock message queues */
        spin_lock(&msg_lock);

        /* Check if message queue still exists */
        if (msg_queues[slot] == NULL) {
            /* Message queue has been removed */
            spin_unlock(&msg_lock);
            return -EIDRM;
        }

        /* Remove sender from queue */
        list_del(&sender->s_list);

        /* Free sender */
        kfree(sender);

        /* Unlock message queues */
        spin_unlock(&msg_lock);

        /* Try again */
        return msgsnd(msqid, msgp, msgsz, msgflg);
    }

    /* Check if there are receivers waiting for this message */
    msg_receiver_t *receiver = NULL;
    list_for_each_entry(receiver, &queue->q_receivers, r_list) {
        /* Check if receiver is waiting for this message */
        if (receiver->r_type == 0 || receiver->r_type == mtype) {
            /* Found a receiver */
            break;
        }
    }

    /* Check if we found a receiver */
    if (receiver != NULL) {
        /* Copy message to receiver */
        if (receiver->r_size >= msgsz) {
            /* Copy message */
            memcpy(receiver->r_buf, msgp, msgsz);
            receiver->r_received = msgsz;

            /* Remove receiver from queue */
            list_del(&receiver->r_list);

            /* Update message queue */
            queue->q_ds.msg_lrpid = receiver->r_task->pid;
            queue->q_ds.msg_rtime = get_timestamp();

            /* Wake up receiver */
            sched_unblock_thread(receiver->r_thread);

            /* Update message queue */
            queue->q_ds.msg_lspid = task_current()->pid;
            queue->q_ds.msg_stime = get_timestamp();

            /* Unlock message queues */
            spin_unlock(&msg_lock);

            return 0;
        }
    }

    /* Allocate message */
    msg_t *msg = kmalloc(sizeof(msg_t) + msgsz, 0);
    if (msg == NULL) {
        spin_unlock(&msg_lock);
        return -ENOMEM;
    }

    /* Initialize message */
    memset(msg, 0, sizeof(msg_t));
    INIT_LIST_HEAD(&msg->m_list);
    msg->m_type = mtype;
    msg->m_size = msgsz;
    memcpy(msg->m_data, (char *)msgp + sizeof(long), msgsz);

    /* Add message to queue */
    list_add_tail(&msg->m_list, &queue->q_messages);

    /* Update message queue */
    queue->q_ds.msg_qnum++;
    queue->q_ds.msg_lspid = task_current()->pid;
    queue->q_ds.msg_stime = get_timestamp();

    /* Unlock message queues */
    spin_unlock(&msg_lock);

    return 0;
}

/**
 * Receive a message from a message queue
 *
 * @param msqid Message queue ID
 * @param msgp Message buffer
 * @param msgsz Message buffer size
 * @param msgtyp Message type
 * @param msgflg Flags
 * @return Message size on success, negative error code on failure
 */
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg) {
    /* Check parameters */
    if (msgp == NULL) {
        return -EINVAL;
    }

    /* Lock message queues */
    spin_lock(&msg_lock);

    /* Find message queue */
    int slot = -1;
    for (int i = 0; i < MSG_QUEUES_MAX; i++) {
        if (msg_queues[i] != NULL && msg_ids[i] == msqid) {
            slot = i;
            break;
        }
    }

    /* Check if we found the message queue */
    if (slot == -1) {
        /* Message queue not found */
        spin_unlock(&msg_lock);
        return -EINVAL;
    }

    /* Get message queue */
    msg_queue_t *queue = msg_queues[slot];

    /* Check if message queue is empty */
    if (list_empty(&queue->q_messages)) {
        /* Check if IPC_NOWAIT is set */
        if (msgflg & IPC_NOWAIT) {
            /* Return error */
            spin_unlock(&msg_lock);
            return -ENOMSG;
        }

        /* Create receiver */
        msg_receiver_t *receiver = kmalloc(sizeof(msg_receiver_t), 0);
        if (receiver == NULL) {
            spin_unlock(&msg_lock);
            return -ENOMEM;
        }

        /* Initialize receiver */
        memset(receiver, 0, sizeof(msg_receiver_t));
        INIT_LIST_HEAD(&receiver->r_list);
        receiver->r_task = task_current();
        receiver->r_thread = thread_self();
        receiver->r_type = msgtyp;
        receiver->r_mode = msgflg;
        receiver->r_buf = msgp;
        receiver->r_size = msgsz;
        receiver->r_received = 0;

        /* Add receiver to queue */
        list_add_tail(&receiver->r_list, &queue->q_receivers);

        /* Unlock message queues */
        spin_unlock(&msg_lock);

        /* Block thread */
        sched_block_thread(receiver->r_thread);

        /* Thread has been woken up */

        /* Lock message queues */
        spin_lock(&msg_lock);

        /* Check if message queue still exists */
        if (msg_queues[slot] == NULL) {
            /* Message queue has been removed */
            spin_unlock(&msg_lock);
            return -EIDRM;
        }

        /* Get received size */
        ssize_t received = receiver->r_received;

        /* Free receiver */
        kfree(receiver);

        /* Unlock message queues */
        spin_unlock(&msg_lock);

        return received;
    }

    /* Find message */
    msg_t *msg = NULL;
    msg_t *tmp = NULL;

    if (msgtyp == 0) {
        /* Get first message */
        msg = list_first_entry(&queue->q_messages, msg_t, m_list);
    } else if (msgtyp > 0) {
        /* Find message with matching type */
        list_for_each_entry(tmp, &queue->q_messages, m_list) {
            if (tmp->m_type == msgtyp) {
                msg = tmp;
                break;
            }
        }
    } else {
        /* Find message with type less than or equal to absolute value of msgtyp */
        long abs_msgtyp = -msgtyp;
        list_for_each_entry(tmp, &queue->q_messages, m_list) {
            if (tmp->m_type <= abs_msgtyp) {
                msg = tmp;
                break;
            }
        }
    }

    /* Check if we found a message */
    if (msg == NULL) {
        /* No matching message */
        spin_unlock(&msg_lock);
        return -ENOMSG;
    }

    /* Check message size */
    if (msg->m_size > msgsz && !(msgflg & MSG_NOERROR)) {
        /* Message too big */
        spin_unlock(&msg_lock);
        return -E2BIG;
    }

    /* Copy message */
    size_t copy_size = msg->m_size;
    if (copy_size > msgsz) {
        copy_size = msgsz;
    }

    /* Set message type */
    *(long *)msgp = msg->m_type;

    /* Copy message data */
    memcpy((char *)msgp + sizeof(long), msg->m_data, copy_size);

    /* Remove message from queue */
    list_del(&msg->m_list);

    /* Update message queue */
    queue->q_ds.msg_qnum--;
    queue->q_ds.msg_lrpid = task_current()->pid;
    queue->q_ds.msg_rtime = get_timestamp();

    /* Check if there are senders waiting */
    if (!list_empty(&queue->q_senders)) {
        /* Get first sender */
        msg_sender_t *sender = list_first_entry(&queue->q_senders, msg_sender_t, s_list);

        /* Remove sender from queue */
        list_del(&sender->s_list);

        /* Wake up sender */
        sched_unblock_thread(sender->s_thread);
    }

    /* Free message */
    kfree(msg);

    /* Unlock message queues */
    spin_unlock(&msg_lock);

    return copy_size;

/**
 * Control a message queue
 *
 * @param msqid Message queue ID
 * @param cmd Command
 * @param buf Buffer
 * @return 0 on success, negative error code on failure
 */
int msgctl(int msqid, int cmd, struct msqid_ds *buf) {
    /* Lock message queues */
    spin_lock(&msg_lock);

    /* Find message queue */
    int slot = -1;
    for (int i = 0; i < MSG_QUEUES_MAX; i++) {
        if (msg_queues[i] != NULL && msg_ids[i] == msqid) {
            slot = i;
            break;
        }
    }

    /* Check if we found the message queue */
    if (slot == -1) {
        /* Message queue not found */
        spin_unlock(&msg_lock);
        return -EINVAL;
    }

    /* Get message queue */
    msg_queue_t *queue = msg_queues[slot];

    /* Process command */
    switch (cmd) {
        case IPC_STAT:
            /* Get message queue status */
            if (buf == NULL) {
                spin_unlock(&msg_lock);
                return -EINVAL;
            }

            /* Copy message queue descriptor */
            memcpy(buf, &queue->q_ds, sizeof(msqid_ds_t));

            break;

        case IPC_SET:
            /* Set message queue status */
            if (buf == NULL) {
                spin_unlock(&msg_lock);
                return -EINVAL;
            }

            /* Check permissions */
            if (task_current()->uid != queue->q_ds.msg_perm.uid &&
                task_current()->uid != queue->q_ds.msg_perm.cuid &&
                task_current()->uid != 0) {
                spin_unlock(&msg_lock);
                return -EPERM;
            }

            /* Update message queue descriptor */
            queue->q_ds.msg_perm.uid = buf->msg_perm.uid;
            queue->q_ds.msg_perm.gid = buf->msg_perm.gid;
            queue->q_ds.msg_perm.mode = buf->msg_perm.mode & 0777;
            queue->q_ds.msg_qbytes = buf->msg_qbytes;
            queue->q_ds.msg_ctime = get_timestamp();

            break;

        case IPC_RMID:
            /* Remove message queue */

            /* Check permissions */
            if (task_current()->uid != queue->q_ds.msg_perm.uid &&
                task_current()->uid != queue->q_ds.msg_perm.cuid &&
                task_current()->uid != 0) {
                spin_unlock(&msg_lock);
                return -EPERM;
            }

            /* Wake up all receivers */
            msg_receiver_t *receiver;
            msg_receiver_t *tmp_receiver;
            list_for_each_entry_safe(receiver, tmp_receiver, &queue->q_receivers, r_list) {
                /* Remove receiver from queue */
                list_del(&receiver->r_list);

                /* Wake up receiver */
                sched_unblock_thread(receiver->r_thread);
            }

            /* Wake up all senders */
            msg_sender_t *sender;
            msg_sender_t *tmp_sender;
            list_for_each_entry_safe(sender, tmp_sender, &queue->q_senders, s_list) {
                /* Remove sender from queue */
                list_del(&sender->s_list);

                /* Wake up sender */
                sched_unblock_thread(sender->s_thread);
            }

            /* Free all messages */
            msg_t *msg;
            msg_t *tmp_msg;
            list_for_each_entry_safe(msg, tmp_msg, &queue->q_messages, m_list) {
                /* Remove message from queue */
                list_del(&msg->m_list);

                /* Free message */
                kfree(msg);
            }

            /* Free message queue */
            kfree(queue);

            /* Remove message queue from array */
            msg_queues[slot] = NULL;
            msg_ids[slot] = 0;

            break;

        default:
            /* Invalid command */
            spin_unlock(&msg_lock);
            return -EINVAL;
    }

    /* Unlock message queues */
    spin_unlock(&msg_lock);

    return 0;
}

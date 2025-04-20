/**
 * msg.c - Message queue implementation
 * 
 * This file contains the implementation of the message queue IPC mechanism.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/ipc.h>
#include <horizon/string.h>
#include <horizon/sched/sched.h>

/* Maximum number of message queues */
#define MAX_MSGQUEUES 32

/* Message queue commands */
#define IPC_RMID    0   /* Remove identifier */
#define IPC_SET     1   /* Set options */
#define IPC_STAT    2   /* Get options */
#define IPC_INFO    3   /* Get info */

/* Message queue list */
static ipc_queue_t *msg_queues = NULL;

/* Next message queue ID */
static u32 next_msgq_id = 1;

/* Initialize the message queue subsystem */
void msg_init(void)
{
    /* Initialize the message queue list */
    msg_queues = NULL;
    next_msgq_id = 1;
}

/* Create or get a message queue */
int msgget(u32 key, int msgflg)
{
    /* Check if the queue already exists */
    ipc_queue_t *queue = msg_queues;
    
    while (queue != NULL) {
        if (queue->id == key) {
            /* Queue exists */
            return queue->id;
        }
        
        queue = queue->next;
    }
    
    /* Queue doesn't exist */
    if (!(msgflg & IPC_CREAT)) {
        /* Don't create a new queue */
        return -1;
    }
    
    /* Create a new queue */
    queue = kmalloc(sizeof(ipc_queue_t), MEM_KERNEL | MEM_ZERO);
    
    if (queue == NULL) {
        return -1;
    }
    
    /* Initialize the queue */
    queue->id = next_msgq_id++;
    queue->owner = sched_current_task()->pid;
    queue->permissions = msgflg & 0777;
    queue->max_size = 4096;
    queue->max_msgs = 10;
    queue->num_msgs = 0;
    queue->msgs = NULL;
    
    /* Add to the queue list */
    queue->next = msg_queues;
    msg_queues = queue;
    
    return queue->id;
}

/* Send a message to a queue */
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg)
{
    /* Find the queue */
    ipc_queue_t *queue = msg_queues;
    
    while (queue != NULL) {
        if (queue->id == msqid) {
            break;
        }
        
        queue = queue->next;
    }
    
    if (queue == NULL) {
        /* Queue doesn't exist */
        return -1;
    }
    
    /* Check if the queue is full */
    if (queue->num_msgs >= queue->max_msgs) {
        if (msgflg & IPC_NOWAIT) {
            /* Don't wait */
            return -1;
        }
        
        /* Wait for space in the queue */
        /* This would be implemented with a wait queue */
        /* For now, just return an error */
        return -1;
    }
    
    /* Check if the message is too large */
    if (msgsz > queue->max_size) {
        return -1;
    }
    
    /* Allocate a message */
    ipc_message_t *msg = kmalloc(sizeof(ipc_message_t), MEM_KERNEL | MEM_ZERO);
    
    if (msg == NULL) {
        return -1;
    }
    
    /* Initialize the message */
    msg->type = *(u32 *)msgp;
    msg->sender = sched_current_task()->pid;
    msg->receiver = 0;
    msg->size = msgsz - sizeof(u32);
    
    /* Allocate memory for the message data */
    msg->data = kmalloc(msg->size, MEM_KERNEL);
    
    if (msg->data == NULL) {
        kfree(msg);
        return -1;
    }
    
    /* Copy the message data */
    memcpy(msg->data, (u8 *)msgp + sizeof(u32), msg->size);
    
    /* Add the message to the queue */
    /* This would be implemented with a proper queue */
    /* For now, just set the message as the only one */
    queue->msgs = msg;
    queue->num_msgs++;
    
    return 0;
}

/* Receive a message from a queue */
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
    /* Find the queue */
    ipc_queue_t *queue = msg_queues;
    
    while (queue != NULL) {
        if (queue->id == msqid) {
            break;
        }
        
        queue = queue->next;
    }
    
    if (queue == NULL) {
        /* Queue doesn't exist */
        return -1;
    }
    
    /* Check if the queue is empty */
    if (queue->num_msgs == 0) {
        if (msgflg & IPC_NOWAIT) {
            /* Don't wait */
            return -1;
        }
        
        /* Wait for a message */
        /* This would be implemented with a wait queue */
        /* For now, just return an error */
        return -1;
    }
    
    /* Find a message of the requested type */
    ipc_message_t *msg = queue->msgs;
    
    if (msg == NULL) {
        /* No messages */
        return -1;
    }
    
    /* Check if the message type matches */
    if (msgtyp > 0 && msg->type != msgtyp) {
        /* Type doesn't match */
        return -1;
    }
    
    /* Check if the message is too large */
    if (msg->size > msgsz - sizeof(u32)) {
        if (!(msgflg & MSG_NOERROR)) {
            /* Message is too large */
            return -1;
        }
        
        /* Truncate the message */
        msg->size = msgsz - sizeof(u32);
    }
    
    /* Copy the message type */
    *(u32 *)msgp = msg->type;
    
    /* Copy the message data */
    memcpy((u8 *)msgp + sizeof(u32), msg->data, msg->size);
    
    /* Remove the message from the queue */
    queue->msgs = NULL;
    queue->num_msgs--;
    
    /* Free the message */
    kfree(msg->data);
    kfree(msg);
    
    return msg->size + sizeof(u32);
}

/* Control a message queue */
int msgctl(int msqid, int cmd, void *buf)
{
    /* Find the queue */
    ipc_queue_t *queue = msg_queues;
    ipc_queue_t *prev = NULL;
    
    while (queue != NULL) {
        if (queue->id == msqid) {
            break;
        }
        
        prev = queue;
        queue = queue->next;
    }
    
    if (queue == NULL) {
        /* Queue doesn't exist */
        return -1;
    }
    
    /* Process the command */
    switch (cmd) {
        case IPC_RMID:
            /* Remove the queue */
            if (prev == NULL) {
                /* Queue is the first in the list */
                msg_queues = queue->next;
            } else {
                /* Queue is not the first in the list */
                prev->next = queue->next;
            }
            
            /* Free all messages */
            ipc_message_t *msg = queue->msgs;
            
            while (msg != NULL) {
                ipc_message_t *next = msg->next;
                
                /* Free the message data */
                if (msg->data != NULL) {
                    kfree(msg->data);
                }
                
                /* Free the message */
                kfree(msg);
                
                msg = next;
            }
            
            /* Free the queue */
            kfree(queue);
            
            return 0;
        
        case IPC_SET:
            /* Set queue attributes */
            /* This would be implemented with proper attribute setting */
            /* For now, just return success */
            return 0;
        
        case IPC_STAT:
            /* Get queue attributes */
            /* This would be implemented with proper attribute getting */
            /* For now, just return success */
            return 0;
        
        default:
            /* Unknown command */
            return -1;
    }
}

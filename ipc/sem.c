/**
 * sem.c - Semaphore implementation
 * 
 * This file contains the implementation of the semaphore IPC mechanism.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/ipc.h>
#include <horizon/string.h>
#include <horizon/sched/sched.h>

/* Maximum number of semaphores */
#define MAX_SEMAPHORES 32

/* Semaphore commands */
#define IPC_RMID    0   /* Remove identifier */
#define IPC_SET     1   /* Set options */
#define IPC_STAT    2   /* Get options */
#define IPC_INFO    3   /* Get info */

/* Semaphore operations */
#define GETVAL      11  /* Get semaphore value */
#define SETVAL      12  /* Set semaphore value */
#define GETPID      13  /* Get sempid */
#define GETNCNT     14  /* Get semncnt */
#define GETZCNT     15  /* Get semzcnt */

/* Semaphore operation structure */
typedef struct sembuf {
    u16 sem_num;        /* Semaphore number */
    s16 sem_op;         /* Semaphore operation */
    s16 sem_flg;        /* Operation flags */
} sembuf_t;

/* Semaphore list */
static semaphore_t *semaphores = NULL;

/* Next semaphore ID */
static u32 next_sem_id = 1;

/* Initialize the semaphore subsystem */
void sem_init(void)
{
    /* Initialize the semaphore list */
    semaphores = NULL;
    next_sem_id = 1;
}

/* Create or get a semaphore */
int semget(u32 key, int nsems, int semflg)
{
    /* Check if the semaphore already exists */
    semaphore_t *sem = semaphores;
    
    while (sem != NULL) {
        if (sem->id == key) {
            /* Semaphore exists */
            return sem->id;
        }
        
        sem = sem->next;
    }
    
    /* Semaphore doesn't exist */
    if (!(semflg & IPC_CREAT)) {
        /* Don't create a new semaphore */
        return -1;
    }
    
    /* Create a new semaphore */
    sem = kmalloc(sizeof(semaphore_t), MEM_KERNEL | MEM_ZERO);
    
    if (sem == NULL) {
        return -1;
    }
    
    /* Initialize the semaphore */
    sem->id = next_sem_id++;
    sem->owner = sched_current_task()->pid;
    sem->permissions = semflg & 0777;
    sem->value = 0;
    sem->waiters = 0;
    
    /* Add to the semaphore list */
    sem->next = semaphores;
    semaphores = sem;
    
    return sem->id;
}

/* Perform operations on a semaphore */
int semop(int semid, void *sops, size_t nsops)
{
    /* Find the semaphore */
    semaphore_t *sem = semaphores;
    
    while (sem != NULL) {
        if (sem->id == semid) {
            break;
        }
        
        sem = sem->next;
    }
    
    if (sem == NULL) {
        /* Semaphore doesn't exist */
        return -1;
    }
    
    /* Process the operations */
    sembuf_t *ops = (sembuf_t *)sops;
    
    for (size_t i = 0; i < nsops; i++) {
        /* Get the operation */
        sembuf_t *op = &ops[i];
        
        /* Check the semaphore number */
        if (op->sem_num != 0) {
            /* Only one semaphore per set is supported */
            return -1;
        }
        
        /* Process the operation */
        if (op->sem_op > 0) {
            /* Increment the semaphore */
            sem->value += op->sem_op;
            
            /* Wake up waiting processes */
            /* This would be implemented with a wait queue */
            /* For now, just decrement the waiters */
            if (sem->waiters > 0) {
                sem->waiters--;
            }
        } else if (op->sem_op < 0) {
            /* Decrement the semaphore */
            if (sem->value < -op->sem_op) {
                if (op->sem_flg & IPC_NOWAIT) {
                    /* Don't wait */
                    return -1;
                }
                
                /* Wait for the semaphore */
                sem->waiters++;
                
                /* This would be implemented with a wait queue */
                /* For now, just return an error */
                return -1;
            }
            
            sem->value += op->sem_op;
        } else {
            /* Wait for zero */
            if (sem->value != 0) {
                if (op->sem_flg & IPC_NOWAIT) {
                    /* Don't wait */
                    return -1;
                }
                
                /* Wait for the semaphore */
                sem->waiters++;
                
                /* This would be implemented with a wait queue */
                /* For now, just return an error */
                return -1;
            }
        }
    }
    
    return 0;
}

/* Control a semaphore */
int semctl(int semid, int semnum, int cmd, void *arg)
{
    /* Find the semaphore */
    semaphore_t *sem = semaphores;
    semaphore_t *prev = NULL;
    
    while (sem != NULL) {
        if (sem->id == semid) {
            break;
        }
        
        prev = sem;
        sem = sem->next;
    }
    
    if (sem == NULL) {
        /* Semaphore doesn't exist */
        return -1;
    }
    
    /* Check the semaphore number */
    if (semnum != 0) {
        /* Only one semaphore per set is supported */
        return -1;
    }
    
    /* Process the command */
    switch (cmd) {
        case IPC_RMID:
            /* Remove the semaphore */
            if (prev == NULL) {
                /* Semaphore is the first in the list */
                semaphores = sem->next;
            } else {
                /* Semaphore is not the first in the list */
                prev->next = sem->next;
            }
            
            /* Free the semaphore */
            kfree(sem);
            
            return 0;
        
        case IPC_SET:
            /* Set semaphore attributes */
            /* This would be implemented with proper attribute setting */
            /* For now, just return success */
            return 0;
        
        case IPC_STAT:
            /* Get semaphore attributes */
            /* This would be implemented with proper attribute getting */
            /* For now, just return success */
            return 0;
        
        case GETVAL:
            /* Get semaphore value */
            return sem->value;
        
        case SETVAL:
            /* Set semaphore value */
            sem->value = (int)arg;
            return 0;
        
        case GETPID:
            /* Get sempid */
            return sem->owner;
        
        case GETNCNT:
            /* Get semncnt */
            return sem->waiters;
        
        case GETZCNT:
            /* Get semzcnt */
            return 0;
        
        default:
            /* Unknown command */
            return -1;
    }
}

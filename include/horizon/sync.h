/**
 * sync.h - Horizon kernel synchronization definitions
 * 
 * This file contains definitions for synchronization primitives.
 */

#ifndef _HORIZON_SYNC_H
#define _HORIZON_SYNC_H

#include <horizon/types.h>
#include <horizon/list.h>

/* Forward declarations */
struct thread;

/* Mutex structure */
typedef struct mutex {
    volatile int locked;            /* Mutex is locked */
    struct thread *owner;           /* Thread that owns the mutex */
    list_head_t waiters;            /* Waiting threads */
} mutex_t;

/* Semaphore structure */
typedef struct sem {
    volatile int value;             /* Semaphore value */
    list_head_t waiters;            /* Waiting threads */
} sem_t;

/* Condition variable structure */
typedef struct cond {
    list_head_t waiters;            /* Waiting threads */
} cond_t;

/* Read-write lock structure */
typedef struct rwlock {
    volatile int readers;           /* Number of readers */
    volatile int writers;           /* Number of writers */
    struct thread *writer;          /* Thread that owns the write lock */
    list_head_t readers_waiters;    /* Waiting readers */
    list_head_t writers_waiters;    /* Waiting writers */
} rwlock_t;

/* Barrier structure */
typedef struct barrier {
    volatile int count;             /* Number of threads to wait for */
    volatile int waiting;           /* Number of waiting threads */
    list_head_t waiters;            /* Waiting threads */
} barrier_t;

/* Mutex functions */
int mutex_init(mutex_t *mutex);
int mutex_destroy(mutex_t *mutex);
int mutex_lock(mutex_t *mutex);
int mutex_trylock(mutex_t *mutex);
int mutex_unlock(mutex_t *mutex);

/* Semaphore functions */
int sem_init(sem_t *sem, u32 value);
int sem_destroy(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_post(sem_t *sem);

/* Condition variable functions */
int cond_init(cond_t *cond);
int cond_destroy(cond_t *cond);
int cond_wait(cond_t *cond, mutex_t *mutex);
int cond_timedwait(cond_t *cond, mutex_t *mutex, u64 timeout);
int cond_signal(cond_t *cond);
int cond_broadcast(cond_t *cond);

/* Read-write lock functions */
int rwlock_init(rwlock_t *rwlock);
int rwlock_destroy(rwlock_t *rwlock);
int rwlock_rdlock(rwlock_t *rwlock);
int rwlock_tryrdlock(rwlock_t *rwlock);
int rwlock_wrlock(rwlock_t *rwlock);
int rwlock_trywrlock(rwlock_t *rwlock);
int rwlock_unlock(rwlock_t *rwlock);

/* Barrier functions */
int barrier_init(barrier_t *barrier, u32 count);
int barrier_destroy(barrier_t *barrier);
int barrier_wait(barrier_t *barrier);

#endif /* _HORIZON_SYNC_H */

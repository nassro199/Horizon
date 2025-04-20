/**
 * rwlock.h - Horizon kernel read-write lock definitions
 * 
 * This file contains definitions for read-write locks.
 */

#ifndef _HORIZON_RWLOCK_H
#define _HORIZON_RWLOCK_H

#include <horizon/types.h>
#include <horizon/spinlock.h>

/* Debug read-write locks */
#define CONFIG_DEBUG_RWLOCK

/* Read-write lock structure */
typedef struct rwlock {
    spinlock_t lock;               /* Spinlock for protection */
    int readers;                   /* Number of readers */
    int writer;                    /* Writer flag */
#ifdef CONFIG_DEBUG_RWLOCK
    const char *name;              /* Lock name */
    const char *file;              /* Source file */
    int line;                      /* Source line */
    unsigned long owner;           /* Owner CPU */
    unsigned long owner_pc;        /* Owner PC */
    unsigned int held_count;       /* Number of times held */
    unsigned int contention_count; /* Number of contentions */
#endif
} rwlock_t;

/* Initialize a read-write lock */
#ifdef CONFIG_DEBUG_RWLOCK
#define RW_LOCK_INITIALIZER { SPIN_LOCK_INITIALIZER, 0, 0, NULL, NULL, 0, 0, 0, 0, 0 }
#define rwlock_init(lock, name) \
    do { \
        spin_lock_init(&(lock)->lock, name "_spinlock"); \
        (lock)->readers = 0; \
        (lock)->writer = 0; \
        (lock)->name = name; \
        (lock)->file = NULL; \
        (lock)->line = 0; \
        (lock)->owner = 0; \
        (lock)->owner_pc = 0; \
        (lock)->held_count = 0; \
        (lock)->contention_count = 0; \
    } while (0)
#else
#define RW_LOCK_INITIALIZER { SPIN_LOCK_INITIALIZER, 0, 0 }
#define rwlock_init(lock, name) \
    do { \
        spin_lock_init(&(lock)->lock, name "_spinlock"); \
        (lock)->readers = 0; \
        (lock)->writer = 0; \
    } while (0)
#endif

/* Acquire a read lock */
#ifdef CONFIG_DEBUG_RWLOCK
void __read_lock(rwlock_t *lock, const char *file, int line);
#define read_lock(lock) __read_lock(lock, __FILE__, __LINE__)
#else
void read_lock(rwlock_t *lock);
#endif

/* Try to acquire a read lock */
#ifdef CONFIG_DEBUG_RWLOCK
int __read_trylock(rwlock_t *lock, const char *file, int line);
#define read_trylock(lock) __read_trylock(lock, __FILE__, __LINE__)
#else
int read_trylock(rwlock_t *lock);
#endif

/* Release a read lock */
#ifdef CONFIG_DEBUG_RWLOCK
void __read_unlock(rwlock_t *lock, const char *file, int line);
#define read_unlock(lock) __read_unlock(lock, __FILE__, __LINE__)
#else
void read_unlock(rwlock_t *lock);
#endif

/* Acquire a write lock */
#ifdef CONFIG_DEBUG_RWLOCK
void __write_lock(rwlock_t *lock, const char *file, int line);
#define write_lock(lock) __write_lock(lock, __FILE__, __LINE__)
#else
void write_lock(rwlock_t *lock);
#endif

/* Try to acquire a write lock */
#ifdef CONFIG_DEBUG_RWLOCK
int __write_trylock(rwlock_t *lock, const char *file, int line);
#define write_trylock(lock) __write_trylock(lock, __FILE__, __LINE__)
#else
int write_trylock(rwlock_t *lock);
#endif

/* Release a write lock */
#ifdef CONFIG_DEBUG_RWLOCK
void __write_unlock(rwlock_t *lock, const char *file, int line);
#define write_unlock(lock) __write_unlock(lock, __FILE__, __LINE__)
#else
void write_unlock(rwlock_t *lock);
#endif

#endif /* _HORIZON_RWLOCK_H */

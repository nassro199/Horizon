/**
 * spinlock.h - Horizon kernel spinlock definitions
 * 
 * This file contains definitions for spinlocks.
 */

#ifndef _HORIZON_SPINLOCK_H
#define _HORIZON_SPINLOCK_H

#include <horizon/types.h>

/* Debug spinlocks */
#define CONFIG_DEBUG_SPINLOCK

/* Spinlock structure */
typedef struct spinlock {
    volatile unsigned int lock;    /* Lock value */
#ifdef CONFIG_DEBUG_SPINLOCK
    const char *name;              /* Lock name */
    const char *file;              /* Source file */
    int line;                      /* Source line */
    unsigned long owner;           /* Owner CPU */
    unsigned long owner_pc;        /* Owner PC */
    unsigned int held_count;       /* Number of times held */
    unsigned int contention_count; /* Number of contentions */
#endif
} spinlock_t;

/* Initialize a spinlock */
#ifdef CONFIG_DEBUG_SPINLOCK
#define SPIN_LOCK_INITIALIZER { 0, NULL, NULL, 0, 0, 0, 0, 0 }
#define spin_lock_init(lock, name) \
    do { \
        (lock)->lock = 0; \
        (lock)->name = name; \
        (lock)->file = NULL; \
        (lock)->line = 0; \
        (lock)->owner = 0; \
        (lock)->owner_pc = 0; \
        (lock)->held_count = 0; \
        (lock)->contention_count = 0; \
    } while (0)
#else
#define SPIN_LOCK_INITIALIZER { 0 }
#define spin_lock_init(lock, name) \
    do { \
        (lock)->lock = 0; \
    } while (0)
#endif

/* Acquire a spinlock */
#ifdef CONFIG_DEBUG_SPINLOCK
void __spin_lock(spinlock_t *lock, const char *file, int line);
#define spin_lock(lock) __spin_lock(lock, __FILE__, __LINE__)
#else
void spin_lock(spinlock_t *lock);
#endif

/* Try to acquire a spinlock */
#ifdef CONFIG_DEBUG_SPINLOCK
int __spin_trylock(spinlock_t *lock, const char *file, int line);
#define spin_trylock(lock) __spin_trylock(lock, __FILE__, __LINE__)
#else
int spin_trylock(spinlock_t *lock);
#endif

/* Release a spinlock */
#ifdef CONFIG_DEBUG_SPINLOCK
void __spin_unlock(spinlock_t *lock, const char *file, int line);
#define spin_unlock(lock) __spin_unlock(lock, __FILE__, __LINE__)
#else
void spin_unlock(spinlock_t *lock);
#endif

/* Check if a spinlock is locked */
int spin_is_locked(spinlock_t *lock);

#endif /* _HORIZON_SPINLOCK_H */

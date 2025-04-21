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

/* Raw spinlock structure */
typedef struct raw_spinlock {
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
} raw_spinlock_t;

/* Spinlock structure */
typedef struct spinlock {
    raw_spinlock_t raw_lock;       /* Raw spinlock */
} spinlock_t;

/* Initialize a raw spinlock */
#ifdef CONFIG_DEBUG_SPINLOCK
#define RAW_SPIN_LOCK_INITIALIZER { 0, NULL, NULL, 0, 0, 0, 0, 0 }
#define raw_spin_lock_init(lock) \
    do { \
        (lock)->lock = 0; \
        (lock)->name = "unknown"; \
        (lock)->file = NULL; \
        (lock)->line = 0; \
        (lock)->owner = 0; \
        (lock)->owner_pc = 0; \
        (lock)->held_count = 0; \
        (lock)->contention_count = 0; \
    } while (0)
#else
#define RAW_SPIN_LOCK_INITIALIZER { 0 }
#define raw_spin_lock_init(lock) \
    do { \
        (lock)->lock = 0; \
    } while (0)
#endif

/* Initialize a spinlock */
#define SPIN_LOCK_INITIALIZER { RAW_SPIN_LOCK_INITIALIZER }
#define spin_lock_init(lock) \
    do { \
        raw_spin_lock_init(&(lock)->raw_lock); \
    } while (0)

/* Acquire a raw spinlock */
#ifdef CONFIG_DEBUG_SPINLOCK
void __raw_spin_lock(raw_spinlock_t *lock, const char *file, int line);
#define raw_spin_lock(lock) __raw_spin_lock(lock, __FILE__, __LINE__)
#else
void raw_spin_lock(raw_spinlock_t *lock);
#endif

/* Try to acquire a raw spinlock */
#ifdef CONFIG_DEBUG_SPINLOCK
int __raw_spin_trylock(raw_spinlock_t *lock, const char *file, int line);
#define raw_spin_trylock(lock) __raw_spin_trylock(lock, __FILE__, __LINE__)
#else
int raw_spin_trylock(raw_spinlock_t *lock);
#endif

/* Release a raw spinlock */
#ifdef CONFIG_DEBUG_SPINLOCK
void __raw_spin_unlock(raw_spinlock_t *lock, const char *file, int line);
#define raw_spin_unlock(lock) __raw_spin_unlock(lock, __FILE__, __LINE__)
#else
void raw_spin_unlock(raw_spinlock_t *lock);
#endif

/* Acquire a spinlock */
#define spin_lock(lock) raw_spin_lock(&(lock)->raw_lock)

/* Try to acquire a spinlock */
#define spin_trylock(lock) raw_spin_trylock(&(lock)->raw_lock)

/* Release a spinlock */
#define spin_unlock(lock) raw_spin_unlock(&(lock)->raw_lock)

/* Check if a raw spinlock is locked */
int raw_spin_is_locked(raw_spinlock_t *lock);

/* Check if a spinlock is locked */
#define spin_is_locked(lock) raw_spin_is_locked(&(lock)->raw_lock)

#endif /* _HORIZON_SPINLOCK_H */

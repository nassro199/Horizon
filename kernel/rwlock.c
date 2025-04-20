/**
 * rwlock.c - Horizon kernel read-write lock implementation
 * 
 * This file contains the implementation of read-write locks.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/rwlock.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>

/**
 * Acquire a read lock with debugging
 * 
 * @param lock Read-write lock to acquire
 * @param file Source file
 * @param line Source line
 */
#ifdef CONFIG_DEBUG_RWLOCK
void __read_lock(rwlock_t *lock, const char *file, int line) {
    unsigned int contended = 0;
    
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Wait for any writers to finish */
    while (lock->writer) {
        contended = 1;
        spin_unlock(&lock->lock);
        __asm__ volatile("pause");
        spin_lock(&lock->lock);
    }
    
    /* Increment reader count */
    lock->readers++;
    
    /* Update debug information */
    lock->file = file;
    lock->line = line;
    lock->owner = 0; /* TODO: Get current CPU */
    lock->owner_pc = (unsigned long)__builtin_return_address(0);
    lock->held_count++;
    
    /* Update contention count */
    if (contended) {
        lock->contention_count++;
    }
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
}
#else
void read_lock(rwlock_t *lock) {
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Wait for any writers to finish */
    while (lock->writer) {
        spin_unlock(&lock->lock);
        __asm__ volatile("pause");
        spin_lock(&lock->lock);
    }
    
    /* Increment reader count */
    lock->readers++;
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
}
#endif

/**
 * Try to acquire a read lock with debugging
 * 
 * @param lock Read-write lock to acquire
 * @param file Source file
 * @param line Source line
 * @return 1 if lock acquired, 0 if not
 */
#ifdef CONFIG_DEBUG_RWLOCK
int __read_trylock(rwlock_t *lock, const char *file, int line) {
    int ret = 0;
    
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Check if there are any writers */
    if (!lock->writer) {
        /* Increment reader count */
        lock->readers++;
        
        /* Update debug information */
        lock->file = file;
        lock->line = line;
        lock->owner = 0; /* TODO: Get current CPU */
        lock->owner_pc = (unsigned long)__builtin_return_address(0);
        lock->held_count++;
        
        ret = 1;
    }
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
    
    return ret;
}
#else
int read_trylock(rwlock_t *lock) {
    int ret = 0;
    
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Check if there are any writers */
    if (!lock->writer) {
        /* Increment reader count */
        lock->readers++;
        ret = 1;
    }
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
    
    return ret;
}
#endif

/**
 * Release a read lock with debugging
 * 
 * @param lock Read-write lock to release
 * @param file Source file
 * @param line Source line
 */
#ifdef CONFIG_DEBUG_RWLOCK
void __read_unlock(rwlock_t *lock, const char *file, int line) {
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Check if the lock is not held */
    if (lock->readers == 0) {
        printk(KERN_WARNING "Read-write lock %s not read-locked at %s:%d\n",
               lock->name ? lock->name : "unknown",
               file, line);
        spin_unlock(&lock->lock);
        return;
    }
    
    /* Decrement reader count */
    lock->readers--;
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
}
#else
void read_unlock(rwlock_t *lock) {
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Decrement reader count */
    if (lock->readers > 0) {
        lock->readers--;
    }
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
}
#endif

/**
 * Acquire a write lock with debugging
 * 
 * @param lock Read-write lock to acquire
 * @param file Source file
 * @param line Source line
 */
#ifdef CONFIG_DEBUG_RWLOCK
void __write_lock(rwlock_t *lock, const char *file, int line) {
    unsigned int contended = 0;
    
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Wait for any readers or writers to finish */
    while (lock->readers > 0 || lock->writer) {
        contended = 1;
        spin_unlock(&lock->lock);
        __asm__ volatile("pause");
        spin_lock(&lock->lock);
    }
    
    /* Set writer flag */
    lock->writer = 1;
    
    /* Update debug information */
    lock->file = file;
    lock->line = line;
    lock->owner = 0; /* TODO: Get current CPU */
    lock->owner_pc = (unsigned long)__builtin_return_address(0);
    lock->held_count++;
    
    /* Update contention count */
    if (contended) {
        lock->contention_count++;
    }
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
}
#else
void write_lock(rwlock_t *lock) {
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Wait for any readers or writers to finish */
    while (lock->readers > 0 || lock->writer) {
        spin_unlock(&lock->lock);
        __asm__ volatile("pause");
        spin_lock(&lock->lock);
    }
    
    /* Set writer flag */
    lock->writer = 1;
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
}
#endif

/**
 * Try to acquire a write lock with debugging
 * 
 * @param lock Read-write lock to acquire
 * @param file Source file
 * @param line Source line
 * @return 1 if lock acquired, 0 if not
 */
#ifdef CONFIG_DEBUG_RWLOCK
int __write_trylock(rwlock_t *lock, const char *file, int line) {
    int ret = 0;
    
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Check if there are any readers or writers */
    if (lock->readers == 0 && !lock->writer) {
        /* Set writer flag */
        lock->writer = 1;
        
        /* Update debug information */
        lock->file = file;
        lock->line = line;
        lock->owner = 0; /* TODO: Get current CPU */
        lock->owner_pc = (unsigned long)__builtin_return_address(0);
        lock->held_count++;
        
        ret = 1;
    }
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
    
    return ret;
}
#else
int write_trylock(rwlock_t *lock) {
    int ret = 0;
    
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Check if there are any readers or writers */
    if (lock->readers == 0 && !lock->writer) {
        /* Set writer flag */
        lock->writer = 1;
        ret = 1;
    }
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
    
    return ret;
}
#endif

/**
 * Release a write lock with debugging
 * 
 * @param lock Read-write lock to release
 * @param file Source file
 * @param line Source line
 */
#ifdef CONFIG_DEBUG_RWLOCK
void __write_unlock(rwlock_t *lock, const char *file, int line) {
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Check if the lock is not held */
    if (!lock->writer) {
        printk(KERN_WARNING "Read-write lock %s not write-locked at %s:%d\n",
               lock->name ? lock->name : "unknown",
               file, line);
        spin_unlock(&lock->lock);
        return;
    }
    
    /* Clear writer flag */
    lock->writer = 0;
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
}
#else
void write_unlock(rwlock_t *lock) {
    /* Acquire the spinlock */
    spin_lock(&lock->lock);
    
    /* Clear writer flag */
    lock->writer = 0;
    
    /* Release the spinlock */
    spin_unlock(&lock->lock);
}
#endif

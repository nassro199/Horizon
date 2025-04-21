/**
 * spinlock.c - Horizon kernel spinlock implementation
 *
 * This file contains the implementation of spinlocks.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/spinlock.h>
#include <horizon/sched.h>
#include <horizon/printk.h>

/**
 * Acquire a raw spinlock with debugging
 *
 * @param lock Raw spinlock to acquire
 * @param file Source file
 * @param line Source line
 */
#ifdef CONFIG_DEBUG_SPINLOCK
void __raw_spin_lock(raw_spinlock_t *lock, const char *file, int line) {
    unsigned int contended = 0;
    unsigned long flags;

    /* Save interrupt flags and disable interrupts */
    __asm__ volatile("pushf; pop %0; cli" : "=r" (flags));

    /* Check if the lock is already held by us */
    if (lock->owner == 0 && lock->lock == 1) {
        printk(KERN_WARNING "Spinlock %s already locked at %s:%d\n",
               lock->name ? lock->name : "unknown",
               lock->file ? lock->file : "unknown",
               lock->line);
    }

    /* Try to acquire the lock */
    while (__sync_lock_test_and_set(&lock->lock, 1) != 0) {
        /* Lock is held by someone else */
        contended = 1;

        /* Pause to prevent excessive bus contention */
        __asm__ volatile("pause");
    }

    /* We now hold the lock */
    lock->file = file;
    lock->line = line;
    lock->owner = 0; /* TODO: Get current CPU */
    lock->owner_pc = (unsigned long)__builtin_return_address(0);
    lock->held_count++;

    /* Update contention count */
    if (contended) {
        lock->contention_count++;
    }

    /* Restore interrupt flags */
    __asm__ volatile("push %0; popf" : : "r" (flags));
}
#else
void raw_spin_lock(raw_spinlock_t *lock) {
    /* Disable interrupts */
    __asm__ volatile("cli");

    /* Try to acquire the lock */
    while (__sync_lock_test_and_set(&lock->lock, 1) != 0) {
        /* Pause to prevent excessive bus contention */
        __asm__ volatile("pause");
    }
}
#endif

/**
 * Try to acquire a raw spinlock with debugging
 *
 * @param lock Raw spinlock to acquire
 * @param file Source file
 * @param line Source line
 * @return 1 if lock acquired, 0 if not
 */
#ifdef CONFIG_DEBUG_SPINLOCK
int __raw_spin_trylock(raw_spinlock_t *lock, const char *file, int line) {
    unsigned long flags;

    /* Save interrupt flags and disable interrupts */
    __asm__ volatile("pushf; pop %0; cli" : "=r" (flags));

    /* Try to acquire the lock */
    if (__sync_lock_test_and_set(&lock->lock, 1) != 0) {
        /* Lock is held by someone else */
        __asm__ volatile("push %0; popf" : : "r" (flags));
        return 0;
    }

    /* We now hold the lock */
    lock->file = file;
    lock->line = line;
    lock->owner = 0; /* TODO: Get current CPU */
    lock->owner_pc = (unsigned long)__builtin_return_address(0);
    lock->held_count++;

    /* Restore interrupt flags */
    __asm__ volatile("push %0; popf" : : "r" (flags));

    return 1;
}
#else
int raw_spin_trylock(raw_spinlock_t *lock) {
    /* Disable interrupts */
    __asm__ volatile("cli");

    /* Try to acquire the lock */
    if (__sync_lock_test_and_set(&lock->lock, 1) != 0) {
        /* Lock is held by someone else */
        __asm__ volatile("sti");
        return 0;
    }

    return 1;
}
#endif

/**
 * Release a raw spinlock with debugging
 *
 * @param lock Raw spinlock to release
 * @param file Source file
 * @param line Source line
 */
#ifdef CONFIG_DEBUG_SPINLOCK
void __raw_spin_unlock(raw_spinlock_t *lock, const char *file, int line) {
    unsigned long flags;

    /* Save interrupt flags and disable interrupts */
    __asm__ volatile("pushf; pop %0; cli" : "=r" (flags));

    /* Check if the lock is not held */
    if (lock->lock == 0) {
        printk(KERN_WARNING "Spinlock %s not locked at %s:%d\n",
               lock->name ? lock->name : "unknown",
               file, line);
        __asm__ volatile("push %0; popf" : : "r" (flags));
        return;
    }

    /* Check if the lock is held by someone else */
    if (lock->owner != 0 /* TODO: Get current CPU */) {
        printk(KERN_WARNING "Spinlock %s held by CPU %lu at %s:%d, unlocking at %s:%d\n",
               lock->name ? lock->name : "unknown",
               lock->owner,
               lock->file ? lock->file : "unknown",
               lock->line,
               file, line);
    }

    /* Clear lock information */
    lock->file = NULL;
    lock->line = 0;
    lock->owner = 0;
    lock->owner_pc = 0;

    /* Release the lock */
    __sync_lock_release(&lock->lock);

    /* Restore interrupt flags */
    __asm__ volatile("push %0; popf" : : "r" (flags));
}
#else
void raw_spin_unlock(raw_spinlock_t *lock) {
    /* Release the lock */
    __sync_lock_release(&lock->lock);

    /* Enable interrupts */
    __asm__ volatile("sti");
}
#endif

/**
 * Check if a raw spinlock is locked
 *
 * @param lock Raw spinlock to check
 * @return 1 if locked, 0 if not
 */
int raw_spin_is_locked(raw_spinlock_t *lock) {
    return lock->lock != 0;
}

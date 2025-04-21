/**
 * thread_tls.c - Horizon kernel thread local storage implementation
 * 
 * This file contains the implementation of thread local storage (TLS).
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/task.h>
#include <horizon/thread.h>
#include <horizon/mm.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of TSD keys */
#define MAX_TSD_KEYS 128

/* TSD key structure */
typedef struct tsd_key {
    int allocated;                  /* Key is allocated */
    void (*destructor)(void *);     /* Destructor function */
} tsd_key_t;

/* TSD keys */
static tsd_key_t tsd_keys[MAX_TSD_KEYS];

/* TSD lock */
static spinlock_t tsd_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize thread local storage
 */
void thread_tls_init(void) {
    /* Initialize the TSD keys */
    for (int i = 0; i < MAX_TSD_KEYS; i++) {
        tsd_keys[i].allocated = 0;
        tsd_keys[i].destructor = NULL;
    }
    
    printk(KERN_INFO "THREAD_TLS: Initialized thread local storage\n");
}

/**
 * Create a TSD key
 * 
 * @param key Pointer to store the key
 * @param destructor Destructor function
 * @return 0 on success, negative error code on failure
 */
int thread_key_create(u32 *key, void (*destructor)(void *)) {
    /* Check parameters */
    if (key == NULL) {
        return -EINVAL;
    }
    
    /* Lock the TSD keys */
    spin_lock(&tsd_lock);
    
    /* Find a free key */
    for (u32 i = 0; i < MAX_TSD_KEYS; i++) {
        if (!tsd_keys[i].allocated) {
            /* Allocate the key */
            tsd_keys[i].allocated = 1;
            tsd_keys[i].destructor = destructor;
            
            /* Store the key */
            *key = i;
            
            /* Unlock the TSD keys */
            spin_unlock(&tsd_lock);
            
            return 0;
        }
    }
    
    /* Unlock the TSD keys */
    spin_unlock(&tsd_lock);
    
    /* No free keys */
    return -ENOMEM;
}

/**
 * Delete a TSD key
 * 
 * @param key Key to delete
 * @return 0 on success, negative error code on failure
 */
int thread_key_delete(u32 key) {
    /* Check parameters */
    if (key >= MAX_TSD_KEYS) {
        return -EINVAL;
    }
    
    /* Lock the TSD keys */
    spin_lock(&tsd_lock);
    
    /* Check if the key is allocated */
    if (!tsd_keys[key].allocated) {
        /* Key is not allocated */
        spin_unlock(&tsd_lock);
        return -EINVAL;
    }
    
    /* Free the key */
    tsd_keys[key].allocated = 0;
    tsd_keys[key].destructor = NULL;
    
    /* Unlock the TSD keys */
    spin_unlock(&tsd_lock);
    
    return 0;
}

/**
 * Set thread-specific data
 * 
 * @param thread Thread to set data for
 * @param key Key to set
 * @param value Value to set
 * @return 0 on success, negative error code on failure
 */
int thread_set_tsd(thread_t *thread, u32 key, void *value) {
    /* Check parameters */
    if (thread == NULL || key >= MAX_TSD_KEYS) {
        return -EINVAL;
    }
    
    /* Lock the TSD keys */
    spin_lock(&tsd_lock);
    
    /* Check if the key is allocated */
    if (!tsd_keys[key].allocated) {
        /* Key is not allocated */
        spin_unlock(&tsd_lock);
        return -EINVAL;
    }
    
    /* Check if the thread has TSD */
    if (thread->tsd == NULL) {
        /* Allocate TSD */
        thread->tsd = kmalloc(sizeof(void *) * MAX_TSD_KEYS, MEM_KERNEL | MEM_ZERO);
        
        if (thread->tsd == NULL) {
            /* Failed to allocate TSD */
            spin_unlock(&tsd_lock);
            return -ENOMEM;
        }
        
        /* Set the TSD count */
        thread->tsd_count = MAX_TSD_KEYS;
    }
    
    /* Set the TSD value */
    thread->tsd[key] = value;
    
    /* Unlock the TSD keys */
    spin_unlock(&tsd_lock);
    
    return 0;
}

/**
 * Get thread-specific data
 * 
 * @param thread Thread to get data for
 * @param key Key to get
 * @return Value, or NULL if not found
 */
void *thread_get_tsd(thread_t *thread, u32 key) {
    /* Check parameters */
    if (thread == NULL || key >= MAX_TSD_KEYS) {
        return NULL;
    }
    
    /* Lock the TSD keys */
    spin_lock(&tsd_lock);
    
    /* Check if the key is allocated */
    if (!tsd_keys[key].allocated) {
        /* Key is not allocated */
        spin_unlock(&tsd_lock);
        return NULL;
    }
    
    /* Check if the thread has TSD */
    if (thread->tsd == NULL) {
        /* No TSD */
        spin_unlock(&tsd_lock);
        return NULL;
    }
    
    /* Get the TSD value */
    void *value = thread->tsd[key];
    
    /* Unlock the TSD keys */
    spin_unlock(&tsd_lock);
    
    return value;
}

/**
 * Run TSD destructors for a thread
 * 
 * @param thread Thread to run destructors for
 */
void thread_run_tsd_destructors(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL || thread->tsd == NULL) {
        return;
    }
    
    /* Lock the TSD keys */
    spin_lock(&tsd_lock);
    
    /* Run the destructors */
    for (u32 i = 0; i < thread->tsd_count; i++) {
        if (tsd_keys[i].allocated && tsd_keys[i].destructor != NULL && thread->tsd[i] != NULL) {
            /* Run the destructor */
            tsd_keys[i].destructor(thread->tsd[i]);
            
            /* Clear the value */
            thread->tsd[i] = NULL;
        }
    }
    
    /* Free the TSD */
    kfree(thread->tsd);
    thread->tsd = NULL;
    thread->tsd_count = 0;
    
    /* Unlock the TSD keys */
    spin_unlock(&tsd_lock);
}

/**
 * thread.c - Horizon kernel thread implementation
 * 
 * This file contains the implementation of kernel threads.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/thread.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/sched.h>
#include <horizon/time.h>

/* Thread ID counter */
static u32 next_tid = 1;

/* Thread-specific data key counter */
static u32 next_tsd_key = 1;

/* Thread-specific data key destructors */
static void (*tsd_destructors[256])(void *);

/* Thread initialization */
void thread_init(void) {
    /* Initialize thread-specific data destructors */
    memset(tsd_destructors, 0, sizeof(tsd_destructors));
    
    /* Initialize the main kernel thread */
    task_struct_t *current_task = task_current();
    if (current_task != NULL) {
        thread_t *main_thread = kmalloc(sizeof(thread_t));
        if (main_thread != NULL) {
            /* Initialize thread structure */
            memset(main_thread, 0, sizeof(thread_t));
            main_thread->tid = next_tid++;
            main_thread->pid = current_task->pid;
            main_thread->state = THREAD_STATE_RUNNING;
            main_thread->flags = THREAD_KERNEL;
            main_thread->priority = THREAD_PRIO_NORMAL;
            main_thread->static_priority = THREAD_PRIO_NORMAL;
            main_thread->dynamic_priority = THREAD_PRIO_NORMAL;
            main_thread->policy = THREAD_SCHED_OTHER;
            main_thread->time_slice = 100; /* 100 ms */
            main_thread->start_time = get_timestamp();
            main_thread->task = current_task;
            
            /* Initialize thread lists */
            INIT_LIST_HEAD(&main_thread->thread_list);
            INIT_LIST_HEAD(&main_thread->process_threads);
            
            /* Add thread to task */
            current_task->main_thread = main_thread;
            current_task->thread_count = 1;
            list_add(&main_thread->process_threads, &current_task->threads);
        }
    }
}

/**
 * Create a new thread
 * 
 * @param start_routine Thread function
 * @param arg Thread argument
 * @param flags Thread flags
 * @return Thread structure, or NULL on failure
 */
thread_t *thread_create(void *(*start_routine)(void *), void *arg, u32 flags) {
    /* Check parameters */
    if (start_routine == NULL) {
        return NULL;
    }
    
    /* Allocate thread structure */
    thread_t *thread = kmalloc(sizeof(thread_t));
    if (thread == NULL) {
        return NULL;
    }
    
    /* Initialize thread structure */
    memset(thread, 0, sizeof(thread_t));
    thread->tid = next_tid++;
    thread->pid = task_current()->pid;
    thread->state = THREAD_STATE_READY;
    thread->flags = flags;
    thread->priority = THREAD_PRIO_NORMAL;
    thread->static_priority = THREAD_PRIO_NORMAL;
    thread->dynamic_priority = THREAD_PRIO_NORMAL;
    thread->policy = THREAD_SCHED_OTHER;
    thread->time_slice = 100; /* 100 ms */
    thread->start_time = get_timestamp();
    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->task = task_current();
    
    /* Allocate kernel stack */
    thread->kernel_stack = kmalloc(KERNEL_STACK_SIZE);
    if (thread->kernel_stack == NULL) {
        kfree(thread);
        return NULL;
    }
    
    /* Initialize thread context */
    thread->context = kmalloc(sizeof(thread_context_t));
    if (thread->context == NULL) {
        kfree(thread->kernel_stack);
        kfree(thread);
        return NULL;
    }
    
    /* Setup thread context */
    thread_context_t *context = (thread_context_t *)thread->context;
    memset(context, 0, sizeof(thread_context_t));
    
    /* Setup stack */
    u32 *stack = (u32 *)((u32)thread->kernel_stack + KERNEL_STACK_SIZE - 4);
    
    /* Setup initial stack frame */
    *--stack = (u32)arg;                /* Argument */
    *--stack = (u32)thread_exit;        /* Return address */
    *--stack = (u32)start_routine;      /* Entry point */
    *--stack = 0;                       /* EBP */
    *--stack = 0;                       /* EDI */
    *--stack = 0;                       /* ESI */
    *--stack = 0;                       /* EBX */
    
    /* Setup context */
    context->esp = (u32)stack;
    context->eip = (u32)thread_entry;
    context->eflags = 0x202;            /* IF = 1, IOPL = 0 */
    
    /* Initialize thread lists */
    INIT_LIST_HEAD(&thread->thread_list);
    INIT_LIST_HEAD(&thread->process_threads);
    
    /* Add thread to task */
    task_struct_t *task = task_current();
    task->thread_count++;
    list_add(&thread->process_threads, &task->threads);
    
    return thread;
}

/**
 * Start a thread
 * 
 * @param thread Thread to start
 * @return 0 on success, negative error code on failure
 */
int thread_start(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check thread state */
    if (thread->state != THREAD_STATE_READY) {
        return -EINVAL;
    }
    
    /* Add thread to scheduler */
    thread->state = THREAD_STATE_RUNNING;
    sched_add_thread(thread);
    
    return 0;
}

/**
 * Join a thread
 * 
 * @param thread Thread to join
 * @param retval Pointer to store return value
 * @return 0 on success, negative error code on failure
 */
int thread_join(thread_t *thread, void **retval) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if thread is joinable */
    if (!(thread->flags & THREAD_JOINABLE)) {
        return -EINVAL;
    }
    
    /* Check if thread is already dead */
    if (thread->state == THREAD_STATE_DEAD) {
        /* Get return value */
        if (retval != NULL) {
            *retval = thread->retval;
        }
        
        /* Free thread resources */
        kfree(thread->context);
        kfree(thread->kernel_stack);
        kfree(thread);
        
        return 0;
    }
    
    /* Wait for thread to exit */
    while (thread->state != THREAD_STATE_DEAD) {
        /* Sleep */
        thread_sleep(10);
    }
    
    /* Get return value */
    if (retval != NULL) {
        *retval = thread->retval;
    }
    
    /* Free thread resources */
    kfree(thread->context);
    kfree(thread->kernel_stack);
    kfree(thread);
    
    return 0;
}

/**
 * Detach a thread
 * 
 * @param thread Thread to detach
 * @return 0 on success, negative error code on failure
 */
int thread_detach(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if thread is joinable */
    if (!(thread->flags & THREAD_JOINABLE)) {
        return -EINVAL;
    }
    
    /* Detach thread */
    thread->flags &= ~THREAD_JOINABLE;
    thread->flags |= THREAD_DETACHED;
    
    return 0;
}

/**
 * Cancel a thread
 * 
 * @param thread Thread to cancel
 * @return 0 on success, negative error code on failure
 */
int thread_cancel(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if thread is already dead */
    if (thread->state == THREAD_STATE_DEAD) {
        return 0;
    }
    
    /* Cancel thread */
    thread->flags |= THREAD_EXITING;
    
    /* If thread is blocked, unblock it */
    if (thread->state == THREAD_STATE_BLOCKED) {
        thread->state = THREAD_STATE_READY;
        sched_add_thread(thread);
    }
    
    return 0;
}

/**
 * Exit a thread
 * 
 * @param retval Return value
 * @return Does not return
 */
int thread_exit(void *retval) {
    /* Get current thread */
    thread_t *thread = thread_self();
    
    /* Set return value */
    thread->retval = retval;
    
    /* Set thread state */
    thread->state = THREAD_STATE_DEAD;
    thread->flags |= THREAD_DEAD;
    
    /* Remove thread from scheduler */
    sched_remove_thread(thread);
    
    /* If thread is detached, free resources */
    if (thread->flags & THREAD_DETACHED) {
        /* Free thread resources */
        kfree(thread->context);
        kfree(thread->kernel_stack);
        kfree(thread);
    }
    
    /* Switch to next thread */
    sched_yield();
    
    /* Should never reach here */
    return 0;
}

/**
 * Get current thread
 * 
 * @return Current thread
 */
thread_t *thread_self(void) {
    /* Get current task */
    task_struct_t *task = task_current();
    
    /* Get current thread */
    return task_current_thread(task);
}

/**
 * Yield the CPU
 * 
 * @return 0 on success, negative error code on failure
 */
int thread_yield(void) {
    /* Yield the CPU */
    sched_yield();
    
    return 0;
}

/**
 * Sleep for a specified time
 * 
 * @param ms Time to sleep in milliseconds
 * @return 0 on success, negative error code on failure
 */
int thread_sleep(u64 ms) {
    /* Get current thread */
    thread_t *thread = thread_self();
    
    /* Set wakeup time */
    thread->wakeup_time = get_timestamp() + ms * 1000;
    
    /* Set thread state */
    thread->state = THREAD_STATE_SLEEPING;
    
    /* Remove thread from scheduler */
    sched_remove_thread(thread);
    
    /* Switch to next thread */
    sched_yield();
    
    return 0;
}

/**
 * Wake up a thread
 * 
 * @param thread Thread to wake up
 * @return 0 on success, negative error code on failure
 */
int thread_wakeup(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if thread is sleeping */
    if (thread->state != THREAD_STATE_SLEEPING) {
        return -EINVAL;
    }
    
    /* Wake up thread */
    thread->state = THREAD_STATE_READY;
    
    /* Add thread to scheduler */
    sched_add_thread(thread);
    
    return 0;
}

/**
 * Set thread priority
 * 
 * @param thread Thread to set priority
 * @param priority Priority
 * @return 0 on success, negative error code on failure
 */
int thread_set_priority(thread_t *thread, int priority) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check priority range */
    if (priority < THREAD_PRIO_IDLE || priority > THREAD_PRIO_REALTIME) {
        return -EINVAL;
    }
    
    /* Set priority */
    thread->priority = priority;
    thread->static_priority = priority;
    thread->dynamic_priority = priority;
    
    return 0;
}

/**
 * Get thread priority
 * 
 * @param thread Thread to get priority
 * @return Priority on success, negative error code on failure
 */
int thread_get_priority(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Get priority */
    return thread->priority;
}

/**
 * Set thread scheduling policy
 * 
 * @param thread Thread to set policy
 * @param policy Policy
 * @return 0 on success, negative error code on failure
 */
int thread_set_policy(thread_t *thread, u32 policy) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check policy range */
    if (policy > THREAD_SCHED_DEADLINE) {
        return -EINVAL;
    }
    
    /* Set policy */
    thread->policy = policy;
    
    return 0;
}

/**
 * Get thread scheduling policy
 * 
 * @param thread Thread to get policy
 * @return Policy on success, negative error code on failure
 */
u32 thread_get_policy(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Get policy */
    return thread->policy;
}

/**
 * Set thread CPU affinity
 * 
 * @param thread Thread to set affinity
 * @param cpu CPU
 * @return 0 on success, negative error code on failure
 */
int thread_set_affinity(thread_t *thread, u32 cpu) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check CPU range */
    if (cpu >= CONFIG_NR_CPUS) {
        return -EINVAL;
    }
    
    /* Set CPU */
    thread->cpu = cpu;
    
    return 0;
}

/**
 * Get thread CPU affinity
 * 
 * @param thread Thread to get affinity
 * @return CPU on success, negative error code on failure
 */
u32 thread_get_affinity(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Get CPU */
    return thread->cpu;
}

/**
 * Set thread name
 * 
 * @param thread Thread to set name
 * @param name Name
 * @return 0 on success, negative error code on failure
 */
int thread_set_name(thread_t *thread, const char *name) {
    /* Check parameters */
    if (thread == NULL || name == NULL) {
        return -EINVAL;
    }
    
    /* Set name */
    strncpy(thread->task->comm, name, 15);
    thread->task->comm[15] = '\0';
    
    return 0;
}

/**
 * Get thread name
 * 
 * @param thread Thread to get name
 * @param name Buffer to store name
 * @param size Buffer size
 * @return 0 on success, negative error code on failure
 */
int thread_get_name(thread_t *thread, char *name, size_t size) {
    /* Check parameters */
    if (thread == NULL || name == NULL || size == 0) {
        return -EINVAL;
    }
    
    /* Get name */
    strncpy(name, thread->task->comm, size - 1);
    name[size - 1] = '\0';
    
    return 0;
}

/**
 * Set thread-local storage
 * 
 * @param thread Thread to set TLS
 * @param tls TLS
 * @return 0 on success, negative error code on failure
 */
int thread_set_tls(thread_t *thread, void *tls) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Set TLS */
    thread->tls = tls;
    
    return 0;
}

/**
 * Get thread-local storage
 * 
 * @param thread Thread to get TLS
 * @return TLS on success, NULL on failure
 */
void *thread_get_tls(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return NULL;
    }
    
    /* Get TLS */
    return thread->tls;
}

/**
 * Set thread-specific data
 * 
 * @param thread Thread to set TSD
 * @param key Key
 * @param value Value
 * @return 0 on success, negative error code on failure
 */
int thread_set_tsd(thread_t *thread, u32 key, void *value) {
    /* Check parameters */
    if (thread == NULL || key == 0 || key >= 256) {
        return -EINVAL;
    }
    
    /* Allocate TSD array if needed */
    if (thread->tsd == NULL) {
        thread->tsd = kmalloc(256 * sizeof(void *));
        if (thread->tsd == NULL) {
            return -ENOMEM;
        }
        memset(thread->tsd, 0, 256 * sizeof(void *));
        thread->tsd_count = 0;
    }
    
    /* Set TSD */
    thread->tsd[key] = value;
    
    /* Update TSD count */
    if (value != NULL && thread->tsd_count < key) {
        thread->tsd_count = key;
    }
    
    return 0;
}

/**
 * Get thread-specific data
 * 
 * @param thread Thread to get TSD
 * @param key Key
 * @return Value on success, NULL on failure
 */
void *thread_get_tsd(thread_t *thread, u32 key) {
    /* Check parameters */
    if (thread == NULL || key == 0 || key >= 256 || thread->tsd == NULL) {
        return NULL;
    }
    
    /* Get TSD */
    return thread->tsd[key];
}

/**
 * Create a thread-specific data key
 * 
 * @param key Pointer to store key
 * @param destructor Destructor function
 * @return 0 on success, negative error code on failure
 */
int thread_key_create(u32 *key, void (*destructor)(void *)) {
    /* Check parameters */
    if (key == NULL) {
        return -EINVAL;
    }
    
    /* Allocate key */
    u32 new_key = next_tsd_key++;
    if (new_key >= 256) {
        return -ENOMEM;
    }
    
    /* Set key */
    *key = new_key;
    
    /* Set destructor */
    tsd_destructors[new_key] = destructor;
    
    return 0;
}

/**
 * Delete a thread-specific data key
 * 
 * @param key Key
 * @return 0 on success, negative error code on failure
 */
int thread_key_delete(u32 key) {
    /* Check parameters */
    if (key == 0 || key >= 256) {
        return -EINVAL;
    }
    
    /* Clear destructor */
    tsd_destructors[key] = NULL;
    
    return 0;
}

/**
 * Thread entry point
 * 
 * This function is called when a thread starts.
 */
void thread_entry(void) {
    /* Get current thread */
    thread_t *thread = thread_self();
    
    /* Call thread function */
    void *retval = thread->start_routine(thread->arg);
    
    /* Exit thread */
    thread_exit(retval);
}

/**
 * Run thread-specific data destructors
 * 
 * @param thread Thread
 */
void thread_run_destructors(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL || thread->tsd == NULL) {
        return;
    }
    
    /* Run destructors */
    for (u32 i = 1; i <= thread->tsd_count; i++) {
        if (thread->tsd[i] != NULL && tsd_destructors[i] != NULL) {
            tsd_destructors[i](thread->tsd[i]);
            thread->tsd[i] = NULL;
        }
    }
}

/**
 * Clean up thread resources
 * 
 * @param thread Thread
 */
void thread_cleanup(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }
    
    /* Run destructors */
    thread_run_destructors(thread);
    
    /* Free TSD */
    if (thread->tsd != NULL) {
        kfree(thread->tsd);
        thread->tsd = NULL;
    }
    
    /* Free context */
    if (thread->context != NULL) {
        kfree(thread->context);
        thread->context = NULL;
    }
    
    /* Free kernel stack */
    if (thread->kernel_stack != NULL) {
        kfree(thread->kernel_stack);
        thread->kernel_stack = NULL;
    }
    
    /* Remove from task */
    if (thread->task != NULL) {
        list_del(&thread->process_threads);
        thread->task->thread_count--;
    }
}

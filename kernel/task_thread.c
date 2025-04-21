/**
 * task_thread.c - Horizon kernel task thread implementation
 * 
 * This file contains the implementation of task thread functions.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/task.h>
#include <horizon/thread.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/sched.h>

/**
 * Create a thread in a task
 * 
 * @param task Task to create thread in
 * @param start_routine Thread function
 * @param arg Thread argument
 * @param flags Thread flags
 * @return Thread structure, or NULL on failure
 */
thread_t *task_create_thread(task_struct_t *task, void *(*start_routine)(void *), void *arg, u32 flags) {
    /* Check parameters */
    if (task == NULL || start_routine == NULL) {
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
    thread->pid = task->pid;
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
    thread->task = task;
    
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
    task->thread_count++;
    list_add(&thread->process_threads, &task->threads);
    
    return thread;
}

/**
 * Exit a thread in a task
 * 
 * @param task Task containing the thread
 * @param thread Thread to exit
 * @param retval Return value
 * @return 0 on success, negative error code on failure
 */
int task_exit_thread(task_struct_t *task, thread_t *thread, void *retval) {
    /* Check parameters */
    if (task == NULL || thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if thread belongs to task */
    if (thread->task != task) {
        return -EINVAL;
    }
    
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
    
    return 0;
}

/**
 * Join a thread in a task
 * 
 * @param task Task containing the thread
 * @param thread Thread to join
 * @param retval Pointer to store return value
 * @return 0 on success, negative error code on failure
 */
int task_join_thread(task_struct_t *task, thread_t *thread, void **retval) {
    /* Check parameters */
    if (task == NULL || thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if thread belongs to task */
    if (thread->task != task) {
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
 * Detach a thread in a task
 * 
 * @param task Task containing the thread
 * @param thread Thread to detach
 * @return 0 on success, negative error code on failure
 */
int task_detach_thread(task_struct_t *task, thread_t *thread) {
    /* Check parameters */
    if (task == NULL || thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if thread belongs to task */
    if (thread->task != task) {
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
 * Cancel a thread in a task
 * 
 * @param task Task containing the thread
 * @param thread Thread to cancel
 * @return 0 on success, negative error code on failure
 */
int task_cancel_thread(task_struct_t *task, thread_t *thread) {
    /* Check parameters */
    if (task == NULL || thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if thread belongs to task */
    if (thread->task != task) {
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
 * Get a thread in a task by thread ID
 * 
 * @param task Task containing the thread
 * @param tid Thread ID
 * @return Thread structure, or NULL on failure
 */
thread_t *task_get_thread(task_struct_t *task, u32 tid) {
    /* Check parameters */
    if (task == NULL) {
        return NULL;
    }
    
    /* Search for thread */
    thread_t *thread;
    list_for_each_entry(thread, &task->threads, process_threads) {
        if (thread->tid == tid) {
            return thread;
        }
    }
    
    return NULL;
}

/**
 * Get the current thread in a task
 * 
 * @param task Task
 * @return Current thread, or NULL on failure
 */
thread_t *task_current_thread(task_struct_t *task) {
    /* Check parameters */
    if (task == NULL) {
        return NULL;
    }
    
    /* Get current thread */
    return task->main_thread;
}

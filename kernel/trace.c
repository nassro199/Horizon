/**
 * trace.c - Horizon kernel tracing implementation
 * 
 * This file contains the implementation of kernel tracing.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/trace.h>
#include <horizon/spinlock.h>
#include <horizon/list.h>
#include <horizon/mm.h>
#include <horizon/errno.h>
#include <horizon/string.h>
#include <horizon/task.h>
#include <horizon/time.h>

/* Trace point list */
static LIST_HEAD(trace_points);

/* Trace lock */
static spinlock_t trace_lock = SPIN_LOCK_INITIALIZER;

/* Trace buffer */
static trace_buffer_t trace_buffer;

/* Trace enabled flags */
static u32 trace_enabled[32];

/**
 * Initialize tracing
 * 
 * @return 0 on success, negative error code on failure
 */
int trace_init(void) {
    int i;
    
    /* Initialize lock */
    spin_lock_init(&trace_lock, "trace");
    
    /* Initialize buffer */
    if (trace_buffer_init(&trace_buffer, 1024 * 1024) != 0) {
        return -ENOMEM;
    }
    
    /* Initialize enabled flags */
    for (i = 0; i < 32; i++) {
        trace_enabled[i] = 0;
    }
    
    return 0;
}

/**
 * Register a trace point
 * 
 * @param point Trace point to register
 * @return 0 on success, negative error code on failure
 */
int trace_register_point(trace_point_t *point) {
    /* Check parameters */
    if (point == NULL || point->name == NULL) {
        return -EINVAL;
    }
    
    /* Register point */
    spin_lock(&trace_lock);
    list_add(&point->list, &trace_points);
    spin_unlock(&trace_lock);
    
    return 0;
}

/**
 * Unregister a trace point
 * 
 * @param point Trace point to unregister
 * @return 0 on success, negative error code on failure
 */
int trace_unregister_point(trace_point_t *point) {
    /* Check parameters */
    if (point == NULL) {
        return -EINVAL;
    }
    
    /* Unregister point */
    spin_lock(&trace_lock);
    list_del(&point->list);
    spin_unlock(&trace_lock);
    
    return 0;
}

/**
 * Trace an event
 * 
 * @param type Event type
 * @param flags Event flags
 * @param data Event data
 * @param data_size Data size
 * @return 0 on success, negative error code on failure
 */
int trace_event(u32 type, u32 flags, void *data, u32 data_size) {
    trace_event_t *event;
    trace_point_t *point;
    u32 size;
    int ret = 0;
    
    /* Check if tracing is enabled for this type */
    if (!trace_is_enabled(type)) {
        return 0;
    }
    
    /* Calculate event size */
    size = sizeof(trace_event_t) + data_size;
    
    /* Allocate event */
    event = kmalloc(size, MEM_KERNEL);
    if (event == NULL) {
        return -ENOMEM;
    }
    
    /* Initialize event */
    event->type = type;
    event->flags = flags;
    event->timestamp = get_timestamp();
    event->cpu = get_cpu_id();
    event->pid = current ? current->pid : 0;
    event->tid = current ? current->tid : 0;
    if (current && current->comm) {
        strncpy(event->comm, current->comm, 15);
        event->comm[15] = '\0';
    } else {
        event->comm[0] = '\0';
    }
    event->caller = __builtin_return_address(0);
    event->backtrace_size = 0;
    event->data_size = data_size;
    
    /* Copy data */
    if (data != NULL && data_size > 0) {
        memcpy(event->data, data, data_size);
    }
    
    /* Call trace points */
    spin_lock(&trace_lock);
    list_for_each_entry(point, &trace_points, list) {
        if (point->type == type && point->handler != NULL) {
            ret = point->handler(event, point->data);
            if (ret != 0) {
                break;
            }
        }
    }
    spin_unlock(&trace_lock);
    
    /* Write to buffer */
    if (ret == 0) {
        trace_buffer_write(&trace_buffer, event, size);
    }
    
    /* Free event */
    kfree(event);
    
    return ret;
}

/**
 * Trace a system call entry
 * 
 * @param syscall System call number
 * @param arg1 Argument 1
 * @param arg2 Argument 2
 * @param arg3 Argument 3
 * @param arg4 Argument 4
 * @param arg5 Argument 5
 * @param arg6 Argument 6
 * @return 0 on success, negative error code on failure
 */
int trace_syscall_enter(u32 syscall, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5, u32 arg6) {
    struct {
        u32 syscall;
        u32 args[6];
    } data;
    
    /* Initialize data */
    data.syscall = syscall;
    data.args[0] = arg1;
    data.args[1] = arg2;
    data.args[2] = arg3;
    data.args[3] = arg4;
    data.args[4] = arg5;
    data.args[5] = arg6;
    
    /* Trace event */
    return trace_event(TRACE_EVENT_SYSCALL, TRACE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Trace a system call exit
 * 
 * @param syscall System call number
 * @param ret Return value
 * @return 0 on success, negative error code on failure
 */
int trace_syscall_exit(u32 syscall, u32 ret) {
    struct {
        u32 syscall;
        u32 ret;
    } data;
    
    /* Initialize data */
    data.syscall = syscall;
    data.ret = ret;
    
    /* Trace event */
    return trace_event(TRACE_EVENT_SYSCALL, TRACE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Trace an interrupt
 * 
 * @param irq IRQ number
 * @return 0 on success, negative error code on failure
 */
int trace_interrupt(u32 irq) {
    /* Trace event */
    return trace_event(TRACE_EVENT_INTERRUPT, TRACE_FLAG_ALL, &irq, sizeof(irq));
}

/**
 * Trace a schedule
 * 
 * @param prev Previous task
 * @param next Next task
 * @return 0 on success, negative error code on failure
 */
int trace_schedule(struct task_struct *prev, struct task_struct *next) {
    struct {
        u32 prev_pid;
        u32 next_pid;
        char prev_comm[16];
        char next_comm[16];
    } data;
    
    /* Initialize data */
    data.prev_pid = prev ? prev->pid : 0;
    data.next_pid = next ? next->pid : 0;
    if (prev && prev->comm) {
        strncpy(data.prev_comm, prev->comm, 15);
        data.prev_comm[15] = '\0';
    } else {
        data.prev_comm[0] = '\0';
    }
    if (next && next->comm) {
        strncpy(data.next_comm, next->comm, 15);
        data.next_comm[15] = '\0';
    } else {
        data.next_comm[0] = '\0';
    }
    
    /* Trace event */
    return trace_event(TRACE_EVENT_SCHEDULE, TRACE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Trace a page fault
 * 
 * @param addr Fault address
 * @param error Error code
 * @return 0 on success, negative error code on failure
 */
int trace_pagefault(void *addr, u32 error) {
    struct {
        void *addr;
        u32 error;
    } data;
    
    /* Initialize data */
    data.addr = addr;
    data.error = error;
    
    /* Trace event */
    return trace_event(TRACE_EVENT_PAGEFAULT, TRACE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Trace a kernel memory allocation
 * 
 * @param ptr Allocated pointer
 * @param size Allocation size
 * @param flags Allocation flags
 * @return 0 on success, negative error code on failure
 */
int trace_kmalloc(void *ptr, u32 size, u32 flags) {
    struct {
        void *ptr;
        u32 size;
        u32 flags;
    } data;
    
    /* Initialize data */
    data.ptr = ptr;
    data.size = size;
    data.flags = flags;
    
    /* Trace event */
    return trace_event(TRACE_EVENT_KMALLOC, TRACE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Trace a kernel memory free
 * 
 * @param ptr Freed pointer
 * @return 0 on success, negative error code on failure
 */
int trace_kfree(void *ptr) {
    /* Trace event */
    return trace_event(TRACE_EVENT_KFREE, TRACE_FLAG_ALL, &ptr, sizeof(ptr));
}

/**
 * Enable tracing for a type
 * 
 * @param type Event type
 * @return 0 on success, negative error code on failure
 */
int trace_enable(u32 type) {
    /* Check parameters */
    if (type >= 32) {
        return -EINVAL;
    }
    
    /* Enable tracing */
    trace_enabled[type] = 1;
    
    return 0;
}

/**
 * Disable tracing for a type
 * 
 * @param type Event type
 * @return 0 on success, negative error code on failure
 */
int trace_disable(u32 type) {
    /* Check parameters */
    if (type >= 32) {
        return -EINVAL;
    }
    
    /* Disable tracing */
    trace_enabled[type] = 0;
    
    return 0;
}

/**
 * Check if tracing is enabled for a type
 * 
 * @param type Event type
 * @return 1 if enabled, 0 if disabled
 */
int trace_is_enabled(u32 type) {
    /* Check parameters */
    if (type >= 32) {
        return 0;
    }
    
    /* Check if enabled */
    return trace_enabled[type] != 0;
}

/**
 * Set trace flags for a type
 * 
 * @param type Event type
 * @param flags Event flags
 * @return 0 on success, negative error code on failure
 */
int trace_set_flags(u32 type, u32 flags) {
    trace_point_t *point;
    
    /* Check parameters */
    if (type >= 32) {
        return -EINVAL;
    }
    
    /* Set flags */
    spin_lock(&trace_lock);
    list_for_each_entry(point, &trace_points, list) {
        if (point->type == type) {
            point->flags = flags;
        }
    }
    spin_unlock(&trace_lock);
    
    return 0;
}

/**
 * Get trace flags for a type
 * 
 * @param type Event type
 * @return Flags, or 0 if not found
 */
u32 trace_get_flags(u32 type) {
    trace_point_t *point;
    u32 flags = 0;
    
    /* Check parameters */
    if (type >= 32) {
        return 0;
    }
    
    /* Get flags */
    spin_lock(&trace_lock);
    list_for_each_entry(point, &trace_points, list) {
        if (point->type == type) {
            flags = point->flags;
            break;
        }
    }
    spin_unlock(&trace_lock);
    
    return flags;
}

/**
 * Initialize a trace buffer
 * 
 * @param buffer Buffer to initialize
 * @param size Buffer size
 * @return 0 on success, negative error code on failure
 */
int trace_buffer_init(trace_buffer_t *buffer, u32 size) {
    /* Check parameters */
    if (buffer == NULL || size == 0) {
        return -EINVAL;
    }
    
    /* Allocate buffer */
    buffer->buffer = kmalloc(size, MEM_KERNEL | MEM_ZERO);
    if (buffer->buffer == NULL) {
        return -ENOMEM;
    }
    
    /* Initialize buffer */
    buffer->size = size;
    buffer->head = 0;
    buffer->tail = 0;
    spin_lock_init(&buffer->lock, "trace_buffer");
    
    return 0;
}

/**
 * Free a trace buffer
 * 
 * @param buffer Buffer to free
 * @return 0 on success, negative error code on failure
 */
int trace_buffer_free(trace_buffer_t *buffer) {
    /* Check parameters */
    if (buffer == NULL) {
        return -EINVAL;
    }
    
    /* Free buffer */
    if (buffer->buffer != NULL) {
        kfree(buffer->buffer);
        buffer->buffer = NULL;
    }
    
    return 0;
}

/**
 * Write to a trace buffer
 * 
 * @param buffer Buffer to write to
 * @param data Data to write
 * @param size Data size
 * @return Number of bytes written, or negative error code on failure
 */
int trace_buffer_write(trace_buffer_t *buffer, void *data, u32 size) {
    u32 space;
    u32 first_chunk;
    u32 second_chunk;
    
    /* Check parameters */
    if (buffer == NULL || data == NULL || size == 0) {
        return -EINVAL;
    }
    
    /* Check if buffer is full */
    spin_lock(&buffer->lock);
    space = trace_buffer_free_space(buffer);
    if (space < size) {
        spin_unlock(&buffer->lock);
        return 0;
    }
    
    /* Write data */
    if (buffer->head + size <= buffer->size) {
        /* Single chunk */
        memcpy(buffer->buffer + buffer->head, data, size);
        buffer->head += size;
        if (buffer->head == buffer->size) {
            buffer->head = 0;
        }
    } else {
        /* Two chunks */
        first_chunk = buffer->size - buffer->head;
        second_chunk = size - first_chunk;
        memcpy(buffer->buffer + buffer->head, data, first_chunk);
        memcpy(buffer->buffer, (u8 *)data + first_chunk, second_chunk);
        buffer->head = second_chunk;
    }
    
    spin_unlock(&buffer->lock);
    
    return size;
}

/**
 * Read from a trace buffer
 * 
 * @param buffer Buffer to read from
 * @param data Data buffer
 * @param size Data size
 * @return Number of bytes read, or negative error code on failure
 */
int trace_buffer_read(trace_buffer_t *buffer, void *data, u32 size) {
    u32 available;
    u32 first_chunk;
    u32 second_chunk;
    
    /* Check parameters */
    if (buffer == NULL || data == NULL || size == 0) {
        return -EINVAL;
    }
    
    /* Check if buffer is empty */
    spin_lock(&buffer->lock);
    available = trace_buffer_used(buffer);
    if (available == 0) {
        spin_unlock(&buffer->lock);
        return 0;
    }
    
    /* Adjust size */
    if (size > available) {
        size = available;
    }
    
    /* Read data */
    if (buffer->tail + size <= buffer->size) {
        /* Single chunk */
        memcpy(data, buffer->buffer + buffer->tail, size);
        buffer->tail += size;
        if (buffer->tail == buffer->size) {
            buffer->tail = 0;
        }
    } else {
        /* Two chunks */
        first_chunk = buffer->size - buffer->tail;
        second_chunk = size - first_chunk;
        memcpy(data, buffer->buffer + buffer->tail, first_chunk);
        memcpy((u8 *)data + first_chunk, buffer->buffer, second_chunk);
        buffer->tail = second_chunk;
    }
    
    spin_unlock(&buffer->lock);
    
    return size;
}

/**
 * Clear a trace buffer
 * 
 * @param buffer Buffer to clear
 * @return 0 on success, negative error code on failure
 */
int trace_buffer_clear(trace_buffer_t *buffer) {
    /* Check parameters */
    if (buffer == NULL) {
        return -EINVAL;
    }
    
    /* Clear buffer */
    spin_lock(&buffer->lock);
    buffer->head = 0;
    buffer->tail = 0;
    spin_unlock(&buffer->lock);
    
    return 0;
}

/**
 * Get used space in a trace buffer
 * 
 * @param buffer Buffer to check
 * @return Used space
 */
u32 trace_buffer_used(trace_buffer_t *buffer) {
    u32 used;
    
    /* Check parameters */
    if (buffer == NULL) {
        return 0;
    }
    
    /* Calculate used space */
    if (buffer->head >= buffer->tail) {
        used = buffer->head - buffer->tail;
    } else {
        used = buffer->size - buffer->tail + buffer->head;
    }
    
    return used;
}

/**
 * Get free space in a trace buffer
 * 
 * @param buffer Buffer to check
 * @return Free space
 */
u32 trace_buffer_free_space(trace_buffer_t *buffer) {
    u32 used;
    
    /* Check parameters */
    if (buffer == NULL) {
        return 0;
    }
    
    /* Calculate free space */
    used = trace_buffer_used(buffer);
    
    return buffer->size - used - 1;
}

/**
 * Check if a trace buffer is empty
 * 
 * @param buffer Buffer to check
 * @return 1 if empty, 0 if not
 */
int trace_buffer_is_empty(trace_buffer_t *buffer) {
    /* Check parameters */
    if (buffer == NULL) {
        return 1;
    }
    
    /* Check if empty */
    return buffer->head == buffer->tail;
}

/**
 * Check if a trace buffer is full
 * 
 * @param buffer Buffer to check
 * @return 1 if full, 0 if not
 */
int trace_buffer_is_full(trace_buffer_t *buffer) {
    u32 next_head;
    
    /* Check parameters */
    if (buffer == NULL) {
        return 0;
    }
    
    /* Calculate next head */
    next_head = buffer->head + 1;
    if (next_head == buffer->size) {
        next_head = 0;
    }
    
    /* Check if full */
    return next_head == buffer->tail;
}

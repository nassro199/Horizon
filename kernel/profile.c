/**
 * profile.c - Horizon kernel profiling implementation
 * 
 * This file contains the implementation of kernel profiling.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/profile.h>
#include <horizon/spinlock.h>
#include <horizon/list.h>
#include <horizon/mm.h>
#include <horizon/errno.h>
#include <horizon/string.h>
#include <horizon/task.h>
#include <horizon/time.h>

/* Profile point list */
static LIST_HEAD(profile_points);

/* Profile function list */
static LIST_HEAD(profile_functions);

/* Profile lock */
static spinlock_t profile_lock = SPIN_LOCK_INITIALIZER;

/* Profile buffer */
static profile_buffer_t profile_buffer;

/* Profile enabled flags */
static u32 profile_enabled[32];

/* Profile running flag */
static int profile_running = 0;

/**
 * Initialize profiling
 * 
 * @return 0 on success, negative error code on failure
 */
int profile_init(void) {
    int i;
    
    /* Initialize lock */
    spin_lock_init(&profile_lock, "profile");
    
    /* Initialize buffer */
    if (profile_buffer_init(&profile_buffer, 1024 * 1024) != 0) {
        return -ENOMEM;
    }
    
    /* Initialize enabled flags */
    for (i = 0; i < 32; i++) {
        profile_enabled[i] = 0;
    }
    
    return 0;
}

/**
 * Register a profile point
 * 
 * @param point Profile point to register
 * @return 0 on success, negative error code on failure
 */
int profile_register_point(profile_point_t *point) {
    /* Check parameters */
    if (point == NULL || point->name == NULL) {
        return -EINVAL;
    }
    
    /* Register point */
    spin_lock(&profile_lock);
    list_add(&point->list, &profile_points);
    spin_unlock(&profile_lock);
    
    return 0;
}

/**
 * Unregister a profile point
 * 
 * @param point Profile point to unregister
 * @return 0 on success, negative error code on failure
 */
int profile_unregister_point(profile_point_t *point) {
    /* Check parameters */
    if (point == NULL) {
        return -EINVAL;
    }
    
    /* Unregister point */
    spin_lock(&profile_lock);
    list_del(&point->list);
    spin_unlock(&profile_lock);
    
    return 0;
}

/**
 * Profile a sample
 * 
 * @param type Event type
 * @param flags Event flags
 * @param data Event data
 * @param data_size Data size
 * @return 0 on success, negative error code on failure
 */
int profile_sample(u32 type, u32 flags, void *data, u32 data_size) {
    profile_sample_t *sample;
    profile_point_t *point;
    u32 size;
    int ret = 0;
    
    /* Check if profiling is enabled for this type */
    if (!profile_is_enabled(type) || !profile_running) {
        return 0;
    }
    
    /* Calculate sample size */
    size = sizeof(profile_sample_t) + data_size;
    
    /* Allocate sample */
    sample = kmalloc(size, MEM_KERNEL);
    if (sample == NULL) {
        return -ENOMEM;
    }
    
    /* Initialize sample */
    sample->type = type;
    sample->flags = flags;
    sample->timestamp = get_timestamp();
    sample->cpu = get_cpu_id();
    sample->pid = current ? current->pid : 0;
    sample->tid = current ? current->tid : 0;
    if (current && current->comm) {
        strncpy(sample->comm, current->comm, 15);
        sample->comm[15] = '\0';
    } else {
        sample->comm[0] = '\0';
    }
    sample->caller = __builtin_return_address(0);
    sample->backtrace_size = 0;
    sample->data_size = data_size;
    
    /* Copy data */
    if (data != NULL && data_size > 0) {
        memcpy(sample->data, data, data_size);
    }
    
    /* Call profile points */
    spin_lock(&profile_lock);
    list_for_each_entry(point, &profile_points, list) {
        if (point->type == type && point->handler != NULL) {
            ret = point->handler(sample, point->data);
            if (ret != 0) {
                break;
            }
        }
    }
    spin_unlock(&profile_lock);
    
    /* Write to buffer */
    if (ret == 0) {
        profile_buffer_write(&profile_buffer, sample, size);
    }
    
    /* Free sample */
    kfree(sample);
    
    return ret;
}

/**
 * Profile a function entry
 * 
 * @param addr Function address
 * @param name Function name
 * @return 0 on success, negative error code on failure
 */
int profile_function_enter(void *addr, const char *name) {
    profile_function_t *func;
    int found = 0;
    
    /* Check if profiling is enabled */
    if (!profile_is_enabled(PROFILE_EVENT_FUNCTION) || !profile_running) {
        return 0;
    }
    
    /* Find or create function */
    spin_lock(&profile_lock);
    list_for_each_entry(func, &profile_functions, list) {
        if (func->addr == addr) {
            /* Found function */
            func->calls++;
            found = 1;
            break;
        }
    }
    
    /* Create new function if not found */
    if (!found) {
        func = kmalloc(sizeof(profile_function_t), MEM_KERNEL);
        if (func == NULL) {
            spin_unlock(&profile_lock);
            return -ENOMEM;
        }
        
        /* Initialize function */
        func->name = name;
        func->addr = addr;
        func->calls = 1;
        func->total_time = 0;
        func->min_time = 0xFFFFFFFFFFFFFFFF;
        func->max_time = 0;
        
        /* Add to list */
        list_add(&func->list, &profile_functions);
    }
    spin_unlock(&profile_lock);
    
    return 0;
}

/**
 * Profile a function exit
 * 
 * @param addr Function address
 * @param time Time spent in function
 * @return 0 on success, negative error code on failure
 */
int profile_function_exit(void *addr, u64 time) {
    profile_function_t *func;
    
    /* Check if profiling is enabled */
    if (!profile_is_enabled(PROFILE_EVENT_FUNCTION) || !profile_running) {
        return 0;
    }
    
    /* Find function */
    spin_lock(&profile_lock);
    list_for_each_entry(func, &profile_functions, list) {
        if (func->addr == addr) {
            /* Update function statistics */
            func->total_time += time;
            if (time < func->min_time) {
                func->min_time = time;
            }
            if (time > func->max_time) {
                func->max_time = time;
            }
            break;
        }
    }
    spin_unlock(&profile_lock);
    
    return 0;
}

/**
 * Profile a system call entry
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
int profile_syscall_enter(u32 syscall, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5, u32 arg6) {
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
    
    /* Profile sample */
    return profile_sample(PROFILE_EVENT_SYSCALL, PROFILE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Profile a system call exit
 * 
 * @param syscall System call number
 * @param ret Return value
 * @param time Time spent in system call
 * @return 0 on success, negative error code on failure
 */
int profile_syscall_exit(u32 syscall, u32 ret, u64 time) {
    struct {
        u32 syscall;
        u32 ret;
        u64 time;
    } data;
    
    /* Initialize data */
    data.syscall = syscall;
    data.ret = ret;
    data.time = time;
    
    /* Profile sample */
    return profile_sample(PROFILE_EVENT_SYSCALL, PROFILE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Profile an interrupt
 * 
 * @param irq IRQ number
 * @param time Time spent in interrupt
 * @return 0 on success, negative error code on failure
 */
int profile_interrupt(u32 irq, u64 time) {
    struct {
        u32 irq;
        u64 time;
    } data;
    
    /* Initialize data */
    data.irq = irq;
    data.time = time;
    
    /* Profile sample */
    return profile_sample(PROFILE_EVENT_INTERRUPT, PROFILE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Profile a schedule
 * 
 * @param prev Previous task
 * @param next Next task
 * @param time Time spent in scheduler
 * @return 0 on success, negative error code on failure
 */
int profile_schedule(struct task_struct *prev, struct task_struct *next, u64 time) {
    struct {
        u32 prev_pid;
        u32 next_pid;
        char prev_comm[16];
        char next_comm[16];
        u64 time;
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
    data.time = time;
    
    /* Profile sample */
    return profile_sample(PROFILE_EVENT_SCHEDULE, PROFILE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Profile a memory operation
 * 
 * @param ptr Memory pointer
 * @param size Memory size
 * @param flags Memory flags
 * @return 0 on success, negative error code on failure
 */
int profile_memory(void *ptr, u32 size, u32 flags) {
    struct {
        void *ptr;
        u32 size;
        u32 flags;
    } data;
    
    /* Initialize data */
    data.ptr = ptr;
    data.size = size;
    data.flags = flags;
    
    /* Profile sample */
    return profile_sample(PROFILE_EVENT_MEMORY, PROFILE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Profile an I/O operation
 * 
 * @param fd File descriptor
 * @param op Operation
 * @param size I/O size
 * @param time Time spent in I/O
 * @return 0 on success, negative error code on failure
 */
int profile_io(u32 fd, u32 op, u32 size, u64 time) {
    struct {
        u32 fd;
        u32 op;
        u32 size;
        u64 time;
    } data;
    
    /* Initialize data */
    data.fd = fd;
    data.op = op;
    data.size = size;
    data.time = time;
    
    /* Profile sample */
    return profile_sample(PROFILE_EVENT_IO, PROFILE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Profile a network operation
 * 
 * @param sock Socket
 * @param op Operation
 * @param size Network size
 * @param time Time spent in network
 * @return 0 on success, negative error code on failure
 */
int profile_network(u32 sock, u32 op, u32 size, u64 time) {
    struct {
        u32 sock;
        u32 op;
        u32 size;
        u64 time;
    } data;
    
    /* Initialize data */
    data.sock = sock;
    data.op = op;
    data.size = size;
    data.time = time;
    
    /* Profile sample */
    return profile_sample(PROFILE_EVENT_NETWORK, PROFILE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Profile a disk operation
 * 
 * @param dev Device
 * @param op Operation
 * @param sector Sector
 * @param count Sector count
 * @param time Time spent in disk
 * @return 0 on success, negative error code on failure
 */
int profile_disk(u32 dev, u32 op, u64 sector, u32 count, u64 time) {
    struct {
        u32 dev;
        u32 op;
        u64 sector;
        u32 count;
        u64 time;
    } data;
    
    /* Initialize data */
    data.dev = dev;
    data.op = op;
    data.sector = sector;
    data.count = count;
    data.time = time;
    
    /* Profile sample */
    return profile_sample(PROFILE_EVENT_DISK, PROFILE_FLAG_ALL, &data, sizeof(data));
}

/**
 * Profile a user-defined event
 * 
 * @param type User event type
 * @param data User data
 * @param data_size User data size
 * @return 0 on success, negative error code on failure
 */
int profile_user(u32 type, void *data, u32 data_size) {
    /* Profile sample */
    return profile_sample(PROFILE_EVENT_USER, PROFILE_FLAG_ALL, data, data_size);
}

/**
 * Enable profiling for a type
 * 
 * @param type Event type
 * @return 0 on success, negative error code on failure
 */
int profile_enable(u32 type) {
    /* Check parameters */
    if (type >= 32) {
        return -EINVAL;
    }
    
    /* Enable profiling */
    profile_enabled[type] = 1;
    
    return 0;
}

/**
 * Disable profiling for a type
 * 
 * @param type Event type
 * @return 0 on success, negative error code on failure
 */
int profile_disable(u32 type) {
    /* Check parameters */
    if (type >= 32) {
        return -EINVAL;
    }
    
    /* Disable profiling */
    profile_enabled[type] = 0;
    
    return 0;
}

/**
 * Check if profiling is enabled for a type
 * 
 * @param type Event type
 * @return 1 if enabled, 0 if disabled
 */
int profile_is_enabled(u32 type) {
    /* Check parameters */
    if (type >= 32) {
        return 0;
    }
    
    /* Check if enabled */
    return profile_enabled[type] != 0;
}

/**
 * Set profile flags for a type
 * 
 * @param type Event type
 * @param flags Event flags
 * @return 0 on success, negative error code on failure
 */
int profile_set_flags(u32 type, u32 flags) {
    profile_point_t *point;
    
    /* Check parameters */
    if (type >= 32) {
        return -EINVAL;
    }
    
    /* Set flags */
    spin_lock(&profile_lock);
    list_for_each_entry(point, &profile_points, list) {
        if (point->type == type) {
            point->flags = flags;
        }
    }
    spin_unlock(&profile_lock);
    
    return 0;
}

/**
 * Get profile flags for a type
 * 
 * @param type Event type
 * @return Flags, or 0 if not found
 */
u32 profile_get_flags(u32 type) {
    profile_point_t *point;
    u32 flags = 0;
    
    /* Check parameters */
    if (type >= 32) {
        return 0;
    }
    
    /* Get flags */
    spin_lock(&profile_lock);
    list_for_each_entry(point, &profile_points, list) {
        if (point->type == type) {
            flags = point->flags;
            break;
        }
    }
    spin_unlock(&profile_lock);
    
    return flags;
}

/**
 * Initialize a profile buffer
 * 
 * @param buffer Buffer to initialize
 * @param size Buffer size
 * @return 0 on success, negative error code on failure
 */
int profile_buffer_init(profile_buffer_t *buffer, u32 size) {
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
    spin_lock_init(&buffer->lock, "profile_buffer");
    
    return 0;
}

/**
 * Free a profile buffer
 * 
 * @param buffer Buffer to free
 * @return 0 on success, negative error code on failure
 */
int profile_buffer_free(profile_buffer_t *buffer) {
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
 * Write to a profile buffer
 * 
 * @param buffer Buffer to write to
 * @param data Data to write
 * @param size Data size
 * @return Number of bytes written, or negative error code on failure
 */
int profile_buffer_write(profile_buffer_t *buffer, void *data, u32 size) {
    u32 space;
    u32 first_chunk;
    u32 second_chunk;
    
    /* Check parameters */
    if (buffer == NULL || data == NULL || size == 0) {
        return -EINVAL;
    }
    
    /* Check if buffer is full */
    spin_lock(&buffer->lock);
    space = profile_buffer_free_space(buffer);
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
 * Read from a profile buffer
 * 
 * @param buffer Buffer to read from
 * @param data Data buffer
 * @param size Data size
 * @return Number of bytes read, or negative error code on failure
 */
int profile_buffer_read(profile_buffer_t *buffer, void *data, u32 size) {
    u32 available;
    u32 first_chunk;
    u32 second_chunk;
    
    /* Check parameters */
    if (buffer == NULL || data == NULL || size == 0) {
        return -EINVAL;
    }
    
    /* Check if buffer is empty */
    spin_lock(&buffer->lock);
    available = profile_buffer_used(buffer);
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
 * Clear a profile buffer
 * 
 * @param buffer Buffer to clear
 * @return 0 on success, negative error code on failure
 */
int profile_buffer_clear(profile_buffer_t *buffer) {
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
 * Get used space in a profile buffer
 * 
 * @param buffer Buffer to check
 * @return Used space
 */
u32 profile_buffer_used(profile_buffer_t *buffer) {
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
 * Get free space in a profile buffer
 * 
 * @param buffer Buffer to check
 * @return Free space
 */
u32 profile_buffer_free_space(profile_buffer_t *buffer) {
    u32 used;
    
    /* Check parameters */
    if (buffer == NULL) {
        return 0;
    }
    
    /* Calculate free space */
    used = profile_buffer_used(buffer);
    
    return buffer->size - used - 1;
}

/**
 * Check if a profile buffer is empty
 * 
 * @param buffer Buffer to check
 * @return 1 if empty, 0 if not
 */
int profile_buffer_is_empty(profile_buffer_t *buffer) {
    /* Check parameters */
    if (buffer == NULL) {
        return 1;
    }
    
    /* Check if empty */
    return buffer->head == buffer->tail;
}

/**
 * Check if a profile buffer is full
 * 
 * @param buffer Buffer to check
 * @return 1 if full, 0 if not
 */
int profile_buffer_is_full(profile_buffer_t *buffer) {
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

/**
 * Get profiled functions
 * 
 * @param functions Pointer to store functions
 * @param count Pointer to store function count
 * @return 0 on success, negative error code on failure
 */
int profile_get_functions(profile_function_t **functions, u32 *count) {
    profile_function_t *func, *func_array;
    u32 func_count = 0;
    u32 i = 0;
    
    /* Check parameters */
    if (functions == NULL || count == NULL) {
        return -EINVAL;
    }
    
    /* Count functions */
    spin_lock(&profile_lock);
    list_for_each_entry(func, &profile_functions, list) {
        func_count++;
    }
    
    /* Allocate function array */
    func_array = kmalloc(func_count * sizeof(profile_function_t), MEM_KERNEL);
    if (func_array == NULL) {
        spin_unlock(&profile_lock);
        return -ENOMEM;
    }
    
    /* Copy functions */
    list_for_each_entry(func, &profile_functions, list) {
        memcpy(&func_array[i], func, sizeof(profile_function_t));
        i++;
    }
    spin_unlock(&profile_lock);
    
    /* Return functions */
    *functions = func_array;
    *count = func_count;
    
    return 0;
}

/**
 * Clear profiled functions
 * 
 * @return 0 on success, negative error code on failure
 */
int profile_clear_functions(void) {
    profile_function_t *func, *next;
    
    /* Clear functions */
    spin_lock(&profile_lock);
    list_for_each_entry_safe(func, next, &profile_functions, list) {
        list_del(&func->list);
        kfree(func);
    }
    spin_unlock(&profile_lock);
    
    return 0;
}

/**
 * Start profiling
 * 
 * @return 0 on success, negative error code on failure
 */
int profile_start(void) {
    /* Start profiling */
    profile_running = 1;
    
    return 0;
}

/**
 * Stop profiling
 * 
 * @return 0 on success, negative error code on failure
 */
int profile_stop(void) {
    /* Stop profiling */
    profile_running = 0;
    
    return 0;
}

/**
 * Check if profiling is running
 * 
 * @return 1 if running, 0 if not
 */
int profile_is_running(void) {
    return profile_running;
}

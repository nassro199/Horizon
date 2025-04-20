/**
 * profile.h - Horizon kernel profiling definitions
 * 
 * This file contains definitions for kernel profiling.
 */

#ifndef _HORIZON_PROFILE_H
#define _HORIZON_PROFILE_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/spinlock.h>

/* Profile event types */
#define PROFILE_EVENT_FUNCTION    0  /* Function entry/exit */
#define PROFILE_EVENT_SYSCALL     1  /* System call */
#define PROFILE_EVENT_INTERRUPT   2  /* Interrupt */
#define PROFILE_EVENT_SCHEDULE    3  /* Schedule */
#define PROFILE_EVENT_MEMORY      4  /* Memory allocation/free */
#define PROFILE_EVENT_IO          5  /* I/O operation */
#define PROFILE_EVENT_NETWORK     6  /* Network operation */
#define PROFILE_EVENT_DISK        7  /* Disk operation */
#define PROFILE_EVENT_USER        8  /* User-defined */

/* Profile event flags */
#define PROFILE_FLAG_ENABLED      0x01  /* Event is enabled */
#define PROFILE_FLAG_TIMESTAMP    0x02  /* Include timestamp */
#define PROFILE_FLAG_CPU          0x04  /* Include CPU ID */
#define PROFILE_FLAG_PID          0x08  /* Include process ID */
#define PROFILE_FLAG_TID          0x10  /* Include thread ID */
#define PROFILE_FLAG_COMM         0x20  /* Include command name */
#define PROFILE_FLAG_BACKTRACE    0x40  /* Include backtrace */
#define PROFILE_FLAG_ALL          0x7F  /* All flags */

/* Profile sample structure */
typedef struct profile_sample {
    u32 type;                     /* Event type */
    u32 flags;                    /* Event flags */
    u64 timestamp;                /* Timestamp */
    u32 cpu;                      /* CPU ID */
    u32 pid;                      /* Process ID */
    u32 tid;                      /* Thread ID */
    char comm[16];                /* Command name */
    void *caller;                 /* Caller address */
    void *backtrace[16];          /* Backtrace */
    u32 backtrace_size;           /* Backtrace size */
    u32 data_size;                /* Data size */
    u8 data[0];                   /* Event data */
} profile_sample_t;

/* Profile buffer structure */
typedef struct profile_buffer {
    u8 *buffer;                   /* Buffer */
    u32 size;                     /* Buffer size */
    u32 head;                     /* Buffer head */
    u32 tail;                     /* Buffer tail */
    spinlock_t lock;              /* Buffer lock */
} profile_buffer_t;

/* Profile point structure */
typedef struct profile_point {
    const char *name;             /* Point name */
    u32 type;                     /* Event type */
    u32 flags;                    /* Event flags */
    struct list_head list;        /* List of points */
    int (*handler)(struct profile_sample *sample, void *data);  /* Event handler */
    void *data;                   /* Handler data */
} profile_point_t;

/* Profile function structure */
typedef struct profile_function {
    const char *name;             /* Function name */
    void *addr;                   /* Function address */
    u32 calls;                    /* Number of calls */
    u64 total_time;               /* Total time spent in function */
    u64 min_time;                 /* Minimum time spent in function */
    u64 max_time;                 /* Maximum time spent in function */
    struct list_head list;        /* List of functions */
} profile_function_t;

/* Profile functions */
int profile_init(void);
int profile_register_point(profile_point_t *point);
int profile_unregister_point(profile_point_t *point);
int profile_sample(u32 type, u32 flags, void *data, u32 data_size);
int profile_function_enter(void *addr, const char *name);
int profile_function_exit(void *addr, u64 time);
int profile_syscall_enter(u32 syscall, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5, u32 arg6);
int profile_syscall_exit(u32 syscall, u32 ret, u64 time);
int profile_interrupt(u32 irq, u64 time);
int profile_schedule(struct task_struct *prev, struct task_struct *next, u64 time);
int profile_memory(void *ptr, u32 size, u32 flags);
int profile_io(u32 fd, u32 op, u32 size, u64 time);
int profile_network(u32 sock, u32 op, u32 size, u64 time);
int profile_disk(u32 dev, u32 op, u64 sector, u32 count, u64 time);
int profile_user(u32 type, void *data, u32 data_size);
int profile_enable(u32 type);
int profile_disable(u32 type);
int profile_is_enabled(u32 type);
int profile_set_flags(u32 type, u32 flags);
u32 profile_get_flags(u32 type);
int profile_buffer_init(profile_buffer_t *buffer, u32 size);
int profile_buffer_free(profile_buffer_t *buffer);
int profile_buffer_write(profile_buffer_t *buffer, void *data, u32 size);
int profile_buffer_read(profile_buffer_t *buffer, void *data, u32 size);
int profile_buffer_clear(profile_buffer_t *buffer);
u32 profile_buffer_used(profile_buffer_t *buffer);
u32 profile_buffer_free_space(profile_buffer_t *buffer);
int profile_buffer_is_empty(profile_buffer_t *buffer);
int profile_buffer_is_full(profile_buffer_t *buffer);
int profile_get_functions(profile_function_t **functions, u32 *count);
int profile_clear_functions(void);
int profile_start(void);
int profile_stop(void);
int profile_is_running(void);

#endif /* _HORIZON_PROFILE_H */

/**
 * trace.h - Horizon kernel tracing definitions
 * 
 * This file contains definitions for kernel tracing.
 */

#ifndef _HORIZON_TRACE_H
#define _HORIZON_TRACE_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/spinlock.h>

/* Trace event types */
#define TRACE_EVENT_SYSCALL      0  /* System call */
#define TRACE_EVENT_INTERRUPT    1  /* Interrupt */
#define TRACE_EVENT_SCHEDULE     2  /* Schedule */
#define TRACE_EVENT_PAGEFAULT    3  /* Page fault */
#define TRACE_EVENT_KMALLOC      4  /* Kernel memory allocation */
#define TRACE_EVENT_KFREE        5  /* Kernel memory free */
#define TRACE_EVENT_OPEN         6  /* File open */
#define TRACE_EVENT_CLOSE        7  /* File close */
#define TRACE_EVENT_READ         8  /* File read */
#define TRACE_EVENT_WRITE        9  /* File write */
#define TRACE_EVENT_MMAP         10 /* Memory map */
#define TRACE_EVENT_MUNMAP       11 /* Memory unmap */
#define TRACE_EVENT_BRK          12 /* Heap break */
#define TRACE_EVENT_FORK         13 /* Process fork */
#define TRACE_EVENT_EXEC         14 /* Process exec */
#define TRACE_EVENT_EXIT         15 /* Process exit */
#define TRACE_EVENT_SIGNAL       16 /* Signal */
#define TRACE_EVENT_LOCK         17 /* Lock */
#define TRACE_EVENT_UNLOCK       18 /* Unlock */
#define TRACE_EVENT_WAIT         19 /* Wait */
#define TRACE_EVENT_WAKEUP       20 /* Wakeup */
#define TRACE_EVENT_TIMER        21 /* Timer */
#define TRACE_EVENT_NETWORK      22 /* Network */
#define TRACE_EVENT_DISK         23 /* Disk */
#define TRACE_EVENT_USER         24 /* User-defined */

/* Trace event flags */
#define TRACE_FLAG_ENABLED       0x01 /* Event is enabled */
#define TRACE_FLAG_TIMESTAMP     0x02 /* Include timestamp */
#define TRACE_FLAG_CPU           0x04 /* Include CPU ID */
#define TRACE_FLAG_PID           0x08 /* Include process ID */
#define TRACE_FLAG_TID           0x10 /* Include thread ID */
#define TRACE_FLAG_COMM          0x20 /* Include command name */
#define TRACE_FLAG_BACKTRACE     0x40 /* Include backtrace */
#define TRACE_FLAG_ALL           0x7F /* All flags */

/* Trace event structure */
typedef struct trace_event {
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
} trace_event_t;

/* Trace buffer structure */
typedef struct trace_buffer {
    u8 *buffer;                   /* Buffer */
    u32 size;                     /* Buffer size */
    u32 head;                     /* Buffer head */
    u32 tail;                     /* Buffer tail */
    spinlock_t lock;              /* Buffer lock */
} trace_buffer_t;

/* Trace point structure */
typedef struct trace_point {
    const char *name;             /* Point name */
    u32 type;                     /* Event type */
    u32 flags;                    /* Event flags */
    struct list_head list;        /* List of points */
    int (*handler)(struct trace_event *event, void *data);  /* Event handler */
    void *data;                   /* Handler data */
} trace_point_t;

/* Trace functions */
int trace_init(void);
int trace_register_point(trace_point_t *point);
int trace_unregister_point(trace_point_t *point);
int trace_event(u32 type, u32 flags, void *data, u32 data_size);
int trace_syscall_enter(u32 syscall, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5, u32 arg6);
int trace_syscall_exit(u32 syscall, u32 ret);
int trace_interrupt(u32 irq);
int trace_schedule(struct task_struct *prev, struct task_struct *next);
int trace_pagefault(void *addr, u32 error);
int trace_kmalloc(void *ptr, u32 size, u32 flags);
int trace_kfree(void *ptr);
int trace_open(const char *path, u32 flags, u32 mode);
int trace_close(u32 fd);
int trace_read(u32 fd, void *buf, u32 count);
int trace_write(u32 fd, void *buf, u32 count);
int trace_mmap(void *addr, u32 length, u32 prot, u32 flags, u32 fd, u32 offset);
int trace_munmap(void *addr, u32 length);
int trace_brk(void *addr);
int trace_fork(u32 pid, u32 child_pid);
int trace_exec(const char *path, char **argv, char **envp);
int trace_exit(u32 code);
int trace_signal(u32 sig, void *handler);
int trace_lock(void *lock);
int trace_unlock(void *lock);
int trace_wait(void *wait);
int trace_wakeup(void *wait);
int trace_timer(u32 expires);
int trace_network(u32 sock, u32 op, void *buf, u32 len);
int trace_disk(u32 dev, u32 op, u64 sector, u32 count);
int trace_user(u32 type, void *data, u32 data_size);
int trace_enable(u32 type);
int trace_disable(u32 type);
int trace_is_enabled(u32 type);
int trace_set_flags(u32 type, u32 flags);
u32 trace_get_flags(u32 type);
int trace_buffer_init(trace_buffer_t *buffer, u32 size);
int trace_buffer_free(trace_buffer_t *buffer);
int trace_buffer_write(trace_buffer_t *buffer, void *data, u32 size);
int trace_buffer_read(trace_buffer_t *buffer, void *data, u32 size);
int trace_buffer_clear(trace_buffer_t *buffer);
u32 trace_buffer_used(trace_buffer_t *buffer);
u32 trace_buffer_free_space(trace_buffer_t *buffer);
int trace_buffer_is_empty(trace_buffer_t *buffer);
int trace_buffer_is_full(trace_buffer_t *buffer);

#endif /* _HORIZON_TRACE_H */

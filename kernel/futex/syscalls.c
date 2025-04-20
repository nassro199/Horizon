/**
 * syscalls.c - Horizon kernel futex system calls
 * 
 * This file contains the implementation of the futex system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/futex.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: futex */
long sys_futex(long uaddr, long op, long val, long timeout, long uaddr2, long val3) {
    /* Fast user-space locking */
    return futex_futex((int *)uaddr, op, val, (struct timespec *)timeout, (int *)uaddr2, val3);
}

/* System call: get_robust_list */
long sys_get_robust_list(long pid, long head_ptr, long len_ptr, long unused1, long unused2, long unused3) {
    /* Get the list of robust futexes */
    return futex_get_robust_list(pid, (struct robust_list_head **)head_ptr, (size_t *)len_ptr);
}

/* System call: set_robust_list */
long sys_set_robust_list(long head, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Set the list of robust futexes */
    return futex_set_robust_list((struct robust_list_head *)head, len);
}

/* Register futex system calls */
void futex_syscalls_init(void) {
    /* Register futex system calls */
    syscall_register(SYS_FUTEX, sys_futex);
    syscall_register(SYS_GET_ROBUST_LIST, sys_get_robust_list);
    syscall_register(SYS_SET_ROBUST_LIST, sys_set_robust_list);
}

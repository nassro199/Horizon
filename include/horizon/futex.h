/**
 * futex.h - Horizon kernel futex definitions
 * 
 * This file contains definitions for the futex subsystem.
 */

#ifndef _KERNEL_FUTEX_H
#define _KERNEL_FUTEX_H

#include <horizon/types.h>

/* Futex operations */
#define FUTEX_WAIT              0
#define FUTEX_WAKE              1
#define FUTEX_FD                2
#define FUTEX_REQUEUE           3
#define FUTEX_CMP_REQUEUE       4
#define FUTEX_WAKE_OP           5
#define FUTEX_LOCK_PI           6
#define FUTEX_UNLOCK_PI         7
#define FUTEX_TRYLOCK_PI        8
#define FUTEX_WAIT_BITSET       9
#define FUTEX_WAKE_BITSET       10
#define FUTEX_WAIT_REQUEUE_PI   11
#define FUTEX_CMP_REQUEUE_PI    12

/* Futex operation flags */
#define FUTEX_PRIVATE_FLAG      128
#define FUTEX_CLOCK_REALTIME    256
#define FUTEX_CMD_MASK          ~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME)

/* Robust list head structure */
struct robust_list {
    struct robust_list *next;
};

/* Robust list head structure */
struct robust_list_head {
    struct robust_list list;
    long futex_offset;
    struct robust_list *list_op_pending;
};

/* Futex functions */
int futex_futex(int *uaddr, int op, int val, struct timespec *timeout, int *uaddr2, int val3);
int futex_get_robust_list(int pid, struct robust_list_head **head_ptr, size_t *len_ptr);
int futex_set_robust_list(struct robust_list_head *head, size_t len);

#endif /* _KERNEL_FUTEX_H */

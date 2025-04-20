/*
 * uaccess.h - Horizon kernel user space access definitions
 *
 * This file contains definitions for accessing user space memory.
 */

#ifndef _HORIZON_UACCESS_H
#define _HORIZON_UACCESS_H

#include <horizon/types.h>

/* User space pointer marker */
#define __user

/* Access functions */
extern int copy_from_user(void *to, const void __user *from, unsigned long n);
extern int copy_to_user(void __user *to, const void *from, unsigned long n);
extern int get_user(void *x, const void __user *ptr);
extern int put_user(void *x, void __user *ptr);
extern int access_ok(int type, const void __user *addr, unsigned long size);

/* Access types */
#define VERIFY_READ  0
#define VERIFY_WRITE 1

/* Error pointer */
#define ERR_PTR(err) ((void *)((long)(err)))
#define PTR_ERR(ptr) ((long)(ptr))
#define IS_ERR(ptr)  ((unsigned long)(ptr) > (unsigned long)(-1000))

#endif /* _HORIZON_UACCESS_H */

/**
 * stddef.h - Standard definitions
 *
 * This file contains standard definitions used throughout the kernel.
 */

#ifndef _HORIZON_STDDEF_H
#define _HORIZON_STDDEF_H

/* NULL definition */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Size type */
typedef unsigned long size_t;

/* Signed size type */
typedef long ssize_t;

/* Offset type */
typedef long off_t;

/* Mode type */
typedef unsigned int mode_t;

/* User ID type */
typedef unsigned int uid_t;

/* Group ID type */
typedef unsigned int gid_t;

/* Process ID type */
typedef unsigned int pid_t;

/* Thread ID type */
typedef unsigned int tid_t;

/* Device type */
typedef unsigned int dev_t;

/* Inode type */
typedef unsigned long ino_t;

/* Block type */
typedef unsigned long blk_t;

/* Sector type */
typedef unsigned long sector_t;

/* Time type */
typedef unsigned long time_t;

/* Clock type */
typedef unsigned long clock_t;

/* Mode type */
typedef unsigned int umode_t;

/* Offset macros */
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/* Container of macro */
#define container_of(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })

#endif /* _HORIZON_STDDEF_H */

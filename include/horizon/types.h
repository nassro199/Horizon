/**
 * types.h - Common type definitions
 *
 * This file contains common type definitions used throughout the kernel.
 */

#ifndef _TYPES_H
#define _TYPES_H

/* NULL definition */
#define NULL ((void *)0)

/* Basic types */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;

typedef u32                size_t;
typedef s32                ssize_t;
typedef s64                off_t;
typedef s64                loff_t;
typedef u32                mode_t;
typedef u32                uid_t;
typedef u32                gid_t;
typedef u32                dev_t;
typedef u32                ino_t;
typedef u32                nlink_t;
typedef u32                blksize_t;
typedef u64                blkcnt_t;
typedef u32                umode_t;

/* Boolean type */
typedef enum {
    false = 0,
    true = 1
} bool;

/* Error codes */
typedef enum {
    SUCCESS = 0,
    ERROR_GENERAL = -1,
    ERROR_NOMEM = -2,
    ERROR_IO = -3,
    ERROR_INVAL = -4,
    ERROR_NOENT = -5
} error_t;

/* File mode flags */
#define FMODE_READ      (1 << 0)
#define FMODE_WRITE     (1 << 1)
#define FMODE_EXEC      (1 << 2)
#define FMODE_APPEND    (1 << 3)
#define FMODE_NONBLOCK  (1 << 4)
#define FMODE_SYNC      (1 << 5)
#define FMODE_DIRECT    (1 << 6)
#define FMODE_LARGEFILE (1 << 7)

/* IPI types */
#define IPI_RESCHEDULE  1
#define IPI_CALL_FUNC   2
#define IPI_STOP        3

/* Seek constants */
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

#endif /* _TYPES_H */

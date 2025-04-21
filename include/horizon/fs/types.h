/**
 * types.h - File system type definitions
 *
 * This file contains definitions for file system types.
 */

#ifndef _HORIZON_FS_TYPES_H
#define _HORIZON_FS_TYPES_H

#include <horizon/types.h>
#include <horizon/stddef.h>

/* File mode type */
typedef u32 umode_t;

/* User and group ID types */
typedef u32 uid_t;
typedef u32 gid_t;

/* Device ID type */
typedef u32 dev_t;

/* File offset type */
typedef s64 loff_t;

/* File mode flags */
#define FMODE_READ     (1 << 0)
#define FMODE_WRITE    (1 << 1)
#define FMODE_EXEC     (1 << 2)
#define FMODE_APPEND   (1 << 3)
#define FMODE_NONBLOCK (1 << 4)
#define FMODE_SYNC     (1 << 5)

#endif /* _HORIZON_FS_TYPES_H */

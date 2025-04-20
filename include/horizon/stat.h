/*
 * stat.h - Horizon kernel file status definitions
 *
 * This file contains definitions for file status.
 */

#ifndef _HORIZON_STAT_H
#define _HORIZON_STAT_H

#include <horizon/types.h>

/* File types */
#define S_IFMT   0170000
#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000

/* File mode bits */
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000
#define S_IRWXU  0000700
#define S_IRUSR  0000400
#define S_IWUSR  0000200
#define S_IXUSR  0000100
#define S_IRWXG  0000070
#define S_IRGRP  0000040
#define S_IWGRP  0000020
#define S_IXGRP  0000010
#define S_IRWXO  0000007
#define S_IROTH  0000004
#define S_IWOTH  0000002
#define S_IXOTH  0000001

/* File type macros */
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

/* Special inode numbers */
#define UTIME_NOW  ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)

/* File attribute flags */
#define ATTR_MODE      (1 << 0)
#define ATTR_UID       (1 << 1)
#define ATTR_GID       (1 << 2)
#define ATTR_SIZE      (1 << 3)
#define ATTR_ATIME     (1 << 4)
#define ATTR_MTIME     (1 << 5)
#define ATTR_CTIME     (1 << 6)
#define ATTR_ATIME_SET (1 << 7)
#define ATTR_MTIME_SET (1 << 8)
#define ATTR_FORCE     (1 << 9)
#define ATTR_KILL_SUID (1 << 10)
#define ATTR_KILL_SGID (1 << 11)
#define ATTR_FILE      (1 << 12)
#define ATTR_KILL_PRIV (1 << 13)
#define ATTR_OPEN      (1 << 14)
#define ATTR_TIMES_SET (1 << 15)

/* Inode attributes */
struct iattr {
    unsigned int ia_valid;
    umode_t ia_mode;
    uid_t ia_uid;
    gid_t ia_gid;
    loff_t ia_size;
    struct timespec ia_atime;
    struct timespec ia_mtime;
    struct timespec ia_ctime;
    struct file *ia_file;
};

/* Stat structure */
struct stat {
    dev_t     st_dev;
    ino_t     st_ino;
    mode_t    st_mode;
    nlink_t   st_nlink;
    uid_t     st_uid;
    gid_t     st_gid;
    dev_t     st_rdev;
    off_t     st_size;
    blksize_t st_blksize;
    blkcnt_t  st_blocks;
    time_t    st_atime;
    time_t    st_mtime;
    time_t    st_ctime;
};

/* Stat64 structure */
struct stat64 {
    dev_t     st_dev;
    ino64_t   st_ino;
    mode_t    st_mode;
    nlink_t   st_nlink;
    uid_t     st_uid;
    gid_t     st_gid;
    dev_t     st_rdev;
    off64_t   st_size;
    blksize_t st_blksize;
    blkcnt64_t st_blocks;
    time_t    st_atime;
    time_t    st_mtime;
    time_t    st_ctime;
};

/* Stat structure for the kernel */
struct kstat {
    u64        st_dev;
    u64        st_ino;
    umode_t    st_mode;
    u32        st_nlink;
    uid_t      st_uid;
    gid_t      st_gid;
    u64        st_rdev;
    u64        st_size;
    u64        st_atime;
    u64        st_mtime;
    u64        st_ctime;
    u64        st_blksize;
    u64        st_blocks;
};

#endif /* _HORIZON_STAT_H */

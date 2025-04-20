/**
 * fcntl.h - Horizon kernel file control definitions
 * 
 * This file contains definitions for file control operations.
 */

#ifndef _HORIZON_FCNTL_H
#define _HORIZON_FCNTL_H

/* File access modes */
#define O_RDONLY        00000000    /* Open for reading only */
#define O_WRONLY        00000001    /* Open for writing only */
#define O_RDWR          00000002    /* Open for reading and writing */
#define O_ACCMODE       00000003    /* Mask for file access modes */

/* File creation flags */
#define O_CREAT         00000100    /* Create file if it doesn't exist */
#define O_EXCL          00000200    /* Exclusive use flag */
#define O_NOCTTY        00000400    /* Do not assign controlling terminal */
#define O_TRUNC         00001000    /* Truncate flag */
#define O_APPEND        00002000    /* Append flag */
#define O_NONBLOCK      00004000    /* Non-blocking I/O */
#define O_DSYNC         00010000    /* Synchronize data */
#define O_ASYNC         00020000    /* Asynchronous I/O */
#define O_DIRECT        00040000    /* Direct I/O */
#define O_LARGEFILE     00100000    /* Large file */
#define O_DIRECTORY     00200000    /* Must be a directory */
#define O_NOFOLLOW      00400000    /* Do not follow links */
#define O_NOATIME       01000000    /* Do not update access time */
#define O_CLOEXEC       02000000    /* Close on exec */
#define O_SYNC          04000000    /* Synchronous I/O */
#define O_PATH          010000000   /* Path only */

/* File descriptor flags */
#define FD_CLOEXEC      1           /* Close on exec */

/* File control commands */
#define F_DUPFD         0           /* Duplicate file descriptor */
#define F_GETFD         1           /* Get file descriptor flags */
#define F_SETFD         2           /* Set file descriptor flags */
#define F_GETFL         3           /* Get file status flags */
#define F_SETFL         4           /* Set file status flags */
#define F_GETLK         5           /* Get record locking information */
#define F_SETLK         6           /* Set record locking information */
#define F_SETLKW        7           /* Set record locking information; wait if blocked */
#define F_SETOWN        8           /* Set owner */
#define F_GETOWN        9           /* Get owner */
#define F_SETSIG        10          /* Set asynchronous notification signal */
#define F_GETSIG        11          /* Get asynchronous notification signal */
#define F_GETLK64       12          /* Get record locking information */
#define F_SETLK64       13          /* Set record locking information */
#define F_SETLKW64      14          /* Set record locking information; wait if blocked */
#define F_DUPFD_CLOEXEC 1030        /* Duplicate file descriptor with close-on-exec */

/* File lock types */
#define F_RDLCK         0           /* Read lock */
#define F_WRLCK         1           /* Write lock */
#define F_UNLCK         2           /* Remove lock */

/* Special value for openat() */
#define AT_FDCWD        -100        /* Current working directory */

/* Flag values for openat() */
#define AT_SYMLINK_NOFOLLOW 0x100   /* Do not follow symbolic links */
#define AT_REMOVEDIR        0x200   /* Remove directory instead of file */
#define AT_SYMLINK_FOLLOW   0x400   /* Follow symbolic links */
#define AT_NO_AUTOMOUNT     0x800   /* Do not automount */
#define AT_EMPTY_PATH       0x1000  /* Allow empty relative pathname */

#endif /* _HORIZON_FCNTL_H */

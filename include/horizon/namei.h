/*
 * namei.h - Horizon kernel name lookup definitions
 *
 * This file contains definitions for path name lookup.
 */

#ifndef _HORIZON_NAMEI_H
#define _HORIZON_NAMEI_H

#include <horizon/types.h>
#include <horizon/fs/vfs.h>

/* Lookup flags */
#define LOOKUP_FOLLOW        0x0001
#define LOOKUP_DIRECTORY     0x0002
#define LOOKUP_AUTOMOUNT     0x0004
#define LOOKUP_PARENT        0x0010
#define LOOKUP_REVAL         0x0020
#define LOOKUP_RCU           0x0040
#define LOOKUP_OPEN          0x0100
#define LOOKUP_CREATE        0x0200
#define LOOKUP_EXCL          0x0400
#define LOOKUP_RENAME_TARGET 0x0800
#define LOOKUP_OPEN_CREATE   0x1000

/* Name data structure */
struct qstr {
    const char *name;
    unsigned int len;
    unsigned int hash;
};

/* Nameidata structure */
struct nameidata {
    struct path path;
    struct qstr last;
    struct path root;
    struct inode *inode;
    unsigned int flags;
    unsigned int seq;
    int last_type;
    unsigned depth;
    char *saved_names[MAX_NESTED_LINKS + 1];
};

/* Function prototypes */
int path_lookupat(struct nameidata *nd, unsigned flags, struct path *path);
void set_nameidata(struct nameidata *nd, int dfd, struct filename *name);
void restore_nameidata(void);
struct filename *getname(const char __user *filename);
void putname(struct filename *name);
int vfs_kern_path(const char *name, unsigned int flags, struct path *path);

#endif /* _HORIZON_NAMEI_H */

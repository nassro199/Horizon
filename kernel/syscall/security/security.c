/**
 * security.c - Horizon kernel security-related system calls
 *
 * This file contains the implementation of security-related system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/security.h>
#include <horizon/task.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Get user ID system call */
long sys_getuid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return the real user ID of the calling process */
    return task_current()->uid;
}

/* Get effective user ID system call */
long sys_geteuid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return the effective user ID of the calling process */
    return task_current()->euid;
}

/* Get group ID system call */
long sys_getgid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return the real group ID of the calling process */
    return task_current()->gid;
}

/* Get effective group ID system call */
long sys_getegid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return the effective group ID of the calling process */
    return task_current()->egid;
}

/* Set user ID system call */
long sys_setuid(long uid, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Set the user ID of the calling process */
    if (task_current()->euid != 0) {
        return -EPERM;
    }

    task_current()->uid = uid;
    task_current()->euid = uid;
    task_current()->suid = uid;

    return 0;
}

/* Set effective user ID system call */
long sys_seteuid(long euid, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Set the effective user ID of the calling process */
    if (task_current()->euid != 0 && task_current()->uid != euid && task_current()->suid != euid) {
        return -EPERM;
    }

    task_current()->euid = euid;

    return 0;
}

/* Set real and effective user ID system call */
long sys_setreuid(long ruid, long euid, long arg3, long arg4, long arg5, long arg6) {
    /* Set the real and effective user IDs of the calling process */
    if (ruid != (uid_t)-1) {
        if (task_current()->euid != 0 && task_current()->uid != ruid && task_current()->euid != ruid) {
            return -EPERM;
        }

        task_current()->uid = ruid;
    }

    if (euid != (uid_t)-1) {
        if (task_current()->euid != 0 && task_current()->uid != euid && task_current()->suid != euid) {
            return -EPERM;
        }

        task_current()->euid = euid;
    }

    return 0;
}

/* Set group ID system call */
long sys_setgid(long gid, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Set the group ID of the calling process */
    if (task_current()->euid != 0) {
        return -EPERM;
    }

    task_current()->gid = gid;
    task_current()->egid = gid;
    task_current()->sgid = gid;

    return 0;
}

/* Set effective group ID system call */
long sys_setegid(long egid, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Set the effective group ID of the calling process */
    if (task_current()->euid != 0 && task_current()->gid != egid && task_current()->sgid != egid) {
        return -EPERM;
    }

    task_current()->egid = egid;

    return 0;
}

/* Set real and effective group ID system call */
long sys_setregid(long rgid, long egid, long arg3, long arg4, long arg5, long arg6) {
    /* Set the real and effective group IDs of the calling process */
    if (rgid != (gid_t)-1) {
        if (task_current()->euid != 0 && task_current()->gid != rgid && task_current()->egid != rgid) {
            return -EPERM;
        }

        task_current()->gid = rgid;
    }

    if (egid != (gid_t)-1) {
        if (task_current()->euid != 0 && task_current()->gid != egid && task_current()->sgid != egid) {
            return -EPERM;
        }

        task_current()->egid = egid;
    }

    return 0;
}

/* Get groups system call */
long sys_getgroups(long size, long list, long arg3, long arg4, long arg5, long arg6) {
    /* Get list of supplementary group IDs */
    if (size == 0) {
        return task_current()->ngroups;
    }

    if (size < task_current()->ngroups) {
        return -EINVAL;
    }

    for (int i = 0; i < task_current()->ngroups; i++) {
        ((gid_t *)list)[i] = task_current()->groups[i];
    }

    return task_current()->ngroups;
}

/* Set groups system call */
long sys_setgroups(long size, long list, long arg3, long arg4, long arg5, long arg6) {
    /* Set list of supplementary group IDs */
    if (task_current()->euid != 0) {
        return -EPERM;
    }

    if (size > NGROUPS_MAX) {
        return -EINVAL;
    }

    task_current()->ngroups = size;

    for (int i = 0; i < size; i++) {
        task_current()->groups[i] = ((gid_t *)list)[i];
    }

    return 0;
}

/* Get resource limit system call */
long sys_getrlimit(long resource, long rlim, long arg3, long arg4, long arg5, long arg6) {
    /* Get resource limits */
    return task_getrlimit(task_current(), resource, (struct rlimit *)rlim);
}

/* Set resource limit system call */
long sys_setrlimit(long resource, long rlim, long arg3, long arg4, long arg5, long arg6) {
    /* Set resource limits */
    return task_setrlimit(task_current(), resource, (const struct rlimit *)rlim);
}

/* Get process resource limit system call */
long sys_prlimit64(long pid, long resource, long new_limit, long old_limit, long arg5, long arg6) {
    /* Get/set resource limits */
    struct task_struct *task;

    if (pid == 0) {
        task = task_current();
    } else {
        task = task_get(pid);
        if (task == NULL) {
            return -ESRCH;
        }
    }

    if (old_limit != 0) {
        int ret = task_getrlimit(task, resource, (struct rlimit *)old_limit);
        if (ret < 0) {
            return ret;
        }
    }

    if (new_limit != 0) {
        int ret = task_setrlimit(task, resource, (const struct rlimit *)new_limit);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

/* Initialize security-related system calls */
void security_syscalls_init(void) {
    /* Register security-related system calls */
    syscall_register(SYS_GETUID, sys_getuid);
    syscall_register(SYS_GETEUID, sys_geteuid);
    syscall_register(SYS_GETGID, sys_getgid);
    syscall_register(SYS_GETEGID, sys_getegid);
    syscall_register(SYS_SETUID, sys_setuid);
    syscall_register(SYS_SETEUID, sys_seteuid);
    syscall_register(SYS_SETREUID, sys_setreuid);
    syscall_register(SYS_SETGID, sys_setgid);
    syscall_register(SYS_SETEGID, sys_setegid);
    syscall_register(SYS_SETREGID, sys_setregid);
    syscall_register(SYS_GETGROUPS, sys_getgroups);
    syscall_register(SYS_SETGROUPS, sys_setgroups);
    syscall_register(SYS_GETRLIMIT, sys_getrlimit);
    syscall_register(SYS_SETRLIMIT, sys_setrlimit);
    syscall_register(SYS_PRLIMIT64, sys_prlimit64);
}

/**
 * thread.c - Horizon kernel thread management implementation
 * 
 * This file contains the implementation of the thread management subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/process.h>
#include <horizon/sched.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Clone flags */
#define CLONE_VM        0x00000100  /* Share memory */
#define CLONE_FS        0x00000200  /* Share filesystem info */
#define CLONE_FILES     0x00000400  /* Share file descriptors */
#define CLONE_SIGHAND   0x00000800  /* Share signal handlers */
#define CLONE_PTRACE    0x00002000  /* Continue tracing */
#define CLONE_VFORK     0x00004000  /* vfork semantics */
#define CLONE_PARENT    0x00008000  /* Share parent */
#define CLONE_THREAD    0x00010000  /* Same thread group */
#define CLONE_NEWNS     0x00020000  /* New namespace */
#define CLONE_SYSVSEM   0x00040000  /* Share System V SEM_UNDO */
#define CLONE_SETTLS    0x00080000  /* Set TLS */
#define CLONE_PARENT_SETTID 0x00100000 /* Set TID in parent */
#define CLONE_CHILD_CLEARTID 0x00200000 /* Clear TID in child */
#define CLONE_DETACHED  0x00400000  /* Detach from parent */
#define CLONE_UNTRACED  0x00800000  /* Don't trace */
#define CLONE_CHILD_SETTID  0x01000000 /* Set TID in child */
#define CLONE_NEWCGROUP 0x02000000  /* New cgroup namespace */
#define CLONE_NEWUTS    0x04000000  /* New utsname namespace */
#define CLONE_NEWIPC    0x08000000  /* New ipc namespace */
#define CLONE_NEWUSER   0x10000000  /* New user namespace */
#define CLONE_NEWPID    0x20000000  /* New pid namespace */
#define CLONE_NEWNET    0x40000000  /* New network namespace */
#define CLONE_IO        0x80000000  /* Clone I/O context */

/* Initialize the thread management subsystem */
void process_thread_init(void) {
    /* Initialize the thread management subsystem */
}

/* Create a child process with more control */
pid_t process_clone(unsigned long flags, void *stack, int *parent_tid, int *child_tid, unsigned long tls) {
    /* Check if the flags are valid */
    if ((flags & CLONE_VM) && !(flags & CLONE_THREAD)) {
        /* Cannot share memory without being in the same thread group */
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *parent = task_current();
    
    if (parent == NULL) {
        return -1;
    }
    
    /* Create a new task */
    task_struct_t *child = task_alloc();
    
    if (child == NULL) {
        return -1;
    }
    
    /* Initialize the child task */
    child->parent = parent;
    child->ppid = parent->pid;
    
    /* Set the thread group ID */
    if (flags & CLONE_THREAD) {
        child->tgid = parent->tgid;
    } else {
        child->tgid = child->pid;
    }
    
    /* Set the process group ID */
    child->pgid = parent->pgid;
    
    /* Set the session ID */
    child->sid = parent->sid;
    
    /* Set the user and group IDs */
    child->uid = parent->uid;
    child->gid = parent->gid;
    child->euid = parent->euid;
    child->egid = parent->egid;
    child->suid = parent->suid;
    child->sgid = parent->sgid;
    child->fsuid = parent->fsuid;
    child->fsgid = parent->fsgid;
    
    /* Set the scheduling parameters */
    child->policy = parent->policy;
    child->static_prio = parent->static_prio;
    child->prio = parent->prio;
    child->rt_priority = parent->rt_priority;
    
    /* Set the CPU affinity */
    memcpy(&child->cpus_allowed, &parent->cpus_allowed, sizeof(cpu_set_t));
    
    /* Set the signal handlers */
    if (flags & CLONE_SIGHAND) {
        /* Share signal handlers */
        child->sighand = parent->sighand;
    } else {
        /* Copy signal handlers */
        child->sighand = kmalloc(sizeof(struct sighand_struct), MEM_KERNEL | MEM_ZERO);
        
        if (child->sighand == NULL) {
            task_free(child);
            return -1;
        }
        
        memcpy(child->sighand, parent->sighand, sizeof(struct sighand_struct));
    }
    
    /* Set the signal mask */
    memcpy(&child->sigmask, &parent->sigmask, sizeof(sigset_t));
    
    /* Set the memory management */
    if (flags & CLONE_VM) {
        /* Share memory */
        child->mm = parent->mm;
    } else {
        /* Copy memory */
        child->mm = vmm_copy_mm(parent->mm);
        
        if (child->mm == NULL) {
            if (!(flags & CLONE_SIGHAND)) {
                kfree(child->sighand);
            }
            
            task_free(child);
            return -1;
        }
    }
    
    /* Set the file system info */
    if (flags & CLONE_FS) {
        /* Share file system info */
        child->fs = parent->fs;
    } else {
        /* Copy file system info */
        child->fs = kmalloc(sizeof(struct fs_struct), MEM_KERNEL | MEM_ZERO);
        
        if (child->fs == NULL) {
            if (!(flags & CLONE_VM)) {
                vmm_free_mm(child->mm);
            }
            
            if (!(flags & CLONE_SIGHAND)) {
                kfree(child->sighand);
            }
            
            task_free(child);
            return -1;
        }
        
        memcpy(child->fs, parent->fs, sizeof(struct fs_struct));
    }
    
    /* Set the file descriptors */
    if (flags & CLONE_FILES) {
        /* Share file descriptors */
        child->files = parent->files;
    } else {
        /* Copy file descriptors */
        child->files = kmalloc(sizeof(struct files_struct), MEM_KERNEL | MEM_ZERO);
        
        if (child->files == NULL) {
            if (!(flags & CLONE_FS)) {
                kfree(child->fs);
            }
            
            if (!(flags & CLONE_VM)) {
                vmm_free_mm(child->mm);
            }
            
            if (!(flags & CLONE_SIGHAND)) {
                kfree(child->sighand);
            }
            
            task_free(child);
            return -1;
        }
        
        memcpy(child->files, parent->files, sizeof(struct files_struct));
    }
    
    /* Set the stack */
    if (stack != NULL) {
        child->stack = stack;
    } else {
        /* Allocate a new stack */
        child->stack = kmalloc(TASK_STACK_SIZE, MEM_KERNEL | MEM_ZERO);
        
        if (child->stack == NULL) {
            if (!(flags & CLONE_FILES)) {
                kfree(child->files);
            }
            
            if (!(flags & CLONE_FS)) {
                kfree(child->fs);
            }
            
            if (!(flags & CLONE_VM)) {
                vmm_free_mm(child->mm);
            }
            
            if (!(flags & CLONE_SIGHAND)) {
                kfree(child->sighand);
            }
            
            task_free(child);
            return -1;
        }
    }
    
    /* Set the TLS */
    if (flags & CLONE_SETTLS) {
        child->tls = tls;
    } else {
        child->tls = parent->tls;
    }
    
    /* Set the TID in the parent */
    if ((flags & CLONE_PARENT_SETTID) && parent_tid != NULL) {
        *parent_tid = child->pid;
    }
    
    /* Set the TID in the child */
    if ((flags & CLONE_CHILD_SETTID) && child_tid != NULL) {
        child->set_child_tid = child_tid;
    }
    
    /* Clear the TID in the child */
    if ((flags & CLONE_CHILD_CLEARTID) && child_tid != NULL) {
        child->clear_child_tid = child_tid;
    }
    
    /* Set the vfork flag */
    if (flags & CLONE_VFORK) {
        child->vfork_done = &parent->vfork_done;
    }
    
    /* Add the child to the task list */
    task_add(child);
    
    /* Start the child */
    task_start(child);
    
    /* Wait for vfork to complete */
    if (flags & CLONE_VFORK) {
        task_wait_vfork(parent);
    }
    
    return child->pid;
}

/* Create a child process and block parent */
pid_t process_vfork(void) {
    /* Clone with vfork semantics */
    return process_clone(CLONE_VM | CLONE_VFORK | CLONE_PARENT_SETTID, NULL, NULL, NULL, 0);
}

/* Set pointer to thread ID */
pid_t process_set_tid_address(int *tidptr) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Set the clear child TID address */
    task->clear_child_tid = tidptr;
    
    return task->pid;
}

/* Get thread identification */
pid_t process_gettid(void) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Return the thread ID */
    return task->pid;
}

/* Set a thread local storage (TLS) area */
int process_set_thread_area(struct user_desc *u_info) {
    /* Check if the parameters are valid */
    if (u_info == NULL) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Set the TLS */
    task->tls = (unsigned long)u_info->base_addr;
    
    return 0;
}

/* Get a thread local storage (TLS) area */
int process_get_thread_area(struct user_desc *u_info) {
    /* Check if the parameters are valid */
    if (u_info == NULL) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Get the TLS */
    u_info->base_addr = (void *)task->tls;
    
    return 0;
}

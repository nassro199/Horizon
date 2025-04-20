/**
 * process.c - Horizon kernel process implementation
 * 
 * This file contains the implementation of the process subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/process.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Initialize the process subsystem */
void process_init(void) {
    /* Initialize the process subsystem */
    process_exec_init();
    process_thread_init();
    process_resource_init();
    process_sched_init();
}

/* Create a child process */
pid_t process_fork(void) {
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
    child->tgid = child->pid;
    child->pgid = parent->pgid;
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
    
    /* Copy the signal handlers */
    child->sighand = kmalloc(sizeof(struct sighand_struct), MEM_KERNEL | MEM_ZERO);
    
    if (child->sighand == NULL) {
        task_free(child);
        return -1;
    }
    
    memcpy(child->sighand, parent->sighand, sizeof(struct sighand_struct));
    
    /* Set the signal mask */
    memcpy(&child->sigmask, &parent->sigmask, sizeof(sigset_t));
    
    /* Copy the memory */
    child->mm = vmm_copy_mm(parent->mm);
    
    if (child->mm == NULL) {
        kfree(child->sighand);
        task_free(child);
        return -1;
    }
    
    /* Copy the file system info */
    child->fs = kmalloc(sizeof(struct fs_struct), MEM_KERNEL | MEM_ZERO);
    
    if (child->fs == NULL) {
        vmm_free_mm(child->mm);
        kfree(child->sighand);
        task_free(child);
        return -1;
    }
    
    memcpy(child->fs, parent->fs, sizeof(struct fs_struct));
    
    /* Copy the file descriptors */
    child->files = kmalloc(sizeof(struct files_struct), MEM_KERNEL | MEM_ZERO);
    
    if (child->files == NULL) {
        kfree(child->fs);
        vmm_free_mm(child->mm);
        kfree(child->sighand);
        task_free(child);
        return -1;
    }
    
    memcpy(child->files, parent->files, sizeof(struct files_struct));
    
    /* Allocate a stack */
    child->stack = kmalloc(TASK_STACK_SIZE, MEM_KERNEL | MEM_ZERO);
    
    if (child->stack == NULL) {
        kfree(child->files);
        kfree(child->fs);
        vmm_free_mm(child->mm);
        kfree(child->sighand);
        task_free(child);
        return -1;
    }
    
    /* Copy the registers */
    memcpy(&child->regs, &parent->regs, sizeof(struct regs));
    
    /* Set the return value for the child */
    child->regs.eax = 0;
    
    /* Add the child to the task list */
    task_add(child);
    
    /* Start the child */
    task_start(child);
    
    /* Return the child PID to the parent */
    return child->pid;
}

/* Terminate the calling process */
void process_exit(int status) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return;
    }
    
    /* Set the exit code */
    task->exit_code = status;
    
    /* Set the task state */
    task->state = TASK_ZOMBIE;
    
    /* Wake up the parent */
    task_wake(task->parent);
    
    /* Schedule another task */
    task_schedule();
}

/* Terminate all threads in a process */
void process_exit_group(int status) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return;
    }
    
    /* Set the exit code */
    task->exit_code = status;
    
    /* Set the task state */
    task->state = TASK_ZOMBIE;
    
    /* Terminate all threads in the thread group */
    task_kill_thread_group(task);
    
    /* Wake up the parent */
    task_wake(task->parent);
    
    /* Schedule another task */
    task_schedule();
}

/* Wait for process to change state */
pid_t process_wait4(pid_t pid, int *status, int options, struct rusage *rusage) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Find a child to wait for */
    task_struct_t *child = NULL;
    
    if (pid > 0) {
        /* Wait for a specific child */
        child = task_get(pid);
        
        if (child == NULL || child->parent != task) {
            return -1;
        }
    } else if (pid == 0) {
        /* Wait for any child in the same process group */
        child = task_get_zombie_child_by_pgid(task, task->pgid);
    } else if (pid == -1) {
        /* Wait for any child */
        child = task_get_zombie_child(task);
    } else {
        /* Wait for any child in the specified process group */
        child = task_get_zombie_child_by_pgid(task, -pid);
    }
    
    /* Check if a child was found */
    if (child == NULL) {
        if (options & WNOHANG) {
            return 0;
        }
        
        /* Wait for a child to exit */
        task_wait_child(task);
        
        /* Try again */
        return process_wait4(pid, status, options, rusage);
    }
    
    /* Get the exit status */
    if (status != NULL) {
        *status = child->exit_code;
    }
    
    /* Get the resource usage */
    if (rusage != NULL) {
        memcpy(rusage, &child->rusage, sizeof(struct rusage));
    }
    
    /* Get the child PID */
    pid_t child_pid = child->pid;
    
    /* Free the child */
    task_free(child);
    
    return child_pid;
}

/* Wait for process to change state */
int process_waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options, struct rusage *rusage) {
    /* Check if the parameters are valid */
    if (infop == NULL) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Find a child to wait for */
    task_struct_t *child = NULL;
    
    switch (idtype) {
        case P_PID:
            /* Wait for a specific child */
            child = task_get(id);
            
            if (child == NULL || child->parent != task) {
                return -1;
            }
            break;
        
        case P_PGID:
            /* Wait for any child in the specified process group */
            child = task_get_zombie_child_by_pgid(task, id);
            break;
        
        case P_ALL:
            /* Wait for any child */
            child = task_get_zombie_child(task);
            break;
        
        default:
            return -1;
    }
    
    /* Check if a child was found */
    if (child == NULL) {
        if (options & WNOHANG) {
            return 0;
        }
        
        /* Wait for a child to exit */
        task_wait_child(task);
        
        /* Try again */
        return process_waitid(idtype, id, infop, options, rusage);
    }
    
    /* Fill the siginfo structure */
    infop->si_signo = SIGCHLD;
    infop->si_errno = 0;
    infop->si_code = CLD_EXITED;
    infop->_sifields._sigchld.si_pid = child->pid;
    infop->_sifields._sigchld.si_uid = child->uid;
    infop->_sifields._sigchld.si_status = child->exit_code;
    
    /* Get the resource usage */
    if (rusage != NULL) {
        memcpy(rusage, &child->rusage, sizeof(struct rusage));
    }
    
    /* Free the child */
    task_free(child);
    
    return 0;
}

/* Get process identification */
pid_t process_getpid(void) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Return the process ID */
    return task->tgid;
}

/* Get parent process identification */
pid_t process_getppid(void) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Return the parent process ID */
    return task->ppid;
}

/* Get process group ID for a process */
pid_t process_getpgid(pid_t pid) {
    /* Get the task */
    task_struct_t *task = NULL;
    
    if (pid == 0) {
        /* Get the current task */
        task = task_current();
    } else {
        /* Get the task by process ID */
        task = task_get(pid);
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Return the process group ID */
    return task->pgid;
}

/* Set process group ID for process */
int process_setpgid(pid_t pid, pid_t pgid) {
    /* Get the task */
    task_struct_t *task = NULL;
    
    if (pid == 0) {
        /* Get the current task */
        task = task_current();
    } else {
        /* Get the task by process ID */
        task = task_get(pid);
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Set the process group ID */
    if (pgid == 0) {
        task->pgid = task->pid;
    } else {
        task->pgid = pgid;
    }
    
    return 0;
}

/* Get process group ID of the calling process */
pid_t process_getpgrp(void) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Return the process group ID */
    return task->pgid;
}

/* Get session ID */
pid_t process_getsid(pid_t pid) {
    /* Get the task */
    task_struct_t *task = NULL;
    
    if (pid == 0) {
        /* Get the current task */
        task = task_current();
    } else {
        /* Get the task by process ID */
        task = task_get(pid);
    }
    
    /* Check if the task was found */
    if (task == NULL) {
        return -1;
    }
    
    /* Return the session ID */
    return task->sid;
}

/* Create session and set process group ID */
pid_t process_setsid(void) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Check if the task is a process group leader */
    if (task->pid == task->pgid) {
        return -1;
    }
    
    /* Set the session ID */
    task->sid = task->pid;
    
    /* Set the process group ID */
    task->pgid = task->pid;
    
    return task->sid;
}

/* Set NIS domain name */
int process_setdomainname(const char *name, size_t len) {
    /* Check if the parameters are valid */
    if (name == NULL || len > __NEW_UTS_LEN) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Check if the task has the capability */
    if (!task_has_capability(task, CAP_SYS_ADMIN)) {
        return -1;
    }
    
    /* Set the domain name */
    strncpy(system_utsname.domainname, name, len);
    system_utsname.domainname[len] = '\0';
    
    return 0;
}

/* Set hostname */
int process_sethostname(const char *name, size_t len) {
    /* Check if the parameters are valid */
    if (name == NULL || len > __NEW_UTS_LEN) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Check if the task has the capability */
    if (!task_has_capability(task, CAP_SYS_ADMIN)) {
        return -1;
    }
    
    /* Set the hostname */
    strncpy(system_utsname.nodename, name, len);
    system_utsname.nodename[len] = '\0';
    
    return 0;
}

/* Get hostname */
int process_gethostname(char *name, size_t len) {
    /* Check if the parameters are valid */
    if (name == NULL) {
        return -1;
    }
    
    /* Get the hostname */
    strncpy(name, system_utsname.nodename, len);
    
    /* Ensure null termination */
    if (len > 0) {
        name[len - 1] = '\0';
    }
    
    return 0;
}

/* Get NIS domain name */
int process_getdomainname(char *name, size_t len) {
    /* Check if the parameters are valid */
    if (name == NULL) {
        return -1;
    }
    
    /* Get the domain name */
    strncpy(name, system_utsname.domainname, len);
    
    /* Ensure null termination */
    if (len > 0) {
        name[len - 1] = '\0';
    }
    
    return 0;
}

/* Reboot or enable/disable Ctrl-Alt-Del */
int process_reboot(int magic1, int magic2, int cmd, void *arg) {
    /* Check if the magic numbers are valid */
    if (magic1 != LINUX_REBOOT_MAGIC1 || magic2 != LINUX_REBOOT_MAGIC2) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Check if the task has the capability */
    if (!task_has_capability(task, CAP_SYS_BOOT)) {
        return -1;
    }
    
    /* Perform the reboot operation */
    switch (cmd) {
        case LINUX_REBOOT_CMD_RESTART:
            /* Restart the system */
            system_reboot();
            break;
        
        case LINUX_REBOOT_CMD_HALT:
            /* Halt the system */
            system_halt();
            break;
        
        case LINUX_REBOOT_CMD_POWER_OFF:
            /* Power off the system */
            system_power_off();
            break;
        
        case LINUX_REBOOT_CMD_RESTART2:
            /* Restart the system with a command */
            system_restart(arg);
            break;
        
        case LINUX_REBOOT_CMD_CAD_ON:
            /* Enable Ctrl-Alt-Del */
            system_cad_enable();
            break;
        
        case LINUX_REBOOT_CMD_CAD_OFF:
            /* Disable Ctrl-Alt-Del */
            system_cad_disable();
            break;
        
        default:
            return -1;
    }
    
    return 0;
}

/* Restart a system call after interruption by a stop signal */
int process_restart_syscall(void) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Restart the system call */
    task->regs.eax = task->regs.orig_eax;
    task->regs.eip -= 2; /* Adjust for the system call instruction */
    
    return 0;
}

/* Load a new kernel for later execution */
int process_kexec_load(unsigned long entry, unsigned long nr_segments, struct kexec_segment *segments, unsigned long flags) {
    /* Check if the parameters are valid */
    if (segments == NULL) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Check if the task has the capability */
    if (!task_has_capability(task, CAP_SYS_BOOT)) {
        return -1;
    }
    
    /* Load the new kernel */
    /* This would be implemented with actual kernel loading */
    
    return 0;
}

/**
 * process.c - Horizon kernel process implementation
 * 
 * This file contains the implementation of the process subsystem.
 * The implementation is compatible with Linux.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/process.h>
#include <horizon/mm.h>
#include <horizon/vmm.h>
#include <horizon/fs.h>
#include <horizon/security.h>
#include <horizon/sched.h>
#include <horizon/elf.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Process list */
static list_head_t process_list;

/* Next process ID */
static u32 next_pid = 1;

/* Current process */
static task_struct_t *current_task = NULL;

/* Initialize the process subsystem */
void process_init(void) {
    /* Initialize the process list */
    list_init(&process_list);
    
    /* Create the init process */
    task_struct_t *init = process_create("init");
    
    if (init == NULL) {
        kernel_panic("Failed to create init process");
    }
    
    /* Set the current process */
    current_task = init;
}

/* Create a new process */
task_struct_t *process_create(const char *name) {
    /* Allocate a process structure */
    task_struct_t *task = kmalloc(sizeof(task_struct_t), MEM_KERNEL | MEM_ZERO);
    
    if (task == NULL) {
        return NULL;
    }
    
    /* Initialize the process */
    task->pid = next_pid++;
    task->tgid = task->pid;
    task->ppid = 0;
    task->state = TASK_RUNNING;
    task->flags = 0;
    task->exit_code = 0;
    task->exit_signal = 0;
    task->pdeath_signal = 0;
    task->personality = 0;
    task->did_exec = 0;
    task->in_execve = 0;
    task->in_iowait = 0;
    task->sched_reset_on_fork = 0;
    task->sched_contributes_to_load = 0;
    task->sched_migrated = 0;
    task->sched_remote_wakeup = 0;
    task->sched_cookie = 0;
    
    /* Set the process name */
    strncpy(task->comm, name, sizeof(task->comm) - 1);
    task->comm[sizeof(task->comm) - 1] = '\0';
    
    /* Allocate memory structure */
    task->mm = kmalloc(sizeof(mm_struct_t), MEM_KERNEL | MEM_ZERO);
    
    if (task->mm == NULL) {
        kfree(task);
        return NULL;
    }
    
    /* Initialize memory structure */
    task->mm->context = vmm_create_context();
    
    if (task->mm->context == NULL) {
        kfree(task->mm);
        kfree(task);
        return NULL;
    }
    
    list_init(&task->mm->mmap);
    task->mm->map_count = 0;
    task->mm->total_vm = 0;
    task->mm->locked_vm = 0;
    task->mm->pinned_vm = 0;
    task->mm->data_vm = 0;
    task->mm->exec_vm = 0;
    task->mm->stack_vm = 0;
    task->mm->def_flags = 0;
    task->mm->nr_ptes = 0;
    
    task->active_mm = task->mm;
    
    /* Allocate files structure */
    task->files = kmalloc(sizeof(files_struct_t), MEM_KERNEL | MEM_ZERO);
    
    if (task->files == NULL) {
        vmm_destroy_context(task->mm->context);
        kfree(task->mm);
        kfree(task);
        return NULL;
    }
    
    /* Initialize files structure */
    task->files->count = 1;
    task->files->max_fds = 16;
    task->files->next_fd = 0;
    
    /* Allocate file descriptor array */
    task->files->fd_array = kmalloc(sizeof(file_t *) * task->files->max_fds, MEM_KERNEL | MEM_ZERO);
    
    if (task->files->fd_array == NULL) {
        kfree(task->files);
        vmm_destroy_context(task->mm->context);
        kfree(task->mm);
        kfree(task);
        return NULL;
    }
    
    /* Allocate close on exec flags */
    task->files->close_on_exec = kmalloc(sizeof(u32) * ((task->files->max_fds + 31) / 32), MEM_KERNEL | MEM_ZERO);
    
    if (task->files->close_on_exec == NULL) {
        kfree(task->files->fd_array);
        kfree(task->files);
        vmm_destroy_context(task->mm->context);
        kfree(task->mm);
        kfree(task);
        return NULL;
    }
    
    /* Allocate signal handler structure */
    task->sighand = kmalloc(sizeof(sighand_struct_t), MEM_KERNEL | MEM_ZERO);
    
    if (task->sighand == NULL) {
        kfree(task->files->close_on_exec);
        kfree(task->files->fd_array);
        kfree(task->files);
        vmm_destroy_context(task->mm->context);
        kfree(task->mm);
        kfree(task);
        return NULL;
    }
    
    /* Initialize signal handler structure */
    task->sighand->count = 1;
    
    /* Allocate security context */
    task->security = security_alloc_context();
    
    if (task->security == NULL) {
        kfree(task->sighand);
        kfree(task->files->close_on_exec);
        kfree(task->files->fd_array);
        kfree(task->files);
        vmm_destroy_context(task->mm->context);
        kfree(task->mm);
        kfree(task);
        return NULL;
    }
    
    /* Allocate kernel stack */
    task->stack = kmalloc(4096, MEM_KERNEL);
    
    if (task->stack == NULL) {
        security_free_context(task->security);
        kfree(task->sighand);
        kfree(task->files->close_on_exec);
        kfree(task->files->fd_array);
        kfree(task->files);
        vmm_destroy_context(task->mm->context);
        kfree(task->mm);
        kfree(task);
        return NULL;
    }
    
    /* Initialize lists */
    list_init(&task->children);
    list_init(&task->sibling);
    list_init(&task->tasks);
    list_init(&task->ptraced);
    list_init(&task->ptrace_entry);
    list_init(&task->thread_group);
    list_init(&task->thread_node);
    
    /* Add to the process list */
    list_add_tail(&task->tasks, &process_list);
    
    return task;
}

/* Destroy a process */
int process_destroy(task_struct_t *task) {
    if (task == NULL) {
        return -1;
    }
    
    /* Remove from the process list */
    list_del(&task->tasks);
    
    /* Free the kernel stack */
    if (task->stack != NULL) {
        kfree(task->stack);
    }
    
    /* Free the security context */
    if (task->security != NULL) {
        security_free_context(task->security);
    }
    
    /* Free the signal handler structure */
    if (task->sighand != NULL) {
        kfree(task->sighand);
    }
    
    /* Free the files structure */
    if (task->files != NULL) {
        if (task->files->close_on_exec != NULL) {
            kfree(task->files->close_on_exec);
        }
        
        if (task->files->fd_array != NULL) {
            /* Close all open files */
            for (u32 i = 0; i < task->files->max_fds; i++) {
                if (task->files->fd_array[i] != NULL) {
                    fs_close(task->files->fd_array[i]);
                }
            }
            
            kfree(task->files->fd_array);
        }
        
        kfree(task->files);
    }
    
    /* Free the memory structure */
    if (task->mm != NULL) {
        if (task->mm->context != NULL) {
            vmm_destroy_context(task->mm->context);
        }
        
        kfree(task->mm);
    }
    
    /* Free the process structure */
    kfree(task);
    
    return 0;
}

/* Execute a program */
int process_exec(task_struct_t *task, const char *path, char *const argv[], char *const envp[]) {
    if (task == NULL || path == NULL) {
        return -1;
    }
    
    /* Set the process state */
    task->state = TASK_RUNNING;
    task->in_execve = 1;
    
    /* Load the ELF file */
    Elf32_Addr entry;
    int result = elf_load(path, &entry);
    
    if (result < 0) {
        task->in_execve = 0;
        return -1;
    }
    
    /* Set the process name */
    const char *name = strrchr(path, '/');
    
    if (name != NULL) {
        name++;
    } else {
        name = path;
    }
    
    strncpy(task->comm, name, sizeof(task->comm) - 1);
    task->comm[sizeof(task->comm) - 1] = '\0';
    
    /* Set up the process stack */
    /* This would be implemented with actual stack setup */
    
    /* Set the process entry point */
    /* This would be implemented with actual entry point setup */
    
    /* Set the process as having exec'd */
    task->did_exec = 1;
    task->in_execve = 0;
    
    return 0;
}

/* Fork a process */
task_struct_t *process_fork(task_struct_t *parent) {
    if (parent == NULL) {
        return NULL;
    }
    
    /* Create a new process */
    task_struct_t *child = process_create(parent->comm);
    
    if (child == NULL) {
        return NULL;
    }
    
    /* Set the parent */
    child->parent = parent;
    child->real_parent = parent;
    child->ppid = parent->pid;
    
    /* Add to the parent's children */
    list_add_tail(&child->sibling, &parent->children);
    
    /* Copy the parent's memory */
    /* This would be implemented with actual memory copying */
    
    /* Copy the parent's files */
    /* This would be implemented with actual file copying */
    
    /* Copy the parent's signal handlers */
    /* This would be implemented with actual signal handler copying */
    
    /* Copy the parent's security context */
    security_task_create(parent->security, child->security);
    
    return child;
}

/* Wait for a process to exit */
int process_wait(task_struct_t *task, int *status) {
    if (task == NULL) {
        return -1;
    }
    
    /* Wait for the process to exit */
    /* This would be implemented with actual process waiting */
    
    /* Get the exit status */
    if (status != NULL) {
        *status = task->exit_code;
    }
    
    return 0;
}

/* Exit a process */
int process_exit(task_struct_t *task, int status) {
    if (task == NULL) {
        return -1;
    }
    
    /* Set the process state */
    task->state = TASK_ZOMBIE;
    task->exit_code = status;
    
    /* Close all files */
    if (task->files != NULL) {
        for (u32 i = 0; i < task->files->max_fds; i++) {
            if (task->files->fd_array[i] != NULL) {
                fs_close(task->files->fd_array[i]);
                task->files->fd_array[i] = NULL;
            }
        }
    }
    
    /* Free the process memory */
    /* This would be implemented with actual memory freeing */
    
    /* Signal the parent */
    if (task->parent != NULL) {
        process_signal(task->parent, task->exit_signal);
    }
    
    return 0;
}

/* Get the current process */
task_struct_t *process_current(void) {
    return current_task;
}

/* Get a process by ID */
task_struct_t *process_get(u32 pid) {
    /* Iterate over the process list */
    list_head_t *pos;
    
    list_for_each(pos, &process_list) {
        task_struct_t *task = list_entry(pos, task_struct_t, tasks);
        
        if (task->pid == pid) {
            return task;
        }
    }
    
    return NULL;
}

/* Set the state of a process */
int process_set_state(task_struct_t *task, u32 state) {
    if (task == NULL) {
        return -1;
    }
    
    task->state = state;
    return 0;
}

/* Set the name of a process */
int process_set_name(task_struct_t *task, const char *name) {
    if (task == NULL || name == NULL) {
        return -1;
    }
    
    strncpy(task->comm, name, sizeof(task->comm) - 1);
    task->comm[sizeof(task->comm) - 1] = '\0';
    
    return 0;
}

/* Add a file to a process */
int process_add_file(task_struct_t *task, file_t *file) {
    if (task == NULL || file == NULL) {
        return -1;
    }
    
    /* Check if the file descriptor array is full */
    if (task->files->next_fd >= task->files->max_fds) {
        /* Resize the file descriptor array */
        u32 new_max_fds = task->files->max_fds * 2;
        file_t **new_fd_array = kmalloc(sizeof(file_t *) * new_max_fds, MEM_KERNEL | MEM_ZERO);
        
        if (new_fd_array == NULL) {
            return -1;
        }
        
        /* Copy the old file descriptors */
        for (u32 i = 0; i < task->files->max_fds; i++) {
            new_fd_array[i] = task->files->fd_array[i];
        }
        
        /* Free the old file descriptor array */
        kfree(task->files->fd_array);
        
        /* Set the new file descriptor array */
        task->files->fd_array = new_fd_array;
        
        /* Resize the close on exec flags */
        u32 *new_close_on_exec = kmalloc(sizeof(u32) * ((new_max_fds + 31) / 32), MEM_KERNEL | MEM_ZERO);
        
        if (new_close_on_exec == NULL) {
            return -1;
        }
        
        /* Copy the old close on exec flags */
        for (u32 i = 0; i < (task->files->max_fds + 31) / 32; i++) {
            new_close_on_exec[i] = task->files->close_on_exec[i];
        }
        
        /* Free the old close on exec flags */
        kfree(task->files->close_on_exec);
        
        /* Set the new close on exec flags */
        task->files->close_on_exec = new_close_on_exec;
        
        /* Set the new maximum number of file descriptors */
        task->files->max_fds = new_max_fds;
    }
    
    /* Find the next free file descriptor */
    u32 fd = task->files->next_fd;
    
    /* Add the file to the file descriptor array */
    task->files->fd_array[fd] = file;
    
    /* Update the next free file descriptor */
    task->files->next_fd = fd + 1;
    
    return fd;
}

/* Remove a file from a process */
int process_remove_file(task_struct_t *task, u32 fd) {
    if (task == NULL || fd >= task->files->max_fds) {
        return -1;
    }
    
    /* Check if the file descriptor is valid */
    if (task->files->fd_array[fd] == NULL) {
        return -1;
    }
    
    /* Close the file */
    fs_close(task->files->fd_array[fd]);
    
    /* Remove the file from the file descriptor array */
    task->files->fd_array[fd] = NULL;
    
    /* Clear the close on exec flag */
    task->files->close_on_exec[fd / 32] &= ~(1 << (fd % 32));
    
    return 0;
}

/* Get a file from a process */
file_t *process_get_file(task_struct_t *task, u32 fd) {
    if (task == NULL || fd >= task->files->max_fds) {
        return NULL;
    }
    
    return task->files->fd_array[fd];
}

/* Send a signal to a process */
int process_signal(task_struct_t *task, u32 sig) {
    if (task == NULL || sig >= 64) {
        return -1;
    }
    
    /* Set the signal as pending */
    task->sighand->pending |= (1 << sig);
    
    return 0;
}

/* Send a signal to a process group */
int process_signal_group(task_struct_t *task, u32 sig) {
    if (task == NULL || sig >= 64) {
        return -1;
    }
    
    /* Get the process group leader */
    task_struct_t *leader = task->group_leader;
    
    if (leader == NULL) {
        leader = task;
    }
    
    /* Send the signal to all processes in the group */
    list_head_t *pos;
    
    list_for_each(pos, &process_list) {
        task_struct_t *t = list_entry(pos, task_struct_t, tasks);
        
        if (t->group_leader == leader) {
            process_signal(t, sig);
        }
    }
    
    return 0;
}

/* Send a signal to all processes */
int process_signal_all(u32 sig) {
    if (sig >= 64) {
        return -1;
    }
    
    /* Send the signal to all processes */
    list_head_t *pos;
    
    list_for_each(pos, &process_list) {
        task_struct_t *task = list_entry(pos, task_struct_t, tasks);
        
        process_signal(task, sig);
    }
    
    return 0;
}

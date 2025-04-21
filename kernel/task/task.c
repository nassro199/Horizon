/**
 * task.c - Horizon kernel task implementation
 *
 * This file contains the implementation of the task subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/mm/vmm.h>
#include <horizon/sched.h>
#include <horizon/string.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/spinlock.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of CPUs */
#define MAX_CPUS 8

/* Forward declarations */
struct file_operations {
    int (*release)(struct file *file);
};

/* Thread states */
#define THREAD_CREATED    0
#define THREAD_READY      1
#define THREAD_RUNNING    2
#define THREAD_WAITING    3
#define THREAD_BLOCKED    4
#define THREAD_EXITED     5

/* Thread flags */
#define THREAD_DETACHED   0x00000001

/* Thread structure */
typedef struct thread {
    u32 tid;                       /* Thread ID */
    task_struct_t *task;           /* Task */
    void *(*start_routine)(void *); /* Start routine */
    void *arg;                     /* Argument */
    void *retval;                  /* Return value */
    u32 flags;                     /* Flags */
    int state;                     /* State */
    struct thread *waiting_for;    /* Thread waiting for */
    struct list_head list;         /* List entry */
} thread_t;

/* Task list */
static list_head_t task_list;

/* Task lock */
static spinlock_t task_lock = SPIN_LOCK_INITIALIZER;

/* Next PID */
static u32 next_pid = 1;

/* Current task */
task_struct_t *current = NULL;

/* Init task */
task_struct_t init_task;

/* Idle tasks */
static task_struct_t *idle_tasks[MAX_CPUS];

/**
 * Get an idle task for a CPU
 *
 * @param cpu CPU number
 * @return Idle task for the CPU
 */
task_struct_t *idle_task(int cpu) {
    /* Check parameters */
    if (cpu < 0 || cpu >= MAX_CPUS) {
        return NULL;
    }

    /* Return the idle task */
    return idle_tasks[cpu];
}

/**
 * Initialize the task subsystem
 */
void task_init(void) {
    /* Initialize the task list */
    list_init(&task_list);

    /* Initialize the init task */
    memset(&init_task, 0, sizeof(task_struct_t));
    init_task.pid = 0;
    init_task.tgid = 0;
    init_task.ppid = 0;
    init_task.state = TASK_RUNNING;
    init_task.prio = 120;
    init_task.static_prio = 120;
    init_task.normal_prio = 120;
    init_task.policy = SCHED_NORMAL;
    init_task.parent = &init_task;
    init_task.real_parent = &init_task;
    list_init(&init_task.children);
    list_init(&init_task.sibling);
    init_task.group_leader = &init_task;
    list_init(&init_task.tasks);
    list_init(&init_task.thread_group);

    /* Add the init task to the task list */
    list_add(&init_task.tasks, &task_list);

    /* Set the current task */
    current = &init_task;

    /* Initialize the idle tasks */
    int i;
    for (i = 0; i < MAX_CPUS; i++) {
        /* Allocate an idle task */
        idle_tasks[i] = kmalloc(sizeof(task_struct_t), MEM_KERNEL | MEM_ZERO);

        if (idle_tasks[i] == NULL) {
            printk(KERN_ERR "TASK: Failed to allocate idle task for CPU %d\n", i);
            continue;
        }

        /* Initialize the idle task */
        idle_tasks[i]->pid = 0;
        idle_tasks[i]->tgid = 0;
        idle_tasks[i]->ppid = 0;
        idle_tasks[i]->state = TASK_RUNNING;
        idle_tasks[i]->prio = 140;
        idle_tasks[i]->static_prio = 140;
        idle_tasks[i]->normal_prio = 140;
        idle_tasks[i]->policy = SCHED_IDLE;
        idle_tasks[i]->parent = &init_task;
        idle_tasks[i]->real_parent = &init_task;
        list_init(&idle_tasks[i]->children);
        list_init(&idle_tasks[i]->sibling);
        idle_tasks[i]->group_leader = idle_tasks[i];
        list_init(&idle_tasks[i]->tasks);
        list_init(&idle_tasks[i]->thread_group);

        /* Set the CPU */
        idle_tasks[i]->cpu = i;
    }

    printk(KERN_INFO "TASK: Initialized task subsystem\n");
}

/**
 * Get a new PID
 *
 * @return New PID
 */
static u32 task_get_pid(void) {
    u32 pid;

    /* Lock the task list */
    spin_lock(&task_lock);

    /* Get the next PID */
    pid = next_pid++;

    /* Unlock the task list */
    spin_unlock(&task_lock);

    return pid;
}

/**
 * Create a new task
 *
 * @param name Task name
 * @return Pointer to the task, or NULL on failure
 */
task_struct_t *task_create(const char *name) {
    /* Allocate a task */
    task_struct_t *task = kmalloc(sizeof(task_struct_t), MEM_KERNEL | MEM_ZERO);

    if (task == NULL) {
        return NULL;
    }

    /* Initialize the task */
    task->pid = task_get_pid();
    task->tgid = task->pid;
    task->ppid = current->pid;
    task->state = TASK_RUNNING;
    task->prio = 120;
    task->static_prio = 120;
    task->normal_prio = 120;
    task->policy = SCHED_NORMAL;
    task->parent = current;
    task->real_parent = current;
    list_init(&task->children);
    list_init(&task->sibling);
    task->group_leader = task;
    list_init(&task->tasks);
    list_init(&task->thread_group);

    /* Set the task name */
    if (name != NULL) {
        strncpy(task->comm, name, 15);
        task->comm[15] = '\0';
    } else {
        strcpy(task->comm, "unnamed");
    }

    /* Add the task to the parent's children */
    list_add(&task->sibling, &current->children);

    /* Add the task to the task list */
    spin_lock(&task_lock);
    list_add(&task->tasks, &task_list);
    spin_unlock(&task_lock);

    printk(KERN_INFO "TASK: Created task '%s' (PID %d)\n", task->comm, task->pid);

    return task;
}

/**
 * Destroy a task
 *
 * @param task Task to destroy
 * @return 0 on success, negative error code on failure
 */
int task_destroy(task_struct_t *task) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }

    /* Check if the task is the current task */
    if (task == current) {
        return -EBUSY;
    }

    /* Check if the task is the init task */
    if (task == &init_task) {
        return -EPERM;
    }

    /* Check if the task is an idle task */
    int i;
    for (i = 0; i < MAX_CPUS; i++) {
        if (task == idle_tasks[i]) {
            return -EPERM;
        }
    }

    /* Remove the task from the task list */
    spin_lock(&task_lock);
    list_del(&task->tasks);
    spin_unlock(&task_lock);

    /* Remove the task from the parent's children */
    list_del(&task->sibling);

    /* Free the task's resources */
    if (task->mm != NULL) {
        vmm_destroy_mm(task->mm);
    }

    if (task->files != NULL) {
        kfree(task->files);
    }

    if (task->fs != NULL) {
        kfree(task->fs);
    }

    if (task->sighand != NULL) {
        kfree(task->sighand);
    }

    if (task->signal != NULL) {
        kfree(task->signal);
    }

    if (task->stack != NULL) {
        kfree(task->stack);
    }

    /* Free the task */
    kfree(task);

    return 0;
}

/**
 * Get the current task
 *
 * @return Pointer to the current task
 */
task_struct_t *task_current(void) {
    return current;
}

/**
 * Get a task by PID
 *
 * @param pid Process ID
 * @return Pointer to the task, or NULL if not found
 */
task_struct_t *task_get(u32 pid) {
    task_struct_t *task = NULL;

    /* Lock the task list */
    spin_lock(&task_lock);

    /* Find the task */
    list_for_each_entry(task, &task_list, tasks) {
        if (task->pid == pid) {
            /* Found the task */
            spin_unlock(&task_lock);
            return task;
        }
    }

    /* Unlock the task list */
    spin_unlock(&task_lock);

    return NULL;
}

/**
 * Set the state of a task
 *
 * @param task Task to set state for
 * @param state New state
 * @return 0 on success, negative error code on failure
 */
int task_set_state(task_struct_t *task, long state) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }

    /* Set the state */
    task->state = state;

    return 0;
}

/**
 * Set the name of a task
 *
 * @param task Task to set name for
 * @param name New name
 * @return 0 on success, negative error code on failure
 */
int task_set_name(task_struct_t *task, const char *name) {
    /* Check parameters */
    if (task == NULL || name == NULL) {
        return -EINVAL;
    }

    /* Set the name */
    strncpy(task->comm, name, 15);
    task->comm[15] = '\0';

    return 0;
}

/**
 * Add a file to a task
 *
 * @param task Task to add file to
 * @param file File to add
 * @return File descriptor on success, negative error code on failure
 */
int task_add_file(task_struct_t *task, file_t *file) {
    /* Check parameters */
    if (task == NULL || file == NULL) {
        return -EINVAL;
    }

    /* Check if the task has a files structure */
    if (task->files == NULL) {
        /* Allocate a files structure */
        task->files = kmalloc(sizeof(files_struct_t), MEM_KERNEL | MEM_ZERO);

        if (task->files == NULL) {
            return -ENOMEM;
        }

        /* Initialize the files structure */
        task->files->count = 1;
        task->files->max_fds = 16;
        task->files->next_fd = 0;

        /* Allocate the file descriptor array */
        task->files->fd_array = kmalloc(sizeof(file_t *) * task->files->max_fds, MEM_KERNEL | MEM_ZERO);

        if (task->files->fd_array == NULL) {
            kfree(task->files);
            task->files = NULL;
            return -ENOMEM;
        }

        /* Allocate the close on exec flags */
        task->files->close_on_exec = kmalloc(sizeof(unsigned int) * ((task->files->max_fds + 31) / 32), MEM_KERNEL | MEM_ZERO);

        if (task->files->close_on_exec == NULL) {
            kfree(task->files->fd_array);
            kfree(task->files);
            task->files = NULL;
            return -ENOMEM;
        }
    }

    /* Find a free file descriptor */
    int fd = task->files->next_fd;
    int i;

    for (i = 0; i < task->files->max_fds; i++) {
        if (task->files->fd_array[fd] == NULL) {
            /* Found a free file descriptor */
            break;
        }

        /* Try the next file descriptor */
        fd = (fd + 1) % task->files->max_fds;
    }

    /* Check if a free file descriptor was found */
    if (i == task->files->max_fds) {
        /* No free file descriptor found */
        return -EMFILE;
    }

    /* Set the next file descriptor */
    task->files->next_fd = (fd + 1) % task->files->max_fds;

    /* Add the file to the file descriptor array */
    task->files->fd_array[fd] = file;

    /* Increment the file reference count */
    file->f_count++;

    return fd;
}

/**
 * Remove a file from a task
 *
 * @param task Task to remove file from
 * @param fd File descriptor
 * @return 0 on success, negative error code on failure
 */
int task_remove_file(task_struct_t *task, unsigned int fd) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }

    /* Check if the task has a files structure */
    if (task->files == NULL) {
        return -EBADF;
    }

    /* Check if the file descriptor is valid */
    if (fd >= task->files->max_fds) {
        return -EBADF;
    }

    /* Check if the file descriptor is in use */
    if (task->files->fd_array[fd] == NULL) {
        return -EBADF;
    }

    /* Decrement the file reference count */
    task->files->fd_array[fd]->f_count--;

    /* Check if the file should be closed */
    if (task->files->fd_array[fd]->f_count == 0) {
        /* Close the file */
        if (task->files->fd_array[fd]->f_op != NULL && task->files->fd_array[fd]->f_op->release != NULL) {
            task->files->fd_array[fd]->f_op->release(task->files->fd_array[fd]);
        }

        /* Free the file */
        kfree(task->files->fd_array[fd]);
    }

    /* Remove the file from the file descriptor array */
    task->files->fd_array[fd] = NULL;

    /* Clear the close on exec flag */
    task->files->close_on_exec[fd / 32] &= ~(1 << (fd % 32));

    return 0;
}

/**
 * Get a file from a task
 *
 * @param task Task to get file from
 * @param fd File descriptor
 * @return Pointer to the file, or NULL on failure
 */
file_t *task_get_file(task_struct_t *task, unsigned int fd) {
    /* Check parameters */
    if (task == NULL) {
        return NULL;
    }

    /* Check if the task has a files structure */
    if (task->files == NULL) {
        return NULL;
    }

    /* Check if the file descriptor is valid */
    if (fd >= task->files->max_fds) {
        return NULL;
    }

    /* Return the file */
    return task->files->fd_array[fd];
}

/**
 * Send a signal to a task
 *
 * @param task Task to send signal to
 * @param sig Signal to send
 * @return 0 on success, negative error code on failure
 */
int task_signal(task_struct_t *task, int sig) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }

    /* Check if the signal is valid */
    if (sig < 1 || sig > 64) {
        return -EINVAL;
    }

    /* Check if the task is alive */
    if (task->state == TASK_ZOMBIE || task->state == TASK_DEAD) {
        return -ESRCH;
    }

    /* Check if the signal is blocked */
    if (task->blocked & (1ULL << (sig - 1))) {
        return 0;
    }

    /* Add the signal to the pending signals */
    task->pending.signal |= (1ULL << (sig - 1));

    /* Wake up the task if it's sleeping */
    if (task->state == TASK_INTERRUPTIBLE) {
        task->state = TASK_RUNNING;
    }

    return 0;
}

/**
 * Send a signal to a task group
 *
 * @param task Task in the group to send signal to
 * @param sig Signal to send
 * @return 0 on success, negative error code on failure
 */
int task_signal_group(task_struct_t *task, int sig) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }

    /* Check if the signal is valid */
    if (sig < 1 || sig > 64) {
        return -EINVAL;
    }

    /* Get the thread group leader */
    task_struct_t *leader = task->group_leader;

    /* Send the signal to all tasks in the thread group */
    task_struct_t *t;
    list_for_each_entry(t, &leader->thread_group, thread_group) {
        task_signal(t, sig);
    }

    /* Send the signal to the leader */
    task_signal(leader, sig);

    return 0;
}

/**
 * Send a signal to all tasks
 *
 * @param sig Signal to send
 * @return 0 on success, negative error code on failure
 */
int task_signal_all(int sig) {
    /* Check if the signal is valid */
    if (sig < 1 || sig > 64) {
        return -EINVAL;
    }

    /* Send the signal to all tasks */
    task_struct_t *task;
    list_for_each_entry(task, &task_list, tasks) {
        task_signal(task, sig);
    }

    return 0;
}

/**
 * Create a thread in a task
 *
 * @param task Task to create thread in
 * @param start_routine Thread start routine
 * @param arg Argument to pass to start routine
 * @param flags Thread flags
 * @return Pointer to the thread, or NULL on failure
 */
thread_t *task_create_thread(task_struct_t *task, void *(*start_routine)(void *), void *arg, u32 flags) {
    /* Check parameters */
    if (task == NULL || start_routine == NULL) {
        return NULL;
    }

    /* Allocate a thread */
    thread_t *thread = kmalloc(sizeof(thread_t), MEM_KERNEL | MEM_ZERO);

    if (thread == NULL) {
        return NULL;
    }

    /* Initialize the thread */
    thread->tid = task->thread_count++;
    thread->task = task;
    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->flags = flags;
    thread->state = THREAD_CREATED;
    list_init(&thread->list);

    /* Add the thread to the task */
    if (task->main_thread == NULL) {
        task->main_thread = thread;
    }

    list_add(&thread->list, &task->threads);

    return thread;
}



/**
 * Get the current thread of a task
 *
 * @param task Task to get current thread of
 * @return Pointer to the current thread, or NULL on failure
 */
thread_t *task_current_thread(task_struct_t *task) {
    /* Check parameters */
    if (task == NULL) {
        return NULL;
    }

    /* Return the main thread */
    return task->main_thread;
}

/**
 * Exit a thread in a task
 *
 * @param task Task containing the thread
 * @param thread Thread to exit
 * @param retval Return value
 * @return 0 on success, negative error code on failure
 */
int task_exit_thread(task_struct_t *task, thread_t *thread, void *retval) {
    /* Check parameters */
    if (task == NULL || thread == NULL) {
        return -EINVAL;
    }

    /* Check if the thread belongs to the task */
    if (thread->task != task) {
        return -EINVAL;
    }

    /* Set the thread state */
    thread->state = THREAD_EXITED;
    thread->retval = retval;

    /* Wake up any threads waiting for this thread */
    thread_t *t;
    list_for_each_entry(t, &task->threads, list) {
        if (t->state == THREAD_WAITING && t->waiting_for == thread) {
            t->state = THREAD_READY;
            t->waiting_for = NULL;
        }
    }

    /* Check if this is the main thread */
    if (thread == task->main_thread) {
        /* Exit the task */
        task_exit(task, (int)(long)retval);
    }
}

/**
 * Exit a task
 *
 * @param task Task to exit
 * @param status Exit status
 * @return 0 on success, negative error code on failure
 */
int task_exit(task_struct_t *task, int status) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }

    /* Set the exit code */
    task->exit_code = status;

    /* Set the task state */
    task->state = TASK_ZOMBIE;

    /* Wake up the parent */
    if (task->parent != NULL && task->parent->state == TASK_INTERRUPTIBLE) {
        task->parent->state = TASK_RUNNING;
    }

    /* Schedule another task */
    /* This would be implemented with actual task scheduling */

    return 0;
}

/**
 * Join a thread in a task
 *
 * @param task Task containing the thread
 * @param thread Thread to join
 * @param retval Pointer to store return value
 * @return 0 on success, negative error code on failure
 */
int task_join_thread(task_struct_t *task, thread_t *thread, void **retval) {
    /* Check parameters */
    if (task == NULL || thread == NULL) {
        return -EINVAL;
    }

    /* Check if the thread belongs to the task */
    if (thread->task != task) {
        return -EINVAL;
    }

    /* Check if the thread is joinable */
    if (thread->flags & THREAD_DETACHED) {
        return -EINVAL;
    }

    /* Get the current thread */
    thread_t *current_thread = task_current_thread(task);

    if (current_thread == NULL) {
        return -EINVAL;
    }

    /* Check if the thread is already exited */
    if (thread->state == THREAD_EXITED) {
        /* Get the return value */
        if (retval != NULL) {
            *retval = thread->retval;
        }

        /* Remove the thread from the task */
        list_del(&thread->list);

        /* Free the thread */
        kfree(thread);

        return 0;
    }

    /* Wait for the thread to exit */
    current_thread->state = THREAD_WAITING;
    current_thread->waiting_for = thread;

    /* Schedule another thread */
    /* This would be implemented with actual thread scheduling */

    /* Get the return value */
    if (retval != NULL) {
        *retval = thread->retval;
    }

    /* Remove the thread from the task */
    list_del(&thread->list);

    /* Free the thread */
    kfree(thread);

    return 0;
}

/**
 * Detach a thread in a task
 *
 * @param task Task containing the thread
 * @param thread Thread to detach
 * @return 0 on success, negative error code on failure
 */
int task_detach_thread(task_struct_t *task, thread_t *thread) {
    /* Check parameters */
    if (task == NULL || thread == NULL) {
        return -EINVAL;
    }

    /* Check if the thread belongs to the task */
    if (thread->task != task) {
        return -EINVAL;
    }

    /* Check if the thread is already detached */
    if (thread->flags & THREAD_DETACHED) {
        return 0;
    }

    /* Set the thread as detached */
    thread->flags |= THREAD_DETACHED;

    return 0;
}

/**
 * Cancel a thread in a task
 *
 * @param task Task containing the thread
 * @param thread Thread to cancel
 * @return 0 on success, negative error code on failure
 */
int task_cancel_thread(task_struct_t *task, thread_t *thread) {
    /* Check parameters */
    if (task == NULL || thread == NULL) {
        return -EINVAL;
    }

    /* Check if the thread belongs to the task */
    if (thread->task != task) {
        return -EINVAL;
    }

    /* Exit the thread */
    return task_exit_thread(task, thread, NULL);
}

/**
 * Get a thread by TID
 *
 * @param task Task containing the thread
 * @param tid Thread ID
 * @return Pointer to the thread, or NULL if not found
 */
thread_t *task_get_thread(task_struct_t *task, u32 tid) {
    /* Check parameters */
    if (task == NULL) {
        return NULL;
    }

    /* Find the thread */
    thread_t *thread;
    list_for_each_entry(thread, &task->threads, list) {
        if (thread->tid == tid) {
            return thread;
        }
    }

    return NULL;
}

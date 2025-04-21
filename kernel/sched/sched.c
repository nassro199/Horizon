/**
 * sched.c - Horizon kernel scheduler implementation
 *
 * This file contains the implementation of the scheduler subsystem.
 * The implementation is compatible with Linux.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/sched.h>
#include <horizon/sched/rt.h>
#include <horizon/sched/sched_domain.h>
#include <horizon/sched/load_balance.h>

/* Global run queue */
static run_queue_t run_queue;

/* Initialize the advanced scheduler */
void sched_init_advanced(void)
{
    /* Initialize the run queue */
    run_queue.nr_running = 0;
    run_queue.nr_switches = 0;
    run_queue.curr_timestamp = 0;
    run_queue.curr = NULL;
    run_queue.idle = NULL;
    run_queue.head = NULL;
    run_queue.tail = NULL;

    /* Create the idle task */
    task_struct_t *idle = sched_create_task("idle", NULL, PF_KTHREAD);

    if (idle == NULL) {
        kernel_panic("Failed to create idle task");
    }

    /* Set the idle task properties */
    idle->state = TASK_RUNNING;
    idle->prio = 0;
    idle->static_prio = 0;
    idle->normal_prio = 0;
    idle->policy = SCHED_IDLE;

    /* Set as the idle task */
    run_queue.idle = idle;

    /* Set as the current task */
    run_queue.curr = idle;

    /* Initialize the real-time scheduler */
    rt_init();

    /* Initialize the scheduler domains */
    sched_domain_init();

    /* Initialize the load balancing subsystem */
    load_balance_init();

    printk(KERN_INFO "SCHED: Initialized advanced scheduler\n");
}

/* Create a new task */
task_struct_t *sched_create_task(const char *name, void (*entry)(void), u32 flags)
{
    /* Allocate task structure */
    task_struct_t *task = kmalloc(sizeof(task_struct_t), MEM_KERNEL | MEM_ZERO);

    if (task == NULL) {
        return NULL;
    }

    /* Initialize task */
    task->state = TASK_RUNNING;
    task->flags = flags;
    task->pid = run_queue.nr_running + 1;
    task->tgid = task->pid;
    task->ppid = run_queue.curr ? run_queue.curr->pid : 0;

    /* Copy name */
    if (name != NULL) {
        strncpy(task->comm, name, 15);
        task->comm[15] = '\0';
    } else {
        strcpy(task->comm, "task");
    }

    /* Set scheduler properties */
    task->prio = 20;
    task->static_prio = 20;
    task->normal_prio = 20;
    task->policy = SCHED_NORMAL;
    task->time_slice = 100;
    task->exec_start = run_queue.curr_timestamp;
    task->sum_exec_runtime = 0;

    /* Allocate memory descriptor */
    task->mm = kmalloc(sizeof(mm_struct_t), MEM_KERNEL | MEM_ZERO);

    if (task->mm == NULL) {
        kfree(task);
        return NULL;
    }

    /* Allocate credentials */
    task->cred = kmalloc(sizeof(cred_t), MEM_KERNEL | MEM_ZERO);

    if (task->cred == NULL) {
        kfree(task->mm);
        kfree(task);
        return NULL;
    }

    /* Set up context */
    if (entry != NULL) {
        /* Set up the context for a new task */
        /* This would be implemented with actual context setup */
        task->context.eip = (u32)entry;
    }

    /* Initialize linked list pointers */
    task->parent = run_queue.curr;
    task->children = NULL;
    task->sibling = NULL;
    task->next = NULL;
    task->prev = NULL;

    /* Add to the run queue */
    sched_add_task(task);

    return task;
}

/* Destroy a task */
void sched_destroy_task(task_struct_t *task)
{
    if (task == NULL) {
        return;
    }

    /* Remove from the run queue */
    sched_remove_task(task);

    /* Free resources */
    if (task->mm != NULL) {
        kfree(task->mm);
    }

    if (task->cred != NULL) {
        kfree(task->cred);
    }

    /* Free the task structure */
    kfree(task);
}

/* Add a task to the run queue */
void sched_add_task(task_struct_t *task)
{
    if (task == NULL) {
        return;
    }

    /* Add to the run queue */
    if (run_queue.head == NULL) {
        /* Empty queue */
        run_queue.head = task;
        run_queue.tail = task;
        task->next = NULL;
        task->prev = NULL;
    } else {
        /* Add to the end of the queue */
        task->prev = run_queue.tail;
        task->next = NULL;
        run_queue.tail->next = task;
        run_queue.tail = task;
    }

    /* Increment the number of runnable processes */
    run_queue.nr_running++;
}

/* Remove a task from the run queue */
void sched_remove_task(task_struct_t *task)
{
    if (task == NULL) {
        return;
    }

    /* Remove from the run queue */
    if (task->prev != NULL) {
        task->prev->next = task->next;
    } else {
        /* Task is the head of the queue */
        run_queue.head = task->next;
    }

    if (task->next != NULL) {
        task->next->prev = task->prev;
    } else {
        /* Task is the tail of the queue */
        run_queue.tail = task->prev;
    }

    /* Decrement the number of runnable processes */
    run_queue.nr_running--;
}

/* Set the priority of a task */
void sched_set_priority(task_struct_t *task, int priority)
{
    if (task == NULL) {
        return;
    }

    /* Clamp the priority to valid range */
    if (priority < 0) {
        priority = 0;
    } else if (priority > 99) {
        priority = 99;
    }

    /* Set the priority */
    task->static_prio = priority;

    /* Recalculate the normal priority */
    if (task->policy == SCHED_NORMAL || task->policy == SCHED_BATCH || task->policy == SCHED_IDLE) {
        task->normal_prio = task->static_prio;
    } else {
        task->normal_prio = 99 - task->static_prio;
    }

    /* Set the current priority */
    task->prio = task->normal_prio;
}

/* Set the scheduling policy of a task */
void sched_set_policy(task_struct_t *task, u32 policy)
{
    if (task == NULL) {
        return;
    }

    /* Check if the policy is valid */
    if (policy != SCHED_NORMAL && policy != SCHED_FIFO &&
        policy != SCHED_RR && policy != SCHED_BATCH &&
        policy != SCHED_IDLE && policy != SCHED_DEADLINE) {
        return;
    }

    /* Set the policy */
    task->policy = policy;

    /* Recalculate the normal priority */
    if (task->policy == SCHED_NORMAL || task->policy == SCHED_BATCH || task->policy == SCHED_IDLE) {
        task->normal_prio = task->static_prio;
    } else {
        /* Real-time priority */
        task->normal_prio = rt_prio_base + (99 - task->static_prio);
    }

    /* Set the current priority */
    task->prio = task->normal_prio;

    /* If this is a real-time task, update the time slice */
    if (task->policy == SCHED_RR) {
        /* Round-robin tasks get a fixed time slice */
        task->time_slice = 100;
    } else if (task->policy == SCHED_FIFO) {
        /* FIFO tasks run until they yield or block */
        task->time_slice = UINT32_MAX;
    }
}

/* Yield the CPU */
void sched_yield_advanced(void)
{
    /* Schedule another task */
    sched_schedule();
}

/* Schedule a task */
void sched_schedule(void)
{
    task_struct_t *task;
    u64 current_jiffies = timer_get_jiffies();

    /* Check for tasks that need to be woken up */
    for (task = run_queue.head; task != NULL; task = task->next) {
        if (task->state == TASK_INTERRUPTIBLE && task->wake_time <= current_jiffies) {
            /* Wake up the task */
            task->state = TASK_RUNNING;
        }
    }

    /* Check if load balancing is needed */
    load_balance_run();

    /* Try to find a real-time task first */
    task_struct_t *next = NULL;

    /* Check for real-time tasks */
    for (task = run_queue.head; task != NULL; task = task->next) {
        if (task->state == TASK_RUNNING &&
            (task->policy == SCHED_FIFO || task->policy == SCHED_RR)) {
            /* Found a real-time task */
            next = task;
            break;
        }
    }

    /* If no real-time task is runnable, find a normal task */
    if (next == NULL) {
        for (task = run_queue.head; task != NULL; task = task->next) {
            if (task->state == TASK_RUNNING) {
                next = task;
                break;
            }
        }
    }

    /* If no task is runnable, use the idle task */
    if (next == NULL) {
        next = run_queue.idle;
    }

    /* Switch to the next task */
    if (next != run_queue.curr) {
        task_struct_t *prev = run_queue.curr;
        run_queue.curr = next;

        /* Increment the number of context switches */
        run_queue.nr_switches++;

        /* Update the timestamp */
        run_queue.curr_timestamp++;

        /* Update the execution time */
        prev->sum_exec_runtime++;

        /* Update real-time statistics if applicable */
        if (rt_is_realtime((thread_t *)next)) {
            rt_switch_count++;

            /* Check if this is a preemption */
            if (prev->state == TASK_RUNNING && rt_is_realtime((thread_t *)prev)) {
                rt_preempt_count++;
            }
        }

        /* Context switch would happen here */
        /* This would be implemented with actual context switching */
    }
}

/* Get the current task */
task_struct_t *sched_current_task(void)
{
    return run_queue.curr;
}

/* Sleep for a specified time */
void sched_sleep_advanced(u32 ms)
{
    /* Set the current task to sleep */
    run_queue.curr->state = TASK_INTERRUPTIBLE;
    run_queue.curr->wake_time = timer_get_jiffies() + timer_msecs_to_jiffies(ms);

    /* Schedule another task */
    sched_schedule();
}

/* Wake up a task */
void sched_wake_up(task_struct_t *task)
{
    if (task == NULL) {
        return;
    }

    /* Set the task to running */
    task->state = TASK_RUNNING;
}

/* Exit the current task */
void sched_exit_advanced(int status)
{
    if (run_queue.curr == NULL || run_queue.curr == run_queue.idle) {
        /* Can't exit the idle task */
        return;
    }

    /* Mark the task as a zombie */
    run_queue.curr->state = TASK_ZOMBIE;

    /* Schedule another task */
    sched_schedule();
}

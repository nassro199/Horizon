/**
 * sched.c - Process scheduler implementation
 * 
 * This file contains the implementation of the process scheduler.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/sched.h>
#include <horizon/mm.h>

/* Process list */
static process_t *process_list = NULL;
static process_t *current_process = NULL;
static u32 next_pid = 1;

/* Initialize the scheduler */
void sched_init(void) {
    /* Create the idle process */
    process_t *idle = kmalloc(sizeof(process_t), MEM_KERNEL | MEM_ZERO);
    
    if (idle == NULL) {
        kernel_panic("Failed to create idle process");
    }
    
    idle->pid = 0;
    idle->ppid = 0;
    idle->state = PROCESS_RUNNING;
    idle->priority = PROCESS_PRIO_MIN;
    
    /* Set as the current process */
    process_list = idle;
    current_process = idle;
}

/* Create a new process */
process_t *sched_create_process(const char *name, void (*entry)(void)) {
    /* Allocate process structure */
    process_t *proc = kmalloc(sizeof(process_t), MEM_KERNEL | MEM_ZERO);
    
    if (proc == NULL) {
        return NULL;
    }
    
    /* Initialize process */
    proc->pid = next_pid++;
    proc->ppid = current_process ? current_process->pid : 0;
    proc->state = PROCESS_RUNNING;
    proc->priority = PROCESS_PRIO_DEFAULT;
    
    /* Copy name */
    if (name != NULL) {
        u32 i;
        for (i = 0; i < 63 && name[i] != '\0'; i++) {
            proc->name[i] = name[i];
        }
        proc->name[i] = '\0';
    } else {
        proc->name[0] = 'P';
        proc->name[1] = 'r';
        proc->name[2] = 'o';
        proc->name[3] = 'c';
        proc->name[4] = '\0';
    }
    
    /* Allocate stack */
    proc->stack = kmalloc(PAGE_SIZE, MEM_KERNEL);
    
    if (proc->stack == NULL) {
        kfree(proc);
        return NULL;
    }
    
    /* Add to process list */
    proc->next = process_list;
    process_list = proc;
    
    return proc;
}

/* Yield the CPU */
void sched_yield(void) {
    /* Find the next runnable process */
    process_t *next = current_process->next;
    
    if (next == NULL) {
        next = process_list;
    }
    
    while (next != current_process) {
        if (next->state == PROCESS_RUNNING) {
            /* Found a runnable process */
            break;
        }
        
        next = next->next;
        
        if (next == NULL) {
            next = process_list;
        }
    }
    
    /* Switch to the next process */
    if (next != current_process) {
        process_t *prev = current_process;
        current_process = next;
        
        /* Context switch would happen here */
    }
}

/* Sleep for a specified time */
void sched_sleep(u32 ms) {
    /* This would be implemented with a timer */
    /* For now, just yield */
    sched_yield();
}

/* Exit the current process */
void sched_exit(int status) {
    if (current_process == NULL || current_process->pid == 0) {
        /* Can't exit the idle process */
        return;
    }
    
    /* Mark the process as zombie */
    current_process->state = PROCESS_ZOMBIE;
    
    /* Free resources */
    if (current_process->stack != NULL) {
        kfree(current_process->stack);
        current_process->stack = NULL;
    }
    
    /* Schedule another process */
    sched_yield();
}

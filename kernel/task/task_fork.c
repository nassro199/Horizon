/**
 * task_fork.c - Task forking
 *
 * This file contains the implementation of task forking.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/string.h>

/**
 * Fork a task
 * 
 * @param parent Parent task
 * @return Pointer to the new task, or NULL on failure
 */
task_struct_t *task_fork(task_struct_t *parent) {
    /* Check parameters */
    if (parent == NULL) {
        return NULL;
    }
    
    /* Not implemented yet */
    printk(KERN_WARNING "TASK: task_fork not implemented yet\n");
    
    return NULL;
}

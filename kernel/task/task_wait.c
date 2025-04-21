/**
 * task_wait.c - Task waiting
 *
 * This file contains the implementation of task waiting.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/string.h>

/**
 * Wait for a task to exit
 * 
 * @param task Task to wait for
 * @param status Pointer to store the exit status
 * @return 0 on success, negative error code on failure
 */
int task_wait(task_struct_t *task, int *status) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Not implemented yet */
    printk(KERN_WARNING "TASK: task_wait not implemented yet\n");
    
    return -ENOSYS;
}

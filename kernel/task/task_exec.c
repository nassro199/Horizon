/**
 * task_exec.c - Task execution
 *
 * This file contains the implementation of task execution.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/fs.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/string.h>

/**
 * Execute a program in a task
 * 
 * @param task Task to execute in
 * @param path Path to the program
 * @param argv Arguments
 * @param envp Environment
 * @return 0 on success, negative error code on failure
 */
int task_exec(task_struct_t *task, const char *path, char *const argv[], char *const envp[]) {
    /* Check parameters */
    if (task == NULL || path == NULL) {
        return -EINVAL;
    }
    
    /* Not implemented yet */
    printk(KERN_WARNING "TASK: task_exec not implemented yet\n");
    
    return -ENOSYS;
}

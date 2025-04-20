/**
 * ipc.c - Inter-Process Communication implementation
 *
 * This file contains the implementation of the inter-process communication subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/ipc.h>

/* External functions */
extern void msg_init(void);
extern void sem_init(void);
extern void shm_init(void);
extern void mqueue_init(void);
extern void ipc_mqueue_syscalls_init(void);

/* Initialize the IPC subsystem */
void ipc_init(void)
{
    /* Initialize the message queue subsystem */
    msg_init();

    /* Initialize the semaphore subsystem */
    sem_init();

    /* Initialize the shared memory subsystem */
    shm_init();

    /* Initialize the POSIX message queue subsystem */
    mqueue_init();

    /* Initialize the POSIX message queue system calls */
    ipc_mqueue_syscalls_init();
}

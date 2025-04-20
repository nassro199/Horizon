/**
 * shm.c - Shared memory implementation
 *
 * This file contains the implementation of the shared memory IPC mechanism.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/vmm.h>
#include <horizon/ipc.h>
#include <horizon/string.h>
#include <horizon/sched/sched.h>

/* Maximum number of shared memory segments */
#define MAX_SHM_SEGMENTS 32

/* Shared memory commands */
#define IPC_RMID    0   /* Remove identifier */
#define IPC_SET     1   /* Set options */
#define IPC_STAT    2   /* Get options */
#define IPC_INFO    3   /* Get info */

/* Shared memory flags */
#define SHM_RDONLY  0x1000  /* Read-only access */
#define SHM_RND     0x2000  /* Round attach address to SHMLBA */
#define SHM_REMAP   0x4000  /* Take-over region on attach */
#define SHM_EXEC    0x8000  /* Execution access */

/* Shared memory segment list */
static shm_segment_t *shm_segments = NULL;

/* Next shared memory segment ID */
static u32 next_shm_id = 1;

/* Initialize the shared memory subsystem */
void shm_init(void)
{
    /* Initialize the shared memory segment list */
    shm_segments = NULL;
    next_shm_id = 1;
}

/* Create or get a shared memory segment */
int shmget(u32 key, size_t size, int shmflg)
{
    /* Check if the segment already exists */
    shm_segment_t *segment = shm_segments;

    while (segment != NULL) {
        if (segment->id == key) {
            /* Segment exists */
            return segment->id;
        }

        segment = segment->next;
    }

    /* Segment doesn't exist */
    if (!(shmflg & IPC_CREAT)) {
        /* Don't create a new segment */
        return -1;
    }

    /* Create a new segment */
    segment = kmalloc(sizeof(shm_segment_t), MEM_KERNEL | MEM_ZERO);

    if (segment == NULL) {
        return -1;
    }

    /* Allocate memory for the segment */
    segment->addr = kmalloc(size, MEM_KERNEL | MEM_ZERO);

    if (segment->addr == NULL) {
        kfree(segment);
        return -1;
    }

    /* Initialize the segment */
    segment->id = next_shm_id++;
    segment->owner = sched_current_task()->pid;
    segment->permissions = shmflg & 0777;
    segment->size = size;
    segment->attachments = 0;

    /* Add to the segment list */
    segment->next = shm_segments;
    shm_segments = segment;

    return segment->id;
}

/* Attach a shared memory segment */
void *shmat(int shmid, const void *shmaddr, int shmflg)
{
    /* Find the segment */
    shm_segment_t *segment = shm_segments;

    while (segment != NULL) {
        if (segment->id == shmid) {
            break;
        }

        segment = segment->next;
    }

    if (segment == NULL) {
        /* Segment doesn't exist */
        return (void *)-1;
    }

    /* Get the current task */
    task_struct_t *task = sched_current_task();

    if (task == NULL) {
        return (void *)-1;
    }

    /* Get the task's memory context */
    vm_context_t *context = task->mm->context;

    if (context == NULL) {
        return (void *)-1;
    }

    /* Determine the attach address */
    void *addr;

    if (shmaddr != NULL) {
        /* Use the specified address */
        addr = (void *)shmaddr;
    } else {
        /* Find a suitable address */
        /* This would be implemented with proper address space management */
        /* For now, just use a fixed address */
        addr = (void *)0x40000000;
    }

    /* Map the segment into the task's address space */
    u32 flags = PTE_PRESENT | PTE_USER;

    if (!(shmflg & SHM_RDONLY)) {
        flags |= PTE_WRITE;
    }

    /* Map each page of the segment */
    for (u32 i = 0; i < segment->size; i += PAGE_SIZE) {
        void *phys = vmm_get_phys_addr(kernel_context, (void *)((u32)segment->addr + i));

        if (phys == NULL) {
            /* Failed to get physical address */
            /* Unmap the pages that were mapped */
            for (u32 j = 0; j < i; j += PAGE_SIZE) {
                vmm_unmap_page(context, (void *)((u32)addr + j));
            }

            return (void *)-1;
        }

        if (vmm_map_page(context, (void *)((u32)addr + i), phys, flags) < 0) {
            /* Failed to map a page */
            /* Unmap the pages that were mapped */
            for (u32 j = 0; j < i; j += PAGE_SIZE) {
                vmm_unmap_page(context, (void *)((u32)addr + j));
            }

            return (void *)-1;
        }
    }

    /* Increment the attachment count */
    segment->attachments++;

    return addr;
}

/* Detach a shared memory segment */
int shmdt(const void *shmaddr)
{
    /* Get the current task */
    task_struct_t *task = sched_current_task();

    if (task == NULL) {
        return -1;
    }

    /* Get the task's memory context */
    vm_context_t *context = task->mm->context;

    if (context == NULL) {
        return -1;
    }

    /* Find the segment */
    shm_segment_t *segment = shm_segments;

    while (segment != NULL) {
        /* Check if this segment is attached at the specified address */
        /* This would be implemented with proper address space management */
        /* For now, just assume it's the right segment */
        break;

        segment = segment->next;
    }

    if (segment == NULL) {
        /* Segment not found */
        return -1;
    }

    /* Unmap the segment from the task's address space */
    for (u32 i = 0; i < segment->size; i += PAGE_SIZE) {
        vmm_unmap_page(context, (void *)((u32)shmaddr + i));
    }

    /* Decrement the attachment count */
    segment->attachments--;

    return 0;
}

/* Control a shared memory segment */
int shmctl(int shmid, int cmd, void *buf)
{
    /* Find the segment */
    shm_segment_t *segment = shm_segments;
    shm_segment_t *prev = NULL;

    while (segment != NULL) {
        if (segment->id == shmid) {
            break;
        }

        prev = segment;
        segment = segment->next;
    }

    if (segment == NULL) {
        /* Segment doesn't exist */
        return -1;
    }

    /* Process the command */
    switch (cmd) {
        case IPC_RMID:
            /* Remove the segment */
            if (prev == NULL) {
                /* Segment is the first in the list */
                shm_segments = segment->next;
            } else {
                /* Segment is not the first in the list */
                prev->next = segment->next;
            }

            /* Free the segment memory */
            if (segment->addr != NULL) {
                kfree(segment->addr);
            }

            /* Free the segment */
            kfree(segment);

            return 0;

        case IPC_SET:
            /* Set segment attributes */
            /* This would be implemented with proper attribute setting */
            /* For now, just return success */
            return 0;

        case IPC_STAT:
            /* Get segment attributes */
            /* This would be implemented with proper attribute getting */
            /* For now, just return success */
            return 0;

        default:
            /* Unknown command */
            return -1;
    }
}

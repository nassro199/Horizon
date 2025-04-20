/**
 * mpolicy.c - Horizon kernel memory policy implementation
 * 
 * This file contains the implementation of memory policy operations.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Memory policy modes */
#define MPOL_DEFAULT     0
#define MPOL_PREFERRED   1
#define MPOL_BIND        2
#define MPOL_INTERLEAVE  3
#define MPOL_LOCAL       4
#define MPOL_MAX         5

/* Memory policy flags */
#define MPOL_F_STATIC_NODES     (1 << 15)
#define MPOL_F_RELATIVE_NODES   (1 << 14)
#define MPOL_F_NUMA_BALANCING   (1 << 13)

/* Maximum number of NUMA nodes */
#define MAX_NUMNODES 64

/* Memory policy structure */
struct mempolicy {
    int mode;                   /* Policy mode */
    unsigned long flags;        /* Policy flags */
    unsigned long *nodemask;    /* Node mask */
    unsigned int nodemask_size; /* Node mask size */
};

/**
 * Set the memory policy for the calling thread
 * 
 * @param mode The policy mode
 * @param nodemask The node mask
 * @param maxnode The maximum node
 * @return 0 on success, or a negative error code
 */
int mm_set_mempolicy(int mode, const unsigned long *nodemask, unsigned long maxnode) {
    /* Check parameters */
    if (mode < 0 || mode >= MPOL_MAX) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Check if the task has a memory policy */
    if (task->mm->mempolicy == NULL) {
        /* Allocate a new memory policy */
        task->mm->mempolicy = kmalloc(sizeof(struct mempolicy), MEM_KERNEL | MEM_ZERO);
        
        if (task->mm->mempolicy == NULL) {
            return -1;
        }
    }
    
    /* Set the policy mode */
    task->mm->mempolicy->mode = mode;
    
    /* Set the node mask */
    if (nodemask != NULL && maxnode > 0) {
        /* Calculate the node mask size */
        unsigned int nodemask_size = (maxnode + BITS_PER_LONG - 1) / BITS_PER_LONG;
        
        /* Check if the node mask size is valid */
        if (nodemask_size > MAX_NUMNODES / BITS_PER_LONG) {
            return -1;
        }
        
        /* Allocate the node mask */
        if (task->mm->mempolicy->nodemask == NULL) {
            task->mm->mempolicy->nodemask = kmalloc(nodemask_size * sizeof(unsigned long), MEM_KERNEL | MEM_ZERO);
            
            if (task->mm->mempolicy->nodemask == NULL) {
                return -1;
            }
        } else if (task->mm->mempolicy->nodemask_size < nodemask_size) {
            /* Reallocate the node mask */
            unsigned long *new_nodemask = kmalloc(nodemask_size * sizeof(unsigned long), MEM_KERNEL | MEM_ZERO);
            
            if (new_nodemask == NULL) {
                return -1;
            }
            
            /* Copy the old node mask */
            memcpy(new_nodemask, task->mm->mempolicy->nodemask, task->mm->mempolicy->nodemask_size * sizeof(unsigned long));
            
            /* Free the old node mask */
            kfree(task->mm->mempolicy->nodemask);
            
            /* Set the new node mask */
            task->mm->mempolicy->nodemask = new_nodemask;
        }
        
        /* Copy the node mask */
        memcpy(task->mm->mempolicy->nodemask, nodemask, nodemask_size * sizeof(unsigned long));
        
        /* Set the node mask size */
        task->mm->mempolicy->nodemask_size = nodemask_size;
    } else {
        /* Free the node mask */
        if (task->mm->mempolicy->nodemask != NULL) {
            kfree(task->mm->mempolicy->nodemask);
            task->mm->mempolicy->nodemask = NULL;
        }
        
        /* Set the node mask size */
        task->mm->mempolicy->nodemask_size = 0;
    }
    
    return 0;
}

/**
 * Get the memory policy for the calling thread
 * 
 * @param policy The policy mode
 * @param nodemask The node mask
 * @param maxnode The maximum node
 * @param flags The flags
 * @return 0 on success, or a negative error code
 */
int mm_get_mempolicy(int *policy, unsigned long *nodemask, unsigned long maxnode, unsigned long addr, unsigned long flags) {
    /* Check parameters */
    if (policy == NULL) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Check if the task has a memory policy */
    if (task->mm->mempolicy == NULL) {
        /* Default policy */
        *policy = MPOL_DEFAULT;
        
        /* Clear the node mask */
        if (nodemask != NULL && maxnode > 0) {
            memset(nodemask, 0, ((maxnode + BITS_PER_LONG - 1) / BITS_PER_LONG) * sizeof(unsigned long));
        }
        
        return 0;
    }
    
    /* Check if we need to get the policy for a specific address */
    if (flags & MPOL_F_ADDR) {
        /* Get the VMA for the address */
        struct vm_area_struct *vma = mm_find_vma(task->mm, addr);
        
        if (vma == NULL) {
            return -1;
        }
        
        /* Check if the VMA has a memory policy */
        if (vma->vm_policy == NULL) {
            /* Use the task's memory policy */
            *policy = task->mm->mempolicy->mode;
            
            /* Copy the node mask */
            if (nodemask != NULL && maxnode > 0) {
                /* Calculate the node mask size */
                unsigned int nodemask_size = (maxnode + BITS_PER_LONG - 1) / BITS_PER_LONG;
                
                /* Check if the node mask size is valid */
                if (nodemask_size > MAX_NUMNODES / BITS_PER_LONG) {
                    return -1;
                }
                
                /* Copy the node mask */
                if (task->mm->mempolicy->nodemask != NULL) {
                    memcpy(nodemask, task->mm->mempolicy->nodemask, MIN(nodemask_size, task->mm->mempolicy->nodemask_size) * sizeof(unsigned long));
                } else {
                    memset(nodemask, 0, nodemask_size * sizeof(unsigned long));
                }
            }
        } else {
            /* Use the VMA's memory policy */
            *policy = vma->vm_policy->mode;
            
            /* Copy the node mask */
            if (nodemask != NULL && maxnode > 0) {
                /* Calculate the node mask size */
                unsigned int nodemask_size = (maxnode + BITS_PER_LONG - 1) / BITS_PER_LONG;
                
                /* Check if the node mask size is valid */
                if (nodemask_size > MAX_NUMNODES / BITS_PER_LONG) {
                    return -1;
                }
                
                /* Copy the node mask */
                if (vma->vm_policy->nodemask != NULL) {
                    memcpy(nodemask, vma->vm_policy->nodemask, MIN(nodemask_size, vma->vm_policy->nodemask_size) * sizeof(unsigned long));
                } else {
                    memset(nodemask, 0, nodemask_size * sizeof(unsigned long));
                }
            }
        }
    } else {
        /* Use the task's memory policy */
        *policy = task->mm->mempolicy->mode;
        
        /* Copy the node mask */
        if (nodemask != NULL && maxnode > 0) {
            /* Calculate the node mask size */
            unsigned int nodemask_size = (maxnode + BITS_PER_LONG - 1) / BITS_PER_LONG;
            
            /* Check if the node mask size is valid */
            if (nodemask_size > MAX_NUMNODES / BITS_PER_LONG) {
                return -1;
            }
            
            /* Copy the node mask */
            if (task->mm->mempolicy->nodemask != NULL) {
                memcpy(nodemask, task->mm->mempolicy->nodemask, MIN(nodemask_size, task->mm->mempolicy->nodemask_size) * sizeof(unsigned long));
            } else {
                memset(nodemask, 0, nodemask_size * sizeof(unsigned long));
            }
        }
    }
    
    return 0;
}

/**
 * Set the memory policy for a memory range
 * 
 * @param start The start address
 * @param len The length
 * @param mode The policy mode
 * @param nodemask The node mask
 * @param maxnode The maximum node
 * @param flags The flags
 * @return 0 on success, or a negative error code
 */
int mm_mbind(void *start, unsigned long len, int mode, const unsigned long *nodemask, unsigned long maxnode, unsigned flags) {
    /* Check parameters */
    if (start == NULL || len == 0) {
        return -1;
    }
    
    /* Check the policy mode */
    if (mode < 0 || mode >= MPOL_MAX) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Get the VMA for the address range */
    struct vm_area_struct *vma = mm_find_vma(task->mm, (unsigned long)start);
    
    if (vma == NULL) {
        return -1;
    }
    
    /* Check if the address range is within the VMA */
    if ((unsigned long)start + len > vma->vm_end) {
        return -1;
    }
    
    /* Check if the VMA has a memory policy */
    if (vma->vm_policy == NULL) {
        /* Allocate a new memory policy */
        vma->vm_policy = kmalloc(sizeof(struct mempolicy), MEM_KERNEL | MEM_ZERO);
        
        if (vma->vm_policy == NULL) {
            return -1;
        }
    }
    
    /* Set the policy mode */
    vma->vm_policy->mode = mode;
    
    /* Set the node mask */
    if (nodemask != NULL && maxnode > 0) {
        /* Calculate the node mask size */
        unsigned int nodemask_size = (maxnode + BITS_PER_LONG - 1) / BITS_PER_LONG;
        
        /* Check if the node mask size is valid */
        if (nodemask_size > MAX_NUMNODES / BITS_PER_LONG) {
            return -1;
        }
        
        /* Allocate the node mask */
        if (vma->vm_policy->nodemask == NULL) {
            vma->vm_policy->nodemask = kmalloc(nodemask_size * sizeof(unsigned long), MEM_KERNEL | MEM_ZERO);
            
            if (vma->vm_policy->nodemask == NULL) {
                return -1;
            }
        } else if (vma->vm_policy->nodemask_size < nodemask_size) {
            /* Reallocate the node mask */
            unsigned long *new_nodemask = kmalloc(nodemask_size * sizeof(unsigned long), MEM_KERNEL | MEM_ZERO);
            
            if (new_nodemask == NULL) {
                return -1;
            }
            
            /* Copy the old node mask */
            memcpy(new_nodemask, vma->vm_policy->nodemask, vma->vm_policy->nodemask_size * sizeof(unsigned long));
            
            /* Free the old node mask */
            kfree(vma->vm_policy->nodemask);
            
            /* Set the new node mask */
            vma->vm_policy->nodemask = new_nodemask;
        }
        
        /* Copy the node mask */
        memcpy(vma->vm_policy->nodemask, nodemask, nodemask_size * sizeof(unsigned long));
        
        /* Set the node mask size */
        vma->vm_policy->nodemask_size = nodemask_size;
    } else {
        /* Free the node mask */
        if (vma->vm_policy->nodemask != NULL) {
            kfree(vma->vm_policy->nodemask);
            vma->vm_policy->nodemask = NULL;
        }
        
        /* Set the node mask size */
        vma->vm_policy->nodemask_size = 0;
    }
    
    return 0;
}

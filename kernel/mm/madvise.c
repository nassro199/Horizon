/**
 * madvise.c - Horizon kernel memory advising implementation
 * 
 * This file contains the implementation of memory advising operations.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Memory advice values */
#define MADV_NORMAL       0     /* No special treatment */
#define MADV_RANDOM       1     /* Expect random page references */
#define MADV_SEQUENTIAL   2     /* Expect sequential page references */
#define MADV_WILLNEED     3     /* Will need these pages */
#define MADV_DONTNEED     4     /* Don't need these pages */
#define MADV_FREE         8     /* Free pages only if memory pressure */
#define MADV_REMOVE       9     /* Remove these pages and resources */
#define MADV_DONTFORK     10    /* Don't inherit across fork */
#define MADV_DOFORK       11    /* Do inherit across fork */
#define MADV_HWPOISON     100   /* Poison a page for testing */
#define MADV_SOFT_OFFLINE 101   /* Soft offline page for testing */
#define MADV_MERGEABLE    12    /* KSM may merge identical pages */
#define MADV_UNMERGEABLE  13    /* KSM may not merge identical pages */
#define MADV_HUGEPAGE     14    /* Worth backing with hugepages */
#define MADV_NOHUGEPAGE   15    /* Not worth backing with hugepages */
#define MADV_DONTDUMP     16    /* Exclude from core dump */
#define MADV_DODUMP       17    /* Include in core dump */
#define MADV_WIPEONFORK   18    /* Zero memory on fork */
#define MADV_KEEPONFORK   19    /* Keep memory on fork */

/**
 * Give advice about use of memory
 * 
 * @param start The start address
 * @param len The length
 * @param advice The advice
 * @return 0 on success, or a negative error code
 */
int mm_madvise(void *start, size_t len, int advice) {
    /* Check parameters */
    if (start == NULL || len == 0) {
        return -1;
    }
    
    /* Check alignment */
    if ((unsigned long)start % PAGE_SIZE != 0) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Get the memory region */
    struct vm_area_struct *vma = mm_find_vma(task->mm, (unsigned long)start);
    
    if (vma == NULL) {
        return -1;
    }
    
    /* Check if the region is within the VMA */
    if ((unsigned long)start + len > vma->vm_end) {
        return -1;
    }
    
    /* Process the advice */
    switch (advice) {
        case MADV_NORMAL:
        case MADV_RANDOM:
        case MADV_SEQUENTIAL:
            /* Update the VMA flags */
            vma->vm_flags &= ~(VM_RAND_READ | VM_SEQ_READ);
            
            if (advice == MADV_RANDOM) {
                vma->vm_flags |= VM_RAND_READ;
            } else if (advice == MADV_SEQUENTIAL) {
                vma->vm_flags |= VM_SEQ_READ;
            }
            break;
        
        case MADV_WILLNEED:
            /* Prefetch the pages */
            mm_prefetch_pages(task->mm, (unsigned long)start, len);
            break;
        
        case MADV_DONTNEED:
            /* Release the pages */
            mm_release_pages(task->mm, (unsigned long)start, len);
            break;
        
        case MADV_FREE:
            /* Mark the pages as free */
            mm_mark_pages_free(task->mm, (unsigned long)start, len);
            break;
        
        case MADV_REMOVE:
            /* Remove the pages */
            mm_remove_pages(task->mm, (unsigned long)start, len);
            break;
        
        case MADV_DONTFORK:
            /* Don't inherit across fork */
            vma->vm_flags |= VM_DONTCOPY;
            break;
        
        case MADV_DOFORK:
            /* Do inherit across fork */
            vma->vm_flags &= ~VM_DONTCOPY;
            break;
        
        case MADV_MERGEABLE:
            /* KSM may merge identical pages */
            vma->vm_flags |= VM_MERGEABLE;
            break;
        
        case MADV_UNMERGEABLE:
            /* KSM may not merge identical pages */
            vma->vm_flags &= ~VM_MERGEABLE;
            break;
        
        case MADV_HUGEPAGE:
            /* Worth backing with hugepages */
            vma->vm_flags |= VM_HUGEPAGE;
            break;
        
        case MADV_NOHUGEPAGE:
            /* Not worth backing with hugepages */
            vma->vm_flags &= ~VM_HUGEPAGE;
            break;
        
        case MADV_DONTDUMP:
            /* Exclude from core dump */
            vma->vm_flags |= VM_DONTDUMP;
            break;
        
        case MADV_DODUMP:
            /* Include in core dump */
            vma->vm_flags &= ~VM_DONTDUMP;
            break;
        
        case MADV_WIPEONFORK:
            /* Zero memory on fork */
            vma->vm_flags |= VM_WIPEONFORK;
            break;
        
        case MADV_KEEPONFORK:
            /* Keep memory on fork */
            vma->vm_flags &= ~VM_WIPEONFORK;
            break;
        
        default:
            return -1;
    }
    
    return 0;
}

/**
 * Prefetch pages
 * 
 * @param mm The memory descriptor
 * @param start The start address
 * @param len The length
 * @return 0 on success, or a negative error code
 */
int mm_prefetch_pages(struct mm_struct *mm, unsigned long start, size_t len) {
    /* Calculate the number of pages */
    unsigned long num_pages = (len + PAGE_SIZE - 1) / PAGE_SIZE;
    
    /* Prefetch the pages */
    for (unsigned long i = 0; i < num_pages; i++) {
        unsigned long addr = start + i * PAGE_SIZE;
        
        /* Get the page */
        struct page *page = mm_get_page(mm, addr);
        
        /* If the page is not present, allocate it */
        if (page == NULL) {
            page = mm_alloc_page(mm, addr);
            
            if (page == NULL) {
                return -1;
            }
        }
    }
    
    return 0;
}

/**
 * Release pages
 * 
 * @param mm The memory descriptor
 * @param start The start address
 * @param len The length
 * @return 0 on success, or a negative error code
 */
int mm_release_pages(struct mm_struct *mm, unsigned long start, size_t len) {
    /* Calculate the number of pages */
    unsigned long num_pages = (len + PAGE_SIZE - 1) / PAGE_SIZE;
    
    /* Release the pages */
    for (unsigned long i = 0; i < num_pages; i++) {
        unsigned long addr = start + i * PAGE_SIZE;
        
        /* Get the page */
        struct page *page = mm_get_page(mm, addr);
        
        /* If the page is present, release it */
        if (page != NULL) {
            mm_release_page(mm, addr);
        }
    }
    
    return 0;
}

/**
 * Mark pages as free
 * 
 * @param mm The memory descriptor
 * @param start The start address
 * @param len The length
 * @return 0 on success, or a negative error code
 */
int mm_mark_pages_free(struct mm_struct *mm, unsigned long start, size_t len) {
    /* Calculate the number of pages */
    unsigned long num_pages = (len + PAGE_SIZE - 1) / PAGE_SIZE;
    
    /* Mark the pages as free */
    for (unsigned long i = 0; i < num_pages; i++) {
        unsigned long addr = start + i * PAGE_SIZE;
        
        /* Get the page */
        struct page *page = mm_get_page(mm, addr);
        
        /* If the page is present, mark it as free */
        if (page != NULL) {
            mm_mark_page_free(mm, addr);
        }
    }
    
    return 0;
}

/**
 * Remove pages
 * 
 * @param mm The memory descriptor
 * @param start The start address
 * @param len The length
 * @return 0 on success, or a negative error code
 */
int mm_remove_pages(struct mm_struct *mm, unsigned long start, size_t len) {
    /* Calculate the number of pages */
    unsigned long num_pages = (len + PAGE_SIZE - 1) / PAGE_SIZE;
    
    /* Remove the pages */
    for (unsigned long i = 0; i < num_pages; i++) {
        unsigned long addr = start + i * PAGE_SIZE;
        
        /* Get the page */
        struct page *page = mm_get_page(mm, addr);
        
        /* If the page is present, remove it */
        if (page != NULL) {
            mm_remove_page(mm, addr);
        }
    }
    
    return 0;
}

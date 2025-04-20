/**
 * mprotect.c - Horizon kernel memory protection implementation
 * 
 * This file contains the implementation of memory protection.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/mprotect.h>
#include <horizon/mm/vmm.h>
#include <horizon/spinlock.h>
#include <horizon/errno.h>

/* Memory protection lock */
static spinlock_t mprotect_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize memory protection
 * 
 * @return 0 on success, negative error code on failure
 */
int mprotect_init(void) {
    /* Initialize lock */
    spin_lock_init(&mprotect_lock, "mprotect");
    
    return 0;
}

/**
 * Check memory protection
 * 
 * @param addr Address to check
 * @param len Length to check
 * @param prot Protection flags
 * @return 0 on success, negative error code on failure
 */
int mprotect_check(void *addr, size_t len, unsigned int prot) {
    vm_context_t *context;
    vm_area_struct_t *vma;
    unsigned long start, end;
    
    /* Check parameters */
    if (addr == NULL || len == 0) {
        return -EINVAL;
    }
    
    /* Align address and length */
    start = (unsigned long)addr & PAGE_MASK;
    end = ((unsigned long)addr + len + PAGE_SIZE - 1) & PAGE_MASK;
    
    /* Get current context */
    context = vmm_get_current_context();
    if (context == NULL) {
        return -EINVAL;
    }
    
    /* Check protection */
    spin_lock(&mprotect_lock);
    
    /* Find VMA for address */
    vma = vmm_find_vma(context, (void *)start);
    if (vma == NULL || (unsigned long)vma->vm_start > start) {
        spin_unlock(&mprotect_lock);
        return -ENOMEM;
    }
    
    /* Check if VMA covers the entire range */
    if ((unsigned long)vma->vm_end < end) {
        spin_unlock(&mprotect_lock);
        return -ENOMEM;
    }
    
    /* Check protection */
    if ((vma->vm_flags & prot) != prot) {
        spin_unlock(&mprotect_lock);
        return -EACCES;
    }
    
    spin_unlock(&mprotect_lock);
    
    return 0;
}

/**
 * Set memory protection
 * 
 * @param addr Address to protect
 * @param len Length to protect
 * @param prot Protection flags
 * @return 0 on success, negative error code on failure
 */
int mprotect_set(void *addr, size_t len, unsigned int prot) {
    vm_context_t *context;
    vm_area_struct_t *vma;
    unsigned long start, end;
    
    /* Check parameters */
    if (addr == NULL || len == 0) {
        return -EINVAL;
    }
    
    /* Align address and length */
    start = (unsigned long)addr & PAGE_MASK;
    end = ((unsigned long)addr + len + PAGE_SIZE - 1) & PAGE_MASK;
    
    /* Get current context */
    context = vmm_get_current_context();
    if (context == NULL) {
        return -EINVAL;
    }
    
    /* Set protection */
    spin_lock(&mprotect_lock);
    
    /* Find VMA for address */
    vma = vmm_find_vma(context, (void *)start);
    if (vma == NULL || (unsigned long)vma->vm_start > start) {
        spin_unlock(&mprotect_lock);
        return -ENOMEM;
    }
    
    /* Check if VMA covers the entire range */
    if ((unsigned long)vma->vm_end < end) {
        spin_unlock(&mprotect_lock);
        return -ENOMEM;
    }
    
    /* Set protection */
    vma->vm_flags = (vma->vm_flags & ~(PROT_READ | PROT_WRITE | PROT_EXEC)) | prot;
    
    /* Update page tables */
    vmm_update_protection(context, (void *)start, (void *)end, vma->vm_page_prot);
    
    spin_unlock(&mprotect_lock);
    
    return 0;
}

/**
 * Get memory protection
 * 
 * @param addr Address to get protection for
 * @param prot Pointer to store protection flags
 * @return 0 on success, negative error code on failure
 */
int mprotect_get(void *addr, unsigned int *prot) {
    vm_context_t *context;
    vm_area_struct_t *vma;
    
    /* Check parameters */
    if (addr == NULL || prot == NULL) {
        return -EINVAL;
    }
    
    /* Get current context */
    context = vmm_get_current_context();
    if (context == NULL) {
        return -EINVAL;
    }
    
    /* Get protection */
    spin_lock(&mprotect_lock);
    
    /* Find VMA for address */
    vma = vmm_find_vma(context, addr);
    if (vma == NULL || (unsigned long)vma->vm_start > (unsigned long)addr) {
        spin_unlock(&mprotect_lock);
        return -ENOMEM;
    }
    
    /* Get protection */
    *prot = vma->vm_flags & (PROT_READ | PROT_WRITE | PROT_EXEC);
    
    spin_unlock(&mprotect_lock);
    
    return 0;
}

/**
 * Lock memory
 * 
 * @param addr Address to lock
 * @param len Length to lock
 * @return 0 on success, negative error code on failure
 */
int mprotect_lock(void *addr, size_t len) {
    vm_context_t *context;
    vm_area_struct_t *vma;
    unsigned long start, end;
    
    /* Check parameters */
    if (addr == NULL || len == 0) {
        return -EINVAL;
    }
    
    /* Align address and length */
    start = (unsigned long)addr & PAGE_MASK;
    end = ((unsigned long)addr + len + PAGE_SIZE - 1) & PAGE_MASK;
    
    /* Get current context */
    context = vmm_get_current_context();
    if (context == NULL) {
        return -EINVAL;
    }
    
    /* Lock memory */
    spin_lock(&mprotect_lock);
    
    /* Find VMA for address */
    vma = vmm_find_vma(context, (void *)start);
    if (vma == NULL || (unsigned long)vma->vm_start > start) {
        spin_unlock(&mprotect_lock);
        return -ENOMEM;
    }
    
    /* Check if VMA covers the entire range */
    if ((unsigned long)vma->vm_end < end) {
        spin_unlock(&mprotect_lock);
        return -ENOMEM;
    }
    
    /* Lock memory */
    vmm_lock_pages(context, (void *)start, (void *)end);
    
    spin_unlock(&mprotect_lock);
    
    return 0;
}

/**
 * Unlock memory
 * 
 * @param addr Address to unlock
 * @param len Length to unlock
 * @return 0 on success, negative error code on failure
 */
int mprotect_unlock(void *addr, size_t len) {
    vm_context_t *context;
    vm_area_struct_t *vma;
    unsigned long start, end;
    
    /* Check parameters */
    if (addr == NULL || len == 0) {
        return -EINVAL;
    }
    
    /* Align address and length */
    start = (unsigned long)addr & PAGE_MASK;
    end = ((unsigned long)addr + len + PAGE_SIZE - 1) & PAGE_MASK;
    
    /* Get current context */
    context = vmm_get_current_context();
    if (context == NULL) {
        return -EINVAL;
    }
    
    /* Unlock memory */
    spin_lock(&mprotect_lock);
    
    /* Find VMA for address */
    vma = vmm_find_vma(context, (void *)start);
    if (vma == NULL || (unsigned long)vma->vm_start > start) {
        spin_unlock(&mprotect_lock);
        return -ENOMEM;
    }
    
    /* Check if VMA covers the entire range */
    if ((unsigned long)vma->vm_end < end) {
        spin_unlock(&mprotect_lock);
        return -ENOMEM;
    }
    
    /* Unlock memory */
    vmm_unlock_pages(context, (void *)start, (void *)end);
    
    spin_unlock(&mprotect_lock);
    
    return 0;
}

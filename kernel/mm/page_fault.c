/**
 * page_fault.c - Horizon kernel page fault handler
 *
 * This file contains the implementation of the page fault handler.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/vmm.h>
#include <horizon/mm/pmm.h>
#include <horizon/mm/page.h>
#include <horizon/mm/tlb.h>
#include <horizon/mm/swap.h>
#include <horizon/interrupt.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/task.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Page fault error codes */
#define PF_PRESENT     0x01    /* Page was present */
#define PF_WRITE       0x02    /* Write access */
#define PF_USER        0x04    /* User mode access */
#define PF_RSVD        0x08    /* Reserved bit violation */
#define PF_INSTR       0x10    /* Instruction fetch */

/* Page fault statistics */
static u64 page_fault_count = 0;
static u64 page_fault_present_count = 0;
static u64 page_fault_write_count = 0;
static u64 page_fault_user_count = 0;
static u64 page_fault_rsvd_count = 0;
static u64 page_fault_instr_count = 0;
static u64 page_fault_kernel_count = 0;
static u64 page_fault_cow_count = 0;
static u64 page_fault_demand_count = 0;
static u64 page_fault_swap_count = 0;

/* Page fault lock */
static spinlock_t page_fault_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize the page fault handler
 */
void page_fault_init(void) {
    /* Register the page fault handler */
    interrupt_register_handler(14, (interrupt_handler_t)page_fault_handler);

    printk(KERN_INFO "PAGE_FAULT: Initialized page fault handler\n");
}

/**
 * Handle a page fault
 *
 * @param frame Interrupt frame
 */
void page_fault_handler(struct interrupt_frame *frame) {
    /* Get the faulting address */
    u32 fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r" (fault_addr));

    /* Get the error code */
    u32 error_code = frame->error_code;

    /* Increment the page fault count */
    spin_lock(&page_fault_lock);
    page_fault_count++;

    /* Update the statistics */
    if (error_code & PF_PRESENT) {
        page_fault_present_count++;
    }

    if (error_code & PF_WRITE) {
        page_fault_write_count++;
    }

    if (error_code & PF_USER) {
        page_fault_user_count++;
    } else {
        page_fault_kernel_count++;
    }

    if (error_code & PF_RSVD) {
        page_fault_rsvd_count++;
    }

    if (error_code & PF_INSTR) {
        page_fault_instr_count++;
    }

    spin_unlock(&page_fault_lock);

    /* Get the current task */
    task_struct_t *task = task_current();

    /* Check if the task has a memory descriptor */
    if (task == NULL || task->mm == NULL) {
        /* Kernel page fault */
        page_fault_kernel(fault_addr, error_code, frame);
        return;
    }

    /* Handle the page fault */
    int ret = page_fault_user(task, fault_addr, error_code, frame);

    /* Check if the page fault was handled */
    if (ret < 0) {
        /* Page fault not handled */
        printk(KERN_ERR "PAGE_FAULT: Unhandled page fault at 0x%08x, error code 0x%08x\n", fault_addr, error_code);

        /* Kill the task */
        task_exit(task, SIGSEGV);
    }
}

/**
 * Handle a kernel page fault
 *
 * @param fault_addr Faulting address
 * @param error_code Error code
 * @param frame Interrupt frame
 */
void page_fault_kernel(u32 fault_addr, u32 error_code, struct interrupt_frame *frame) {
    /* Check if the page fault was in the kernel */
    if (!(error_code & PF_USER)) {
        /* Kernel page fault */
        printk(KERN_ERR "PAGE_FAULT: Kernel page fault at 0x%08x, error code 0x%08x\n", fault_addr, error_code);
        printk(KERN_ERR "PAGE_FAULT: EIP: 0x%08x, ESP: 0x%08x\n", frame->eip, frame->esp);

        /* Panic */
        kernel_panic("Kernel page fault");
    }
}

/**
 * Handle a user page fault
 *
 * @param task Task that caused the page fault
 * @param fault_addr Faulting address
 * @param error_code Error code
 * @param frame Interrupt frame
 * @return 0 on success, negative error code on failure
 */
int page_fault_user(task_struct_t *task, u32 fault_addr, u32 error_code, struct interrupt_frame *frame) {
    /* Check if the page fault was in user mode */
    if (!(error_code & PF_USER)) {
        /* Kernel page fault */
        return -EFAULT;
    }

    /* Find the virtual memory area */
    vm_area_struct_t *vma = vmm_find_vma(task->mm, fault_addr);

    /* Check if the virtual memory area was found */
    if (vma == NULL) {
        /* No virtual memory area found */
        return -EFAULT;
    }

    /* Check if the address is in the virtual memory area */
    if (fault_addr < vma->vm_start) {
        /* Address is not in the virtual memory area */
        return -EFAULT;
    }

    /* Check if the virtual memory area has the required permissions */
    if ((error_code & PF_WRITE) && !(vma->vm_flags & VM_WRITE)) {
        /* Write access to a read-only page */
        return page_fault_cow(task, vma, fault_addr, error_code);
    }

    if ((error_code & PF_INSTR) && !(vma->vm_flags & VM_EXEC)) {
        /* Execute access to a non-executable page */
        return -EFAULT;
    }

    if (!(error_code & PF_PRESENT)) {
        /* Page not present */
        return page_fault_demand(task, vma, fault_addr, error_code);
    }

    /* Check if the page is swapped out */
    if (page_fault_is_swap(task, fault_addr)) {
        /* Page is swapped out */
        return page_fault_swap(task, vma, fault_addr, error_code);
    }

    /* Page fault not handled */
    return -EFAULT;
}

/**
 * Handle a copy-on-write page fault
 *
 * @param task Task that caused the page fault
 * @param vma Virtual memory area
 * @param fault_addr Faulting address
 * @param error_code Error code
 * @return 0 on success, negative error code on failure
 */
int page_fault_cow(task_struct_t *task, vm_area_struct_t *vma, u32 fault_addr, u32 error_code) {
    /* Check if the page is copy-on-write */
    if (!(vma->vm_flags & VM_SHARED)) {
        /* Not a shared page */
        return -EFAULT;
    }

    /* Get the page */
    page_t *page = vmm_get_page(task->mm, fault_addr);

    /* Check if the page was found */
    if (page == NULL) {
        /* No page found */
        return -EFAULT;
    }

    /* Check if the page is read-only */
    if (!(page_test_flags(page, (1 << PG_readonly)))) {
        /* Page is not read-only */
        return -EFAULT;
    }

    /* Allocate a new page */
    page_t *new_page = page_alloc(0);

    if (new_page == NULL) {
        /* Failed to allocate a page */
        return -ENOMEM;
    }

    /* Copy the page */
    memcpy(pmm_page_to_virt(new_page), pmm_page_to_virt(page), PAGE_SIZE);

    /* Map the new page */
    int ret = vmm_map_page(task->mm, fault_addr & ~(PAGE_SIZE - 1), new_page, vma->vm_flags);

    if (ret < 0) {
        /* Failed to map the page */
        page_free(new_page, 0);
        return ret;
    }

    /* Flush the TLB entry */
    tlb_flush_single(fault_addr & ~(PAGE_SIZE - 1));

    /* Increment the copy-on-write count */
    spin_lock(&page_fault_lock);
    page_fault_cow_count++;
    spin_unlock(&page_fault_lock);

    return 0;
}

/**
 * Handle a demand paging page fault
 *
 * @param task Task that caused the page fault
 * @param vma Virtual memory area
 * @param fault_addr Faulting address
 * @param error_code Error code
 * @return 0 on success, negative error code on failure
 */
int page_fault_demand(task_struct_t *task, vm_area_struct_t *vma, u32 fault_addr, u32 error_code) {
    /* Allocate a page */
    page_t *page = page_alloc(0);

    if (page == NULL) {
        /* Failed to allocate a page */
        return -ENOMEM;
    }

    /* Clear the page */
    memset(pmm_page_to_virt(page), 0, PAGE_SIZE);

    /* Map the page */
    int ret = vmm_map_page(task->mm, fault_addr & ~(PAGE_SIZE - 1), page, vma->vm_flags);

    if (ret < 0) {
        /* Failed to map the page */
        page_free(page, 0);
        return ret;
    }

    /* Flush the TLB entry */
    tlb_flush_single(fault_addr & ~(PAGE_SIZE - 1));

    /* Increment the demand paging count */
    spin_lock(&page_fault_lock);
    page_fault_demand_count++;
    spin_unlock(&page_fault_lock);

    return 0;
}

/**
 * Check if a page is swapped out
 *
 * @param task Task to check
 * @param addr Address to check
 * @return 1 if the page is swapped out, 0 if not
 */
int page_fault_is_swap(task_struct_t *task, u32 addr) {
    /* Check if the task has a swap map */
    if (task->mm == NULL || task->mm->swap_map == NULL) {
        /* No swap map */
        return 0;
    }

    /* Get the swap entry */
    u32 swap_entry = task->mm->swap_map[(addr & ~(PAGE_SIZE - 1)) / PAGE_SIZE];

    /* Check if the swap entry is valid */
    return swap_entry != 0;
}

/**
 * Handle a swap page fault
 *
 * @param task Task that caused the page fault
 * @param vma Virtual memory area
 * @param fault_addr Faulting address
 * @param error_code Error code
 * @return 0 on success, negative error code on failure
 */
int page_fault_swap(task_struct_t *task, vm_area_struct_t *vma, u32 fault_addr, u32 error_code) {
    /* Check if the task has a swap map */
    if (task->mm == NULL || task->mm->swap_map == NULL) {
        /* No swap map */
        return -EFAULT;
    }

    /* Get the swap entry */
    u32 swap_entry = task->mm->swap_map[(fault_addr & ~(PAGE_SIZE - 1)) / PAGE_SIZE];

    /* Check if the swap entry is valid */
    if (swap_entry == 0) {
        /* No swap entry */
        return -EFAULT;
    }

    /* Allocate a page */
    page_t *page = page_alloc(0);

    if (page == NULL) {
        /* Failed to allocate a page */
        return -ENOMEM;
    }

    /* Read the page from swap */
    int ret = swap_read(swap_entry, pmm_page_to_virt(page));

    if (ret < 0) {
        /* Failed to read the page from swap */
        page_free(page, 0);
        return ret;
    }

    /* Map the page */
    ret = vmm_map_page(task->mm, fault_addr & ~(PAGE_SIZE - 1), page, vma->vm_flags);

    if (ret < 0) {
        /* Failed to map the page */
        page_free(page, 0);
        return ret;
    }

    /* Flush the TLB entry */
    tlb_flush_single(fault_addr & ~(PAGE_SIZE - 1));

    /* Clear the swap entry */
    task->mm->swap_map[(fault_addr & ~(PAGE_SIZE - 1)) / PAGE_SIZE] = 0;

    /* Increment the swap count */
    spin_lock(&page_fault_lock);
    page_fault_swap_count++;
    spin_unlock(&page_fault_lock);

    return 0;
}

/**
 * Print page fault statistics
 */
void page_fault_print_stats(void) {
    /* Print the statistics */
    printk(KERN_INFO "PAGE_FAULT: Total: %llu\n", page_fault_count);
    printk(KERN_INFO "PAGE_FAULT: Present: %llu\n", page_fault_present_count);
    printk(KERN_INFO "PAGE_FAULT: Write: %llu\n", page_fault_write_count);
    printk(KERN_INFO "PAGE_FAULT: User: %llu\n", page_fault_user_count);
    printk(KERN_INFO "PAGE_FAULT: Reserved: %llu\n", page_fault_rsvd_count);
    printk(KERN_INFO "PAGE_FAULT: Instruction: %llu\n", page_fault_instr_count);
    printk(KERN_INFO "PAGE_FAULT: Kernel: %llu\n", page_fault_kernel_count);
    printk(KERN_INFO "PAGE_FAULT: Copy-on-write: %llu\n", page_fault_cow_count);
    printk(KERN_INFO "PAGE_FAULT: Demand paging: %llu\n", page_fault_demand_count);
    printk(KERN_INFO "PAGE_FAULT: Swap: %llu\n", page_fault_swap_count);
}

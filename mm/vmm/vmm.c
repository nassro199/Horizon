/**
 * vmm.c - Virtual Memory Manager implementation
 * 
 * This file contains the implementation of the virtual memory manager.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/vmm.h>
#include <horizon/string.h>
#include <asm/io.h>

/* Current virtual memory context */
static vm_context_t *current_context = NULL;

/* Kernel virtual memory context */
static vm_context_t *kernel_context = NULL;

/* Initialize the virtual memory manager */
void vmm_init(void)
{
    /* Create the kernel context */
    kernel_context = vmm_create_context();
    
    if (kernel_context == NULL) {
        kernel_panic("Failed to create kernel virtual memory context");
    }
    
    /* Identity map the first 4MB of memory */
    for (u32 i = 0; i < 0x400000; i += PAGE_SIZE) {
        vmm_map_page(kernel_context, (void *)i, (void *)i, PTE_PRESENT | PTE_WRITE);
    }
    
    /* Map the kernel to the higher half of the address space */
    for (u32 i = 0; i < 0x400000; i += PAGE_SIZE) {
        vmm_map_page(kernel_context, (void *)(0xC0000000 + i), (void *)i, PTE_PRESENT | PTE_WRITE);
    }
    
    /* Switch to the kernel context */
    vmm_switch_context(kernel_context);
}

/* Create a virtual memory context */
vm_context_t *vmm_create_context(void)
{
    /* Allocate a context structure */
    vm_context_t *context = kmalloc(sizeof(vm_context_t), MEM_KERNEL | MEM_ZERO);
    
    if (context == NULL) {
        return NULL;
    }
    
    /* Allocate a page directory */
    context->page_dir = mm_alloc_pages(1, MEM_KERNEL | MEM_ZERO);
    
    if (context->page_dir == NULL) {
        kfree(context);
        return NULL;
    }
    
    /* Get the physical address of the page directory */
    context->page_dir_phys = (u32)context->page_dir;
    
    return context;
}

/* Destroy a virtual memory context */
void vmm_destroy_context(vm_context_t *context)
{
    if (context == NULL) {
        return;
    }
    
    /* Free the page directory */
    if (context->page_dir != NULL) {
        /* Free all page tables */
        for (u32 i = 0; i < 1024; i++) {
            if (context->page_dir->entries[i] & PDE_PRESENT) {
                /* Get the page table */
                page_table_t *table = (page_table_t *)(context->page_dir->entries[i] & 0xFFFFF000);
                
                /* Free the page table */
                mm_free_pages(table, 1);
            }
        }
        
        /* Free the page directory */
        mm_free_pages(context->page_dir, 1);
    }
    
    /* Free the context structure */
    kfree(context);
}

/* Switch to a virtual memory context */
void vmm_switch_context(vm_context_t *context)
{
    if (context == NULL) {
        return;
    }
    
    /* Set the current context */
    current_context = context;
    
    /* Load the page directory */
    __asm__ volatile("mov %0, %%cr3" : : "r"(context->page_dir_phys));
}

/* Get the current virtual memory context */
vm_context_t *vmm_get_current_context(void)
{
    return current_context;
}

/* Allocate pages in a virtual memory context */
void *vmm_alloc_pages(vm_context_t *context, void *addr, u32 count, u32 flags)
{
    if (context == NULL || count == 0) {
        return NULL;
    }
    
    /* Align the address to a page boundary */
    addr = (void *)((u32)addr & 0xFFFFF000);
    
    /* Check if the address is already mapped */
    for (u32 i = 0; i < count; i++) {
        void *virt = (void *)((u32)addr + i * PAGE_SIZE);
        
        if (vmm_get_phys_addr(context, virt) != NULL) {
            /* Address is already mapped */
            return NULL;
        }
    }
    
    /* Allocate physical pages */
    void *phys = mm_alloc_pages(count, MEM_KERNEL);
    
    if (phys == NULL) {
        return NULL;
    }
    
    /* Map the pages */
    for (u32 i = 0; i < count; i++) {
        void *virt = (void *)((u32)addr + i * PAGE_SIZE);
        void *p = (void *)((u32)phys + i * PAGE_SIZE);
        
        if (vmm_map_page(context, virt, p, flags) < 0) {
            /* Failed to map a page */
            /* Free the physical pages */
            mm_free_pages(phys, count);
            
            /* Unmap the pages that were mapped */
            for (u32 j = 0; j < i; j++) {
                void *v = (void *)((u32)addr + j * PAGE_SIZE);
                vmm_unmap_page(context, v);
            }
            
            return NULL;
        }
    }
    
    return addr;
}

/* Free pages in a virtual memory context */
void vmm_free_pages(vm_context_t *context, void *addr, u32 count)
{
    if (context == NULL || addr == NULL || count == 0) {
        return;
    }
    
    /* Align the address to a page boundary */
    addr = (void *)((u32)addr & 0xFFFFF000);
    
    /* Unmap and free the pages */
    for (u32 i = 0; i < count; i++) {
        void *virt = (void *)((u32)addr + i * PAGE_SIZE);
        void *phys = vmm_get_phys_addr(context, virt);
        
        if (phys != NULL) {
            /* Unmap the page */
            vmm_unmap_page(context, virt);
            
            /* Free the physical page */
            mm_free_pages(phys, 1);
        }
    }
}

/* Map a physical page to a virtual address */
int vmm_map_page(vm_context_t *context, void *virt, void *phys, u32 flags)
{
    if (context == NULL || virt == NULL || phys == NULL) {
        return -1;
    }
    
    /* Align the addresses to page boundaries */
    virt = (void *)((u32)virt & 0xFFFFF000);
    phys = (void *)((u32)phys & 0xFFFFF000);
    
    /* Get the page directory index */
    u32 pd_index = (u32)virt >> 22;
    
    /* Get the page table index */
    u32 pt_index = ((u32)virt >> 12) & 0x3FF;
    
    /* Check if the page table exists */
    if (!(context->page_dir->entries[pd_index] & PDE_PRESENT)) {
        /* Allocate a page table */
        page_table_t *table = mm_alloc_pages(1, MEM_KERNEL | MEM_ZERO);
        
        if (table == NULL) {
            return -1;
        }
        
        /* Add the page table to the page directory */
        context->page_dir->entries[pd_index] = (u32)table | PDE_PRESENT | PDE_WRITE | PDE_USER;
    }
    
    /* Get the page table */
    page_table_t *table = (page_table_t *)(context->page_dir->entries[pd_index] & 0xFFFFF000);
    
    /* Map the page */
    table->entries[pt_index] = (u32)phys | flags;
    
    /* Invalidate the TLB entry */
    __asm__ volatile("invlpg %0" : : "m"(*(char *)virt));
    
    return 0;
}

/* Unmap a virtual address */
int vmm_unmap_page(vm_context_t *context, void *virt)
{
    if (context == NULL || virt == NULL) {
        return -1;
    }
    
    /* Align the address to a page boundary */
    virt = (void *)((u32)virt & 0xFFFFF000);
    
    /* Get the page directory index */
    u32 pd_index = (u32)virt >> 22;
    
    /* Get the page table index */
    u32 pt_index = ((u32)virt >> 12) & 0x3FF;
    
    /* Check if the page table exists */
    if (!(context->page_dir->entries[pd_index] & PDE_PRESENT)) {
        /* Page is not mapped */
        return -1;
    }
    
    /* Get the page table */
    page_table_t *table = (page_table_t *)(context->page_dir->entries[pd_index] & 0xFFFFF000);
    
    /* Check if the page is mapped */
    if (!(table->entries[pt_index] & PTE_PRESENT)) {
        /* Page is not mapped */
        return -1;
    }
    
    /* Unmap the page */
    table->entries[pt_index] = 0;
    
    /* Invalidate the TLB entry */
    __asm__ volatile("invlpg %0" : : "m"(*(char *)virt));
    
    return 0;
}

/* Get the physical address of a virtual address */
void *vmm_get_phys_addr(vm_context_t *context, void *virt)
{
    if (context == NULL || virt == NULL) {
        return NULL;
    }
    
    /* Get the page directory index */
    u32 pd_index = (u32)virt >> 22;
    
    /* Get the page table index */
    u32 pt_index = ((u32)virt >> 12) & 0x3FF;
    
    /* Check if the page table exists */
    if (!(context->page_dir->entries[pd_index] & PDE_PRESENT)) {
        /* Page is not mapped */
        return NULL;
    }
    
    /* Get the page table */
    page_table_t *table = (page_table_t *)(context->page_dir->entries[pd_index] & 0xFFFFF000);
    
    /* Check if the page is mapped */
    if (!(table->entries[pt_index] & PTE_PRESENT)) {
        /* Page is not mapped */
        return NULL;
    }
    
    /* Get the physical address */
    void *phys = (void *)(table->entries[pt_index] & 0xFFFFF000);
    
    /* Add the offset */
    phys = (void *)((u32)phys | ((u32)virt & 0xFFF));
    
    return phys;
}

/* Find a virtual memory area */
vm_area_struct_t *vmm_find_vma(vm_context_t *context, void *addr)
{
    if (context == NULL || addr == NULL) {
        return NULL;
    }
    
    /* This would be implemented with actual VMA tracking */
    /* For now, just return NULL */
    return NULL;
}

/* Create a virtual memory area */
vm_area_struct_t *vmm_create_vma(vm_context_t *context, void *addr, u32 size, u32 flags)
{
    if (context == NULL || addr == NULL || size == 0) {
        return NULL;
    }
    
    /* Allocate a VMA structure */
    vm_area_struct_t *vma = kmalloc(sizeof(vm_area_struct_t), MEM_KERNEL | MEM_ZERO);
    
    if (vma == NULL) {
        return NULL;
    }
    
    /* Initialize the VMA */
    vma->vm_start = addr;
    vma->vm_end = (void *)((u32)addr + size);
    vma->vm_flags = flags;
    
    /* Set the page protection */
    vma->vm_page_prot = 0;
    
    if (flags & PROT_READ) {
        vma->vm_page_prot |= PTE_PRESENT;
    }
    
    if (flags & PROT_WRITE) {
        vma->vm_page_prot |= PTE_WRITE;
    }
    
    if (flags & PROT_EXEC) {
        /* No execute bit on x86 */
    }
    
    /* This would be implemented with actual VMA tracking */
    /* For now, just return the VMA */
    return vma;
}

/* Destroy a virtual memory area */
int vmm_destroy_vma(vm_context_t *context, vm_area_struct_t *vma)
{
    if (context == NULL || vma == NULL) {
        return -1;
    }
    
    /* Free the VMA structure */
    kfree(vma);
    
    return 0;
}

/* Handle a page fault */
void vmm_handle_page_fault(void *addr, u32 error_code)
{
    /* Get the current context */
    vm_context_t *context = vmm_get_current_context();
    
    if (context == NULL) {
        kernel_panic("Page fault with no virtual memory context");
    }
    
    /* Find the VMA for the address */
    vm_area_struct_t *vma = vmm_find_vma(context, addr);
    
    if (vma == NULL) {
        /* No VMA for this address */
        kernel_panic("Page fault at invalid address");
    }
    
    /* Check if the access is allowed */
    if ((error_code & 2) && !(vma->vm_flags & PROT_WRITE)) {
        /* Write access to read-only memory */
        kernel_panic("Page fault: write access to read-only memory");
    }
    
    /* Allocate a page */
    void *page = mm_alloc_pages(1, MEM_KERNEL | MEM_ZERO);
    
    if (page == NULL) {
        kernel_panic("Page fault: failed to allocate page");
    }
    
    /* Map the page */
    if (vmm_map_page(context, (void *)((u32)addr & 0xFFFFF000), page, vma->vm_page_prot) < 0) {
        /* Failed to map the page */
        mm_free_pages(page, 1);
        kernel_panic("Page fault: failed to map page");
    }
}

/**
 * vmm.h - Virtual Memory Manager definitions
 * 
 * This file contains definitions for the virtual memory manager.
 */

#ifndef _KERNEL_VMM_H
#define _KERNEL_VMM_H

#include <horizon/types.h>
#include <horizon/mm.h>

/* Page directory entry flags */
#define PDE_PRESENT     0x001   /* Page directory entry is present */
#define PDE_WRITE       0x002   /* Page directory entry is writable */
#define PDE_USER        0x004   /* Page directory entry is user-accessible */
#define PDE_PWT         0x008   /* Page directory entry has write-through caching */
#define PDE_PCD         0x010   /* Page directory entry has cache disabled */
#define PDE_ACCESSED    0x020   /* Page directory entry has been accessed */
#define PDE_DIRTY       0x040   /* Page directory entry has been written to */
#define PDE_4MB         0x080   /* Page directory entry maps a 4MB page */
#define PDE_GLOBAL      0x100   /* Page directory entry is global */
#define PDE_AVAIL       0xE00   /* Available for use by the OS */

/* Page table entry flags */
#define PTE_PRESENT     0x001   /* Page table entry is present */
#define PTE_WRITE       0x002   /* Page table entry is writable */
#define PTE_USER        0x004   /* Page table entry is user-accessible */
#define PTE_PWT         0x008   /* Page table entry has write-through caching */
#define PTE_PCD         0x010   /* Page table entry has cache disabled */
#define PTE_ACCESSED    0x020   /* Page table entry has been accessed */
#define PTE_DIRTY       0x040   /* Page table entry has been written to */
#define PTE_PAT         0x080   /* Page table entry has page attribute table */
#define PTE_GLOBAL      0x100   /* Page table entry is global */
#define PTE_AVAIL       0xE00   /* Available for use by the OS */

/* Memory protection flags */
#define PROT_NONE       0x0     /* Memory cannot be accessed */
#define PROT_READ       0x1     /* Memory can be read */
#define PROT_WRITE      0x2     /* Memory can be written */
#define PROT_EXEC       0x4     /* Memory can be executed */

/* Memory mapping flags */
#define MAP_SHARED      0x01    /* Share changes */
#define MAP_PRIVATE     0x02    /* Changes are private */
#define MAP_FIXED       0x04    /* Interpret addr exactly */
#define MAP_ANONYMOUS   0x08    /* Don't use a file */
#define MAP_GROWSDOWN   0x10    /* Stack-like segment */
#define MAP_DENYWRITE   0x20    /* ETXTBSY */
#define MAP_EXECUTABLE  0x40    /* Mark it as an executable */
#define MAP_LOCKED      0x80    /* Lock the mapping */

/* Virtual memory area structure */
typedef struct vm_area_struct {
    void *vm_start;                /* Start address */
    void *vm_end;                  /* End address */
    u32 vm_flags;                  /* Flags */
    u32 vm_page_prot;              /* Page protection */
    struct vm_area_struct *vm_next; /* Next area in list */
} vm_area_struct_t;

/* Page directory structure */
typedef struct page_directory {
    u32 entries[1024];             /* Page directory entries */
} page_directory_t;

/* Page table structure */
typedef struct page_table {
    u32 entries[1024];             /* Page table entries */
} page_table_t;

/* Virtual memory context structure */
typedef struct vm_context {
    page_directory_t *page_dir;    /* Page directory */
    u32 page_dir_phys;             /* Physical address of page directory */
} vm_context_t;

/* Virtual memory manager functions */
void vmm_init(void);
vm_context_t *vmm_create_context(void);
void vmm_destroy_context(vm_context_t *context);
void vmm_switch_context(vm_context_t *context);
vm_context_t *vmm_get_current_context(void);
void *vmm_alloc_pages(vm_context_t *context, void *addr, u32 count, u32 flags);
void vmm_free_pages(vm_context_t *context, void *addr, u32 count);
int vmm_map_page(vm_context_t *context, void *virt, void *phys, u32 flags);
int vmm_unmap_page(vm_context_t *context, void *virt);
void *vmm_get_phys_addr(vm_context_t *context, void *virt);
vm_area_struct_t *vmm_find_vma(vm_context_t *context, void *addr);
vm_area_struct_t *vmm_create_vma(vm_context_t *context, void *addr, u32 size, u32 flags);
int vmm_destroy_vma(vm_context_t *context, vm_area_struct_t *vma);
void vmm_handle_page_fault(void *addr, u32 error_code);

#endif /* _KERNEL_VMM_H */

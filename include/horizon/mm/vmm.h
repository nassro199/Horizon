/**
 * vmm.h - Horizon kernel virtual memory management definitions
 * 
 * This file contains definitions for the virtual memory management subsystem.
 * The definitions are compatible with Linux.
 */

#ifndef _KERNEL_MM_VMM_H
#define _KERNEL_MM_VMM_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/mm/page.h>

/* Memory protection flags */
#define PROT_NONE  0x0  /* No access */
#define PROT_READ  0x1  /* Pages can be read */
#define PROT_WRITE 0x2  /* Pages can be written */
#define PROT_EXEC  0x4  /* Pages can be executed */

/* Memory mapping flags */
#define MAP_SHARED    0x01  /* Share changes */
#define MAP_PRIVATE   0x02  /* Changes are private */
#define MAP_FIXED     0x10  /* Interpret addr exactly */
#define MAP_ANONYMOUS 0x20  /* Don't use a file */
#define MAP_GROWSDOWN 0x100 /* Stack-like segment */
#define MAP_DENYWRITE 0x800 /* ETXTBSY */
#define MAP_EXECUTABLE 0x1000 /* Mark it as an executable */
#define MAP_LOCKED    0x2000 /* Lock the mapping */
#define MAP_NORESERVE 0x4000 /* Don't check for reservations */
#define MAP_POPULATE  0x8000 /* Populate (prefault) pagetables */
#define MAP_NONBLOCK  0x10000 /* Do not block on IO */
#define MAP_STACK     0x20000 /* Give out an address that is best suited for process/thread stacks */
#define MAP_HUGETLB   0x40000 /* Create huge page mapping */

/* Virtual memory area flags */
#define VM_READ         0x00000001 /* Read permission */
#define VM_WRITE        0x00000002 /* Write permission */
#define VM_EXEC         0x00000004 /* Execute permission */
#define VM_SHARED       0x00000008 /* Shared mapping */
#define VM_MAYREAD      0x00000010 /* Can read */
#define VM_MAYWRITE     0x00000020 /* Can write */
#define VM_MAYEXEC      0x00000040 /* Can execute */
#define VM_MAYSHARE     0x00000080 /* Can share */
#define VM_GROWSDOWN    0x00000100 /* Stack segment grows down */
#define VM_GROWSUP      0x00000200 /* Data segment grows up */
#define VM_PFNMAP       0x00000400 /* Page-ranges managed without "struct page" */
#define VM_DENYWRITE    0x00000800 /* ETXTBSY on write attempts */
#define VM_EXECUTABLE   0x00001000 /* Executable mapping */
#define VM_LOCKED       0x00002000 /* Pages are locked */
#define VM_IO           0x00004000 /* Memory mapped I/O or similar */
#define VM_SEQ_READ     0x00008000 /* App will access data sequentially */
#define VM_RAND_READ    0x00010000 /* App will not benefit from clustered reads */
#define VM_DONTCOPY     0x00020000 /* Do not copy this vma on fork */
#define VM_DONTEXPAND   0x00040000 /* Cannot expand with mremap() */
#define VM_RESERVED     0x00080000 /* Count as reserved_vm like IO */
#define VM_ACCOUNT      0x00100000 /* Is a VM accounted object */
#define VM_NORESERVE    0x00200000 /* Don't check for reservations */
#define VM_HUGETLB      0x00400000 /* Huge TLB page */
#define VM_NONLINEAR    0x00800000 /* Is non-linear (remap_file_pages) */
#define VM_ARCH_1       0x01000000 /* Architecture-specific flag */
#define VM_DONTDUMP     0x04000000 /* Do not include in the core dump */

/* Forward declarations */
struct vm_area_struct;
struct mm_struct;

/* Virtual memory operations */
typedef struct vm_operations_struct {
    void (*open)(struct vm_area_struct *area);
    void (*close)(struct vm_area_struct *area);
    int (*fault)(struct vm_area_struct *area, struct vm_fault *vmf);
    int (*page_mkwrite)(struct vm_area_struct *area, struct vm_fault *vmf);
    int (*access)(struct vm_area_struct *area, unsigned long addr, void *buf, int len, int write);
} vm_operations_struct_t;

/* Virtual memory area structure */
typedef struct vm_area_struct {
    struct mm_struct *vm_mm;       /* The address space we belong to */
    unsigned long vm_start;        /* Our start address within vm_mm */
    unsigned long vm_end;          /* The first byte after our end address within vm_mm */
    unsigned long vm_flags;        /* Flags */
    struct vm_operations_struct *vm_ops; /* Associated operations */
    unsigned long vm_pgoff;        /* Offset (within vm_file) in PAGE_SIZE units */
    struct file *vm_file;          /* File we map to (can be NULL) */
    void *vm_private_data;         /* Private data */
    struct list_head vm_list;      /* List of VMAs */
} vm_area_struct_t;

/* Memory descriptor */
typedef struct mm_struct {
    struct vm_area_struct *mmap;   /* List of memory areas */
    struct rb_root mm_rb;          /* Red-black tree of VMAs */
    struct vm_area_struct *mmap_cache; /* Last used memory area */
    unsigned long free_area_cache; /* First hole of size cached_hole_size or larger */
    pgd_t *pgd;                    /* Page global directory */
    atomic_t mm_users;             /* How many users with user space? */
    atomic_t mm_count;             /* How many references to "struct mm_struct" (users count as 1) */
    int map_count;                 /* Number of VMAs */
    struct rw_semaphore mmap_sem;  /* Protects mmap operations */
    spinlock_t page_table_lock;    /* Protects page tables and some counters */
    struct list_head mmlist;       /* List of all mm_structs */
    unsigned long start_code;      /* Start address of code */
    unsigned long end_code;        /* End address of code */
    unsigned long start_data;      /* Start address of data */
    unsigned long end_data;        /* End address of data */
    unsigned long start_brk;       /* Start address of heap */
    unsigned long brk;             /* End address of heap */
    unsigned long start_stack;     /* Start address of stack */
    unsigned long arg_start;       /* Start of arguments */
    unsigned long arg_end;         /* End of arguments */
    unsigned long env_start;       /* Start of environment */
    unsigned long env_end;         /* End of environment */
    unsigned long total_vm;        /* Total pages mapped */
    unsigned long locked_vm;       /* Pages that have PG_mlocked set */
    unsigned long pinned_vm;       /* Refcount permanently increased */
    unsigned long shared_vm;       /* Shared pages (files) */
    unsigned long exec_vm;         /* VM_EXEC & ~VM_WRITE */
    unsigned long stack_vm;        /* VM_GROWSUP/DOWN */
    unsigned long def_flags;       /* Default flags for VMA */
    unsigned long nr_ptes;         /* Page table pages */
    unsigned long hiwater_rss;     /* High-watermark of RSS usage */
    unsigned long hiwater_vm;      /* High-water virtual memory usage */
    unsigned long total_rss;       /* Total pages mapped */
    unsigned long locked_rss;      /* Pages that have PG_mlocked set */
    unsigned long pinned_rss;      /* Refcount permanently increased */
    unsigned long shared_rss;      /* Shared pages (files) */
    unsigned long exec_rss;        /* VM_EXEC & ~VM_WRITE */
    unsigned long stack_rss;       /* VM_GROWSUP/DOWN */
    unsigned long reserved_rss;    /* Reserved pages */
} mm_struct_t;

/* Virtual memory fault */
typedef struct vm_fault {
    unsigned int flags;            /* Fault flags */
    unsigned long address;         /* Faulting address */
    unsigned long pgoff;           /* Logical page offset based on vma */
    unsigned long error_code;      /* Error code */
    pte_t *pte;                    /* Pointer to the page table entry */
    page_t *page;                  /* Faulted page */
    struct page *cow_page;         /* Copy-on-write page */
    struct vm_area_struct *vma;    /* Faulting VMA */
} vm_fault_t;

/* Virtual memory functions */
void vmm_init(void);
mm_struct_t *vmm_create_mm(void);
void vmm_destroy_mm(mm_struct_t *mm);
vm_area_struct_t *vmm_create_vma(mm_struct_t *mm, unsigned long start, unsigned long size, unsigned long flags);
void vmm_destroy_vma(mm_struct_t *mm, vm_area_struct_t *vma);
vm_area_struct_t *vmm_find_vma(mm_struct_t *mm, unsigned long addr);
int vmm_map_page(mm_struct_t *mm, unsigned long addr, page_t *page, unsigned long flags);
int vmm_unmap_page(mm_struct_t *mm, unsigned long addr);
page_t *vmm_get_page(mm_struct_t *mm, unsigned long addr);
int vmm_handle_fault(mm_struct_t *mm, unsigned long addr, unsigned long error_code);
void *vmm_mmap(mm_struct_t *mm, void *addr, unsigned long size, unsigned long prot, unsigned long flags, struct file *file, unsigned long offset);
int vmm_munmap(mm_struct_t *mm, void *addr, unsigned long size);
int vmm_mprotect(mm_struct_t *mm, void *addr, unsigned long size, unsigned long prot);
int vmm_brk(mm_struct_t *mm, unsigned long brk);
int vmm_mremap(mm_struct_t *mm, void *old_addr, unsigned long old_size, unsigned long new_size, unsigned long flags, void *new_addr);
int vmm_msync(mm_struct_t *mm, void *addr, unsigned long size, int flags);
int vmm_mlock(mm_struct_t *mm, void *addr, unsigned long size);
int vmm_munlock(mm_struct_t *mm, void *addr, unsigned long size);
int vmm_mlockall(mm_struct_t *mm, int flags);
int vmm_munlockall(mm_struct_t *mm);
int vmm_mincore(mm_struct_t *mm, void *addr, unsigned long size, unsigned char *vec);
int vmm_madvise(mm_struct_t *mm, void *addr, unsigned long size, int advice);
int vmm_remap_file_pages(mm_struct_t *mm, void *addr, unsigned long size, unsigned long prot, unsigned long pgoff, int flags);

#endif /* _KERNEL_MM_VMM_H */

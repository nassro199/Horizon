/**
 * mm_types.h - Memory management types
 *
 * This file contains the memory management types.
 */

#ifndef _HORIZON_MM_TYPES_H
#define _HORIZON_MM_TYPES_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/spinlock.h>

/* Red-black tree node */
typedef struct rb_node {
    u32 rb_parent_color;                  /* Parent and color */
    struct rb_node *rb_right;             /* Right child */
    struct rb_node *rb_left;              /* Left child */
} rb_node_t;

/* Red-black tree root */
typedef struct rb_root {
    rb_node_t *rb_node;                   /* Root node */
} rb_root_t;

/* Page protection */
typedef u32 pgprot_t;

/* Forward declarations */
struct task_struct;
struct file;

/* Memory descriptor */
typedef struct mm_struct {
    /* Memory areas */
    struct vm_area_struct *mmap;          /* List of memory areas */
    struct rb_root mm_rb;                 /* Red-black tree of memory areas */
    struct vm_area_struct *mmap_cache;    /* Last used memory area */

    /* Memory limits */
    u32 map_count;                        /* Number of memory areas */
    u32 hiwater_rss;                      /* High-water RSS usage */
    u32 hiwater_vm;                       /* High-water virtual memory usage */

    /* Memory usage */
    u32 total_vm;                         /* Total virtual memory */
    u32 locked_vm;                        /* Locked virtual memory */
    u32 pinned_vm;                        /* Pinned virtual memory */
    u32 shared_vm;                        /* Shared virtual memory */
    u32 exec_vm;                          /* Executable virtual memory */
    u32 stack_vm;                         /* Stack virtual memory */
    u32 reserved_vm;                      /* Reserved virtual memory */
    u32 def_flags;                        /* Default flags */

    /* Memory layout */
    u32 start_code;                       /* Start of code segment */
    u32 end_code;                         /* End of code segment */
    u32 start_data;                       /* Start of data segment */
    u32 end_data;                         /* End of data segment */
    u32 start_brk;                        /* Start of heap */
    u32 brk;                              /* Current heap pointer */
    u32 start_stack;                      /* Start of stack */
    u32 arg_start;                        /* Start of arguments */
    u32 arg_end;                          /* End of arguments */
    u32 env_start;                        /* Start of environment */
    u32 env_end;                          /* End of environment */

    /* Page tables */
    u32 pgd;                              /* Page global directory */

    /* Reference count */
    u32 mm_count;                         /* Reference count */

    /* Locks */
    spinlock_t page_table_lock;           /* Page table lock */
    spinlock_t mmap_lock;                 /* Memory map lock */

    /* Lists */
    list_head_t mmlist;                   /* List of memory descriptors */

    /* Owner */
    struct task_struct *owner;            /* Owner task */
} mm_struct_t;

/* Memory area flags */
#define VM_READ         0x00000001  /* Readable */
#define VM_WRITE        0x00000002  /* Writable */
#define VM_EXEC         0x00000004  /* Executable */
#define VM_SHARED       0x00000008  /* Shared */
#define VM_MAYREAD      0x00000010  /* Can be made readable */
#define VM_MAYWRITE     0x00000020  /* Can be made writable */
#define VM_MAYEXEC      0x00000040  /* Can be made executable */
#define VM_MAYSHARE     0x00000080  /* Can be made shared */
#define VM_GROWSDOWN    0x00000100  /* Grows down */
#define VM_GROWSUP      0x00000200  /* Grows up */
#define VM_PFNMAP       0x00000400  /* Page frame number mapping */
#define VM_DENYWRITE    0x00000800  /* Deny write access */
#define VM_EXECUTABLE   0x00001000  /* Executable mapping */
#define VM_LOCKED       0x00002000  /* Locked */
#define VM_IO           0x00004000  /* I/O mapping */
#define VM_SEQ_READ     0x00008000  /* Sequential read */
#define VM_RAND_READ    0x00010000  /* Random read */
#define VM_DONTCOPY     0x00020000  /* Don't copy on fork */
#define VM_DONTEXPAND   0x00040000  /* Don't expand on remapping */
#define VM_RESERVED     0x00080000  /* Reserved */
#define VM_ACCOUNT      0x00100000  /* Account */
#define VM_HUGETLB      0x00200000  /* Huge TLB page */
#define VM_NONLINEAR    0x00400000  /* Nonlinear mapping */
#define VM_MAPPED_COPY  0x00800000  /* Mapped copy */
#define VM_INSERTPAGE   0x01000000  /* Insert page */
#define VM_ALWAYSDUMP   0x02000000  /* Always include in core dump */
#define VM_CAN_NONLINEAR 0x04000000 /* Can be nonlinear */
#define VM_MIXEDMAP     0x08000000  /* Mixed map */
#define VM_SAO          0x10000000  /* Strong access ordering */
#define VM_MERGEABLE    0x20000000  /* Mergeable */
#define VM_IOREMAP      0x40000000  /* I/O remap */
#define VM_ARCH_1       0x80000000  /* Architecture-specific flag */

/* Virtual memory area structure */
typedef struct vm_area_struct {
    /* Memory area */
    u32 vm_start;                         /* Start address */
    u32 vm_end;                           /* End address */

    /* Memory area flags */
    u32 vm_flags;                         /* Flags */

    /* Memory area lists */
    struct vm_area_struct *vm_next;       /* Next memory area */
    struct vm_area_struct *vm_prev;       /* Previous memory area */

    /* Red-black tree */
    struct rb_node vm_rb;                 /* Red-black tree node */

    /* Memory descriptor */
    mm_struct_t *vm_mm;                   /* Memory descriptor */

    /* Page protection */
    pgprot_t vm_page_prot;                /* Page protection */

    /* File mapping */
    u32 vm_pgoff;                         /* Page offset */
    struct file *vm_file;                 /* File */
    void *vm_private_data;                /* Private data */
} vm_area_struct_t;



#endif /* _HORIZON_MM_TYPES_H */

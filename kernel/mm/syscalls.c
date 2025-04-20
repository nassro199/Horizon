/**
 * syscalls.c - Horizon kernel memory management system calls
 * 
 * This file contains the implementation of the memory management system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/mm.h>
#include <horizon/mm/vmm.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: brk */
long sys_brk(long brk, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Change the program break */
    return vmm_brk(task->mm, brk);
}

/* System call: sbrk */
long sys_sbrk(long increment, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Get the current program break */
    unsigned long old_brk = task->mm->brk;
    
    /* Change the program break */
    long error = vmm_brk(task->mm, old_brk + increment);
    
    if (error < 0) {
        return error;
    }
    
    /* Return the old program break */
    return old_brk;
}

/* System call: mmap */
long sys_mmap(long addr, long length, long prot, long flags, long fd, long offset) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Get the file */
    file_t *file = NULL;
    
    if (fd >= 0) {
        file = task_get_file(task, fd);
        
        if (file == NULL && !(flags & MAP_ANONYMOUS)) {
            return -1;
        }
    }
    
    /* Map memory */
    void *mapped_addr;
    int error = vmm_mmap(task->mm, (void *)addr, length, prot, flags, file, offset, &mapped_addr);
    
    if (error) {
        return error;
    }
    
    return (long)mapped_addr;
}

/* System call: munmap */
long sys_munmap(long addr, long length, long unused1, long unused2, long unused3, long unused4) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Unmap memory */
    return vmm_munmap(task->mm, (void *)addr, length);
}

/* System call: mprotect */
long sys_mprotect(long addr, long length, long prot, long unused1, long unused2, long unused3) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Change memory protection */
    return vmm_mprotect(task->mm, (void *)addr, length, prot);
}

/* System call: msync */
long sys_msync(long addr, long length, long flags, long unused1, long unused2, long unused3) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Synchronize memory */
    return vmm_msync(task->mm, (void *)addr, length, flags);
}

/* System call: madvise */
long sys_madvise(long addr, long length, long advice, long unused1, long unused2, long unused3) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Advise memory usage */
    return vmm_madvise(task->mm, (void *)addr, length, advice);
}

/* System call: mincore */
long sys_mincore(long addr, long length, long vec, long unused1, long unused2, long unused3) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Get memory residency */
    return vmm_mincore(task->mm, (void *)addr, length, (unsigned char *)vec);
}

/* System call: mlock */
long sys_mlock(long addr, long length, long unused1, long unused2, long unused3, long unused4) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Lock memory */
    return vmm_mlock(task->mm, (void *)addr, length);
}

/* System call: munlock */
long sys_munlock(long addr, long length, long unused1, long unused2, long unused3, long unused4) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Unlock memory */
    return vmm_munlock(task->mm, (void *)addr, length);
}

/* System call: mlockall */
long sys_mlockall(long flags, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Lock all memory */
    return vmm_mlockall(task->mm, flags);
}

/* System call: munlockall */
long sys_munlockall(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Unlock all memory */
    return vmm_munlockall(task->mm);
}

/* System call: mremap */
long sys_mremap(long old_addr, long old_size, long new_size, long flags, long new_addr, long unused1) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Remap memory */
    void *mapped_addr;
    int error = vmm_mremap(task->mm, (void *)old_addr, old_size, new_size, flags, (void *)new_addr, &mapped_addr);
    
    if (error) {
        return error;
    }
    
    return (long)mapped_addr;
}

/* System call: remap_file_pages */
long sys_remap_file_pages(long addr, long size, long prot, long pgoff, long flags, long unused1) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Remap file pages */
    return vmm_remap_file_pages(task->mm, (void *)addr, size, prot, pgoff, flags);
}

/* System call: mbind */
long sys_mbind(long addr, long length, long mode, long nodemask, long maxnode, long flags) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Set memory policy */
    return vmm_mbind(task->mm, (void *)addr, length, mode, (unsigned long *)nodemask, maxnode, flags);
}

/* System call: get_mempolicy */
long sys_get_mempolicy(long policy, long nodemask, long maxnode, long addr, long flags, long unused1) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Get memory policy */
    return vmm_get_mempolicy(task->mm, (int *)policy, (unsigned long *)nodemask, maxnode, (void *)addr, flags);
}

/* System call: set_mempolicy */
long sys_set_mempolicy(long mode, long nodemask, long maxnode, long unused1, long unused2, long unused3) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    /* Set memory policy */
    return vmm_set_mempolicy(task->mm, mode, (unsigned long *)nodemask, maxnode);
}

/* System call: migrate_pages */
long sys_migrate_pages(long pid, long maxnode, long old_nodes, long new_nodes, long unused1, long unused2) {
    /* Get the task */
    task_struct_t *task = task_get(pid);
    
    if (task == NULL) {
        return -1;
    }
    
    /* Migrate pages */
    return vmm_migrate_pages(task->mm, maxnode, (unsigned long *)old_nodes, (unsigned long *)new_nodes);
}

/* System call: move_pages */
long sys_move_pages(long pid, long count, long pages, long nodes, long status, long flags) {
    /* Get the task */
    task_struct_t *task = task_get(pid);
    
    if (task == NULL) {
        return -1;
    }
    
    /* Move pages */
    return vmm_move_pages(task->mm, count, (void **)pages, (int *)nodes, (int *)status, flags);
}

/* Register memory management system calls */
void mm_syscalls_init(void) {
    /* Register memory management system calls */
    syscall_register(SYS_BRK, sys_brk);
    syscall_register(SYS_SBRK, sys_sbrk);
    syscall_register(SYS_MMAP, sys_mmap);
    syscall_register(SYS_MUNMAP, sys_munmap);
    syscall_register(SYS_MPROTECT, sys_mprotect);
    syscall_register(SYS_MSYNC, sys_msync);
    syscall_register(SYS_MADVISE, sys_madvise);
    syscall_register(SYS_MINCORE, sys_mincore);
    syscall_register(SYS_MLOCK, sys_mlock);
    syscall_register(SYS_MUNLOCK, sys_munlock);
    syscall_register(SYS_MLOCKALL, sys_mlockall);
    syscall_register(SYS_MUNLOCKALL, sys_munlockall);
    syscall_register(SYS_MREMAP, sys_mremap);
    syscall_register(SYS_REMAP_FILE_PAGES, sys_remap_file_pages);
    syscall_register(SYS_MBIND, sys_mbind);
    syscall_register(SYS_GET_MEMPOLICY, sys_get_mempolicy);
    syscall_register(SYS_SET_MEMPOLICY, sys_set_mempolicy);
    syscall_register(SYS_MIGRATE_PAGES, sys_migrate_pages);
    syscall_register(SYS_MOVE_PAGES, sys_move_pages);
}

/**
 * mm.c - Horizon kernel memory management-related system calls
 *
 * This file contains the implementation of memory management-related system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/mm.h>
#include <horizon/vmm.h>
#include <horizon/task.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Memory map system call */
long sys_mmap(long addr, long length, long prot, long flags, long fd, long offset) {
    /* Map a file into memory */
    struct file *file = NULL;
    if (fd >= 0 && fd < task_current()->files->max_fds) {
        file = task_current()->files->fd_array[fd];
    }

    /* Map the memory */
    void *mapped_addr = vmm_mmap(task_current()->mm, (void *)addr, length, prot, flags, file, offset);
    if (mapped_addr == NULL) {
        return -ENOMEM;
    }

    return (long)mapped_addr;
}

/* Memory unmap system call */
long sys_munmap(long addr, long length, long arg3, long arg4, long arg5, long arg6) {
    /* Unmap memory */
    return vmm_munmap(task_current()->mm, (void *)addr, length);
}

/* Break system call */
long sys_brk(long brk, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Change the program break */
    return vmm_brk(task_current()->mm, brk);
}

/* Memory protect system call */
long sys_mprotect(long addr, long len, long prot, long arg4, long arg5, long arg6) {
    /* Change protection of memory mapping */
    return vmm_mprotect(task_current()->mm, (void *)addr, len, prot);
}

/* Memory remap system call */
long sys_mremap(long old_address, long old_size, long new_size, long flags, long new_address, long arg6) {
    /* Remap a virtual memory area */
    return (long)vmm_mremap(task_current()->mm, (void *)old_address, old_size, new_size, flags, (void *)new_address);
}

/* Memory lock system call */
long sys_mlock(long addr, long len, long arg3, long arg4, long arg5, long arg6) {
    /* Lock memory */
    return vmm_mlock(task_current()->mm, (void *)addr, len);
}

/* Memory unlock system call */
long sys_munlock(long addr, long len, long arg3, long arg4, long arg5, long arg6) {
    /* Unlock memory */
    return vmm_munlock(task_current()->mm, (void *)addr, len);
}

/* Memory lock all system call */
long sys_mlockall(long flags, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Lock all memory */
    return vmm_mlockall(task_current()->mm, flags);
}

/* Memory unlock all system call */
long sys_munlockall(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Unlock all memory */
    return vmm_munlockall(task_current()->mm);
}

/* Memory advise system call */
long sys_madvise(long addr, long length, long advice, long arg4, long arg5, long arg6) {
    /* Give advice about memory usage */
    return vmm_madvise(task_current()->mm, (void *)addr, length, advice);
}

/* Memory sync system call */
long sys_msync(long addr, long length, long flags, long arg4, long arg5, long arg6) {
    /* Synchronize memory with physical storage */
    return vmm_msync(task_current()->mm, (void *)addr, length, flags);
}

/* Memory core system call */
long sys_mincore(long addr, long length, long vec, long arg4, long arg5, long arg6) {
    /* Determine whether pages are in core */
    return vmm_mincore(task_current()->mm, (void *)addr, length, (unsigned char *)vec);
}

/* Initialize memory management-related system calls */
void mm_syscalls_init(void) {
    /* Register memory management-related system calls */
    syscall_register(SYS_BRK, sys_brk);
    syscall_register(SYS_MMAP, sys_mmap);
    syscall_register(SYS_MUNMAP, sys_munmap);
    syscall_register(SYS_MPROTECT, sys_mprotect);
    syscall_register(SYS_MREMAP, sys_mremap);
    syscall_register(SYS_MLOCK, sys_mlock);
    syscall_register(SYS_MUNLOCK, sys_munlock);
    syscall_register(SYS_MLOCKALL, sys_mlockall);
    syscall_register(SYS_MUNLOCKALL, sys_munlockall);
    syscall_register(SYS_MADVISE, sys_madvise);
    syscall_register(SYS_MSYNC, sys_msync);
    syscall_register(SYS_MINCORE, sys_mincore);
}

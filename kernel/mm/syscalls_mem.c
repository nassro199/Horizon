/**
 * syscalls_mem.c - Horizon kernel memory system calls
 * 
 * This file contains the implementation of the memory system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: madvise */
long sys_madvise(long start, long len, long advice, long unused1, long unused2, long unused3) {
    /* Give advice about use of memory */
    return mm_madvise((void *)start, len, advice);
}

/* System call: mbind */
long sys_mbind(long start, long len, long mode, long nodemask, long maxnode, long flags) {
    /* Set the memory policy for a memory range */
    return mm_mbind((void *)start, len, mode, (const unsigned long *)nodemask, maxnode, flags);
}

/* System call: get_mempolicy */
long sys_get_mempolicy(long policy, long nodemask, long maxnode, long addr, long flags, long unused1) {
    /* Get the memory policy */
    return mm_get_mempolicy((int *)policy, (unsigned long *)nodemask, maxnode, addr, flags);
}

/* System call: set_mempolicy */
long sys_set_mempolicy(long mode, long nodemask, long maxnode, long unused1, long unused2, long unused3) {
    /* Set the memory policy */
    return mm_set_mempolicy(mode, (const unsigned long *)nodemask, maxnode);
}

/* System call: migrate_pages */
long sys_migrate_pages(long pid, long maxnode, long old_nodes, long new_nodes, long unused1, long unused2) {
    /* Migrate pages */
    return mm_migrate_pages(pid, maxnode, (const unsigned long *)old_nodes, (const unsigned long *)new_nodes);
}

/* System call: move_pages */
long sys_move_pages(long pid, long nr_pages, long pages, long nodes, long status, long flags) {
    /* Move pages */
    return mm_move_pages(pid, nr_pages, (void **)pages, (const int *)nodes, (int *)status, flags);
}

/* System call: mincore */
long sys_mincore(long start, long len, long vec, long unused1, long unused2, long unused3) {
    /* Determine whether pages are in memory */
    return mm_mincore((void *)start, len, (unsigned char *)vec);
}

/* System call: mlock */
long sys_mlock(long start, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Lock memory */
    return mm_mlock((void *)start, len);
}

/* System call: munlock */
long sys_munlock(long start, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Unlock memory */
    return mm_munlock((void *)start, len);
}

/* System call: mlockall */
long sys_mlockall(long flags, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Lock all memory */
    return mm_mlockall(flags);
}

/* System call: munlockall */
long sys_munlockall(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Unlock all memory */
    return mm_munlockall();
}

/* System call: mprotect */
long sys_mprotect(long start, long len, long prot, long unused1, long unused2, long unused3) {
    /* Change memory protection */
    return mm_mprotect((void *)start, len, prot);
}

/* System call: msync */
long sys_msync(long start, long len, long flags, long unused1, long unused2, long unused3) {
    /* Synchronize memory with physical storage */
    return mm_msync((void *)start, len, flags);
}

/* System call: mmap */
long sys_mmap(long addr, long len, long prot, long flags, long fd, long offset) {
    /* Map files or devices into memory */
    return mm_mmap((void *)addr, len, prot, flags, fd, offset);
}

/* System call: mmap2 */
long sys_mmap2(long addr, long len, long prot, long flags, long fd, long pgoff) {
    /* Map files or devices into memory */
    return mm_mmap2((void *)addr, len, prot, flags, fd, pgoff);
}

/* System call: munmap */
long sys_munmap(long addr, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Unmap files or devices from memory */
    return mm_munmap((void *)addr, len);
}

/* System call: mremap */
long sys_mremap(long old_addr, long old_size, long new_size, long flags, long new_addr, long unused1) {
    /* Remap a virtual memory address */
    return mm_mremap((void *)old_addr, old_size, new_size, flags, (void *)new_addr);
}

/* System call: remap_file_pages */
long sys_remap_file_pages(long start, long size, long prot, long pgoff, long flags) {
    /* Create a nonlinear file mapping */
    return mm_remap_file_pages((void *)start, size, prot, pgoff, flags);
}

/* System call: brk */
long sys_brk(long brk, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Change data segment size */
    return mm_brk((void *)brk);
}

/* System call: sbrk */
long sys_sbrk(long increment, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Change data segment size */
    return mm_sbrk(increment);
}

/* Register memory system calls */
void mm_syscalls_init(void) {
    /* Register memory system calls */
    syscall_register(SYS_MADVISE, sys_madvise);
    syscall_register(SYS_MBIND, sys_mbind);
    syscall_register(SYS_GET_MEMPOLICY, sys_get_mempolicy);
    syscall_register(SYS_SET_MEMPOLICY, sys_set_mempolicy);
    syscall_register(SYS_MIGRATE_PAGES, sys_migrate_pages);
    syscall_register(SYS_MOVE_PAGES, sys_move_pages);
    syscall_register(SYS_MINCORE, sys_mincore);
    syscall_register(SYS_MLOCK, sys_mlock);
    syscall_register(SYS_MUNLOCK, sys_munlock);
    syscall_register(SYS_MLOCKALL, sys_mlockall);
    syscall_register(SYS_MUNLOCKALL, sys_munlockall);
    syscall_register(SYS_MPROTECT, sys_mprotect);
    syscall_register(SYS_MSYNC, sys_msync);
    syscall_register(SYS_MMAP, sys_mmap);
    syscall_register(SYS_MMAP2, sys_mmap2);
    syscall_register(SYS_MUNMAP, sys_munmap);
    syscall_register(SYS_MREMAP, sys_mremap);
    syscall_register(SYS_REMAP_FILE_PAGES, sys_remap_file_pages);
    syscall_register(SYS_BRK, sys_brk);
    syscall_register(SYS_SBRK, sys_sbrk);
}

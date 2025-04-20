/**
 * syscalls_advio.c - Horizon kernel advanced I/O system calls
 * 
 * This file contains the implementation of the advanced I/O system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: readv */
long sys_readv(long fd, long iov, long iovcnt, long unused1, long unused2, long unused3) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Read from the file */
    return file_readv(file, (const struct iovec *)iov, iovcnt);
}

/* System call: writev */
long sys_writev(long fd, long iov, long iovcnt, long unused1, long unused2, long unused3) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Write to the file */
    return file_writev(file, (const struct iovec *)iov, iovcnt);
}

/* System call: preadv */
long sys_preadv(long fd, long iov, long iovcnt, long pos_l, long pos_h, long unused1) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Save the current position */
    loff_t old_pos = file->f_pos;
    
    /* Set the new position */
    file->f_pos = ((loff_t)pos_h << 32) | pos_l;
    
    /* Read from the file */
    ssize_t result = file_readv(file, (const struct iovec *)iov, iovcnt);
    
    /* Restore the old position */
    file->f_pos = old_pos;
    
    return result;
}

/* System call: pwritev */
long sys_pwritev(long fd, long iov, long iovcnt, long pos_l, long pos_h, long unused1) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Save the current position */
    loff_t old_pos = file->f_pos;
    
    /* Set the new position */
    file->f_pos = ((loff_t)pos_h << 32) | pos_l;
    
    /* Write to the file */
    ssize_t result = file_writev(file, (const struct iovec *)iov, iovcnt);
    
    /* Restore the old position */
    file->f_pos = old_pos;
    
    return result;
}

/* System call: preadv2 */
long sys_preadv2(long fd, long iov, long iovcnt, long pos_l, long pos_h, long flags) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Save the current position */
    loff_t old_pos = file->f_pos;
    
    /* Set the new position */
    file->f_pos = ((loff_t)pos_h << 32) | pos_l;
    
    /* Save the current flags */
    unsigned int old_flags = file->f_flags;
    
    /* Set the new flags */
    if (flags & RWF_HIPRI) {
        file->f_flags |= O_NONBLOCK;
    }
    
    /* Read from the file */
    ssize_t result = file_readv(file, (const struct iovec *)iov, iovcnt);
    
    /* Restore the old flags */
    file->f_flags = old_flags;
    
    /* Restore the old position */
    file->f_pos = old_pos;
    
    return result;
}

/* System call: pwritev2 */
long sys_pwritev2(long fd, long iov, long iovcnt, long pos_l, long pos_h, long flags) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Save the current position */
    loff_t old_pos = file->f_pos;
    
    /* Set the new position */
    file->f_pos = ((loff_t)pos_h << 32) | pos_l;
    
    /* Save the current flags */
    unsigned int old_flags = file->f_flags;
    
    /* Set the new flags */
    if (flags & RWF_HIPRI) {
        file->f_flags |= O_NONBLOCK;
    }
    
    if (flags & RWF_DSYNC) {
        file->f_flags |= O_SYNC;
    }
    
    if (flags & RWF_SYNC) {
        file->f_flags |= O_SYNC;
    }
    
    if (flags & RWF_APPEND) {
        file->f_flags |= O_APPEND;
    }
    
    /* Write to the file */
    ssize_t result = file_writev(file, (const struct iovec *)iov, iovcnt);
    
    /* Restore the old flags */
    file->f_flags = old_flags;
    
    /* Restore the old position */
    file->f_pos = old_pos;
    
    return result;
}

/* System call: sendfile */
long sys_sendfile(long out_fd, long in_fd, long offset, long count, long unused1, long unused2) {
    /* Get the files */
    file_t *out_file = process_get_file(task_current(), out_fd);
    file_t *in_file = process_get_file(task_current(), in_fd);
    
    if (out_file == NULL || in_file == NULL) {
        return -1;
    }
    
    /* Transfer the data */
    return file_sendfile(out_file, in_file, (off_t *)offset, count);
}

/* System call: sendfile64 */
long sys_sendfile64(long out_fd, long in_fd, long offset, long count, long unused1, long unused2) {
    /* Call the normal sendfile */
    return sys_sendfile(out_fd, in_fd, offset, count, unused1, unused2);
}

/* System call: copy_file_range */
long sys_copy_file_range(long fd_in, long off_in, long fd_out, long off_out, long len, long flags) {
    /* Get the files */
    file_t *in_file = process_get_file(task_current(), fd_in);
    file_t *out_file = process_get_file(task_current(), fd_out);
    
    if (in_file == NULL || out_file == NULL) {
        return -1;
    }
    
    /* Copy the data */
    return file_copy_file_range(in_file, (loff_t *)off_in, out_file, (loff_t *)off_out, len, flags);
}

/* System call: splice */
long sys_splice(long fd_in, long off_in, long fd_out, long off_out, long len, long flags) {
    /* Get the files */
    file_t *in_file = process_get_file(task_current(), fd_in);
    file_t *out_file = process_get_file(task_current(), fd_out);
    
    if (in_file == NULL || out_file == NULL) {
        return -1;
    }
    
    /* Splice the data */
    return file_splice(in_file, (loff_t *)off_in, out_file, (loff_t *)off_out, len, flags);
}

/* System call: tee */
long sys_tee(long fd_in, long fd_out, long len, long flags, long unused1, long unused2) {
    /* Get the files */
    file_t *in_file = process_get_file(task_current(), fd_in);
    file_t *out_file = process_get_file(task_current(), fd_out);
    
    if (in_file == NULL || out_file == NULL) {
        return -1;
    }
    
    /* Tee the data */
    return file_tee(in_file, out_file, len, flags);
}

/* System call: vmsplice */
long sys_vmsplice(long fd, long iov, long nr_segs, long flags, long unused1, long unused2) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Splice the data */
    return file_vmsplice(file, (const struct iovec *)iov, nr_segs, flags);
}

/* System call: sync_file_range */
long sys_sync_file_range(long fd, long offset_low, long offset_high, long nbytes_low, long nbytes_high, long flags) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Calculate the offset and nbytes */
    loff_t offset = ((loff_t)offset_high << 32) | offset_low;
    loff_t nbytes = ((loff_t)nbytes_high << 32) | nbytes_low;
    
    /* Synchronize the file range */
    return file_sync_file_range(file, offset, nbytes, flags);
}

/* System call: sync_file_range2 */
long sys_sync_file_range2(long fd, long flags, long offset_low, long offset_high, long nbytes_low, long nbytes_high) {
    /* Call the normal sync_file_range */
    return sys_sync_file_range(fd, offset_low, offset_high, nbytes_low, nbytes_high, flags);
}

/* System call: fallocate */
long sys_fallocate(long fd, long mode, long offset_low, long offset_high, long len_low, long len_high) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Calculate the offset and len */
    loff_t offset = ((loff_t)offset_high << 32) | offset_low;
    loff_t len = ((loff_t)len_high << 32) | len_low;
    
    /* Allocate space for the file */
    return file_fallocate(file, mode, offset, len);
}

/* System call: fsync */
long sys_fsync(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Synchronize the file */
    return file_fsync(file, 0);
}

/* System call: fdatasync */
long sys_fdatasync(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Synchronize the file data */
    return file_fdatasync(file);
}

/* System call: fadvise64 */
long sys_fadvise64(long fd, long offset_low, long offset_high, long len_low, long len_high, long advice) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Calculate the offset and len */
    loff_t offset = ((loff_t)offset_high << 32) | offset_low;
    loff_t len = ((loff_t)len_high << 32) | len_low;
    
    /* Advise the file */
    return file_fadvise(file, offset, len, advice);
}

/* System call: fadvise64_64 */
long sys_fadvise64_64(long fd, long advice, long offset_low, long offset_high, long len_low, long len_high) {
    /* Call the normal fadvise64 */
    return sys_fadvise64(fd, offset_low, offset_high, len_low, len_high, advice);
}

/* System call: readahead */
long sys_readahead(long fd, long offset_low, long offset_high, long count, long unused1, long unused2) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Calculate the offset */
    loff_t offset = ((loff_t)offset_high << 32) | offset_low;
    
    /* Read ahead */
    return file_readahead(file, offset, count);
}

/* System call: get_file_seals */
long sys_get_file_seals(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the file seals */
    return file_get_seals(file);
}

/* System call: set_file_seals */
long sys_set_file_seals(long fd, long seals, long unused1, long unused2, long unused3, long unused4) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Set the file seals */
    return file_set_seals(file, seals);
}

/* Register advanced I/O system calls */
void fs_advio_syscalls_init(void) {
    /* Register advanced I/O system calls */
    syscall_register(SYS_READV, sys_readv);
    syscall_register(SYS_WRITEV, sys_writev);
    syscall_register(SYS_PREADV, sys_preadv);
    syscall_register(SYS_PWRITEV, sys_pwritev);
    syscall_register(SYS_PREADV2, sys_preadv2);
    syscall_register(SYS_PWRITEV2, sys_pwritev2);
    syscall_register(SYS_SENDFILE, sys_sendfile);
    syscall_register(SYS_SENDFILE64, sys_sendfile64);
    syscall_register(SYS_COPY_FILE_RANGE, sys_copy_file_range);
    syscall_register(SYS_SPLICE, sys_splice);
    syscall_register(SYS_TEE, sys_tee);
    syscall_register(SYS_VMSPLICE, sys_vmsplice);
    syscall_register(SYS_SYNC_FILE_RANGE, sys_sync_file_range);
    syscall_register(SYS_SYNC_FILE_RANGE2, sys_sync_file_range2);
    syscall_register(SYS_FALLOCATE, sys_fallocate);
    syscall_register(SYS_FSYNC, sys_fsync);
    syscall_register(SYS_FDATASYNC, sys_fdatasync);
    syscall_register(SYS_FADVISE64, sys_fadvise64);
    syscall_register(SYS_FADVISE64_64, sys_fadvise64_64);
    syscall_register(SYS_READAHEAD, sys_readahead);
    syscall_register(SYS_GET_FILE_SEALS, sys_get_file_seals);
    syscall_register(SYS_SET_FILE_SEALS, sys_set_file_seals);
}

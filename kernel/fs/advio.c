/**
 * advio.c - Horizon kernel advanced I/O operations
 * 
 * This file contains the implementation of advanced I/O operations.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Scatter/gather I/O vector structure
 */
struct iovec {
    void *iov_base;     /* Starting address */
    size_t iov_len;     /* Number of bytes to transfer */
};

/**
 * Read data from a file descriptor into multiple buffers
 * 
 * @param file The file to read from
 * @param iov The I/O vector array
 * @param iovcnt The number of I/O vectors
 * @return The number of bytes read, or a negative error code
 */
ssize_t file_readv(file_t *file, const struct iovec *iov, int iovcnt) {
    /* Check parameters */
    if (file == NULL || iov == NULL || iovcnt <= 0) {
        return -1;
    }
    
    /* Check if the file is readable */
    if (!(file->f_mode & FMODE_READ)) {
        return -1;
    }
    
    /* Call the readv operation if available */
    if (file->f_op && file->f_op->readv) {
        return file->f_op->readv(file, iov, iovcnt, &file->f_pos);
    }
    
    /* Fallback to read */
    ssize_t total = 0;
    
    for (int i = 0; i < iovcnt; i++) {
        /* Skip empty buffers */
        if (iov[i].iov_len == 0) {
            continue;
        }
        
        /* Read into the buffer */
        ssize_t bytes = file_read(file, iov[i].iov_base, iov[i].iov_len);
        
        if (bytes < 0) {
            /* Return error if no data has been read */
            if (total == 0) {
                return bytes;
            }
            
            /* Otherwise, return the data read so far */
            break;
        }
        
        /* Update the total */
        total += bytes;
        
        /* Stop if we read less than requested */
        if (bytes < iov[i].iov_len) {
            break;
        }
    }
    
    return total;
}

/**
 * Write data from multiple buffers to a file descriptor
 * 
 * @param file The file to write to
 * @param iov The I/O vector array
 * @param iovcnt The number of I/O vectors
 * @return The number of bytes written, or a negative error code
 */
ssize_t file_writev(file_t *file, const struct iovec *iov, int iovcnt) {
    /* Check parameters */
    if (file == NULL || iov == NULL || iovcnt <= 0) {
        return -1;
    }
    
    /* Check if the file is writable */
    if (!(file->f_mode & FMODE_WRITE)) {
        return -1;
    }
    
    /* Call the writev operation if available */
    if (file->f_op && file->f_op->writev) {
        return file->f_op->writev(file, iov, iovcnt, &file->f_pos);
    }
    
    /* Fallback to write */
    ssize_t total = 0;
    
    for (int i = 0; i < iovcnt; i++) {
        /* Skip empty buffers */
        if (iov[i].iov_len == 0) {
            continue;
        }
        
        /* Write from the buffer */
        ssize_t bytes = file_write(file, iov[i].iov_base, iov[i].iov_len);
        
        if (bytes < 0) {
            /* Return error if no data has been written */
            if (total == 0) {
                return bytes;
            }
            
            /* Otherwise, return the data written so far */
            break;
        }
        
        /* Update the total */
        total += bytes;
        
        /* Stop if we wrote less than requested */
        if (bytes < iov[i].iov_len) {
            break;
        }
    }
    
    return total;
}

/**
 * Transfer data between file descriptors
 * 
 * @param out_fd The output file descriptor
 * @param in_fd The input file descriptor
 * @param offset The offset in the input file, or NULL to use the current offset
 * @param count The number of bytes to transfer
 * @return The number of bytes transferred, or a negative error code
 */
ssize_t file_sendfile(file_t *out_file, file_t *in_file, off_t *offset, size_t count) {
    /* Check parameters */
    if (out_file == NULL || in_file == NULL) {
        return -1;
    }
    
    /* Check if the files are readable/writable */
    if (!(in_file->f_mode & FMODE_READ) || !(out_file->f_mode & FMODE_WRITE)) {
        return -1;
    }
    
    /* Call the sendfile operation if available */
    if (in_file->f_op && in_file->f_op->sendfile) {
        return in_file->f_op->sendfile(out_file, in_file, offset, count);
    }
    
    /* Fallback to read/write */
    off_t pos = 0;
    
    /* Get the current position if offset is NULL */
    if (offset == NULL) {
        pos = in_file->f_pos;
    } else {
        pos = *offset;
    }
    
    /* Allocate a buffer */
    char *buffer = kmalloc(PAGE_SIZE, MEM_KERNEL);
    
    if (buffer == NULL) {
        return -1;
    }
    
    /* Transfer the data */
    ssize_t total = 0;
    
    while (count > 0) {
        /* Calculate the number of bytes to read */
        size_t bytes_to_read = count;
        
        if (bytes_to_read > PAGE_SIZE) {
            bytes_to_read = PAGE_SIZE;
        }
        
        /* Set the position */
        in_file->f_pos = pos;
        
        /* Read from the input file */
        ssize_t bytes_read = file_read(in_file, buffer, bytes_to_read);
        
        if (bytes_read <= 0) {
            break;
        }
        
        /* Write to the output file */
        ssize_t bytes_written = file_write(out_file, buffer, bytes_read);
        
        if (bytes_written < bytes_read) {
            /* Update the position */
            pos += bytes_written;
            
            /* Update the offset if provided */
            if (offset != NULL) {
                *offset = pos;
            }
            
            /* Update the total */
            total += bytes_written;
            
            /* Free the buffer */
            kfree(buffer);
            
            return total;
        }
        
        /* Update the position */
        pos += bytes_read;
        
        /* Update the count */
        count -= bytes_read;
        
        /* Update the total */
        total += bytes_read;
    }
    
    /* Update the offset if provided */
    if (offset != NULL) {
        *offset = pos;
    }
    
    /* Free the buffer */
    kfree(buffer);
    
    return total;
}

/**
 * Synchronize a file's in-core state with storage device
 * 
 * @param file The file to synchronize
 * @param datasync Only synchronize essential metadata if non-zero
 * @return 0 on success, or a negative error code
 */
int file_fsync(file_t *file, int datasync) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Call the fsync operation if available */
    if (file->f_op && file->f_op->fsync) {
        return file->f_op->fsync(file, datasync);
    }
    
    return 0;
}

/**
 * Synchronize a file's in-core state with storage device (data only)
 * 
 * @param file The file to synchronize
 * @return 0 on success, or a negative error code
 */
int file_fdatasync(file_t *file) {
    /* Call fsync with datasync flag */
    return file_fsync(file, 1);
}

/**
 * Synchronize a file range with storage device
 * 
 * @param file The file to synchronize
 * @param offset The offset in the file
 * @param nbytes The number of bytes to synchronize
 * @param flags The flags
 * @return 0 on success, or a negative error code
 */
int file_sync_file_range(file_t *file, off_t offset, off_t nbytes, unsigned int flags) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Call the sync_file_range operation if available */
    if (file->f_op && file->f_op->sync_file_range) {
        return file->f_op->sync_file_range(file, offset, nbytes, flags);
    }
    
    /* Fallback to fsync */
    return file_fsync(file, 0);
}

/**
 * Advise the kernel about the expected behavior of file access
 * 
 * @param file The file to advise
 * @param offset The offset in the file
 * @param len The length of the region
 * @param advice The advice
 * @return 0 on success, or a negative error code
 */
int file_fadvise(file_t *file, off_t offset, off_t len, int advice) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Call the fadvise operation if available */
    if (file->f_op && file->f_op->fadvise) {
        return file->f_op->fadvise(file, offset, len, advice);
    }
    
    return 0;
}

/**
 * Allocate space for a file
 * 
 * @param file The file to allocate space for
 * @param mode The mode
 * @param offset The offset in the file
 * @param len The length to allocate
 * @return 0 on success, or a negative error code
 */
int file_fallocate(file_t *file, int mode, off_t offset, off_t len) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Call the fallocate operation if available */
    if (file->f_op && file->f_op->fallocate) {
        return file->f_op->fallocate(file, mode, offset, len);
    }
    
    return -1;
}

/**
 * Get file seals
 * 
 * @param file The file to get seals for
 * @return The file seals, or a negative error code
 */
int file_get_seals(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Call the get_seals operation if available */
    if (file->f_op && file->f_op->get_seals) {
        return file->f_op->get_seals(file);
    }
    
    return 0;
}

/**
 * Set file seals
 * 
 * @param file The file to set seals for
 * @param seals The seals to set
 * @return 0 on success, or a negative error code
 */
int file_set_seals(file_t *file, int seals) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Call the set_seals operation if available */
    if (file->f_op && file->f_op->set_seals) {
        return file->f_op->set_seals(file, seals);
    }
    
    return -1;
}

/**
 * Get the size of a file
 * 
 * @param file The file to get the size of
 * @return The file size, or a negative error code
 */
off_t file_size(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Get the size from the inode */
    return file->f_inode->i_size;
}

/**
 * Check if a file is a directory
 * 
 * @param file The file to check
 * @return 1 if the file is a directory, 0 otherwise
 */
int file_is_dir(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return 0;
    }
    
    /* Check if the file is a directory */
    return S_ISDIR(file->f_inode->i_mode);
}

/**
 * Check if a file is a regular file
 * 
 * @param file The file to check
 * @return 1 if the file is a regular file, 0 otherwise
 */
int file_is_regular(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return 0;
    }
    
    /* Check if the file is a regular file */
    return S_ISREG(file->f_inode->i_mode);
}

/**
 * Check if a file is a symbolic link
 * 
 * @param file The file to check
 * @return 1 if the file is a symbolic link, 0 otherwise
 */
int file_is_symlink(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return 0;
    }
    
    /* Check if the file is a symbolic link */
    return S_ISLNK(file->f_inode->i_mode);
}

/**
 * Check if a file is a block device
 * 
 * @param file The file to check
 * @return 1 if the file is a block device, 0 otherwise
 */
int file_is_block_device(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return 0;
    }
    
    /* Check if the file is a block device */
    return S_ISBLK(file->f_inode->i_mode);
}

/**
 * Check if a file is a character device
 * 
 * @param file The file to check
 * @return 1 if the file is a character device, 0 otherwise
 */
int file_is_char_device(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return 0;
    }
    
    /* Check if the file is a character device */
    return S_ISCHR(file->f_inode->i_mode);
}

/**
 * Check if a file is a FIFO
 * 
 * @param file The file to check
 * @return 1 if the file is a FIFO, 0 otherwise
 */
int file_is_fifo(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return 0;
    }
    
    /* Check if the file is a FIFO */
    return S_ISFIFO(file->f_inode->i_mode);
}

/**
 * Check if a file is a socket
 * 
 * @param file The file to check
 * @return 1 if the file is a socket, 0 otherwise
 */
int file_is_socket(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return 0;
    }
    
    /* Check if the file is a socket */
    return S_ISSOCK(file->f_inode->i_mode);
}

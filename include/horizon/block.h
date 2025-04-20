/**
 * block.h - Block device subsystem definitions
 * 
 * This file contains definitions for the block device subsystem.
 */

#ifndef _KERNEL_BLOCK_H
#define _KERNEL_BLOCK_H

#include <horizon/types.h>
#include <horizon/device.h>

/* Block device operations */
typedef struct block_device_ops {
    int (*open)(struct block_device *dev);
    int (*close)(struct block_device *dev);
    int (*read)(struct block_device *dev, u64 sector, u32 count, void *buffer);
    int (*write)(struct block_device *dev, u64 sector, u32 count, const void *buffer);
    int (*ioctl)(struct block_device *dev, u32 request, void *arg);
    int (*flush)(struct block_device *dev);
} block_device_ops_t;

/* Block device structure */
typedef struct block_device {
    device_t device;                /* Base device structure */
    u32 sector_size;                /* Sector size in bytes */
    u64 sector_count;               /* Number of sectors */
    block_device_ops_t *ops;        /* Block device operations */
    void *private_data;             /* Private data */
    struct block_device *next;      /* Next block device in list */
} block_device_t;

/* Block device functions */
void block_init(void);
int block_register_device(block_device_t *dev);
int block_unregister_device(block_device_t *dev);
block_device_t *block_get_device(const char *name);
int block_read(block_device_t *dev, u64 sector, u32 count, void *buffer);
int block_write(block_device_t *dev, u64 sector, u32 count, const void *buffer);
int block_ioctl(block_device_t *dev, u32 request, void *arg);
int block_flush(block_device_t *dev);

#endif /* _KERNEL_BLOCK_H */

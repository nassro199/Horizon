/**
 * char.h - Horizon kernel character device definitions
 * 
 * This file contains definitions for the character device subsystem.
 */

#ifndef _KERNEL_CHAR_H
#define _KERNEL_CHAR_H

#include <horizon/types.h>
#include <horizon/device.h>

/* Character device operations */
typedef struct char_device_ops {
    int (*open)(struct char_device *dev, u32 flags);
    int (*close)(struct char_device *dev);
    ssize_t (*read)(struct char_device *dev, void *buf, size_t count);
    ssize_t (*write)(struct char_device *dev, const void *buf, size_t count);
    int (*ioctl)(struct char_device *dev, u32 request, void *arg);
    off_t (*seek)(struct char_device *dev, off_t offset, int whence);
    int (*flush)(struct char_device *dev);
} char_device_ops_t;

/* Character device structure */
typedef struct char_device {
    device_t device;                /* Base device structure */
    u32 major;                      /* Major device number */
    u32 minor;                      /* Minor device number */
    char_device_ops_t *ops;         /* Character device operations */
    void *private_data;             /* Private data */
    struct char_device *next;       /* Next character device in list */
} char_device_t;

/* Character device functions */
void char_init(void);
int char_register_device(char_device_t *dev);
int char_unregister_device(char_device_t *dev);
char_device_t *char_get_device(u32 major, u32 minor);
int char_open(char_device_t *dev, u32 flags);
int char_close(char_device_t *dev);
ssize_t char_read(char_device_t *dev, void *buf, size_t count);
ssize_t char_write(char_device_t *dev, const void *buf, size_t count);
int char_ioctl(char_device_t *dev, u32 request, void *arg);
off_t char_seek(char_device_t *dev, off_t offset, int whence);
int char_flush(char_device_t *dev);

#endif /* _KERNEL_CHAR_H */

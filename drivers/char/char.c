/**
 * char.c - Horizon kernel character device implementation
 * 
 * This file contains the implementation of the character device subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/char.h>
#include <horizon/device.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Character device class */
static device_class_t char_device_class;

/* List of character devices */
static list_head_t char_devices_list;

/* Initialize the character device subsystem */
void char_init(void) {
    /* Initialize the character devices list */
    list_init(&char_devices_list);
    
    /* Initialize the character device class */
    strcpy(char_device_class.name, "char");
    char_device_class.type = DEVICE_CLASS_CHAR;
    
    /* Register the character device class */
    class_register(&char_device_class);
}

/* Register a character device */
int char_register_device(char_device_t *dev) {
    if (dev == NULL || dev->ops == NULL) {
        return -1;
    }
    
    /* Check if the device already exists */
    char_device_t *existing = char_get_device(dev->major, dev->minor);
    if (existing != NULL) {
        return -1;
    }
    
    /* Initialize the device structure */
    if (dev->device.name[0] == '\0') {
        snprintf(dev->device.name, sizeof(dev->device.name), "char%u:%u", dev->major, dev->minor);
    }
    
    dev->device.class = &char_device_class;
    dev->device.private_data = dev;
    
    /* Add the device to the character devices list */
    list_add_tail(&dev->device.driver_list, &char_devices_list);
    
    /* Register the device */
    return device_register(&dev->device);
}

/* Unregister a character device */
int char_unregister_device(char_device_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Unregister the device */
    int result = device_unregister(&dev->device);
    
    if (result < 0) {
        return result;
    }
    
    /* Remove the device from the character devices list */
    list_del(&dev->device.driver_list);
    
    return 0;
}

/* Get a character device by major and minor numbers */
char_device_t *char_get_device(u32 major, u32 minor) {
    /* Iterate over the character devices list */
    list_head_t *pos;
    
    list_for_each(pos, &char_devices_list) {
        device_t *dev = list_entry(pos, device_t, driver_list);
        char_device_t *char_dev = (char_device_t *)dev->private_data;
        
        if (char_dev != NULL && char_dev->major == major && char_dev->minor == minor) {
            return char_dev;
        }
    }
    
    return NULL;
}

/* Open a character device */
int char_open(char_device_t *dev, u32 flags) {
    if (dev == NULL || dev->ops == NULL || dev->ops->open == NULL) {
        return -1;
    }
    
    return dev->ops->open(dev, flags);
}

/* Close a character device */
int char_close(char_device_t *dev) {
    if (dev == NULL || dev->ops == NULL || dev->ops->close == NULL) {
        return -1;
    }
    
    return dev->ops->close(dev);
}

/* Read from a character device */
ssize_t char_read(char_device_t *dev, void *buf, size_t count) {
    if (dev == NULL || dev->ops == NULL || dev->ops->read == NULL || buf == NULL) {
        return -1;
    }
    
    return dev->ops->read(dev, buf, count);
}

/* Write to a character device */
ssize_t char_write(char_device_t *dev, const void *buf, size_t count) {
    if (dev == NULL || dev->ops == NULL || dev->ops->write == NULL || buf == NULL) {
        return -1;
    }
    
    return dev->ops->write(dev, buf, count);
}

/* Perform an I/O control operation on a character device */
int char_ioctl(char_device_t *dev, u32 request, void *arg) {
    if (dev == NULL || dev->ops == NULL || dev->ops->ioctl == NULL) {
        return -1;
    }
    
    return dev->ops->ioctl(dev, request, arg);
}

/* Seek to a position in a character device */
off_t char_seek(char_device_t *dev, off_t offset, int whence) {
    if (dev == NULL || dev->ops == NULL || dev->ops->seek == NULL) {
        return -1;
    }
    
    return dev->ops->seek(dev, offset, whence);
}

/* Flush a character device */
int char_flush(char_device_t *dev) {
    if (dev == NULL || dev->ops == NULL || dev->ops->flush == NULL) {
        return -1;
    }
    
    return dev->ops->flush(dev);
}

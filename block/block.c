/**
 * block.c - Block device subsystem implementation
 * 
 * This file contains the implementation of the block device subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/block.h>
#include <horizon/string.h>

/* Block device list */
static block_device_t *block_devices = NULL;

/* Initialize the block device subsystem */
void block_init(void)
{
    /* Initialize the block device list */
    block_devices = NULL;
}

/* Register a block device */
int block_register_device(block_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL) {
        return -1;
    }
    
    /* Check if the device already exists */
    block_device_t *existing = block_devices;
    
    while (existing != NULL) {
        if (strcmp(existing->device.name, dev->device.name) == 0) {
            /* Device already exists */
            return -1;
        }
        
        existing = existing->next;
    }
    
    /* Add to the block device list */
    dev->next = block_devices;
    block_devices = dev;
    
    return 0;
}

/* Unregister a block device */
int block_unregister_device(block_device_t *dev)
{
    if (dev == NULL) {
        return -1;
    }
    
    /* Find the device in the list */
    block_device_t *current = block_devices;
    block_device_t *prev = NULL;
    
    while (current != NULL) {
        if (current == dev) {
            /* Found the device */
            if (prev == NULL) {
                /* First device in the list */
                block_devices = current->next;
            } else {
                /* Not the first device in the list */
                prev->next = current->next;
            }
            
            return 0;
        }
        
        prev = current;
        current = current->next;
    }
    
    /* Device not found */
    return -1;
}

/* Get a block device by name */
block_device_t *block_get_device(const char *name)
{
    if (name == NULL) {
        return NULL;
    }
    
    /* Find the device in the list */
    block_device_t *dev = block_devices;
    
    while (dev != NULL) {
        if (strcmp(dev->device.name, name) == 0) {
            /* Found the device */
            return dev;
        }
        
        dev = dev->next;
    }
    
    /* Device not found */
    return NULL;
}

/* Read from a block device */
int block_read(block_device_t *dev, u64 sector, u32 count, void *buffer)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->read == NULL || buffer == NULL) {
        return -1;
    }
    
    /* Check if the sector is valid */
    if (sector >= dev->sector_count) {
        return -1;
    }
    
    /* Check if the count is valid */
    if (count == 0 || sector + count > dev->sector_count) {
        return -1;
    }
    
    /* Read from the device */
    return dev->ops->read(dev, sector, count, buffer);
}

/* Write to a block device */
int block_write(block_device_t *dev, u64 sector, u32 count, const void *buffer)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->write == NULL || buffer == NULL) {
        return -1;
    }
    
    /* Check if the sector is valid */
    if (sector >= dev->sector_count) {
        return -1;
    }
    
    /* Check if the count is valid */
    if (count == 0 || sector + count > dev->sector_count) {
        return -1;
    }
    
    /* Write to the device */
    return dev->ops->write(dev, sector, count, buffer);
}

/* Perform an I/O control operation on a block device */
int block_ioctl(block_device_t *dev, u32 request, void *arg)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->ioctl == NULL) {
        return -1;
    }
    
    /* Perform the I/O control operation */
    return dev->ops->ioctl(dev, request, arg);
}

/* Flush a block device */
int block_flush(block_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->flush == NULL) {
        return -1;
    }
    
    /* Flush the device */
    return dev->ops->flush(dev);
}

/**
 * device.c - Horizon kernel device management implementation
 * 
 * This file contains the implementation of the device management subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/device.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Device list */
static list_head_t device_list;

/* Bus list */
static bus_type_t *bus_list = NULL;

/* Class list */
static device_class_t *class_list = NULL;

/* Device lock */
static spinlock_t device_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize the device management subsystem
 */
void device_init(void) {
    /* Initialize the device list */
    list_init(&device_list);
    
    printk(KERN_INFO "DEVICE: Initialized device management subsystem\n");
}

/**
 * Register a device
 * 
 * @param dev Device to register
 * @return 0 on success, negative error code on failure
 */
int device_register(device_t *dev) {
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Check if the device is already registered */
    device_t *existing = device_find_by_name(dev->name);
    if (existing != NULL) {
        return -EEXIST;
    }
    
    /* Initialize the device lists */
    list_init(&dev->driver_list);
    list_init(&dev->bus_list);
    list_init(&dev->class_list);
    list_init(&dev->children);
    list_init(&dev->siblings);
    
    /* Set the device state */
    dev->state = DEVICE_STATE_DISABLED;
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Add the device to the device list */
    list_add(&dev->siblings, &device_list);
    
    /* Add the device to the parent's children list */
    if (dev->parent != NULL) {
        list_add(&dev->siblings, &dev->parent->children);
    }
    
    /* Add the device to the bus */
    if (dev->bus != NULL) {
        list_add(&dev->bus_list, &dev->bus->devices);
    }
    
    /* Add the device to the class */
    if (dev->class != NULL) {
        list_add(&dev->class_list, &dev->class->devices);
    }
    
    /* Add the device to the driver */
    if (dev->driver != NULL) {
        list_add(&dev->driver_list, &dev->driver->devices);
    }
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    /* Probe the device */
    if (dev->ops != NULL && dev->ops->probe != NULL) {
        dev->ops->probe(dev);
    }
    
    /* Set the device state */
    dev->state = DEVICE_STATE_ENABLED;
    
    printk(KERN_INFO "DEVICE: Registered device '%s'\n", dev->name);
    
    return 0;
}

/**
 * Unregister a device
 * 
 * @param dev Device to unregister
 * @return 0 on success, negative error code on failure
 */
int device_unregister(device_t *dev) {
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Set the device state */
    dev->state = DEVICE_STATE_DISABLED;
    
    /* Remove the device */
    if (dev->ops != NULL && dev->ops->remove != NULL) {
        dev->ops->remove(dev);
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Remove the device from the device list */
    list_del(&dev->siblings);
    
    /* Remove the device from the parent's children list */
    if (dev->parent != NULL) {
        list_del(&dev->siblings);
    }
    
    /* Remove the device from the bus */
    if (dev->bus != NULL) {
        list_del(&dev->bus_list);
    }
    
    /* Remove the device from the class */
    if (dev->class != NULL) {
        list_del(&dev->class_list);
    }
    
    /* Remove the device from the driver */
    if (dev->driver != NULL) {
        list_del(&dev->driver_list);
    }
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    printk(KERN_INFO "DEVICE: Unregistered device '%s'\n", dev->name);
    
    return 0;
}

/**
 * Find a device by name
 * 
 * @param name Device name
 * @return Pointer to the device, or NULL if not found
 */
device_t *device_find_by_name(const char *name) {
    /* Check parameters */
    if (name == NULL) {
        return NULL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Find the device */
    device_t *dev;
    list_for_each_entry(dev, &device_list, siblings) {
        if (strcmp(dev->name, name) == 0) {
            /* Found the device */
            spin_unlock(&device_lock);
            return dev;
        }
    }
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return NULL;
}

/**
 * Find a device by device number
 * 
 * @param major Major device number
 * @param minor Minor device number
 * @return Pointer to the device, or NULL if not found
 */
device_t *device_find_by_devnum(u32 major, u32 minor) {
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Find the device */
    device_t *dev;
    list_for_each_entry(dev, &device_list, siblings) {
        if (dev->major == major && dev->minor == minor) {
            /* Found the device */
            spin_unlock(&device_lock);
            return dev;
        }
    }
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return NULL;
}

/**
 * Register a bus
 * 
 * @param bus Bus to register
 * @return 0 on success, negative error code on failure
 */
int bus_register(bus_type_t *bus) {
    /* Check parameters */
    if (bus == NULL) {
        return -EINVAL;
    }
    
    /* Check if the bus is already registered */
    bus_type_t *existing = bus_find_by_name(bus->name);
    if (existing != NULL) {
        return -EEXIST;
    }
    
    /* Initialize the bus lists */
    list_init(&bus->drivers);
    list_init(&bus->devices);
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Add the bus to the bus list */
    bus->next = bus_list;
    bus_list = bus;
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    printk(KERN_INFO "DEVICE: Registered bus '%s'\n", bus->name);
    
    return 0;
}

/**
 * Unregister a bus
 * 
 * @param bus Bus to unregister
 * @return 0 on success, negative error code on failure
 */
int bus_unregister(bus_type_t *bus) {
    /* Check parameters */
    if (bus == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Remove the bus from the bus list */
    if (bus_list == bus) {
        bus_list = bus->next;
    } else {
        bus_type_t *prev = bus_list;
        while (prev != NULL && prev->next != bus) {
            prev = prev->next;
        }
        
        if (prev != NULL) {
            prev->next = bus->next;
        }
    }
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    printk(KERN_INFO "DEVICE: Unregistered bus '%s'\n", bus->name);
    
    return 0;
}

/**
 * Find a bus by name
 * 
 * @param name Bus name
 * @return Pointer to the bus, or NULL if not found
 */
bus_type_t *bus_find_by_name(const char *name) {
    /* Check parameters */
    if (name == NULL) {
        return NULL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Find the bus */
    bus_type_t *bus = bus_list;
    while (bus != NULL) {
        if (strcmp(bus->name, name) == 0) {
            /* Found the bus */
            spin_unlock(&device_lock);
            return bus;
        }
        
        bus = bus->next;
    }
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return NULL;
}

/**
 * Add a device to a bus
 * 
 * @param bus Bus to add the device to
 * @param dev Device to add
 * @return 0 on success, negative error code on failure
 */
int bus_add_device(bus_type_t *bus, device_t *dev) {
    /* Check parameters */
    if (bus == NULL || dev == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Set the device's bus */
    dev->bus = bus;
    
    /* Add the device to the bus */
    list_add(&dev->bus_list, &bus->devices);
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return 0;
}

/**
 * Remove a device from a bus
 * 
 * @param bus Bus to remove the device from
 * @param dev Device to remove
 * @return 0 on success, negative error code on failure
 */
int bus_remove_device(bus_type_t *bus, device_t *dev) {
    /* Check parameters */
    if (bus == NULL || dev == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Check if the device is on the bus */
    if (dev->bus != bus) {
        spin_unlock(&device_lock);
        return -EINVAL;
    }
    
    /* Remove the device from the bus */
    list_del(&dev->bus_list);
    
    /* Clear the device's bus */
    dev->bus = NULL;
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return 0;
}

/**
 * Add a driver to a bus
 * 
 * @param bus Bus to add the driver to
 * @param drv Driver to add
 * @return 0 on success, negative error code on failure
 */
int bus_add_driver(bus_type_t *bus, device_driver_t *drv) {
    /* Check parameters */
    if (bus == NULL || drv == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Set the driver's bus */
    drv->bus = bus;
    
    /* Add the driver to the bus */
    list_add(&drv->bus_list, &bus->drivers);
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return 0;
}

/**
 * Remove a driver from a bus
 * 
 * @param bus Bus to remove the driver from
 * @param drv Driver to remove
 * @return 0 on success, negative error code on failure
 */
int bus_remove_driver(bus_type_t *bus, device_driver_t *drv) {
    /* Check parameters */
    if (bus == NULL || drv == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Check if the driver is on the bus */
    if (drv->bus != bus) {
        spin_unlock(&device_lock);
        return -EINVAL;
    }
    
    /* Remove the driver from the bus */
    list_del(&drv->bus_list);
    
    /* Clear the driver's bus */
    drv->bus = NULL;
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return 0;
}

/**
 * Match a device with a driver
 * 
 * @param bus Bus to match on
 * @param dev Device to match
 * @return 0 on success, negative error code on failure
 */
int bus_match_device(bus_type_t *bus, device_t *dev) {
    /* Check parameters */
    if (bus == NULL || dev == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Check if the device is on the bus */
    if (dev->bus != bus) {
        spin_unlock(&device_lock);
        return -EINVAL;
    }
    
    /* Match the device with a driver */
    device_driver_t *drv;
    list_for_each_entry(drv, &bus->drivers, bus_list) {
        /* Check if the driver can handle the device */
        if (bus->ops != NULL && bus->ops->match != NULL) {
            if (bus->ops->match(dev, drv) == 0) {
                /* Driver can handle the device */
                dev->driver = drv;
                list_add(&dev->driver_list, &drv->devices);
                
                /* Probe the device */
                if (drv->ops != NULL && drv->ops->probe != NULL) {
                    drv->ops->probe(dev);
                }
                
                spin_unlock(&device_lock);
                return 0;
            }
        }
    }
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return -ENODEV;
}

/**
 * Register a driver
 * 
 * @param drv Driver to register
 * @return 0 on success, negative error code on failure
 */
int driver_register(device_driver_t *drv) {
    /* Check parameters */
    if (drv == NULL) {
        return -EINVAL;
    }
    
    /* Check if the driver is already registered */
    device_driver_t *existing = driver_find_by_name(drv->name);
    if (existing != NULL) {
        return -EEXIST;
    }
    
    /* Initialize the driver lists */
    list_init(&drv->devices);
    list_init(&drv->bus_list);
    
    /* Add the driver to the bus */
    if (drv->bus != NULL) {
        bus_add_driver(drv->bus, drv);
    }
    
    printk(KERN_INFO "DEVICE: Registered driver '%s'\n", drv->name);
    
    return 0;
}

/**
 * Unregister a driver
 * 
 * @param drv Driver to unregister
 * @return 0 on success, negative error code on failure
 */
int driver_unregister(device_driver_t *drv) {
    /* Check parameters */
    if (drv == NULL) {
        return -EINVAL;
    }
    
    /* Remove the driver from the bus */
    if (drv->bus != NULL) {
        bus_remove_driver(drv->bus, drv);
    }
    
    /* Remove all devices from the driver */
    device_t *dev, *next;
    list_for_each_entry_safe(dev, next, &drv->devices, driver_list) {
        /* Remove the device from the driver */
        list_del(&dev->driver_list);
        
        /* Clear the device's driver */
        dev->driver = NULL;
    }
    
    printk(KERN_INFO "DEVICE: Unregistered driver '%s'\n", drv->name);
    
    return 0;
}

/**
 * Find a driver by name
 * 
 * @param name Driver name
 * @return Pointer to the driver, or NULL if not found
 */
device_driver_t *driver_find_by_name(const char *name) {
    /* Check parameters */
    if (name == NULL) {
        return NULL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Find the driver */
    bus_type_t *bus = bus_list;
    while (bus != NULL) {
        device_driver_t *drv;
        list_for_each_entry(drv, &bus->drivers, bus_list) {
            if (strcmp(drv->name, name) == 0) {
                /* Found the driver */
                spin_unlock(&device_lock);
                return drv;
            }
        }
        
        bus = bus->next;
    }
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return NULL;
}

/**
 * Add a device to a driver
 * 
 * @param drv Driver to add the device to
 * @param dev Device to add
 * @return 0 on success, negative error code on failure
 */
int driver_add_device(device_driver_t *drv, device_t *dev) {
    /* Check parameters */
    if (drv == NULL || dev == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Set the device's driver */
    dev->driver = drv;
    
    /* Add the device to the driver */
    list_add(&dev->driver_list, &drv->devices);
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return 0;
}

/**
 * Remove a device from a driver
 * 
 * @param drv Driver to remove the device from
 * @param dev Device to remove
 * @return 0 on success, negative error code on failure
 */
int driver_remove_device(device_driver_t *drv, device_t *dev) {
    /* Check parameters */
    if (drv == NULL || dev == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Check if the device is using the driver */
    if (dev->driver != drv) {
        spin_unlock(&device_lock);
        return -EINVAL;
    }
    
    /* Remove the device from the driver */
    list_del(&dev->driver_list);
    
    /* Clear the device's driver */
    dev->driver = NULL;
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return 0;
}

/**
 * Probe a device with a driver
 * 
 * @param drv Driver to probe with
 * @param dev Device to probe
 * @return 0 on success, negative error code on failure
 */
int driver_probe_device(device_driver_t *drv, device_t *dev) {
    /* Check parameters */
    if (drv == NULL || dev == NULL) {
        return -EINVAL;
    }
    
    /* Check if the driver can handle the device */
    if (drv->bus != NULL && drv->bus->ops != NULL && drv->bus->ops->match != NULL) {
        if (drv->bus->ops->match(dev, drv) != 0) {
            return -ENODEV;
        }
    }
    
    /* Set the device's driver */
    dev->driver = drv;
    
    /* Add the device to the driver */
    list_add(&dev->driver_list, &drv->devices);
    
    /* Probe the device */
    if (drv->ops != NULL && drv->ops->probe != NULL) {
        return drv->ops->probe(dev);
    }
    
    return 0;
}

/**
 * Register a class
 * 
 * @param class Class to register
 * @return 0 on success, negative error code on failure
 */
int class_register(device_class_t *class) {
    /* Check parameters */
    if (class == NULL) {
        return -EINVAL;
    }
    
    /* Check if the class is already registered */
    device_class_t *existing = class_find_by_name(class->name);
    if (existing != NULL) {
        return -EEXIST;
    }
    
    /* Initialize the class lists */
    list_init(&class->devices);
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Add the class to the class list */
    class->next = class_list;
    class_list = class;
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    printk(KERN_INFO "DEVICE: Registered class '%s'\n", class->name);
    
    return 0;
}

/**
 * Unregister a class
 * 
 * @param class Class to unregister
 * @return 0 on success, negative error code on failure
 */
int class_unregister(device_class_t *class) {
    /* Check parameters */
    if (class == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Remove the class from the class list */
    if (class_list == class) {
        class_list = class->next;
    } else {
        device_class_t *prev = class_list;
        while (prev != NULL && prev->next != class) {
            prev = prev->next;
        }
        
        if (prev != NULL) {
            prev->next = class->next;
        }
    }
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    printk(KERN_INFO "DEVICE: Unregistered class '%s'\n", class->name);
    
    return 0;
}

/**
 * Find a class by name
 * 
 * @param name Class name
 * @return Pointer to the class, or NULL if not found
 */
device_class_t *class_find_by_name(const char *name) {
    /* Check parameters */
    if (name == NULL) {
        return NULL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Find the class */
    device_class_t *class = class_list;
    while (class != NULL) {
        if (strcmp(class->name, name) == 0) {
            /* Found the class */
            spin_unlock(&device_lock);
            return class;
        }
        
        class = class->next;
    }
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return NULL;
}

/**
 * Add a device to a class
 * 
 * @param class Class to add the device to
 * @param dev Device to add
 * @return 0 on success, negative error code on failure
 */
int class_add_device(device_class_t *class, device_t *dev) {
    /* Check parameters */
    if (class == NULL || dev == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Set the device's class */
    dev->class = class;
    
    /* Add the device to the class */
    list_add(&dev->class_list, &class->devices);
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    /* Create the device */
    if (class->ops != NULL && class->ops->dev_create != NULL) {
        class->ops->dev_create(dev);
    }
    
    return 0;
}

/**
 * Remove a device from a class
 * 
 * @param class Class to remove the device from
 * @param dev Device to remove
 * @return 0 on success, negative error code on failure
 */
int class_remove_device(device_class_t *class, device_t *dev) {
    /* Check parameters */
    if (class == NULL || dev == NULL) {
        return -EINVAL;
    }
    
    /* Lock the device list */
    spin_lock(&device_lock);
    
    /* Check if the device is in the class */
    if (dev->class != class) {
        spin_unlock(&device_lock);
        return -EINVAL;
    }
    
    /* Destroy the device */
    if (class->ops != NULL && class->ops->dev_destroy != NULL) {
        class->ops->dev_destroy(dev);
    }
    
    /* Remove the device from the class */
    list_del(&dev->class_list);
    
    /* Clear the device's class */
    dev->class = NULL;
    
    /* Unlock the device list */
    spin_unlock(&device_lock);
    
    return 0;
}

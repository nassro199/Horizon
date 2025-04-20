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

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Global lists */
static list_head_t devices_list;
static list_head_t buses_list;
static list_head_t classes_list;
static list_head_t drivers_list;

/* Initialize the device management subsystem */
void device_init(void) {
    /* Initialize the lists */
    list_init(&devices_list);
    list_init(&buses_list);
    list_init(&classes_list);
    list_init(&drivers_list);
}

/* Device management functions */

/* Register a device */
int device_register(device_t *dev) {
    if (dev == NULL) {
        return -1;
    }

    /* Check if the device is already registered */
    device_t *existing = device_find_by_name(dev->name);
    if (existing != NULL) {
        return -1;
    }

    /* Initialize the device lists */
    list_init(&dev->driver_list);
    list_init(&dev->bus_list);
    list_init(&dev->class_list);
    list_init(&dev->children);
    list_init(&dev->siblings);

    /* Set the device state */
    dev->state = DEVICE_STATE_DISABLED;

    /* Add the device to the global list */
    list_add_tail(&dev->driver_list, &devices_list);

    /* Add the device to its bus */
    if (dev->bus != NULL) {
        bus_add_device(dev->bus, dev);
    }

    /* Add the device to its class */
    if (dev->class != NULL) {
        class_add_device(dev->class, dev);
    }

    /* Add the device to its parent */
    if (dev->parent != NULL) {
        list_add_tail(&dev->siblings, &dev->parent->children);
    }

    /* Probe the device */
    if (dev->ops != NULL && dev->ops->probe != NULL) {
        int result = dev->ops->probe(dev);
        if (result < 0) {
            /* Probe failed */
            device_unregister(dev);
            return result;
        }
    }

    /* Match the device with a driver */
    if (dev->bus != NULL) {
        bus_match_device(dev->bus, dev);
    }

    /* Set the device state */
    dev->state = DEVICE_STATE_ENABLED;

    return 0;
}

/* Unregister a device */
int device_unregister(device_t *dev) {
    if (dev == NULL) {
        return -1;
    }

    /* Set the device state */
    dev->state = DEVICE_STATE_DISABLED;

    /* Remove the device from its driver */
    if (dev->driver != NULL) {
        driver_remove_device(dev->driver, dev);
    }

    /* Remove the device from its bus */
    if (dev->bus != NULL) {
        bus_remove_device(dev->bus, dev);
    }

    /* Remove the device from its class */
    if (dev->class != NULL) {
        class_remove_device(dev->class, dev);
    }

    /* Remove the device from its parent */
    if (dev->parent != NULL) {
        list_del(&dev->siblings);
    }

    /* Remove the device from the global list */
    list_del(&dev->driver_list);

    /* Remove the device */
    if (dev->ops != NULL && dev->ops->remove != NULL) {
        dev->ops->remove(dev);
    }

    return 0;
}

/* Find a device by name */
device_t *device_find_by_name(const char *name) {
    if (name == NULL) {
        return NULL;
    }

    /* Iterate over the devices list */
    list_head_t *pos;
    list_for_each(pos, &devices_list) {
        device_t *dev = list_entry(pos, device_t, driver_list);

        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
    }

    return NULL;
}

/* Find a device by major and minor numbers */
device_t *device_find_by_devnum(u32 major, u32 minor) {
    /* Iterate over the devices list */
    list_head_t *pos;
    list_for_each(pos, &devices_list) {
        device_t *dev = list_entry(pos, device_t, driver_list);

        if (dev->major == major && dev->minor == minor) {
            return dev;
        }
    }

    return NULL;
}

/* Bus management functions */

/* Register a bus */
int bus_register(bus_type_t *bus) {
    if (bus == NULL) {
        return -1;
    }

    /* Check if the bus is already registered */
    bus_type_t *existing = bus_find_by_name(bus->name);
    if (existing != NULL) {
        return -1;
    }

    /* Initialize the bus lists */
    list_init(&bus->drivers);
    list_init(&bus->devices);

    /* Add the bus to the global list */
    bus->next = NULL;

    if (list_empty(&buses_list)) {
        list_add(&bus->drivers, &buses_list);
    } else {
        bus_type_t *last = list_entry(buses_list.prev, bus_type_t, drivers);
        last->next = bus;
        list_add_tail(&bus->drivers, &buses_list);
    }

    return 0;
}

/* Unregister a bus */
int bus_unregister(bus_type_t *bus) {
    if (bus == NULL) {
        return -1;
    }

    /* Check if the bus has devices */ 
    if (!list_empty(&bus->devices)) {
        return -1;
    }

    /* Check if the bus has drivers */
    if (!list_empty(&bus->drivers)) {
        return -1;
    }

    /* Remove the bus from the global list */
    list_del(&bus->drivers);

    /* Remove the bus from the linked list */
    if (bus->next != NULL) {
        bus_type_t *prev = NULL;
        bus_type_t *current = list_entry(buses_list.next, bus_type_t, drivers);

        while (current != NULL) {
            if (current == bus) {
                if (prev == NULL) {
                    /* First bus in the list */
                    if (current->next != NULL) {
                        list_add(&current->next->drivers, &buses_list);
                    }
                } else {
                    /* Not the first bus in the list */
                    prev->next = current->next;
                }

                break;
            }

            prev = current;
            current = current->next;
        }
    }

    return 0;
}

/* Find a bus by name */
bus_type_t *bus_find_by_name(const char *name) {
    if (name == NULL) {
        return NULL;
    }

    /* Iterate over the buses list */
    list_head_t *pos;
    list_for_each(pos, &buses_list) {
        bus_type_t *bus = list_entry(pos, bus_type_t, drivers);

        if (strcmp(bus->name, name) == 0) {
            return bus;
        }
    }

    return NULL;
}

/* Add a device to a bus */
int bus_add_device(bus_type_t *bus, device_t *dev) {
    if (bus == NULL || dev == NULL) {
        return -1;
    }

    /* Add the device to the bus */
    list_add_tail(&dev->bus_list, &bus->devices);

    /* Set the device's bus */
    dev->bus = bus;

    return 0;
}

/* Remove a device from a bus */
int bus_remove_device(bus_type_t *bus, device_t *dev) {
    if (bus == NULL || dev == NULL) {
        return -1;
    }

    /* Remove the device from the bus */
    list_del(&dev->bus_list);

    /* Clear the device's bus */
    dev->bus = NULL;

    return 0;
}

/* Add a driver to a bus */
int bus_add_driver(bus_type_t *bus, device_driver_t *drv) {
    if (bus == NULL || drv == NULL) {
        return -1;
    }

    /* Add the driver to the bus */
    list_add_tail(&drv->bus_list, &bus->drivers);

    /* Set the driver's bus */
    drv->bus = bus;

    return 0;
}

/* Remove a driver from a bus */
int bus_remove_driver(bus_type_t *bus, device_driver_t *drv) {
    if (bus == NULL || drv == NULL) {
        return -1;
    }

    /* Remove the driver from the bus */
    list_del(&drv->bus_list);

    /* Clear the driver's bus */
    drv->bus = NULL;

    return 0;
}

/* Match a device with a driver */
int bus_match_device(bus_type_t *bus, device_t *dev) {
    if (bus == NULL || dev == NULL) {
        return -1;
    }

    /* Iterate over the bus's drivers */
    list_head_t *pos;
    list_for_each(pos, &bus->drivers) {
        device_driver_t *drv = list_entry(pos, device_driver_t, bus_list);

        /* Match the device and driver */
        if (bus->ops != NULL && bus->ops->match != NULL) {
            if (bus->ops->match(dev, drv) == 0) {
                /* Match found */
                driver_probe_device(drv, dev);
                return 0;
            }
        }
    }

    return -1;
}

/* Driver management functions */

/* Register a driver */
int driver_register(device_driver_t *drv) {
    if (drv == NULL) {
        return -1;
    }

    /* Check if the driver is already registered */
    device_driver_t *existing = driver_find_by_name(drv->name);
    if (existing != NULL) {
        return -1;
    }

    /* Initialize the driver lists */
    list_init(&drv->devices);
    list_init(&drv->bus_list);

    /* Add the driver to the global list */
    list_add_tail(&drv->bus_list, &drivers_list);

    /* Add the driver to its bus */
    if (drv->bus != NULL) {
        bus_add_driver(drv->bus, drv);
    }

    return 0;
}

/* Unregister a driver */
int driver_unregister(device_driver_t *drv) {
    if (drv == NULL) {
        return -1;
    }

    /* Check if the driver has devices */
    if (!list_empty(&drv->devices)) {
        return -1;
    }

    /* Remove the driver from its bus */
    if (drv->bus != NULL) {
        bus_remove_driver(drv->bus, drv);
    }

    /* Remove the driver from the global list */
    list_del(&drv->bus_list);

    return 0;
}

/* Find a driver by name */
device_driver_t *driver_find_by_name(const char *name) {
    if (name == NULL) {
        return NULL;
    }

    /* Iterate over the drivers list */
    list_head_t *pos;
    list_for_each(pos, &drivers_list) {
        device_driver_t *drv = list_entry(pos, device_driver_t, bus_list);

        if (strcmp(drv->name, name) == 0) {
            return drv;
        }
    }

    return NULL;
}

/* Add a device to a driver */
int driver_add_device(device_driver_t *drv, device_t *dev) {
    if (drv == NULL || dev == NULL) {
        return -1;
    }

    /* Add the device to the driver */
    list_add_tail(&dev->driver_list, &drv->devices);

    /* Set the device's driver */
    dev->driver = drv;

    return 0;
}

/* Remove a device from a driver */
int driver_remove_device(device_driver_t *drv, device_t *dev) {
    if (drv == NULL || dev == NULL) {
        return -1;
    }

    /* Remove the device from the driver */
    list_del(&dev->driver_list);

    /* Clear the device's driver */
    dev->driver = NULL;

    return 0;
}

/* Probe a device with a driver */
int driver_probe_device(device_driver_t *drv, device_t *dev) {
    if (drv == NULL || dev == NULL) {
        return -1;
    }

    /* Check if the device already has a driver */
    if (dev->driver != NULL) {
        return -1;
    }

    /* Probe the device */
    if (drv->ops != NULL && drv->ops->probe != NULL) {
        int result = drv->ops->probe(dev);
        if (result < 0) {
            /* Probe failed */
            return result;
        }
    }

    /* Add the device to the driver */
    driver_add_device(drv, dev);

    return 0;
}

/* Class management functions */

/* Register a class */
int class_register(device_class_t *class) {
    if (class == NULL) {
        return -1;
    }

    /* Check if the class is already registered */
    device_class_t *existing = class_find_by_name(class->name);
    if (existing != NULL) {
        return -1;
    }

    /* Initialize the class lists */
    list_init(&class->devices);

    /* Add the class to the global list */
    class->next = NULL;

    if (list_empty(&classes_list)) {
        list_add(&class->devices, &classes_list);
    } else {
        device_class_t *last = list_entry(classes_list.prev, device_class_t, devices);
        last->next = class;
        list_add_tail(&class->devices, &classes_list);
    }

    return 0;
}

/* Unregister a class */
int class_unregister(device_class_t *class) {
    if (class == NULL) {
        return -1;
    }

    /* Check if the class has devices */
    if (!list_empty(&class->devices)) {
        return -1;
    }

    /* Remove the class from the global list */
    list_del(&class->devices);

    /* Remove the class from the linked list */
    if (class->next != NULL) {
        device_class_t *prev = NULL;
        device_class_t *current = list_entry(classes_list.next, device_class_t, devices);

        while (current != NULL) {
            if (current == class) {
                if (prev == NULL) {
                    /* First class in the list */
                    if (current->next != NULL) {
                        list_add(&current->next->devices, &classes_list);
                    }
                } else {
                    /* Not the first class in the list */
                    prev->next = current->next;
                }

                break;
            }

            prev = current;
            current = current->next;
        }
    }

    return 0;
}

/* Find a class by name */
device_class_t *class_find_by_name(const char *name) {
    if (name == NULL) {
        return NULL;
    }

    /* Iterate over the classes list */
    list_head_t *pos;
    list_for_each(pos, &classes_list) {
        device_class_t *class = list_entry(pos, device_class_t, devices);

        if (strcmp(class->name, name) == 0) {
            return class;
        }
    }

    return NULL;
}

/* Add a device to a class */
int class_add_device(device_class_t *class, device_t *dev) {
    if (class == NULL || dev == NULL) {
        return -1;
    }

    /* Add the device to the class */
    list_add_tail(&dev->class_list, &class->devices);

    /* Set the device's class */
    dev->class = class;

    /* Create the device */
    if (class->ops != NULL && class->ops->dev_create != NULL) {
        return class->ops->dev_create(dev);
    }

    return 0;
}

/* Remove a device from a class */
int class_remove_device(device_class_t *class, device_t *dev) {
    if (class == NULL || dev == NULL) {
        return -1;
    }

    /* Destroy the device */
    if (class->ops != NULL && class->ops->dev_destroy != NULL) {
        class->ops->dev_destroy(dev);
    }

    /* Remove the device from the class */
    list_del(&dev->class_list);

    /* Clear the device's class */
    dev->class = NULL;

    return 0;
}

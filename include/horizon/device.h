/**
 * device.h - Horizon kernel device management definitions
 *
 * This file contains definitions for the device management subsystem.
 */

#ifndef _HORIZON_DEVICE_H
#define _HORIZON_DEVICE_H

#include <horizon/types.h>
#include <horizon/list.h>

/* Device state */
typedef enum {
    DEVICE_STATE_DISABLED,   /* Device is disabled */
    DEVICE_STATE_ENABLED,    /* Device is enabled */
    DEVICE_STATE_SUSPENDED,  /* Device is suspended */
    DEVICE_STATE_ERROR       /* Device is in error state */
} device_state_t;

/* Forward declarations */
struct device;
struct device_driver;
struct device_class;
struct bus_type;

/* Device operations */
typedef struct device_operations {
    int (*probe)(struct device *dev);                /* Probe the device */
    int (*remove)(struct device *dev);               /* Remove the device */
    int (*suspend)(struct device *dev);              /* Suspend the device */
    int (*resume)(struct device *dev);               /* Resume the device */
    int (*shutdown)(struct device *dev);             /* Shutdown the device */
} device_operations_t;

/* Device driver operations */
typedef struct driver_operations {
    int (*probe)(struct device *dev);                /* Probe a device */
    int (*remove)(struct device *dev);               /* Remove a device */
    int (*suspend)(struct device *dev);              /* Suspend a device */
    int (*resume)(struct device *dev);               /* Resume a device */
    int (*shutdown)(struct device *dev);             /* Shutdown a device */
} driver_operations_t;

/* Bus operations */
typedef struct bus_operations {
    int (*match)(struct device *dev, struct device_driver *drv); /* Match a device and driver */
    int (*probe)(struct device *dev);                /* Probe a device */
    int (*remove)(struct device *dev);               /* Remove a device */
    int (*suspend)(struct device *dev);              /* Suspend a device */
    int (*resume)(struct device *dev);               /* Resume a device */
    int (*shutdown)(struct device *dev);             /* Shutdown a device */
} bus_operations_t;

/* Class operations */
typedef struct class_operations {
    int (*dev_create)(struct device *dev);           /* Create a device */
    void (*dev_destroy)(struct device *dev);         /* Destroy a device */
} class_operations_t;

/* Bus type structure */
typedef struct bus_type {
    char name[32];                                   /* Bus name */
    bus_operations_t *ops;                           /* Bus operations */
    list_head_t drivers;                             /* List of drivers */
    list_head_t devices;                             /* List of devices */
    struct bus_type *next;                           /* Next bus type */
} bus_type_t;

/* Forward declaration for device_class_enum */
typedef enum device_class_enum device_class_enum_t;

/* Device class structure */
typedef struct device_class {
    char name[32];                                   /* Class name */
    device_class_enum_t type;                        /* Class type */
    class_operations_t *ops;                         /* Class operations */
    list_head_t devices;                             /* List of devices */
    struct device_class *next;                       /* Next device class */
} device_class_t;

/* Rename the enum to avoid naming conflict */
typedef enum device_class_enum {
    DEVICE_CLASS_CHAR,       /* Character devices */
    DEVICE_CLASS_BLOCK,      /* Block devices */
    DEVICE_CLASS_NET,        /* Network devices */
    DEVICE_CLASS_INPUT,      /* Input devices */
    DEVICE_CLASS_GRAPHICS,   /* Graphics devices */
    DEVICE_CLASS_SOUND,      /* Sound devices */
    DEVICE_CLASS_USB,        /* USB devices */
    DEVICE_CLASS_PCI,        /* PCI devices */
    DEVICE_CLASS_PLATFORM,   /* Platform devices */
    DEVICE_CLASS_VIRTUAL,    /* Virtual devices */
    DEVICE_CLASS_MISC        /* Miscellaneous devices */
} device_class_enum_t;

/* Device driver structure */
typedef struct device_driver {
    char name[32];                                   /* Driver name */
    bus_type_t *bus;                                 /* Bus type */
    driver_operations_t *ops;                        /* Driver operations */
    list_head_t devices;                             /* List of devices */
    list_head_t bus_list;                            /* Bus list entry */
    void *private_data;                              /* Private data */
} device_driver_t;

/* Device structure */
typedef struct device {
    char name[64];                                   /* Device name */
    device_class_t *class;                           /* Device class */
    device_driver_t *driver;                         /* Device driver */
    bus_type_t *bus;                                 /* Bus type */
    device_state_t state;                            /* Device state */
    u32 major;                                       /* Major device number */
    u32 minor;                                       /* Minor device number */
    device_operations_t *ops;                        /* Device operations */
    list_head_t driver_list;                         /* Driver list entry */
    list_head_t bus_list;                            /* Bus list entry */
    list_head_t class_list;                          /* Class list entry */
    void *private_data;                              /* Private data */
    struct device *parent;                           /* Parent device */
    list_head_t children;                            /* Child devices */
    list_head_t siblings;                            /* Sibling devices */
} device_t;

/* Device management functions */
void device_init(void);
int device_register(device_t *dev);
int device_unregister(device_t *dev);
device_t *device_find_by_name(const char *name);
device_t *device_find_by_devnum(u32 major, u32 minor);

/* Bus management functions */
int bus_register(bus_type_t *bus);
int bus_unregister(bus_type_t *bus);
bus_type_t *bus_find_by_name(const char *name);
int bus_add_device(bus_type_t *bus, device_t *dev);
int bus_remove_device(bus_type_t *bus, device_t *dev);
int bus_add_driver(bus_type_t *bus, device_driver_t *drv);
int bus_remove_driver(bus_type_t *bus, device_driver_t *drv);
int bus_match_device(bus_type_t *bus, device_t *dev);

/* Driver management functions */
int driver_register(device_driver_t *drv);
int driver_unregister(device_driver_t *drv);
device_driver_t *driver_find_by_name(const char *name);
int driver_add_device(device_driver_t *drv, device_t *dev);
int driver_remove_device(device_driver_t *drv, device_t *dev);
int driver_probe_device(device_driver_t *drv, device_t *dev);

/* Class management functions */
int class_register(device_class_t *class);
int class_unregister(device_class_t *class);
device_class_t *class_find_by_name(const char *name);
int class_add_device(device_class_t *class, device_t *dev);
int class_remove_device(device_class_t *class, device_t *dev);

#endif /* _HORIZON_DEVICE_H */

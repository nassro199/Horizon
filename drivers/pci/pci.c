/**
 * pci.c - Horizon kernel PCI bus implementation
 * 
 * This file contains the implementation of the PCI bus subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/pci.h>
#include <horizon/device.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <asm/io.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* PCI I/O ports */
#define PCI_CONFIG_ADDRESS      0xCF8
#define PCI_CONFIG_DATA         0xCFC

/* PCI bus type */
static bus_type_t pci_bus_type;

/* PCI device class */
static device_class_t pci_device_class;

/* List of PCI devices */
static list_head_t pci_devices_list;

/* List of PCI drivers */
static list_head_t pci_drivers_list;

/* Read a byte from the PCI configuration space */
u8 pci_read_config_byte(u8 bus, u8 device, u8 function, u8 offset) {
    u32 address = (u32)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inb(PCI_CONFIG_DATA + (offset & 0x03));
}

/* Read a word from the PCI configuration space */
u16 pci_read_config_word(u8 bus, u8 device, u8 function, u8 offset) {
    u32 address = (u32)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inw(PCI_CONFIG_DATA + (offset & 0x02));
}

/* Read a dword from the PCI configuration space */
u32 pci_read_config_dword(u8 bus, u8 device, u8 function, u8 offset) {
    u32 address = (u32)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

/* Write a byte to the PCI configuration space */
void pci_write_config_byte(u8 bus, u8 device, u8 function, u8 offset, u8 value) {
    u32 address = (u32)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outb(PCI_CONFIG_DATA + (offset & 0x03), value);
}

/* Write a word to the PCI configuration space */
void pci_write_config_word(u8 bus, u8 device, u8 function, u8 offset, u16 value) {
    u32 address = (u32)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outw(PCI_CONFIG_DATA + (offset & 0x02), value);
}

/* Write a dword to the PCI configuration space */
void pci_write_config_dword(u8 bus, u8 device, u8 function, u8 offset, u32 value) {
    u32 address = (u32)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

/* Check if a PCI device exists */
static int pci_device_exists(u8 bus, u8 device, u8 function) {
    u16 vendor_id = pci_read_config_word(bus, device, function, PCI_VENDOR_ID);
    return vendor_id != 0xFFFF;
}

/* Create a PCI device */
static pci_device_t *pci_create_device(u8 bus, u8 device, u8 function) {
    /* Allocate a PCI device structure */
    pci_device_t *pci_dev = kmalloc(sizeof(pci_device_t), MEM_KERNEL | MEM_ZERO);
    
    if (pci_dev == NULL) {
        return NULL;
    }
    
    /* Initialize the PCI device */
    pci_dev->bus = bus;
    pci_dev->device = device;
    pci_dev->function = function;
    
    /* Read the device information */
    pci_dev->vendor_id = pci_read_config_word(bus, device, function, PCI_VENDOR_ID);
    pci_dev->device_id = pci_read_config_word(bus, device, function, PCI_DEVICE_ID);
    pci_dev->class_code = pci_read_config_byte(bus, device, function, PCI_CLASS);
    pci_dev->subclass = pci_read_config_byte(bus, device, function, PCI_SUBCLASS);
    pci_dev->prog_if = pci_read_config_byte(bus, device, function, PCI_PROG_IF);
    pci_dev->revision = pci_read_config_byte(bus, device, function, PCI_REVISION_ID);
    pci_dev->header_type = pci_read_config_byte(bus, device, function, PCI_HEADER_TYPE);
    pci_dev->interrupt_line = pci_read_config_byte(bus, device, function, PCI_INTERRUPT_LINE);
    pci_dev->interrupt_pin = pci_read_config_byte(bus, device, function, PCI_INTERRUPT_PIN);
    
    /* Read the BARs */
    pci_dev->bar[0] = pci_read_config_dword(bus, device, function, PCI_BAR0);
    pci_dev->bar[1] = pci_read_config_dword(bus, device, function, PCI_BAR1);
    pci_dev->bar[2] = pci_read_config_dword(bus, device, function, PCI_BAR2);
    pci_dev->bar[3] = pci_read_config_dword(bus, device, function, PCI_BAR3);
    pci_dev->bar[4] = pci_read_config_dword(bus, device, function, PCI_BAR4);
    pci_dev->bar[5] = pci_read_config_dword(bus, device, function, PCI_BAR5);
    
    /* Initialize the device structure */
    snprintf(pci_dev->dev.name, sizeof(pci_dev->dev.name), "pci%02x:%02x.%x", bus, device, function);
    pci_dev->dev.bus = &pci_bus_type;
    pci_dev->dev.class = &pci_device_class;
    pci_dev->dev.private_data = pci_dev;
    
    /* Add the device to the PCI devices list */
    list_add_tail(&pci_dev->dev.driver_list, &pci_devices_list);
    
    return pci_dev;
}

/* Scan the PCI bus for devices */
static void pci_scan_bus(void) {
    /* Scan all buses */
    for (u16 bus = 0; bus < 256; bus++) {
        /* Scan all devices */
        for (u8 device = 0; device < 32; device++) {
            /* Check if the device exists */
            if (pci_device_exists(bus, device, 0)) {
                /* Create the device */
                pci_device_t *pci_dev = pci_create_device(bus, device, 0);
                
                if (pci_dev != NULL) {
                    /* Register the device */
                    device_register(&pci_dev->dev);
                    
                    /* Check if the device is multi-function */
                    if (pci_dev->header_type & PCI_HEADER_TYPE_MULTI) {
                        /* Scan all functions */
                        for (u8 function = 1; function < 8; function++) {
                            /* Check if the function exists */
                            if (pci_device_exists(bus, device, function)) {
                                /* Create the function */
                                pci_device_t *pci_func = pci_create_device(bus, device, function);
                                
                                if (pci_func != NULL) {
                                    /* Register the function */
                                    device_register(&pci_func->dev);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Match a PCI device with a PCI driver */
static int pci_match_device(device_t *dev, device_driver_t *drv) {
    if (dev == NULL || drv == NULL) {
        return -1;
    }
    
    /* Get the PCI device */
    pci_device_t *pci_dev = (pci_device_t *)dev->private_data;
    
    if (pci_dev == NULL) {
        return -1;
    }
    
    /* Get the PCI driver */
    pci_driver_t *pci_drv = (pci_driver_t *)drv->private_data;
    
    if (pci_drv == NULL) {
        return -1;
    }
    
    /* Match by vendor and device ID */
    if (pci_drv->vendor_id != 0xFFFF && pci_drv->device_id != 0xFFFF) {
        if (pci_dev->vendor_id == pci_drv->vendor_id && pci_dev->device_id == pci_drv->device_id) {
            return 0;
        }
    }
    
    /* Match by class, subclass, and prog_if */
    if (pci_drv->class_code != 0xFF) {
        if (pci_dev->class_code == pci_drv->class_code) {
            if (pci_drv->subclass != 0xFF) {
                if (pci_dev->subclass == pci_drv->subclass) {
                    if (pci_drv->prog_if != 0xFF) {
                        if (pci_dev->prog_if == pci_drv->prog_if) {
                            return 0;
                        }
                    } else {
                        return 0;
                    }
                }
            } else {
                return 0;
            }
        }
    }
    
    return -1;
}

/* Probe a PCI device */
static int pci_probe_device(device_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Get the PCI device */
    pci_device_t *pci_dev = (pci_device_t *)dev->private_data;
    
    if (pci_dev == NULL) {
        return -1;
    }
    
    /* Enable the device */
    pci_enable_device(pci_dev);
    
    return 0;
}

/* Remove a PCI device */
static int pci_remove_device(device_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Get the PCI device */
    pci_device_t *pci_dev = (pci_device_t *)dev->private_data;
    
    if (pci_dev == NULL) {
        return -1;
    }
    
    /* Disable the device */
    pci_disable_device(pci_dev);
    
    return 0;
}

/* PCI bus operations */
static bus_operations_t pci_bus_ops = {
    .match = pci_match_device,
    .probe = pci_probe_device,
    .remove = pci_remove_device,
    .suspend = NULL,
    .resume = NULL,
    .shutdown = NULL
};

/* Initialize the PCI bus subsystem */
void pci_init(void) {
    /* Initialize the PCI devices list */
    list_init(&pci_devices_list);
    
    /* Initialize the PCI drivers list */
    list_init(&pci_drivers_list);
    
    /* Initialize the PCI bus type */
    strcpy(pci_bus_type.name, "pci");
    pci_bus_type.ops = &pci_bus_ops;
    
    /* Register the PCI bus type */
    bus_register(&pci_bus_type);
    
    /* Initialize the PCI device class */
    strcpy(pci_device_class.name, "pci");
    pci_device_class.type = DEVICE_CLASS_PCI;
    
    /* Register the PCI device class */
    class_register(&pci_device_class);
    
    /* Scan the PCI bus for devices */
    pci_scan_bus();
}

/* Register a PCI driver */
int pci_register_driver(pci_driver_t *driver) {
    if (driver == NULL) {
        return -1;
    }
    
    /* Initialize the driver structure */
    strcpy(driver->driver.name, driver->name);
    driver->driver.bus = &pci_bus_type;
    driver->driver.private_data = driver;
    
    /* Register the driver */
    int result = driver_register(&driver->driver);
    
    if (result < 0) {
        return result;
    }
    
    /* Add the driver to the PCI drivers list */
    list_add_tail(&driver->driver.bus_list, &pci_drivers_list);
    
    return 0;
}

/* Unregister a PCI driver */
int pci_unregister_driver(pci_driver_t *driver) {
    if (driver == NULL) {
        return -1;
    }
    
    /* Remove the driver from the PCI drivers list */
    list_del(&driver->driver.bus_list);
    
    /* Unregister the driver */
    return driver_unregister(&driver->driver);
}

/* Get a PCI device by vendor and device ID */
pci_device_t *pci_get_device(u16 vendor_id, u16 device_id, pci_device_t *from) {
    /* Iterate over the PCI devices list */
    list_head_t *pos;
    int found = 0;
    
    list_for_each(pos, &pci_devices_list) {
        device_t *dev = list_entry(pos, device_t, driver_list);
        pci_device_t *pci_dev = (pci_device_t *)dev->private_data;
        
        if (pci_dev != NULL) {
            if (from != NULL && !found) {
                /* Skip until we find the 'from' device */
                if (pci_dev == from) {
                    found = 1;
                }
                continue;
            }
            
            if (pci_dev->vendor_id == vendor_id && pci_dev->device_id == device_id) {
                return pci_dev;
            }
        }
    }
    
    return NULL;
}

/* Get a PCI device by class, subclass, and prog_if */
pci_device_t *pci_get_class(u8 class_code, u8 subclass, u8 prog_if, pci_device_t *from) {
    /* Iterate over the PCI devices list */
    list_head_t *pos;
    int found = 0;
    
    list_for_each(pos, &pci_devices_list) {
        device_t *dev = list_entry(pos, device_t, driver_list);
        pci_device_t *pci_dev = (pci_device_t *)dev->private_data;
        
        if (pci_dev != NULL) {
            if (from != NULL && !found) {
                /* Skip until we find the 'from' device */
                if (pci_dev == from) {
                    found = 1;
                }
                continue;
            }
            
            if (pci_dev->class_code == class_code) {
                if (subclass == 0xFF || pci_dev->subclass == subclass) {
                    if (prog_if == 0xFF || pci_dev->prog_if == prog_if) {
                        return pci_dev;
                    }
                }
            }
        }
    }
    
    return NULL;
}

/* Enable a PCI device */
int pci_enable_device(pci_device_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Read the command register */
    u16 command = pci_read_config_word(dev->bus, dev->device, dev->function, PCI_COMMAND);
    
    /* Enable I/O and memory space */
    command |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY;
    
    /* Write the command register */
    pci_write_config_word(dev->bus, dev->device, dev->function, PCI_COMMAND, command);
    
    return 0;
}

/* Disable a PCI device */
int pci_disable_device(pci_device_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Read the command register */
    u16 command = pci_read_config_word(dev->bus, dev->device, dev->function, PCI_COMMAND);
    
    /* Disable I/O and memory space */
    command &= ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY);
    
    /* Write the command register */
    pci_write_config_word(dev->bus, dev->device, dev->function, PCI_COMMAND, command);
    
    return 0;
}

/* Map a PCI resource */
void *pci_map_resource(pci_device_t *dev, u32 bar) {
    if (dev == NULL || bar >= 6) {
        return NULL;
    }
    
    /* Get the BAR value */
    u32 bar_value = dev->bar[bar];
    
    /* Check if the BAR is I/O or memory */
    if (bar_value & 0x01) {
        /* I/O BAR */
        return (void *)(bar_value & ~0x03);
    } else {
        /* Memory BAR */
        return (void *)(bar_value & ~0x0F);
    }
}

/* Unmap a PCI resource */
void pci_unmap_resource(pci_device_t *dev, u32 bar) {
    /* Nothing to do for now */
}

/* Enable bus mastering for a PCI device */
int pci_enable_bus_mastering(pci_device_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Read the command register */
    u16 command = pci_read_config_word(dev->bus, dev->device, dev->function, PCI_COMMAND);
    
    /* Enable bus mastering */
    command |= PCI_COMMAND_MASTER;
    
    /* Write the command register */
    pci_write_config_word(dev->bus, dev->device, dev->function, PCI_COMMAND, command);
    
    return 0;
}

/* Disable bus mastering for a PCI device */
int pci_disable_bus_mastering(pci_device_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Read the command register */
    u16 command = pci_read_config_word(dev->bus, dev->device, dev->function, PCI_COMMAND);
    
    /* Disable bus mastering */
    command &= ~PCI_COMMAND_MASTER;
    
    /* Write the command register */
    pci_write_config_word(dev->bus, dev->device, dev->function, PCI_COMMAND, command);
    
    return 0;
}

/* Set the interrupt line for a PCI device */
int pci_set_interrupt(pci_device_t *dev, u8 line) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Write the interrupt line register */
    pci_write_config_byte(dev->bus, dev->device, dev->function, PCI_INTERRUPT_LINE, line);
    
    /* Update the device structure */
    dev->interrupt_line = line;
    
    return 0;
}

/**
 * usb.c - USB subsystem implementation
 * 
 * This file contains the implementation of the USB subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/usb.h>
#include <horizon/string.h>

/* USB device list */
static usb_device_t *usb_devices[USB_MAX_DEVICES];

/* USB driver list */
static usb_driver_t *usb_drivers = NULL;

/* USB host controller list */
static usb_hc_t *usb_hcs = NULL;

/* Next USB device address */
static u8 next_usb_address = 1;

/* Initialize the USB subsystem */
void usb_init(void)
{
    /* Initialize the device list */
    for (u32 i = 0; i < USB_MAX_DEVICES; i++) {
        usb_devices[i] = NULL;
    }
    
    /* Initialize the driver list */
    usb_drivers = NULL;
    
    /* Initialize the host controller list */
    usb_hcs = NULL;
    
    /* Initialize the next device address */
    next_usb_address = 1;
}

/* Register a USB host controller */
int usb_register_hc(usb_hc_t *hc)
{
    if (hc == NULL) {
        return -1;
    }
    
    /* Add to the host controller list */
    hc->next = usb_hcs;
    usb_hcs = hc;
    
    /* Initialize the host controller */
    if (hc->init != NULL) {
        return hc->init(hc);
    }
    
    return 0;
}

/* Unregister a USB host controller */
int usb_unregister_hc(usb_hc_t *hc)
{
    if (hc == NULL) {
        return -1;
    }
    
    /* Remove from the host controller list */
    if (usb_hcs == hc) {
        usb_hcs = hc->next;
    } else {
        usb_hc_t *prev = usb_hcs;
        
        while (prev != NULL && prev->next != hc) {
            prev = prev->next;
        }
        
        if (prev != NULL) {
            prev->next = hc->next;
        }
    }
    
    /* Shutdown the host controller */
    if (hc->shutdown != NULL) {
        return hc->shutdown(hc);
    }
    
    return 0;
}

/* Register a USB driver */
int usb_register_driver(usb_driver_t *driver)
{
    if (driver == NULL) {
        return -1;
    }
    
    /* Add to the driver list */
    driver->next = usb_drivers;
    usb_drivers = driver;
    
    /* Probe existing devices */
    for (u32 i = 0; i < USB_MAX_DEVICES; i++) {
        usb_device_t *dev = usb_devices[i];
        
        if (dev != NULL && dev->driver == NULL) {
            /* Check if the driver matches the device */
            if ((driver->vendor_id == 0 || driver->vendor_id == dev->vendor_id) &&
                (driver->product_id == 0 || driver->product_id == dev->product_id) &&
                (driver->class == 0 || driver->class == dev->class) &&
                (driver->subclass == 0 || driver->subclass == dev->subclass) &&
                (driver->protocol == 0 || driver->protocol == dev->protocol)) {
                /* Driver matches the device */
                if (driver->probe != NULL) {
                    if (driver->probe(driver, dev) == 0) {
                        /* Driver successfully probed the device */
                        dev->driver = driver;
                    }
                }
            }
        }
    }
    
    return 0;
}

/* Unregister a USB driver */
int usb_unregister_driver(usb_driver_t *driver)
{
    if (driver == NULL) {
        return -1;
    }
    
    /* Remove from the driver list */
    if (usb_drivers == driver) {
        usb_drivers = driver->next;
    } else {
        usb_driver_t *prev = usb_drivers;
        
        while (prev != NULL && prev->next != driver) {
            prev = prev->next;
        }
        
        if (prev != NULL) {
            prev->next = driver->next;
        }
    }
    
    /* Disconnect from devices */
    for (u32 i = 0; i < USB_MAX_DEVICES; i++) {
        usb_device_t *dev = usb_devices[i];
        
        if (dev != NULL && dev->driver == driver) {
            /* Disconnect the driver from the device */
            if (driver->disconnect != NULL) {
                driver->disconnect(driver, dev);
            }
            
            dev->driver = NULL;
            dev->driver_data = NULL;
        }
    }
    
    return 0;
}

/* Allocate a USB device */
usb_device_t *usb_alloc_device(usb_hc_t *hc)
{
    if (hc == NULL) {
        return NULL;
    }
    
    /* Find a free device slot */
    u32 i;
    
    for (i = 0; i < USB_MAX_DEVICES; i++) {
        if (usb_devices[i] == NULL) {
            break;
        }
    }
    
    if (i == USB_MAX_DEVICES) {
        /* No free device slots */
        return NULL;
    }
    
    /* Allocate a device structure */
    usb_device_t *dev = kmalloc(sizeof(usb_device_t), MEM_KERNEL | MEM_ZERO);
    
    if (dev == NULL) {
        return NULL;
    }
    
    /* Initialize the device */
    dev->address = 0;
    dev->speed = USB_SPEED_UNKNOWN;
    dev->hc = hc;
    
    /* Add to the device list */
    usb_devices[i] = dev;
    
    return dev;
}

/* Free a USB device */
void usb_free_device(usb_device_t *dev)
{
    if (dev == NULL) {
        return;
    }
    
    /* Remove from the device list */
    for (u32 i = 0; i < USB_MAX_DEVICES; i++) {
        if (usb_devices[i] == dev) {
            usb_devices[i] = NULL;
            break;
        }
    }
    
    /* Free the device structure */
    kfree(dev);
}

/* Set the address of a USB device */
int usb_set_address(usb_device_t *dev, u8 address)
{
    if (dev == NULL) {
        return -1;
    }
    
    /* Send a SET_ADDRESS request */
    usb_setup_packet_t setup;
    
    setup.bmRequestType = USB_DIR_OUT;
    setup.bRequest = USB_REQ_SET_ADDRESS;
    setup.wValue = address;
    setup.wIndex = 0;
    setup.wLength = 0;
    
    int result = usb_control_transfer(dev, setup.bmRequestType, setup.bRequest, setup.wValue, setup.wIndex, NULL, setup.wLength);
    
    if (result < 0) {
        return result;
    }
    
    /* Set the device address */
    dev->address = address;
    
    return 0;
}

/* Get a descriptor from a USB device */
int usb_get_descriptor(usb_device_t *dev, u8 type, u8 index, u16 lang_id, void *data, u16 size)
{
    if (dev == NULL || data == NULL) {
        return -1;
    }
    
    /* Send a GET_DESCRIPTOR request */
    usb_setup_packet_t setup;
    
    setup.bmRequestType = USB_DIR_IN;
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wValue = (type << 8) | index;
    setup.wIndex = lang_id;
    setup.wLength = size;
    
    return usb_control_transfer(dev, setup.bmRequestType, setup.bRequest, setup.wValue, setup.wIndex, data, setup.wLength);
}

/* Set the configuration of a USB device */
int usb_set_configuration(usb_device_t *dev, u8 config)
{
    if (dev == NULL) {
        return -1;
    }
    
    /* Send a SET_CONFIGURATION request */
    usb_setup_packet_t setup;
    
    setup.bmRequestType = USB_DIR_OUT;
    setup.bRequest = USB_REQ_SET_CONFIGURATION;
    setup.wValue = config;
    setup.wIndex = 0;
    setup.wLength = 0;
    
    return usb_control_transfer(dev, setup.bmRequestType, setup.bRequest, setup.wValue, setup.wIndex, NULL, setup.wLength);
}

/* Perform a control transfer */
int usb_control_transfer(usb_device_t *dev, u8 request_type, u8 request, u16 value, u16 index, void *data, u16 size)
{
    if (dev == NULL) {
        return -1;
    }
    
    /* Check if the host controller supports control transfers */
    if (dev->hc == NULL || dev->hc->control == NULL) {
        return -1;
    }
    
    /* Create a setup packet */
    usb_setup_packet_t setup;
    
    setup.bmRequestType = request_type;
    setup.bRequest = request;
    setup.wValue = value;
    setup.wIndex = index;
    setup.wLength = size;
    
    /* Perform the control transfer */
    return dev->hc->control(dev->hc, dev, &setup, data);
}

/* Perform a bulk transfer */
int usb_bulk_transfer(usb_device_t *dev, u8 endpoint, void *data, u32 size)
{
    if (dev == NULL || data == NULL) {
        return -1;
    }
    
    /* Check if the host controller supports bulk transfers */
    if (dev->hc == NULL || dev->hc->bulk == NULL) {
        return -1;
    }
    
    /* Perform the bulk transfer */
    return dev->hc->bulk(dev->hc, dev, endpoint, data, size);
}

/* Perform an interrupt transfer */
int usb_interrupt_transfer(usb_device_t *dev, u8 endpoint, void *data, u32 size)
{
    if (dev == NULL || data == NULL) {
        return -1;
    }
    
    /* Check if the host controller supports interrupt transfers */
    if (dev->hc == NULL || dev->hc->interrupt == NULL) {
        return -1;
    }
    
    /* Perform the interrupt transfer */
    return dev->hc->interrupt(dev->hc, dev, endpoint, data, size);
}

/* Perform an isochronous transfer */
int usb_isochronous_transfer(usb_device_t *dev, u8 endpoint, void *data, u32 size)
{
    if (dev == NULL || data == NULL) {
        return -1;
    }
    
    /* Check if the host controller supports isochronous transfers */
    if (dev->hc == NULL || dev->hc->isochronous == NULL) {
        return -1;
    }
    
    /* Perform the isochronous transfer */
    return dev->hc->isochronous(dev->hc, dev, endpoint, data, size);
}

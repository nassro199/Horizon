/**
 * usb.h - USB subsystem definitions
 * 
 * This file contains definitions for the USB subsystem.
 */

#ifndef _KERNEL_USB_H
#define _KERNEL_USB_H

#include <horizon/types.h>
#include <horizon/device.h>

/* USB constants */
#define USB_MAX_DEVICES      32
#define USB_MAX_ENDPOINTS    16
#define USB_MAX_INTERFACES   8
#define USB_MAX_CONFIGS      8

/* USB speeds */
#define USB_SPEED_UNKNOWN    0
#define USB_SPEED_LOW        1
#define USB_SPEED_FULL       2
#define USB_SPEED_HIGH       3
#define USB_SPEED_SUPER      4

/* USB request types */
#define USB_REQ_GET_STATUS           0x00
#define USB_REQ_CLEAR_FEATURE        0x01
#define USB_REQ_SET_FEATURE          0x03
#define USB_REQ_SET_ADDRESS          0x05
#define USB_REQ_GET_DESCRIPTOR       0x06
#define USB_REQ_SET_DESCRIPTOR       0x07
#define USB_REQ_GET_CONFIGURATION    0x08
#define USB_REQ_SET_CONFIGURATION    0x09
#define USB_REQ_GET_INTERFACE        0x0A
#define USB_REQ_SET_INTERFACE        0x0B
#define USB_REQ_SYNCH_FRAME          0x0C

/* USB descriptor types */
#define USB_DESC_DEVICE              0x01
#define USB_DESC_CONFIG              0x02
#define USB_DESC_STRING              0x03
#define USB_DESC_INTERFACE           0x04
#define USB_DESC_ENDPOINT            0x05
#define USB_DESC_DEVICE_QUALIFIER    0x06
#define USB_DESC_OTHER_SPEED_CONFIG  0x07
#define USB_DESC_INTERFACE_POWER     0x08
#define USB_DESC_OTG                 0x09
#define USB_DESC_DEBUG               0x0A
#define USB_DESC_INTERFACE_ASSOC     0x0B
#define USB_DESC_BOS                 0x0F
#define USB_DESC_DEVICE_CAPABILITY   0x10
#define USB_DESC_HID                 0x21
#define USB_DESC_REPORT              0x22
#define USB_DESC_PHYSICAL            0x23
#define USB_DESC_HUB                 0x29

/* USB device classes */
#define USB_CLASS_PER_INTERFACE      0x00
#define USB_CLASS_AUDIO              0x01
#define USB_CLASS_COMM               0x02
#define USB_CLASS_HID                0x03
#define USB_CLASS_PHYSICAL           0x05
#define USB_CLASS_STILL_IMAGE        0x06
#define USB_CLASS_PRINTER            0x07
#define USB_CLASS_MASS_STORAGE       0x08
#define USB_CLASS_HUB                0x09
#define USB_CLASS_CDC_DATA           0x0A
#define USB_CLASS_CSCID              0x0B
#define USB_CLASS_CONTENT_SEC        0x0D
#define USB_CLASS_VIDEO              0x0E
#define USB_CLASS_WIRELESS_CONTROLLER 0xE0
#define USB_CLASS_MISC               0xEF
#define USB_CLASS_APP_SPEC           0xFE
#define USB_CLASS_VENDOR_SPEC        0xFF

/* USB endpoint types */
#define USB_ENDPOINT_CONTROL         0x00
#define USB_ENDPOINT_ISOCHRONOUS     0x01
#define USB_ENDPOINT_BULK            0x02
#define USB_ENDPOINT_INTERRUPT       0x03

/* USB endpoint directions */
#define USB_DIR_OUT                  0x00
#define USB_DIR_IN                   0x80

/* USB setup packet */
typedef struct usb_setup_packet {
    u8 bmRequestType;
    u8 bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
} __attribute__((packed)) usb_setup_packet_t;

/* USB device descriptor */
typedef struct usb_device_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u16 bcdUSB;
    u8 bDeviceClass;
    u8 bDeviceSubClass;
    u8 bDeviceProtocol;
    u8 bMaxPacketSize0;
    u16 idVendor;
    u16 idProduct;
    u16 bcdDevice;
    u8 iManufacturer;
    u8 iProduct;
    u8 iSerialNumber;
    u8 bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

/* USB configuration descriptor */
typedef struct usb_config_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u16 wTotalLength;
    u8 bNumInterfaces;
    u8 bConfigurationValue;
    u8 iConfiguration;
    u8 bmAttributes;
    u8 bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

/* USB interface descriptor */
typedef struct usb_interface_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bInterfaceNumber;
    u8 bAlternateSetting;
    u8 bNumEndpoints;
    u8 bInterfaceClass;
    u8 bInterfaceSubClass;
    u8 bInterfaceProtocol;
    u8 iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;

/* USB endpoint descriptor */
typedef struct usb_endpoint_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bEndpointAddress;
    u8 bmAttributes;
    u16 wMaxPacketSize;
    u8 bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

/* USB string descriptor */
typedef struct usb_string_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u16 wString[];
} __attribute__((packed)) usb_string_descriptor_t;

/* USB endpoint structure */
typedef struct usb_endpoint {
    u8 address;
    u8 type;
    u16 max_packet_size;
    u8 interval;
} usb_endpoint_t;

/* USB interface structure */
typedef struct usb_interface {
    u8 number;
    u8 alt_setting;
    u8 class;
    u8 subclass;
    u8 protocol;
    u8 num_endpoints;
    usb_endpoint_t endpoints[USB_MAX_ENDPOINTS];
} usb_interface_t;

/* USB configuration structure */
typedef struct usb_config {
    u8 value;
    u8 num_interfaces;
    usb_interface_t interfaces[USB_MAX_INTERFACES];
} usb_config_t;

/* USB device structure */
typedef struct usb_device {
    u8 address;
    u8 speed;
    u16 vendor_id;
    u16 product_id;
    u16 device_version;
    u8 class;
    u8 subclass;
    u8 protocol;
    u8 max_packet_size0;
    u8 num_configs;
    usb_config_t configs[USB_MAX_CONFIGS];
    struct usb_hc *hc;
    void *hc_data;
    struct usb_driver *driver;
    void *driver_data;
} usb_device_t;

/* USB host controller structure */
typedef struct usb_hc {
    char name[32];
    u32 type;
    int (*init)(struct usb_hc *hc);
    int (*shutdown)(struct usb_hc *hc);
    int (*control)(struct usb_hc *hc, usb_device_t *dev, usb_setup_packet_t *setup, void *data);
    int (*bulk)(struct usb_hc *hc, usb_device_t *dev, u8 endpoint, void *data, u32 size);
    int (*interrupt)(struct usb_hc *hc, usb_device_t *dev, u8 endpoint, void *data, u32 size);
    int (*isochronous)(struct usb_hc *hc, usb_device_t *dev, u8 endpoint, void *data, u32 size);
    void *private;
} usb_hc_t;

/* USB driver structure */
typedef struct usb_driver {
    char name[32];
    u16 vendor_id;
    u16 product_id;
    u8 class;
    u8 subclass;
    u8 protocol;
    int (*probe)(struct usb_driver *driver, usb_device_t *dev);
    int (*disconnect)(struct usb_driver *driver, usb_device_t *dev);
    struct usb_driver *next;
} usb_driver_t;

/* USB subsystem functions */
void usb_init(void);
int usb_register_hc(usb_hc_t *hc);
int usb_unregister_hc(usb_hc_t *hc);
int usb_register_driver(usb_driver_t *driver);
int usb_unregister_driver(usb_driver_t *driver);
usb_device_t *usb_alloc_device(usb_hc_t *hc);
void usb_free_device(usb_device_t *dev);
int usb_set_address(usb_device_t *dev, u8 address);
int usb_get_descriptor(usb_device_t *dev, u8 type, u8 index, u16 lang_id, void *data, u16 size);
int usb_set_configuration(usb_device_t *dev, u8 config);
int usb_control_transfer(usb_device_t *dev, u8 request_type, u8 request, u16 value, u16 index, void *data, u16 size);
int usb_bulk_transfer(usb_device_t *dev, u8 endpoint, void *data, u32 size);
int usb_interrupt_transfer(usb_device_t *dev, u8 endpoint, void *data, u32 size);
int usb_isochronous_transfer(usb_device_t *dev, u8 endpoint, void *data, u32 size);

#endif /* _KERNEL_USB_H */

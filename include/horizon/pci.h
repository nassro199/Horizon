/**
 * pci.h - Horizon kernel PCI bus definitions
 * 
 * This file contains definitions for the PCI bus subsystem.
 */

#ifndef _KERNEL_PCI_H
#define _KERNEL_PCI_H

#include <horizon/types.h>
#include <horizon/device.h>

/* PCI configuration space registers */
#define PCI_VENDOR_ID           0x00    /* 16 bits */
#define PCI_DEVICE_ID           0x02    /* 16 bits */
#define PCI_COMMAND             0x04    /* 16 bits */
#define PCI_STATUS              0x06    /* 16 bits */
#define PCI_REVISION_ID         0x08    /* 8 bits */
#define PCI_PROG_IF             0x09    /* 8 bits */
#define PCI_SUBCLASS            0x0A    /* 8 bits */
#define PCI_CLASS               0x0B    /* 8 bits */
#define PCI_CACHE_LINE_SIZE     0x0C    /* 8 bits */
#define PCI_LATENCY_TIMER       0x0D    /* 8 bits */
#define PCI_HEADER_TYPE         0x0E    /* 8 bits */
#define PCI_BIST                0x0F    /* 8 bits */
#define PCI_BAR0                0x10    /* 32 bits */
#define PCI_BAR1                0x14    /* 32 bits */
#define PCI_BAR2                0x18    /* 32 bits */
#define PCI_BAR3                0x1C    /* 32 bits */
#define PCI_BAR4                0x20    /* 32 bits */
#define PCI_BAR5                0x24    /* 32 bits */
#define PCI_CARDBUS_CIS         0x28    /* 32 bits */
#define PCI_SUBSYSTEM_VENDOR_ID 0x2C    /* 16 bits */
#define PCI_SUBSYSTEM_ID        0x2E    /* 16 bits */
#define PCI_ROM_ADDRESS         0x30    /* 32 bits */
#define PCI_INTERRUPT_LINE      0x3C    /* 8 bits */
#define PCI_INTERRUPT_PIN       0x3D    /* 8 bits */
#define PCI_MIN_GNT             0x3E    /* 8 bits */
#define PCI_MAX_LAT             0x3F    /* 8 bits */

/* PCI command register bits */
#define PCI_COMMAND_IO          0x0001  /* I/O space enable */
#define PCI_COMMAND_MEMORY      0x0002  /* Memory space enable */
#define PCI_COMMAND_MASTER      0x0004  /* Bus master enable */
#define PCI_COMMAND_SPECIAL     0x0008  /* Special cycles enable */
#define PCI_COMMAND_INVALIDATE  0x0010  /* Memory write and invalidate enable */
#define PCI_COMMAND_VGA_PALETTE 0x0020  /* VGA palette snooping enable */
#define PCI_COMMAND_PARITY      0x0040  /* Parity error response enable */
#define PCI_COMMAND_WAIT        0x0080  /* SERR# enable */
#define PCI_COMMAND_SERR        0x0100  /* Fast back-to-back enable */
#define PCI_COMMAND_FAST_BACK   0x0200  /* Interrupt disable */

/* PCI status register bits */
#define PCI_STATUS_CAP_LIST     0x0010  /* Capabilities list */
#define PCI_STATUS_66MHZ        0x0020  /* 66 MHz capable */
#define PCI_STATUS_UDF          0x0040  /* UDF supported */
#define PCI_STATUS_FAST_BACK    0x0080  /* Fast back-to-back capable */
#define PCI_STATUS_PARITY       0x0100  /* Parity error detected */
#define PCI_STATUS_DEVSEL_MASK  0x0600  /* DEVSEL timing */
#define PCI_STATUS_DEVSEL_FAST  0x0000
#define PCI_STATUS_DEVSEL_MEDIUM 0x0200
#define PCI_STATUS_DEVSEL_SLOW  0x0400
#define PCI_STATUS_SIG_TARGET_ABORT 0x0800 /* Signaled target abort */
#define PCI_STATUS_REC_TARGET_ABORT 0x1000 /* Received target abort */
#define PCI_STATUS_REC_MASTER_ABORT 0x2000 /* Received master abort */
#define PCI_STATUS_SIG_SYSTEM_ERROR 0x4000 /* Signaled system error */
#define PCI_STATUS_DETECTED_PARITY 0x8000 /* Detected parity error */

/* PCI header type register bits */
#define PCI_HEADER_TYPE_NORMAL  0x00
#define PCI_HEADER_TYPE_BRIDGE  0x01
#define PCI_HEADER_TYPE_CARDBUS 0x02
#define PCI_HEADER_TYPE_MULTI   0x80    /* Multi-function device */

/* PCI device structure */
typedef struct pci_device {
    u8 bus;                     /* Bus number */
    u8 device;                  /* Device number */
    u8 function;                /* Function number */
    u16 vendor_id;              /* Vendor ID */
    u16 device_id;              /* Device ID */
    u8 class_code;              /* Class code */
    u8 subclass;                /* Subclass */
    u8 prog_if;                 /* Programming interface */
    u8 revision;                /* Revision ID */
    u8 header_type;             /* Header type */
    u8 interrupt_line;          /* Interrupt line */
    u8 interrupt_pin;           /* Interrupt pin */
    u32 bar[6];                 /* Base address registers */
    device_t dev;               /* Device structure */
} pci_device_t;

/* PCI driver structure */
typedef struct pci_driver {
    char name[32];              /* Driver name */
    u16 vendor_id;              /* Vendor ID */
    u16 device_id;              /* Device ID */
    u8 class_code;              /* Class code */
    u8 subclass;                /* Subclass */
    u8 prog_if;                 /* Programming interface */
    int (*probe)(pci_device_t *dev); /* Probe function */
    int (*remove)(pci_device_t *dev); /* Remove function */
    int (*suspend)(pci_device_t *dev); /* Suspend function */
    int (*resume)(pci_device_t *dev); /* Resume function */
    device_driver_t driver;     /* Device driver structure */
} pci_driver_t;

/* PCI functions */
void pci_init(void);
u8 pci_read_config_byte(u8 bus, u8 device, u8 function, u8 offset);
u16 pci_read_config_word(u8 bus, u8 device, u8 function, u8 offset);
u32 pci_read_config_dword(u8 bus, u8 device, u8 function, u8 offset);
void pci_write_config_byte(u8 bus, u8 device, u8 function, u8 offset, u8 value);
void pci_write_config_word(u8 bus, u8 device, u8 function, u8 offset, u16 value);
void pci_write_config_dword(u8 bus, u8 device, u8 function, u8 offset, u32 value);
int pci_register_driver(pci_driver_t *driver);
int pci_unregister_driver(pci_driver_t *driver);
pci_device_t *pci_get_device(u16 vendor_id, u16 device_id, pci_device_t *from);
pci_device_t *pci_get_class(u8 class_code, u8 subclass, u8 prog_if, pci_device_t *from);
int pci_enable_device(pci_device_t *dev);
int pci_disable_device(pci_device_t *dev);
void *pci_map_resource(pci_device_t *dev, u32 bar);
void pci_unmap_resource(pci_device_t *dev, u32 bar);
int pci_enable_bus_mastering(pci_device_t *dev);
int pci_disable_bus_mastering(pci_device_t *dev);
int pci_set_interrupt(pci_device_t *dev, u8 line);

#endif /* _KERNEL_PCI_H */

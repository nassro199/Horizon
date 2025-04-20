/**
 * uhci.c - USB UHCI host controller driver
 * 
 * This file contains the implementation of the USB UHCI host controller driver.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/usb.h>
#include <horizon/string.h>
#include <asm/io.h>

/* UHCI registers */
#define UHCI_REG_USBCMD      0x00    /* USB command */
#define UHCI_REG_USBSTS      0x02    /* USB status */
#define UHCI_REG_USBINTR     0x04    /* USB interrupt enable */
#define UHCI_REG_FRNUM       0x06    /* Frame number */
#define UHCI_REG_FRBASEADD   0x08    /* Frame list base address */
#define UHCI_REG_SOFMOD      0x0C    /* Start of frame modify */
#define UHCI_REG_PORTSC1     0x10    /* Port 1 status/control */
#define UHCI_REG_PORTSC2     0x12    /* Port 2 status/control */

/* UHCI command register bits */
#define UHCI_CMD_RS          0x0001  /* Run/Stop */
#define UHCI_CMD_HCRESET     0x0002  /* Host controller reset */
#define UHCI_CMD_GRESET      0x0004  /* Global reset */
#define UHCI_CMD_EGSM        0x0008  /* Enter global suspend mode */
#define UHCI_CMD_FGR         0x0010  /* Force global resume */
#define UHCI_CMD_SWDBG       0x0020  /* Software debug */
#define UHCI_CMD_CF          0x0040  /* Configure flag */
#define UHCI_CMD_MAXP        0x0080  /* Max packet (0 = 32, 1 = 64) */

/* UHCI status register bits */
#define UHCI_STS_USBINT      0x0001  /* USB interrupt */
#define UHCI_STS_ERRINT      0x0002  /* USB error interrupt */
#define UHCI_STS_RESDET      0x0004  /* Resume detect */
#define UHCI_STS_HSERR       0x0008  /* Host system error */
#define UHCI_STS_HCPERR      0x0010  /* Host controller process error */
#define UHCI_STS_HCHALTED    0x0020  /* Host controller halted */

/* UHCI interrupt enable register bits */
#define UHCI_INTR_TIMEOUT    0x0001  /* Timeout/CRC interrupt enable */
#define UHCI_INTR_RESUME     0x0002  /* Resume interrupt enable */
#define UHCI_INTR_IOC        0x0004  /* Interrupt on complete enable */
#define UHCI_INTR_SP         0x0008  /* Short packet interrupt enable */

/* UHCI port status and control register bits */
#define UHCI_PORT_CONNECT    0x0001  /* Current connect status */
#define UHCI_PORT_CONNCHG    0x0002  /* Connect status change */
#define UHCI_PORT_ENABLE     0x0004  /* Port enabled/disabled */
#define UHCI_PORT_ENCHG      0x0008  /* Port enable/disable change */
#define UHCI_PORT_RESET      0x0010  /* Port reset */
#define UHCI_PORT_RESUME     0x0040  /* Resume detect */
#define UHCI_PORT_SUSPEND    0x0080  /* Suspend */
#define UHCI_PORT_LSDA       0x0100  /* Low speed device attached */
#define UHCI_PORT_RESET_CHANGE 0x0200 /* Port reset change */
#define UHCI_PORT_RESERVED   0xFFE0  /* Reserved bits */

/* UHCI transfer descriptor */
typedef struct uhci_td {
    u32 link;
    u32 status;
    u32 token;
    u32 buffer;
} __attribute__((packed)) uhci_td_t;

/* UHCI queue head */
typedef struct uhci_qh {
    u32 link;
    u32 element;
} __attribute__((packed)) uhci_qh_t;

/* UHCI host controller data */
typedef struct uhci_data {
    u16 io_base;
    u32 frame_list_phys;
    u32 *frame_list;
    uhci_qh_t *qh_control;
    uhci_qh_t *qh_bulk;
    uhci_qh_t *qh_interrupt;
} uhci_data_t;

/* Initialize the UHCI host controller */
static int uhci_init(usb_hc_t *hc)
{
    if (hc == NULL) {
        return -1;
    }
    
    /* Allocate host controller data */
    uhci_data_t *data = kmalloc(sizeof(uhci_data_t), MEM_KERNEL | MEM_ZERO);
    
    if (data == NULL) {
        return -1;
    }
    
    /* Set the I/O base address */
    data->io_base = 0xC000;
    
    /* Reset the host controller */
    outw(data->io_base + UHCI_REG_USBCMD, UHCI_CMD_HCRESET);
    
    /* Wait for the reset to complete */
    for (u32 i = 0; i < 10; i++) {
        if ((inw(data->io_base + UHCI_REG_USBCMD) & UHCI_CMD_HCRESET) == 0) {
            break;
        }
        
        /* Delay */
        for (u32 j = 0; j < 1000000; j++) {
            __asm__ volatile("nop");
        }
    }
    
    /* Check if the reset completed */
    if (inw(data->io_base + UHCI_REG_USBCMD) & UHCI_CMD_HCRESET) {
        /* Reset failed */
        kfree(data);
        return -1;
    }
    
    /* Allocate the frame list */
    data->frame_list = kmalloc(4096, MEM_KERNEL | MEM_ZERO);
    
    if (data->frame_list == NULL) {
        kfree(data);
        return -1;
    }
    
    /* Get the physical address of the frame list */
    data->frame_list_phys = (u32)data->frame_list;
    
    /* Allocate the queue heads */
    data->qh_control = kmalloc(sizeof(uhci_qh_t), MEM_KERNEL | MEM_ZERO);
    data->qh_bulk = kmalloc(sizeof(uhci_qh_t), MEM_KERNEL | MEM_ZERO);
    data->qh_interrupt = kmalloc(sizeof(uhci_qh_t), MEM_KERNEL | MEM_ZERO);
    
    if (data->qh_control == NULL || data->qh_bulk == NULL || data->qh_interrupt == NULL) {
        if (data->qh_control != NULL) {
            kfree(data->qh_control);
        }
        
        if (data->qh_bulk != NULL) {
            kfree(data->qh_bulk);
        }
        
        if (data->qh_interrupt != NULL) {
            kfree(data->qh_interrupt);
        }
        
        kfree(data->frame_list);
        kfree(data);
        return -1;
    }
    
    /* Initialize the queue heads */
    data->qh_control->link = 0x00000002;
    data->qh_control->element = 0x00000001;
    
    data->qh_bulk->link = 0x00000002;
    data->qh_bulk->element = 0x00000001;
    
    data->qh_interrupt->link = (u32)data->qh_control | 0x00000002;
    data->qh_interrupt->element = 0x00000001;
    
    /* Initialize the frame list */
    for (u32 i = 0; i < 1024; i++) {
        data->frame_list[i] = (u32)data->qh_interrupt | 0x00000002;
    }
    
    /* Set the frame list base address */
    outl(data->io_base + UHCI_REG_FRBASEADD, data->frame_list_phys);
    
    /* Set the frame number */
    outw(data->io_base + UHCI_REG_FRNUM, 0);
    
    /* Enable the controller */
    outw(data->io_base + UHCI_REG_USBCMD, UHCI_CMD_RS | UHCI_CMD_MAXP);
    
    /* Enable interrupts */
    outw(data->io_base + UHCI_REG_USBINTR, UHCI_INTR_TIMEOUT | UHCI_INTR_RESUME | UHCI_INTR_IOC | UHCI_INTR_SP);
    
    /* Set the host controller data */
    hc->private = data;
    
    return 0;
}

/* Shutdown the UHCI host controller */
static int uhci_shutdown(usb_hc_t *hc)
{
    if (hc == NULL) {
        return -1;
    }
    
    /* Get the host controller data */
    uhci_data_t *data = (uhci_data_t *)hc->private;
    
    if (data == NULL) {
        return -1;
    }
    
    /* Disable the controller */
    outw(data->io_base + UHCI_REG_USBCMD, 0);
    
    /* Free the queue heads */
    if (data->qh_control != NULL) {
        kfree(data->qh_control);
    }
    
    if (data->qh_bulk != NULL) {
        kfree(data->qh_bulk);
    }
    
    if (data->qh_interrupt != NULL) {
        kfree(data->qh_interrupt);
    }
    
    /* Free the frame list */
    if (data->frame_list != NULL) {
        kfree(data->frame_list);
    }
    
    /* Free the host controller data */
    kfree(data);
    
    /* Clear the host controller data */
    hc->private = NULL;
    
    return 0;
}

/* Perform a control transfer */
static int uhci_control(usb_hc_t *hc, usb_device_t *dev, usb_setup_packet_t *setup, void *data)
{
    if (hc == NULL || dev == NULL || setup == NULL) {
        return -1;
    }
    
    /* This would be implemented with actual UHCI control transfers */
    /* For now, just return success */
    return 0;
}

/* Perform a bulk transfer */
static int uhci_bulk(usb_hc_t *hc, usb_device_t *dev, u8 endpoint, void *data, u32 size)
{
    if (hc == NULL || dev == NULL || data == NULL) {
        return -1;
    }
    
    /* This would be implemented with actual UHCI bulk transfers */
    /* For now, just return success */
    return 0;
}

/* Perform an interrupt transfer */
static int uhci_interrupt(usb_hc_t *hc, usb_device_t *dev, u8 endpoint, void *data, u32 size)
{
    if (hc == NULL || dev == NULL || data == NULL) {
        return -1;
    }
    
    /* This would be implemented with actual UHCI interrupt transfers */
    /* For now, just return success */
    return 0;
}

/* UHCI host controller */
static usb_hc_t uhci_hc = {
    .name = "UHCI",
    .type = 0,
    .init = uhci_init,
    .shutdown = uhci_shutdown,
    .control = uhci_control,
    .bulk = uhci_bulk,
    .interrupt = uhci_interrupt,
    .isochronous = NULL,
    .private = NULL
};

/* Initialize the UHCI driver */
void uhci_driver_init(void)
{
    /* Register the UHCI host controller */
    usb_register_hc(&uhci_hc);
}

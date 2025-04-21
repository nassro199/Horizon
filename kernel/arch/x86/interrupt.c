/**
 * arch/x86/interrupt.c - Horizon kernel x86 interrupt handling
 * 
 * This file contains the implementation of the x86 interrupt handling subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/interrupt.h>
#include <horizon/arch/interrupt.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>

/* Interrupt descriptor table */
static struct idt_entry idt[256];

/* Interrupt descriptor table pointer */
static struct idt_ptr idtp;

/* Programmable interrupt controller ports */
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

/* Programmable interrupt controller commands */
#define PIC_EOI         0x20    /* End of interrupt */
#define PIC_READ_IRR    0x0A    /* Read interrupt request register */
#define PIC_READ_ISR    0x0B    /* Read in-service register */

/* Programmable interrupt controller initialization */
#define ICW1_ICW4       0x01    /* ICW4 needed */
#define ICW1_SINGLE     0x02    /* Single mode */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 */
#define ICW1_LEVEL      0x08    /* Level triggered mode */
#define ICW1_INIT       0x10    /* Initialization */

#define ICW4_8086       0x01    /* 8086/88 mode */
#define ICW4_AUTO       0x02    /* Auto EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode/master */
#define ICW4_SFNM       0x10    /* Special fully nested */

/* Interrupt descriptor table entry */
struct idt_entry {
    unsigned short base_lo;     /* Lower 16 bits of handler function address */
    unsigned short sel;         /* Kernel segment selector */
    unsigned char always0;      /* Always 0 */
    unsigned char flags;        /* Flags */
    unsigned short base_hi;     /* Upper 16 bits of handler function address */
} __attribute__((packed));

/* Interrupt descriptor table pointer */
struct idt_ptr {
    unsigned short limit;       /* Size of IDT */
    unsigned int base;          /* Base address of IDT */
} __attribute__((packed));

/* Interrupt handlers */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

/* IRQ handlers */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* Interrupt controller */
static struct interrupt_controller pic_controller = {
    .name = "PIC",
    .startup = NULL,
    .shutdown = NULL,
    .enable = arch_interrupt_enable,
    .disable = arch_interrupt_disable,
    .ack = arch_interrupt_ack,
    .mask = NULL,
    .unmask = NULL,
    .eoi = arch_interrupt_eoi,
    .set_type = arch_interrupt_setup,
    .set_affinity = NULL,
    .next = NULL
};

/**
 * Set an entry in the interrupt descriptor table
 * 
 * @param num Entry number
 * @param base Handler function address
 * @param sel Segment selector
 * @param flags Flags
 */
static void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags) {
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

/**
 * Initialize the interrupt descriptor table
 */
static void idt_init(void) {
    /* Set up the IDT pointer */
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (unsigned int)&idt;
    
    /* Clear the IDT */
    memset(&idt, 0, sizeof(struct idt_entry) * 256);
    
    /* Set up the IDT gates */
    idt_set_gate(0, (unsigned int)isr0, 0x08, 0x8E);
    idt_set_gate(1, (unsigned int)isr1, 0x08, 0x8E);
    idt_set_gate(2, (unsigned int)isr2, 0x08, 0x8E);
    idt_set_gate(3, (unsigned int)isr3, 0x08, 0x8E);
    idt_set_gate(4, (unsigned int)isr4, 0x08, 0x8E);
    idt_set_gate(5, (unsigned int)isr5, 0x08, 0x8E);
    idt_set_gate(6, (unsigned int)isr6, 0x08, 0x8E);
    idt_set_gate(7, (unsigned int)isr7, 0x08, 0x8E);
    idt_set_gate(8, (unsigned int)isr8, 0x08, 0x8E);
    idt_set_gate(9, (unsigned int)isr9, 0x08, 0x8E);
    idt_set_gate(10, (unsigned int)isr10, 0x08, 0x8E);
    idt_set_gate(11, (unsigned int)isr11, 0x08, 0x8E);
    idt_set_gate(12, (unsigned int)isr12, 0x08, 0x8E);
    idt_set_gate(13, (unsigned int)isr13, 0x08, 0x8E);
    idt_set_gate(14, (unsigned int)isr14, 0x08, 0x8E);
    idt_set_gate(15, (unsigned int)isr15, 0x08, 0x8E);
    idt_set_gate(16, (unsigned int)isr16, 0x08, 0x8E);
    idt_set_gate(17, (unsigned int)isr17, 0x08, 0x8E);
    idt_set_gate(18, (unsigned int)isr18, 0x08, 0x8E);
    idt_set_gate(19, (unsigned int)isr19, 0x08, 0x8E);
    idt_set_gate(20, (unsigned int)isr20, 0x08, 0x8E);
    idt_set_gate(21, (unsigned int)isr21, 0x08, 0x8E);
    idt_set_gate(22, (unsigned int)isr22, 0x08, 0x8E);
    idt_set_gate(23, (unsigned int)isr23, 0x08, 0x8E);
    idt_set_gate(24, (unsigned int)isr24, 0x08, 0x8E);
    idt_set_gate(25, (unsigned int)isr25, 0x08, 0x8E);
    idt_set_gate(26, (unsigned int)isr26, 0x08, 0x8E);
    idt_set_gate(27, (unsigned int)isr27, 0x08, 0x8E);
    idt_set_gate(28, (unsigned int)isr28, 0x08, 0x8E);
    idt_set_gate(29, (unsigned int)isr29, 0x08, 0x8E);
    idt_set_gate(30, (unsigned int)isr30, 0x08, 0x8E);
    idt_set_gate(31, (unsigned int)isr31, 0x08, 0x8E);
    
    /* Set up the IRQ gates */
    idt_set_gate(32, (unsigned int)irq0, 0x08, 0x8E);
    idt_set_gate(33, (unsigned int)irq1, 0x08, 0x8E);
    idt_set_gate(34, (unsigned int)irq2, 0x08, 0x8E);
    idt_set_gate(35, (unsigned int)irq3, 0x08, 0x8E);
    idt_set_gate(36, (unsigned int)irq4, 0x08, 0x8E);
    idt_set_gate(37, (unsigned int)irq5, 0x08, 0x8E);
    idt_set_gate(38, (unsigned int)irq6, 0x08, 0x8E);
    idt_set_gate(39, (unsigned int)irq7, 0x08, 0x8E);
    idt_set_gate(40, (unsigned int)irq8, 0x08, 0x8E);
    idt_set_gate(41, (unsigned int)irq9, 0x08, 0x8E);
    idt_set_gate(42, (unsigned int)irq10, 0x08, 0x8E);
    idt_set_gate(43, (unsigned int)irq11, 0x08, 0x8E);
    idt_set_gate(44, (unsigned int)irq12, 0x08, 0x8E);
    idt_set_gate(45, (unsigned int)irq13, 0x08, 0x8E);
    idt_set_gate(46, (unsigned int)irq14, 0x08, 0x8E);
    idt_set_gate(47, (unsigned int)irq15, 0x08, 0x8E);
    
    /* Load the IDT */
    __asm__ volatile("lidt %0" : : "m" (idtp));
}

/**
 * Initialize the programmable interrupt controller
 */
static void pic_init(void) {
    /* Save masks */
    unsigned char mask1 = inb(PIC1_DATA);
    unsigned char mask2 = inb(PIC2_DATA);
    
    /* Start initialization sequence */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    
    /* Set vector offsets */
    outb(PIC1_DATA, 32);   /* IRQ 0-7: interrupts 32-39 */
    outb(PIC2_DATA, 40);   /* IRQ 8-15: interrupts 40-47 */
    
    /* Set up cascading */
    outb(PIC1_DATA, 4);    /* IRQ 2 connected to slave */
    outb(PIC2_DATA, 2);    /* Slave ID is 2 */
    
    /* Set 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    
    /* Restore masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/**
 * Initialize the architecture-specific interrupt handling
 */
void arch_interrupt_init(void) {
    /* Initialize the IDT */
    idt_init();
    
    /* Initialize the PIC */
    pic_init();
    
    /* Register the PIC controller */
    interrupt_register_controller(&pic_controller);
    
    /* Set up the interrupt controllers for the IRQs */
    int i;
    for (i = 0; i < 16; i++) {
        interrupt_descs[i + 32].controller = &pic_controller;
    }
    
    /* Enable interrupts */
    __asm__ volatile("sti");
    
    printk(KERN_INFO "INTERRUPT: Initialized x86 interrupt handling\n");
}

/**
 * Set up an interrupt
 * 
 * @param irq IRQ number
 * @param flow_type Flow type
 * @return 0 on success, negative error code on failure
 */
int arch_interrupt_setup(unsigned int irq, unsigned int flow_type) {
    /* Check parameters */
    if (irq >= 16) {
        return -EINVAL;
    }
    
    /* Set up the interrupt */
    /* Nothing to do for x86 */
    
    return 0;
}

/**
 * Enable an interrupt
 * 
 * @param irq IRQ number
 */
void arch_interrupt_enable(unsigned int irq) {
    /* Check parameters */
    if (irq >= 16) {
        return;
    }
    
    /* Enable the interrupt */
    if (irq < 8) {
        /* IRQ 0-7 */
        unsigned char mask = inb(PIC1_DATA);
        mask &= ~(1 << irq);
        outb(PIC1_DATA, mask);
    } else {
        /* IRQ 8-15 */
        unsigned char mask = inb(PIC2_DATA);
        mask &= ~(1 << (irq - 8));
        outb(PIC2_DATA, mask);
    }
}

/**
 * Disable an interrupt
 * 
 * @param irq IRQ number
 */
void arch_interrupt_disable(unsigned int irq) {
    /* Check parameters */
    if (irq >= 16) {
        return;
    }
    
    /* Disable the interrupt */
    if (irq < 8) {
        /* IRQ 0-7 */
        unsigned char mask = inb(PIC1_DATA);
        mask |= (1 << irq);
        outb(PIC1_DATA, mask);
    } else {
        /* IRQ 8-15 */
        unsigned char mask = inb(PIC2_DATA);
        mask |= (1 << (irq - 8));
        outb(PIC2_DATA, mask);
    }
}

/**
 * Acknowledge an interrupt
 * 
 * @param irq IRQ number
 */
void arch_interrupt_ack(unsigned int irq) {
    /* Check parameters */
    if (irq >= 16) {
        return;
    }
    
    /* Acknowledge the interrupt */
    if (irq >= 8) {
        /* IRQ 8-15 */
        outb(PIC2_COMMAND, PIC_EOI);
    }
    
    /* Always acknowledge the master PIC */
    outb(PIC1_COMMAND, PIC_EOI);
}

/**
 * End of interrupt
 * 
 * @param irq IRQ number
 */
void arch_interrupt_eoi(unsigned int irq) {
    /* Check parameters */
    if (irq >= 16) {
        return;
    }
    
    /* End of interrupt */
    if (irq >= 8) {
        /* IRQ 8-15 */
        outb(PIC2_COMMAND, PIC_EOI);
    }
    
    /* Always acknowledge the master PIC */
    outb(PIC1_COMMAND, PIC_EOI);
}

/**
 * Set interrupt affinity
 * 
 * @param irq IRQ number
 * @param dest CPU mask
 * @return 0 on success, negative error code on failure
 */
int arch_interrupt_set_affinity(unsigned int irq, const struct cpumask *dest) {
    /* Check parameters */
    if (irq >= 16 || dest == NULL) {
        return -EINVAL;
    }
    
    /* Set interrupt affinity */
    /* Not supported on x86 */
    
    return -ENOSYS;
}

/**
 * Interrupt entry point
 * 
 * @param frame Interrupt frame
 */
void arch_interrupt_entry(struct interrupt_frame *frame) {
    /* Increment the interrupt nesting level */
    interrupt_nesting_level++;
    
    /* Dispatch the interrupt */
    interrupt_dispatch(frame);
}

/**
 * Interrupt exit point
 * 
 * @param frame Interrupt frame
 */
void arch_interrupt_exit(struct interrupt_frame *frame) {
    /* Decrement the interrupt nesting level */
    interrupt_nesting_level--;
}

/**
 * Save interrupt flags
 * 
 * @param flags Pointer to store flags
 */
void arch_interrupt_save_flags(unsigned long *flags) {
    __asm__ volatile("pushf; pop %0" : "=r" (*flags));
}

/**
 * Restore interrupt flags
 * 
 * @param flags Flags to restore
 */
void arch_interrupt_restore_flags(unsigned long flags) {
    __asm__ volatile("push %0; popf" : : "r" (flags));
}

/**
 * Enable all interrupts
 */
void arch_interrupt_enable_all(void) {
    __asm__ volatile("sti");
}

/**
 * Disable all interrupts
 */
void arch_interrupt_disable_all(void) {
    __asm__ volatile("cli");
}

/**
 * Set up the interrupt descriptor table
 */
void arch_interrupt_setup_idt(void) {
    idt_init();
}

/**
 * Set up an interrupt vector
 * 
 * @param vector Vector number
 * @param handler Handler function
 */
void arch_interrupt_setup_vector(unsigned int vector, void (*handler)(void)) {
    /* Check parameters */
    if (vector >= 256 || handler == NULL) {
        return;
    }
    
    /* Set up the vector */
    idt_set_gate(vector, (unsigned int)handler, 0x08, 0x8E);
}

/**
 * Set up the interrupt controller
 */
void arch_interrupt_setup_controller(void) {
    pic_init();
}

/**
 * Initialize the interrupt controller
 */
void arch_interrupt_init_controller(void) {
    pic_init();
}

/**
 * Shut down the interrupt controller
 */
void arch_interrupt_shutdown_controller(void) {
    /* Disable all interrupts */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/**
 * Enable the interrupt controller
 */
void arch_interrupt_enable_controller(void) {
    /* Enable the interrupt controller */
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);
}

/**
 * Disable the interrupt controller
 */
void arch_interrupt_disable_controller(void) {
    /* Disable the interrupt controller */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/**
 * Mask the interrupt controller
 */
void arch_interrupt_mask_controller(void) {
    /* Mask the interrupt controller */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/**
 * Unmask the interrupt controller
 */
void arch_interrupt_unmask_controller(void) {
    /* Unmask the interrupt controller */
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);
}

/**
 * End of interrupt for the interrupt controller
 */
void arch_interrupt_eoi_controller(void) {
    /* End of interrupt */
    outb(PIC1_COMMAND, PIC_EOI);
    outb(PIC2_COMMAND, PIC_EOI);
}

/**
 * Set the interrupt type for the interrupt controller
 * 
 * @param irq IRQ number
 * @param flow_type Flow type
 * @return 0 on success, negative error code on failure
 */
int arch_interrupt_set_type_controller(unsigned int irq, unsigned int flow_type) {
    /* Check parameters */
    if (irq >= 16) {
        return -EINVAL;
    }
    
    /* Set the interrupt type */
    /* Not supported on x86 */
    
    return -ENOSYS;
}

/**
 * Set the interrupt affinity for the interrupt controller
 * 
 * @param irq IRQ number
 * @param dest CPU mask
 * @return 0 on success, negative error code on failure
 */
int arch_interrupt_set_affinity_controller(unsigned int irq, const struct cpumask *dest) {
    /* Check parameters */
    if (irq >= 16 || dest == NULL) {
        return -EINVAL;
    }
    
    /* Set the interrupt affinity */
    /* Not supported on x86 */
    
    return -ENOSYS;
}

/**
 * Input a byte from a port
 * 
 * @param port Port number
 * @return Byte read from port
 */
unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

/**
 * Output a byte to a port
 * 
 * @param port Port number
 * @param val Byte to write
 */
void outb(unsigned short port, unsigned char val) {
    __asm__ volatile("outb %0, %1" : : "a" (val), "dN" (port));
}

/**
 * Input a word from a port
 * 
 * @param port Port number
 * @return Word read from port
 */
unsigned short inw(unsigned short port) {
    unsigned short ret;
    __asm__ volatile("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

/**
 * Output a word to a port
 * 
 * @param port Port number
 * @param val Word to write
 */
void outw(unsigned short port, unsigned short val) {
    __asm__ volatile("outw %0, %1" : : "a" (val), "dN" (port));
}

/**
 * Input a long from a port
 * 
 * @param port Port number
 * @return Long read from port
 */
unsigned long inl(unsigned short port) {
    unsigned long ret;
    __asm__ volatile("inl %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

/**
 * Output a long to a port
 * 
 * @param port Port number
 * @param val Long to write
 */
void outl(unsigned short port, unsigned long val) {
    __asm__ volatile("outl %0, %1" : : "a" (val), "dN" (port));
}

/**
 * Input a string from a port
 * 
 * @param port Port number
 * @param buf Buffer to read into
 * @param count Number of bytes to read
 */
void insb(unsigned short port, void *buf, unsigned long count) {
    __asm__ volatile("rep insb" : "+D" (buf), "+c" (count) : "d" (port) : "memory");
}

/**
 * Output a string to a port
 * 
 * @param port Port number
 * @param buf Buffer to write from
 * @param count Number of bytes to write
 */
void outsb(unsigned short port, const void *buf, unsigned long count) {
    __asm__ volatile("rep outsb" : "+S" (buf), "+c" (count) : "d" (port) : "memory");
}

/**
 * Input a string of words from a port
 * 
 * @param port Port number
 * @param buf Buffer to read into
 * @param count Number of words to read
 */
void insw(unsigned short port, void *buf, unsigned long count) {
    __asm__ volatile("rep insw" : "+D" (buf), "+c" (count) : "d" (port) : "memory");
}

/**
 * Output a string of words to a port
 * 
 * @param port Port number
 * @param buf Buffer to write from
 * @param count Number of words to write
 */
void outsw(unsigned short port, const void *buf, unsigned long count) {
    __asm__ volatile("rep outsw" : "+S" (buf), "+c" (count) : "d" (port) : "memory");
}

/**
 * Input a string of longs from a port
 * 
 * @param port Port number
 * @param buf Buffer to read into
 * @param count Number of longs to read
 */
void insl(unsigned short port, void *buf, unsigned long count) {
    __asm__ volatile("rep insl" : "+D" (buf), "+c" (count) : "d" (port) : "memory");
}

/**
 * Output a string of longs to a port
 * 
 * @param port Port number
 * @param buf Buffer to write from
 * @param count Number of longs to write
 */
void outsl(unsigned short port, const void *buf, unsigned long count) {
    __asm__ volatile("rep outsl" : "+S" (buf), "+c" (count) : "d" (port) : "memory");
}

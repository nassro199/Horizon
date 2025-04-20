/**
 * setup.c - Architecture-specific setup code
 * 
 * This file contains the architecture-specific setup code for x86.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <asm/interrupt.h>

/* GDT entries */
static segment_descriptor_t gdt[6];
/* GDT pointer */
static struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) gdt_ptr;

/* IDT entries */
static interrupt_descriptor_t idt[256];
/* IDT pointer */
static struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) idt_ptr;

/* Forward declarations for ISR handlers */
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

/* Forward declarations for IRQ handlers */
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

/* Initialize the GDT */
static void gdt_init(void) {
    /* Set up the GDT pointer */
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (u32)&gdt;
    
    /* Set up the GDT entries */
    /* Null descriptor */
    segment_descriptor_set(&gdt[0], 0, 0, 0, 0);
    
    /* Kernel code segment */
    segment_descriptor_set(&gdt[1], 0, 0xFFFFF,
        SEG_PRESENT | SEG_DPL_0 | SEG_NON_SYSTEM | SEG_CODE | SEG_READABLE,
        SEG_GRAN_PAGE | SEG_32BIT);
    
    /* Kernel data segment */
    segment_descriptor_set(&gdt[2], 0, 0xFFFFF,
        SEG_PRESENT | SEG_DPL_0 | SEG_NON_SYSTEM | SEG_DATA | SEG_WRITABLE,
        SEG_GRAN_PAGE | SEG_32BIT);
    
    /* User code segment */
    segment_descriptor_set(&gdt[3], 0, 0xFFFFF,
        SEG_PRESENT | SEG_DPL_3 | SEG_NON_SYSTEM | SEG_CODE | SEG_READABLE,
        SEG_GRAN_PAGE | SEG_32BIT);
    
    /* User data segment */
    segment_descriptor_set(&gdt[4], 0, 0xFFFFF,
        SEG_PRESENT | SEG_DPL_3 | SEG_NON_SYSTEM | SEG_DATA | SEG_WRITABLE,
        SEG_GRAN_PAGE | SEG_32BIT);
    
    /* TSS segment (placeholder) */
    segment_descriptor_set(&gdt[5], 0, 0, 0, 0);
    
    /* Load the GDT */
    __asm__ volatile("lgdt %0" : : "m"(gdt_ptr));
    
    /* Update the segment registers */
    __asm__ volatile(
        "movw $0x10, %%ax \n"
        "movw %%ax, %%ds \n"
        "movw %%ax, %%es \n"
        "movw %%ax, %%fs \n"
        "movw %%ax, %%gs \n"
        "movw %%ax, %%ss \n"
        "ljmp $0x08, $1f \n"
        "1:"
        : : : "eax"
    );
}

/* Initialize the PIC */
static void pic_init(void) {
    /* Remap the PIC */
    outb(0x20, 0x11);    /* ICW1: Initialize PIC1 */
    outb(0xA0, 0x11);    /* ICW1: Initialize PIC2 */
    outb(0x21, 0x20);    /* ICW2: PIC1 starts at INT 0x20 */
    outb(0xA1, 0x28);    /* ICW2: PIC2 starts at INT 0x28 */
    outb(0x21, 0x04);    /* ICW3: PIC1 has PIC2 at IRQ2 */
    outb(0xA1, 0x02);    /* ICW3: PIC2 is cascaded from IRQ2 */
    outb(0x21, 0x01);    /* ICW4: 8086 mode for PIC1 */
    outb(0xA1, 0x01);    /* ICW4: 8086 mode for PIC2 */
    outb(0x21, 0x0);     /* Enable all IRQs on PIC1 */
    outb(0xA1, 0x0);     /* Enable all IRQs on PIC2 */
}

/* Initialize the IDT */
static void idt_init(void) {
    /* Set up the IDT pointer */
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (u32)&idt;
    
    /* Set up the IDT entries */
    /* CPU exceptions */
    interrupt_descriptor_set(&idt[0], (u32)isr0, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[1], (u32)isr1, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[2], (u32)isr2, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[3], (u32)isr3, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[4], (u32)isr4, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[5], (u32)isr5, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[6], (u32)isr6, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[7], (u32)isr7, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[8], (u32)isr8, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[9], (u32)isr9, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[10], (u32)isr10, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[11], (u32)isr11, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[12], (u32)isr12, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[13], (u32)isr13, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[14], (u32)isr14, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[15], (u32)isr15, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[16], (u32)isr16, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[17], (u32)isr17, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[18], (u32)isr18, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[19], (u32)isr19, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[20], (u32)isr20, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[21], (u32)isr21, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[22], (u32)isr22, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[23], (u32)isr23, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[24], (u32)isr24, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[25], (u32)isr25, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[26], (u32)isr26, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[27], (u32)isr27, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[28], (u32)isr28, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[29], (u32)isr29, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[30], (u32)isr30, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[31], (u32)isr31, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    
    /* IRQs */
    interrupt_descriptor_set(&idt[32], (u32)irq0, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[33], (u32)irq1, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[34], (u32)irq2, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[35], (u32)irq3, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[36], (u32)irq4, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[37], (u32)irq5, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[38], (u32)irq6, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[39], (u32)irq7, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[40], (u32)irq8, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[41], (u32)irq9, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[42], (u32)irq10, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[43], (u32)irq11, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[44], (u32)irq12, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[45], (u32)irq13, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[46], (u32)irq14, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    interrupt_descriptor_set(&idt[47], (u32)irq15, 0x08, IDT_PRESENT | IDT_GATE_INT32);
    
    /* Load the IDT */
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

/* Architecture-specific setup */
void arch_setup(void) {
    /* Initialize the GDT */
    gdt_init();
    
    /* Initialize the PIC */
    pic_init();
    
    /* Initialize the IDT */
    idt_init();
}

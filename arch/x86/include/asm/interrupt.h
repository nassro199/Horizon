/**
 * interrupt.h - Interrupt descriptor definitions
 * 
 * This file contains definitions for interrupt descriptors on x86 architecture.
 */

#ifndef _ASM_INTERRUPT_H
#define _ASM_INTERRUPT_H

#include <horizon/types.h>

/* Interrupt descriptor structure */
typedef struct interrupt_descriptor {
    u16 offset_low;       /* Offset (bits 0-15) */
    u16 selector;         /* Selector */
    u8 zero;              /* Always zero */
    u8 type_attr;         /* Type and attributes */
    u16 offset_high;      /* Offset (bits 16-31) */
} __attribute__((packed)) interrupt_descriptor_t;

/* Interrupt frame structure */
typedef struct interrupt_frame {
    u32 eip;              /* Instruction pointer */
    u32 cs;               /* Code segment */
    u32 eflags;           /* Flags */
    u32 esp;              /* Stack pointer */
    u32 ss;               /* Stack segment */
} __attribute__((packed)) interrupt_frame_t;

/* Interrupt handler type */
typedef void (*interrupt_handler_t)(interrupt_frame_t *frame);

/* Type and attributes flags */
#define IDT_PRESENT     0x80    /* Present bit */
#define IDT_DPL_0       0x00    /* Privilege level 0 */
#define IDT_DPL_1       0x20    /* Privilege level 1 */
#define IDT_DPL_2       0x40    /* Privilege level 2 */
#define IDT_DPL_3       0x60    /* Privilege level 3 */
#define IDT_STORAGE     0x00    /* Storage segment */
#define IDT_GATE_TASK   0x05    /* Task gate */
#define IDT_GATE_INT16  0x06    /* 16-bit interrupt gate */
#define IDT_GATE_TRAP16 0x07    /* 16-bit trap gate */
#define IDT_GATE_INT32  0x0E    /* 32-bit interrupt gate */
#define IDT_GATE_TRAP32 0x0F    /* 32-bit trap gate */

/* Interrupt numbers */
#define INT_DIVIDE_ERROR        0x00    /* Divide error */
#define INT_DEBUG               0x01    /* Debug */
#define INT_NMI                 0x02    /* Non-maskable interrupt */
#define INT_BREAKPOINT          0x03    /* Breakpoint */
#define INT_OVERFLOW            0x04    /* Overflow */
#define INT_BOUND_RANGE         0x05    /* Bound range exceeded */
#define INT_INVALID_OPCODE      0x06    /* Invalid opcode */
#define INT_DEVICE_NOT_AVAIL    0x07    /* Device not available */
#define INT_DOUBLE_FAULT        0x08    /* Double fault */
#define INT_COPROCESSOR_SEG     0x09    /* Coprocessor segment overrun */
#define INT_INVALID_TSS         0x0A    /* Invalid TSS */
#define INT_SEGMENT_NOT_PRESENT 0x0B    /* Segment not present */
#define INT_STACK_FAULT         0x0C    /* Stack fault */
#define INT_GENERAL_PROTECTION  0x0D    /* General protection fault */
#define INT_PAGE_FAULT          0x0E    /* Page fault */
#define INT_RESERVED            0x0F    /* Reserved */
#define INT_FPU_ERROR           0x10    /* FPU error */
#define INT_ALIGNMENT_CHECK     0x11    /* Alignment check */
#define INT_MACHINE_CHECK       0x12    /* Machine check */
#define INT_SIMD_EXCEPTION      0x13    /* SIMD floating-point exception */

/* IRQ numbers */
#define IRQ_BASE                0x20    /* Base IRQ number */
#define IRQ_TIMER               0x20    /* Timer IRQ */
#define IRQ_KEYBOARD            0x21    /* Keyboard IRQ */
#define IRQ_CASCADE             0x22    /* Cascade IRQ */
#define IRQ_COM2                0x23    /* COM2 IRQ */
#define IRQ_COM1                0x24    /* COM1 IRQ */
#define IRQ_LPT2                0x25    /* LPT2 IRQ */
#define IRQ_FLOPPY              0x26    /* Floppy IRQ */
#define IRQ_LPT1                0x27    /* LPT1 IRQ */
#define IRQ_RTC                 0x28    /* RTC IRQ */
#define IRQ_MOUSE               0x2C    /* PS/2 mouse IRQ */
#define IRQ_FPU                 0x2D    /* FPU IRQ */
#define IRQ_PRIMARY_ATA         0x2E    /* Primary ATA IRQ */
#define IRQ_SECONDARY_ATA       0x2F    /* Secondary ATA IRQ */

/* Set up an interrupt descriptor */
static inline void interrupt_descriptor_set(interrupt_descriptor_t *desc, u32 offset, u16 selector, u8 type_attr) {
    desc->offset_low = offset & 0xFFFF;
    desc->offset_high = (offset >> 16) & 0xFFFF;
    desc->selector = selector;
    desc->zero = 0;
    desc->type_attr = type_attr;
}

/* Enable interrupts */
static inline void interrupt_enable(void) {
    __asm__ volatile("sti");
}

/* Disable interrupts */
static inline void interrupt_disable(void) {
    __asm__ volatile("cli");
}

#endif /* _ASM_INTERRUPT_H */

/**
 * interrupt.c - Interrupt handler implementation
 *
 * This file contains the implementation of the interrupt handlers.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <asm/io.h>
#include <asm/interrupt.h>
#include <horizon/spinlock.h>

/* Forward declarations */
extern void check_deferred_work(void);

/* Interrupt handler function pointers */
static interrupt_handler_t interrupt_handlers[256];

/* Interrupt nesting level */
static int interrupt_nesting_level = 0;

/* Interrupt lock */
static spinlock_t interrupt_lock = SPIN_LOCK_INITIALIZER;

/* Exception messages */
static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

/* Register an interrupt handler */
void interrupt_register_handler(u8 num, interrupt_handler_t handler) {
    interrupt_handlers[num] = handler;
}

/* Unregister an interrupt handler */
void interrupt_unregister_handler(u8 num) {
    interrupt_handlers[num] = NULL;
}

/* ISR handler with nesting support */
void isr_handler(interrupt_frame_t *frame) {
    u32 int_num = frame->eip;

    /* Increment nesting level */
    interrupt_nesting_level++;

    /* Re-enable interrupts for nested handling if not a critical exception */
    if (int_num >= 32) {
        __asm__ volatile("sti");
    }

    /* Check if we have a handler for this interrupt */
    if (interrupt_handlers[int_num] != NULL) {
        interrupt_handlers[int_num](frame);
    } else {
        /* No handler, print an error message */
        if (int_num < 32) {
            kernel_panic(exception_messages[int_num]);
        } else {
            kernel_panic("Unknown interrupt");
        }
    }

    /* Disable interrupts before returning */
    __asm__ volatile("cli");

    /* Decrement nesting level */
    interrupt_nesting_level--;

    /* If we're back to level 0, check for deferred work */
    if (interrupt_nesting_level == 0) {
        check_deferred_work();
    }
}

/* IRQ handler with nesting support */
void irq_handler(interrupt_frame_t *frame) {
    u32 irq_num = frame->eip - 32;

    /* Increment nesting level */
    interrupt_nesting_level++;

    /* Send EOI to the PIC */
    if (irq_num >= 8) {
        /* Send EOI to slave PIC */
        outb(0xA0, 0x20);
    }

    /* Send EOI to master PIC */
    outb(0x20, 0x20);

    /* Re-enable interrupts for nested handling */
    __asm__ volatile("sti");

    /* Check if we have a handler for this IRQ */
    if (interrupt_handlers[frame->eip] != NULL) {
        interrupt_handlers[frame->eip](frame);
    }

    /* Disable interrupts before returning */
    __asm__ volatile("cli");

    /* Decrement nesting level */
    interrupt_nesting_level--;

    /* If we're back to level 0, check for deferred work */
    if (interrupt_nesting_level == 0) {
        check_deferred_work();
    }
}

/* Initialize interrupts */
void interrupt_init(void) {
    /* Initialize the interrupt handlers */
    for (u32 i = 0; i < 256; i++) {
        interrupt_handlers[i] = NULL;
    }
}

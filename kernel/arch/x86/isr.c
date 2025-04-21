/**
 * arch/x86/isr.c - Horizon kernel x86 interrupt service routines
 * 
 * This file contains the implementation of the x86 interrupt service routines.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/interrupt.h>
#include <horizon/arch/interrupt.h>
#include <horizon/printk.h>

/* Exception messages */
static const char *exception_messages[] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 floating-point exception",
    "Alignment check",
    "Machine check",
    "SIMD floating-point exception",
    "Virtualization exception",
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

/**
 * Handle an interrupt service routine
 * 
 * @param frame Interrupt frame
 */
void isr_handler(struct interrupt_frame *frame) {
    /* Check if this is a valid exception */
    if (frame->ip < 32) {
        /* Print the exception message */
        printk(KERN_EMERG "EXCEPTION: %s (code %d)\n", exception_messages[frame->ip], frame->ip);
        
        /* Print the error code */
        printk(KERN_EMERG "Error code: %d\n", frame->cs);
        
        /* Print the registers */
        printk(KERN_EMERG "Registers: eip=%08x cs=%04x flags=%08x esp=%08x ss=%04x\n",
               frame->ip, frame->cs, frame->flags, frame->sp, frame->ss);
        
        /* Halt the system */
        for (;;) {
            __asm__ volatile("hlt");
        }
    }
    
    /* Handle the interrupt */
    interrupt_handle(frame->ip, frame);
}

/**
 * Handle an interrupt request
 * 
 * @param frame Interrupt frame
 */
void irq_handler(struct interrupt_frame *frame) {
    /* Increment the interrupt nesting level */
    interrupt_nesting_level++;
    
    /* Handle the interrupt */
    interrupt_handle(frame->ip, frame);
    
    /* Send EOI */
    if (frame->ip >= 32 && frame->ip < 48) {
        interrupt_eoi(frame->ip - 32);
    }
    
    /* Decrement the interrupt nesting level */
    interrupt_nesting_level--;
}

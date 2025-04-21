/**
 * arch/x86/interrupt.S - Horizon kernel x86 interrupt handlers
 * 
 * This file contains the assembly code for the x86 interrupt handlers.
 */

.section .text

/* Macro to create an ISR without an error code */
.macro ISR_NOERR num
.global isr\num
isr\num:
    cli
    push $0          /* Push a dummy error code */
    push $\num       /* Push the interrupt number */
    jmp isr_common   /* Jump to the common handler */
.endm

/* Macro to create an ISR with an error code */
.macro ISR_ERR num
.global isr\num
isr\num:
    cli
    push $\num       /* Push the interrupt number */
    jmp isr_common   /* Jump to the common handler */
.endm

/* Macro to create an IRQ handler */
.macro IRQ num, irq
.global irq\num
irq\num:
    cli
    push $0          /* Push a dummy error code */
    push $\irq       /* Push the interrupt number */
    jmp irq_common   /* Jump to the common handler */
.endm

/* Exception handlers */
ISR_NOERR 0    /* Division by zero */
ISR_NOERR 1    /* Debug */
ISR_NOERR 2    /* Non-maskable interrupt */
ISR_NOERR 3    /* Breakpoint */
ISR_NOERR 4    /* Overflow */
ISR_NOERR 5    /* Bound range exceeded */
ISR_NOERR 6    /* Invalid opcode */
ISR_NOERR 7    /* Device not available */
ISR_ERR   8    /* Double fault */
ISR_NOERR 9    /* Coprocessor segment overrun */
ISR_ERR   10   /* Invalid TSS */
ISR_ERR   11   /* Segment not present */
ISR_ERR   12   /* Stack-segment fault */
ISR_ERR   13   /* General protection fault */
ISR_ERR   14   /* Page fault */
ISR_NOERR 15   /* Reserved */
ISR_NOERR 16   /* x87 floating-point exception */
ISR_ERR   17   /* Alignment check */
ISR_NOERR 18   /* Machine check */
ISR_NOERR 19   /* SIMD floating-point exception */
ISR_NOERR 20   /* Virtualization exception */
ISR_NOERR 21   /* Reserved */
ISR_NOERR 22   /* Reserved */
ISR_NOERR 23   /* Reserved */
ISR_NOERR 24   /* Reserved */
ISR_NOERR 25   /* Reserved */
ISR_NOERR 26   /* Reserved */
ISR_NOERR 27   /* Reserved */
ISR_NOERR 28   /* Reserved */
ISR_NOERR 29   /* Reserved */
ISR_NOERR 30   /* Reserved */
ISR_NOERR 31   /* Reserved */

/* IRQ handlers */
IRQ 0, 32      /* Programmable Interrupt Timer */
IRQ 1, 33      /* Keyboard */
IRQ 2, 34      /* Cascade for 8259A Slave controller */
IRQ 3, 35      /* COM2 */
IRQ 4, 36      /* COM1 */
IRQ 5, 37      /* LPT2 */
IRQ 6, 38      /* Floppy disk */
IRQ 7, 39      /* LPT1 / Unreliable "spurious" interrupt */
IRQ 8, 40      /* CMOS real-time clock */
IRQ 9, 41      /* Free for peripherals / legacy SCSI / NIC */
IRQ 10, 42     /* Free for peripherals / SCSI / NIC */
IRQ 11, 43     /* Free for peripherals / SCSI / NIC */
IRQ 12, 44     /* PS/2 mouse */
IRQ 13, 45     /* FPU / Coprocessor / Inter-processor */
IRQ 14, 46     /* Primary ATA hard disk */
IRQ 15, 47     /* Secondary ATA hard disk */

/* Common ISR handler */
isr_common:
    /* Save registers */
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    
    /* Load kernel data segment */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    /* Call the C handler */
    push %esp       /* Push the pointer to the interrupt frame */
    call isr_handler
    add $4, %esp    /* Clean up the stack */
    
    /* Restore registers */
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    
    /* Clean up the stack */
    add $8, %esp    /* Skip the error code and interrupt number */
    iret            /* Return from the interrupt */

/* Common IRQ handler */
irq_common:
    /* Save registers */
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    
    /* Load kernel data segment */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    /* Call the C handler */
    push %esp       /* Push the pointer to the interrupt frame */
    call irq_handler
    add $4, %esp    /* Clean up the stack */
    
    /* Restore registers */
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    
    /* Clean up the stack */
    add $8, %esp    /* Skip the error code and interrupt number */
    iret            /* Return from the interrupt */

/* C handlers */
.extern isr_handler
.extern irq_handler

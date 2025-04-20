/**
 * interrupt.S - Interrupt handlers for x86 architecture
 * 
 * This file contains the interrupt handlers for the x86 architecture.
 */

/* Macro for ISR without error code */
.macro ISR_NOERR num
.global isr\num
isr\num:
    cli
    pushl $0          /* Push dummy error code */
    pushl $\num       /* Push interrupt number */
    jmp isr_common
.endm

/* Macro for ISR with error code */
.macro ISR_ERR num
.global isr\num
isr\num:
    cli
    /* Error code is already pushed by the CPU */
    pushl $\num       /* Push interrupt number */
    jmp isr_common
.endm

/* Macro for IRQ */
.macro IRQ num, irq_num
.global irq\num
irq\num:
    cli
    pushl $0          /* Push dummy error code */
    pushl $\irq_num   /* Push interrupt number */
    jmp irq_common
.endm

/* ISR handlers */
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

/* IRQ handlers */
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

/* Common ISR handler */
isr_common:
    /* Save registers */
    pusha
    
    /* Save data segment */
    movw %ds, %ax
    pushl %eax
    
    /* Load kernel data segment */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    /* Call C handler */
    call isr_handler
    
    /* Restore data segment */
    popl %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    /* Restore registers */
    popa
    
    /* Clean up stack */
    addl $8, %esp
    
    /* Return from interrupt */
    iret

/* Common IRQ handler */
irq_common:
    /* Save registers */
    pusha
    
    /* Save data segment */
    movw %ds, %ax
    pushl %eax
    
    /* Load kernel data segment */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    /* Call C handler */
    call irq_handler
    
    /* Restore data segment */
    popl %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    /* Restore registers */
    popa
    
    /* Clean up stack */
    addl $8, %esp
    
    /* Return from interrupt */
    iret

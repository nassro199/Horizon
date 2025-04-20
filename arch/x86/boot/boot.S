/**
 * boot.S - Boot code for Horizon kernel on x86 architecture
 *
 * This file contains the boot code for the Horizon kernel on x86 architecture.
 */

.set ALIGN,     1<<0                /* Align loaded modules on page boundaries */
.set MEMINFO,   1<<1                /* Provide memory map */
.set VIDMODE,   1<<2                /* Video mode information */
.set AOUT_KLUDGE, 1<<16             /* Load address info */
.set FLAGS,     ALIGN | MEMINFO | VIDMODE | AOUT_KLUDGE  /* Multiboot flags */
.set MAGIC,     0x1BADB002          /* Magic number for bootloader */
.set CHECKSUM,  -(MAGIC + FLAGS)    /* Checksum required */

/* Multiboot header */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Address information for AOUT_KLUDGE */
.long 0x00100000                    /* header_addr */
.long 0x00100000                    /* load_addr */
.long 0x00000000                    /* load_end_addr */
.long 0x00000000                    /* bss_end_addr */
.long 0x00100000                    /* entry_addr */

/* Video mode information */
.long 0                             /* Mode type (0 = text) */
.long 80                            /* Width */
.long 25                            /* Height */
.long 0                             /* Depth */

/* Stack setup */
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 /* 16 KiB */
stack_top:

/* Entry point */
.section .text
.global _start
.type _start, @function
_start:
    /* Set up the stack */
    movl $stack_top, %esp

    /* Save multiboot info */
    pushl %ebx
    pushl %eax

    /* Call the kernel main function */
    call kernel_main

    /* If kernel_main returns, halt the CPU */
    cli
1:  hlt
    jmp 1b

/* Set the size of the _start symbol */
.size _start, . - _start

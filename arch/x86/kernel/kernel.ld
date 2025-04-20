/**
 * kernel.ld - Linker script for the kernel
 * 
 * This file contains the linker script for the kernel.
 */

ENTRY(_start)

SECTIONS
{
    /* The kernel will be loaded at 1MB */
    . = 1M;

    /* Multiboot header first */
    .multiboot ALIGN(4K) : {
        *(.multiboot)
    }

    /* Text section */
    .text ALIGN(4K) : {
        *(.text)
    }

    /* Read-only data section */
    .rodata ALIGN(4K) : {
        *(.rodata)
    }

    /* Data section */
    .data ALIGN(4K) : {
        *(.data)
    }

    /* BSS section */
    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }
}

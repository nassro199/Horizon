/**
 * io.h - I/O port access functions
 * 
 * This file contains functions for accessing I/O ports on x86 architecture.
 */

#ifndef _ASM_IO_H
#define _ASM_IO_H

#include <horizon/types.h>

/* Read a byte from an I/O port */
static inline u8 inb(u16 port) {
    u8 value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "dN"(port));
    return value;
}

/* Write a byte to an I/O port */
static inline void outb(u16 port, u8 value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "dN"(port));
}

/* Read a word from an I/O port */
static inline u16 inw(u16 port) {
    u16 value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "dN"(port));
    return value;
}

/* Write a word to an I/O port */
static inline void outw(u16 port, u16 value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "dN"(port));
}

/* Read a long from an I/O port */
static inline u32 inl(u16 port) {
    u32 value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "dN"(port));
    return value;
}

/* Write a long to an I/O port */
static inline void outl(u16 port, u32 value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "dN"(port));
}

/* I/O delay */
static inline void io_delay(void) {
    __asm__ volatile("outb %%al, $0x80" : : "a"(0));
}

#endif /* _ASM_IO_H */

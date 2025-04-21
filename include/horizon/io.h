/**
 * io.h - I/O port access functions
 *
 * This file contains functions for accessing I/O ports.
 */

#ifndef _HORIZON_IO_H
#define _HORIZON_IO_H

#include <horizon/types.h>

/**
 * Read a byte from an I/O port
 *
 * @param port Port to read from
 * @return Value read
 */
static inline u8 inb(u16 port) {
    u8 value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * Write a byte to an I/O port
 *
 * @param port Port to write to
 * @param value Value to write
 */
static inline void outb(u16 port, u8 value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read a word from an I/O port
 *
 * @param port Port to read from
 * @return Value read
 */
static inline u16 inw(u16 port) {
    u16 value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * Write a word to an I/O port
 *
 * @param port Port to write to
 * @param value Value to write
 */
static inline void outw(u16 port, u16 value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read a long from an I/O port
 *
 * @param port Port to read from
 * @return Value read
 */
static inline u32 inl(u16 port) {
    u32 value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * Write a long to an I/O port
 *
 * @param port Port to write to
 * @param value Value to write
 */
static inline void outl(u16 port, u32 value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read multiple bytes from an I/O port
 *
 * @param port Port to read from
 * @param buffer Buffer to read into
 * @param count Number of bytes to read
 */
static inline void insb(u16 port, void *buffer, u32 count) {
    __asm__ volatile("rep insb" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

/**
 * Write multiple bytes to an I/O port
 *
 * @param port Port to write to
 * @param buffer Buffer to write from
 * @param count Number of bytes to write
 */
static inline void outsb(u16 port, const void *buffer, u32 count) {
    __asm__ volatile("rep outsb" : "+S"(buffer), "+c"(count) : "d"(port) : "memory");
}

/**
 * Read multiple words from an I/O port
 *
 * @param port Port to read from
 * @param buffer Buffer to read into
 * @param count Number of words to read
 */
static inline void insw(u16 port, void *buffer, u32 count) {
    __asm__ volatile("rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

/**
 * Write multiple words to an I/O port
 *
 * @param port Port to write to
 * @param buffer Buffer to write from
 * @param count Number of words to write
 */
static inline void outsw(u16 port, const void *buffer, u32 count) {
    __asm__ volatile("rep outsw" : "+S"(buffer), "+c"(count) : "d"(port) : "memory");
}

/**
 * Read multiple longs from an I/O port
 *
 * @param port Port to read from
 * @param buffer Buffer to read into
 * @param count Number of longs to read
 */
static inline void insl(u16 port, void *buffer, u32 count) {
    __asm__ volatile("rep insl" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

/**
 * Write multiple longs to an I/O port
 *
 * @param port Port to write to
 * @param buffer Buffer to write from
 * @param count Number of longs to write
 */
static inline void outsl(u16 port, const void *buffer, u32 count) {
    __asm__ volatile("rep outsl" : "+S"(buffer), "+c"(count) : "d"(port) : "memory");
}

/**
 * Delay by reading from an unused port
 */
static inline void io_delay(void) {
    outb(0x80, 0);
}

/**
 * Wait for I/O operations to complete
 */
static inline void io_wait(void) {
    io_delay();
}

#endif /* _HORIZON_IO_H */

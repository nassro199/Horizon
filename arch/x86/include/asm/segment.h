/**
 * segment.h - Segment descriptor definitions
 * 
 * This file contains definitions for segment descriptors on x86 architecture.
 */

#ifndef _ASM_SEGMENT_H
#define _ASM_SEGMENT_H

#include <horizon/types.h>

/* Segment descriptor structure */
typedef struct segment_descriptor {
    u16 limit_low;        /* Limit (bits 0-15) */
    u16 base_low;         /* Base (bits 0-15) */
    u8 base_middle;       /* Base (bits 16-23) */
    u8 access;            /* Access byte */
    u8 granularity;       /* Granularity byte */
    u8 base_high;         /* Base (bits 24-31) */
} __attribute__((packed)) segment_descriptor_t;

/* Segment selector */
typedef u16 segment_selector_t;

/* Access byte flags */
#define SEG_PRESENT     0x80    /* Present bit */
#define SEG_DPL_0       0x00    /* Privilege level 0 */
#define SEG_DPL_1       0x20    /* Privilege level 1 */
#define SEG_DPL_2       0x40    /* Privilege level 2 */
#define SEG_DPL_3       0x60    /* Privilege level 3 */
#define SEG_SYSTEM      0x00    /* System segment */
#define SEG_NON_SYSTEM  0x10    /* Non-system segment */
#define SEG_CODE        0x08    /* Code segment */
#define SEG_DATA        0x00    /* Data segment */
#define SEG_EXPAND_DOWN 0x04    /* Expand-down segment */
#define SEG_CONFORM     0x04    /* Conforming segment */
#define SEG_READABLE    0x02    /* Readable segment */
#define SEG_WRITABLE    0x02    /* Writable segment */
#define SEG_ACCESSED    0x01    /* Accessed bit */

/* Granularity byte flags */
#define SEG_GRAN_BYTE   0x00    /* Byte granularity */
#define SEG_GRAN_PAGE   0x80    /* Page granularity */
#define SEG_32BIT       0x40    /* 32-bit segment */
#define SEG_16BIT       0x00    /* 16-bit segment */

/* GDT selectors */
#define GDT_NULL        0x00    /* Null selector */
#define GDT_KERNEL_CODE 0x08    /* Kernel code selector */
#define GDT_KERNEL_DATA 0x10    /* Kernel data selector */
#define GDT_USER_CODE   0x18    /* User code selector */
#define GDT_USER_DATA   0x20    /* User data selector */
#define GDT_TSS         0x28    /* TSS selector */

/* Set up a segment descriptor */
static inline void segment_descriptor_set(segment_descriptor_t *desc, u32 base, u32 limit, u8 access, u8 granularity) {
    desc->base_low = base & 0xFFFF;
    desc->base_middle = (base >> 16) & 0xFF;
    desc->base_high = (base >> 24) & 0xFF;
    
    desc->limit_low = limit & 0xFFFF;
    desc->granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
    
    desc->access = access;
}

#endif /* _ASM_SEGMENT_H */

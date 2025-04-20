/**
 * vga.c - VGA console driver
 * 
 * This file contains the implementation of the VGA console driver.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/device.h>
#include <horizon/fs.h>
#include <asm/io.h>

/* VGA text mode constants */
#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_MEMORY      0xB8000

/* VGA colors */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15
};

/* VGA state */
static u16 *vga_memory = (u16 *)VGA_MEMORY;
static u8 vga_x = 0;
static u8 vga_y = 0;
static u8 vga_attr = 0;

/* Create a VGA entry */
static inline u16 vga_entry(u8 c, u8 attr) {
    return (u16)c | ((u16)attr << 8);
}

/* Create a VGA color attribute */
static inline u8 vga_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

/* Clear the screen */
static void vga_clear(void) {
    for (u32 y = 0; y < VGA_HEIGHT; y++) {
        for (u32 x = 0; x < VGA_WIDTH; x++) {
            vga_memory[y * VGA_WIDTH + x] = vga_entry(' ', vga_attr);
        }
    }
    
    vga_x = 0;
    vga_y = 0;
}

/* Update the cursor position */
static void vga_update_cursor(void) {
    u16 pos = vga_y * VGA_WIDTH + vga_x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

/* Scroll the screen */
static void vga_scroll(void) {
    /* Move all lines up */
    for (u32 y = 0; y < VGA_HEIGHT - 1; y++) {
        for (u32 x = 0; x < VGA_WIDTH; x++) {
            vga_memory[y * VGA_WIDTH + x] = vga_memory[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    /* Clear the last line */
    for (u32 x = 0; x < VGA_WIDTH; x++) {
        vga_memory[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', vga_attr);
    }
    
    vga_y = VGA_HEIGHT - 1;
}

/* Put a character on the screen */
static void vga_putchar(char c) {
    /* Handle special characters */
    if (c == '\n') {
        vga_x = 0;
        vga_y++;
    } else if (c == '\r') {
        vga_x = 0;
    } else if (c == '\t') {
        vga_x = (vga_x + 8) & ~7;
    } else if (c == '\b') {
        if (vga_x > 0) {
            vga_x--;
        } else if (vga_y > 0) {
            vga_x = VGA_WIDTH - 1;
            vga_y--;
        }
        vga_memory[vga_y * VGA_WIDTH + vga_x] = vga_entry(' ', vga_attr);
    } else {
        /* Regular character */
        vga_memory[vga_y * VGA_WIDTH + vga_x] = vga_entry(c, vga_attr);
        vga_x++;
    }
    
    /* Handle line wrapping */
    if (vga_x >= VGA_WIDTH) {
        vga_x = 0;
        vga_y++;
    }
    
    /* Handle scrolling */
    if (vga_y >= VGA_HEIGHT) {
        vga_scroll();
    }
    
    /* Update the cursor */
    vga_update_cursor();
}

/* Write a string to the screen */
void vga_puts(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

/* Set the text color */
void vga_set_color(enum vga_color fg, enum vga_color bg) {
    vga_attr = vga_color(fg, bg);
}

/* Initialize the VGA console */
void vga_init(void) {
    /* Set the default color */
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    /* Clear the screen */
    vga_clear();
    
    /* Print a welcome message */
    vga_puts("VGA console initialized\n");
}

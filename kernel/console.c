/**
 * console.c - Horizon kernel console implementation
 *
 * This file contains the implementation of the kernel console.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/console.h>
#include <horizon/string.h>
#include <horizon/stdarg.h>
#include <horizon/io.h>

/* VGA text mode constants */
#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_MEMORY      0xB8000

/* Console state */
static u16 *vga_memory = (u16 *)VGA_MEMORY;
static u32 console_x = 0;
static u32 console_y = 0;
static u8 console_attr = CONSOLE_DEFAULT_ATTR;
static u32 saved_x = 0;
static u32 saved_y = 0;
static u8 cursor_visible = 1;

/**
 * Initialize console
 */
void console_init(void) {
    /* Clear console */
    console_clear();

    /* Show cursor */
    console_show_cursor();
}

/**
 * Clear console
 */
void console_clear(void) {
    /* Clear screen */
    for (u32 i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_memory[i] = (console_attr << 8) | ' ';
    }

    /* Reset cursor */
    console_x = 0;
    console_y = 0;

    /* Update hardware cursor */
    if (cursor_visible) {
        u16 pos = console_y * VGA_WIDTH + console_x;
        outb(0x3D4, 14);
        outb(0x3D5, pos >> 8);
        outb(0x3D4, 15);
        outb(0x3D5, pos);
    }
}

/**
 * Put a character on the console
 *
 * @param c Character to put
 */
void console_putchar(char c) {
    /* Handle special characters */
    switch (c) {
        case '\n':
            console_newline();
            return;
        case '\r':
            console_x = 0;
            return;
        case '\b':
            console_backspace();
            return;
        case '\t':
            console_tab();
            return;
    }

    /* Put character */
    vga_memory[console_y * VGA_WIDTH + console_x] = (console_attr << 8) | c;

    /* Advance cursor */
    console_x++;
    if (console_x >= VGA_WIDTH) {
        console_x = 0;
        console_y++;
        if (console_y >= VGA_HEIGHT) {
            console_scroll();
        }
    }

    /* Update hardware cursor */
    if (cursor_visible) {
        u16 pos = console_y * VGA_WIDTH + console_x;
        outb(0x3D4, 14);
        outb(0x3D5, pos >> 8);
        outb(0x3D4, 15);
        outb(0x3D5, pos);
    }
}

/**
 * Write a string to the console
 *
 * @param str String to write
 */
void console_write(const char *str) {
    while (*str) {
        console_putchar(*str++);
    }
}

/**
 * Write a string to the console with attributes
 *
 * @param str String to write
 * @param attr Attributes
 */
void console_write_attr(const char *str, u8 attr) {
    u8 old_attr = console_attr;
    console_attr = attr;
    console_write(str);
    console_attr = old_attr;
}

/**
 * Write a string to the console with colors
 *
 * @param str String to write
 * @param fg Foreground color
 * @param bg Background color
 */
void console_write_color(const char *str, u8 fg, u8 bg) {
    console_write_attr(str, CONSOLE_ATTR(fg, bg));
}

/**
 * Print formatted string to the console
 *
 * @param fmt Format string
 * @param ... Arguments
 */
void console_printf(const char *fmt, ...) {
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    console_write(buf);
}

/**
 * Print formatted string to the console with attributes
 *
 * @param attr Attributes
 * @param fmt Format string
 * @param ... Arguments
 */
void console_printf_attr(u8 attr, const char *fmt, ...) {
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    console_write_attr(buf, attr);
}

/**
 * Print formatted string to the console with colors
 *
 * @param fg Foreground color
 * @param bg Background color
 * @param fmt Format string
 * @param ... Arguments
 */
void console_printf_color(u8 fg, u8 bg, const char *fmt, ...) {
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    console_write_color(buf, fg, bg);
}

/**
 * Set console attributes
 *
 * @param attr Attributes
 */
void console_set_attr(u8 attr) {
    console_attr = attr;
}

/**
 * Set console colors
 *
 * @param fg Foreground color
 * @param bg Background color
 */
void console_set_color(u8 fg, u8 bg) {
    console_attr = CONSOLE_ATTR(fg, bg);
}

/**
 * Get console attributes
 *
 * @return Attributes
 */
u8 console_get_attr(void) {
    return console_attr;
}

/**
 * Get console colors
 *
 * @param fg Pointer to store foreground color
 * @param bg Pointer to store background color
 */
void console_get_color(u8 *fg, u8 *bg) {
    if (fg) {
        *fg = console_attr & 0x0F;
    }
    if (bg) {
        *bg = (console_attr >> 4) & 0x0F;
    }
}

/**
 * Set cursor position
 *
 * @param x X position
 * @param y Y position
 */
void console_set_cursor(u32 x, u32 y) {
    /* Check bounds */
    if (x >= VGA_WIDTH) {
        x = VGA_WIDTH - 1;
    }
    if (y >= VGA_HEIGHT) {
        y = VGA_HEIGHT - 1;
    }

    /* Set cursor position */
    console_x = x;
    console_y = y;

    /* Update hardware cursor */
    if (cursor_visible) {
        u16 pos = console_y * VGA_WIDTH + console_x;
        outb(0x3D4, 14);
        outb(0x3D5, pos >> 8);
        outb(0x3D4, 15);
        outb(0x3D5, pos);
    }
}

/**
 * Get cursor position
 *
 * @param x Pointer to store X position
 * @param y Pointer to store Y position
 */
void console_get_cursor(u32 *x, u32 *y) {
    if (x) {
        *x = console_x;
    }
    if (y) {
        *y = console_y;
    }
}

/**
 * Show cursor
 */
void console_show_cursor(void) {
    cursor_visible = 1;

    /* Enable cursor */
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0x0E);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 0x0F);

    /* Update cursor position */
    u16 pos = console_y * VGA_WIDTH + console_x;
    outb(0x3D4, 14);
    outb(0x3D5, pos >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, pos);
}

/**
 * Hide cursor
 */
void console_hide_cursor(void) {
    cursor_visible = 0;

    /* Disable cursor */
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

/**
 * Scroll console
 */
void console_scroll(void) {
    /* Move lines up */
    for (u32 i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_memory[i] = vga_memory[i + VGA_WIDTH];
    }

    /* Clear last line */
    for (u32 i = 0; i < VGA_WIDTH; i++) {
        vga_memory[(VGA_HEIGHT - 1) * VGA_WIDTH + i] = (console_attr << 8) | ' ';
    }

    /* Adjust cursor */
    console_y--;
}

/**
 * Move to next line
 */
void console_newline(void) {
    console_x = 0;
    console_y++;
    if (console_y >= VGA_HEIGHT) {
        console_scroll();
    }

    /* Update hardware cursor */
    if (cursor_visible) {
        u16 pos = console_y * VGA_WIDTH + console_x;
        outb(0x3D4, 14);
        outb(0x3D5, pos >> 8);
        outb(0x3D4, 15);
        outb(0x3D5, pos);
    }
}

/**
 * Backspace
 */
void console_backspace(void) {
    if (console_x > 0) {
        console_x--;
    } else if (console_y > 0) {
        console_y--;
        console_x = VGA_WIDTH - 1;
    }

    /* Clear character */
    vga_memory[console_y * VGA_WIDTH + console_x] = (console_attr << 8) | ' ';

    /* Update hardware cursor */
    if (cursor_visible) {
        u16 pos = console_y * VGA_WIDTH + console_x;
        outb(0x3D4, 14);
        outb(0x3D5, pos >> 8);
        outb(0x3D4, 15);
        outb(0x3D5, pos);
    }
}

/**
 * Tab
 */
void console_tab(void) {
    /* Move to next tab stop */
    console_x = (console_x + 8) & ~7;
    if (console_x >= VGA_WIDTH) {
        console_x = 0;
        console_y++;
        if (console_y >= VGA_HEIGHT) {
            console_scroll();
        }
    }

    /* Update hardware cursor */
    if (cursor_visible) {
        u16 pos = console_y * VGA_WIDTH + console_x;
        outb(0x3D4, 14);
        outb(0x3D5, pos >> 8);
        outb(0x3D4, 15);
        outb(0x3D5, pos);
    }
}

/**
 * Save cursor position
 */
void console_save_cursor(void) {
    saved_x = console_x;
    saved_y = console_y;
}

/**
 * Restore cursor position
 */
void console_restore_cursor(void) {
    console_set_cursor(saved_x, saved_y);
}

/**
 * Print a string to the early console
 *
 * @param str String to print
 */
void early_console_print(const char *str) {
    console_write(str);
}



/**
 * Print decimal number
 *
 * @param n Number to print
 */
void console_print_dec(u32 n) {
    char buf[16];
    u32 i = 0;

    /* Handle 0 */
    if (n == 0) {
        console_putchar('0');
        return;
    }

    /* Convert to string */
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }

    /* Print in reverse order */
    while (i > 0) {
        console_putchar(buf[--i]);
    }
}

/**
 * Print hexadecimal number
 *
 * @param n Number to print
 */
void console_print_hex(u32 n) {
    char buf[16];
    u32 i = 0;

    /* Handle 0 */
    if (n == 0) {
        console_write("0x0");
        return;
    }

    /* Convert to string */
    while (n > 0) {
        u32 digit = n & 0xF;
        buf[i++] = digit < 10 ? '0' + digit : 'A' + digit - 10;
        n >>= 4;
    }

    /* Print prefix */
    console_write("0x");

    /* Print in reverse order */
    while (i > 0) {
        console_putchar(buf[--i]);
    }
}

/**
 * Print binary number
 *
 * @param n Number to print
 */
void console_print_bin(u32 n) {
    char buf[32];
    u32 i = 0;

    /* Handle 0 */
    if (n == 0) {
        console_write("0b0");
        return;
    }

    /* Convert to string */
    while (n > 0) {
        buf[i++] = '0' + (n & 1);
        n >>= 1;
    }

    /* Print prefix */
    console_write("0b");

    /* Print in reverse order */
    while (i > 0) {
        console_putchar(buf[--i]);
    }
}

/**
 * Print octal number
 *
 * @param n Number to print
 */
void console_print_oct(u32 n) {
    char buf[16];
    u32 i = 0;

    /* Handle 0 */
    if (n == 0) {
        console_write("0o0");
        return;
    }

    /* Convert to string */
    while (n > 0) {
        buf[i++] = '0' + (n & 7);
        n >>= 3;
    }

    /* Print prefix */
    console_write("0o");

    /* Print in reverse order */
    while (i > 0) {
        console_putchar(buf[--i]);
    }
}

/* Early console functions */

/**
 * Initialize early console
 */
void early_console_init(void) {
    /* Clear console */
    early_console_clear();

    /* Show cursor */
    early_console_show_cursor();
}

/**
 * Clear early console
 */
void early_console_clear(void) {
    console_clear();
}

/**
 * Put a character on the early console
 *
 * @param c Character to put
 */
void early_console_putchar(char c) {
    console_putchar(c);
}

/**
 * Write a string to the early console
 *
 * @param str String to write
 */
void early_console_write(const char *str) {
    console_write(str);
}

/**
 * Write a string to the early console with attributes
 *
 * @param str String to write
 * @param attr Attributes
 */
void early_console_write_attr(const char *str, u8 attr) {
    console_write_attr(str, attr);
}

/**
 * Write a string to the early console with colors
 *
 * @param str String to write
 * @param fg Foreground color
 * @param bg Background color
 */
void early_console_write_color(const char *str, u8 fg, u8 bg) {
    console_write_color(str, fg, bg);
}

/**
 * Set early console attributes
 *
 * @param attr Attributes
 */
void early_console_set_attr(u8 attr) {
    console_set_attr(attr);
}

/**
 * Set early console colors
 *
 * @param fg Foreground color
 * @param bg Background color
 */
void early_console_set_color(u8 fg, u8 bg) {
    console_set_color(fg, bg);
}

/**
 * Get early console attributes
 *
 * @return Attributes
 */
u8 early_console_get_attr(void) {
    return console_get_attr();
}

/**
 * Get early console colors
 *
 * @param fg Pointer to store foreground color
 * @param bg Pointer to store background color
 */
void early_console_get_color(u8 *fg, u8 *bg) {
    console_get_color(fg, bg);
}

/**
 * Set early cursor position
 *
 * @param x X position
 * @param y Y position
 */
void early_console_set_cursor(u32 x, u32 y) {
    console_set_cursor(x, y);
}

/**
 * Get early cursor position
 *
 * @param x Pointer to store X position
 * @param y Pointer to store Y position
 */
void early_console_get_cursor(u32 *x, u32 *y) {
    console_get_cursor(x, y);
}

/**
 * Show early cursor
 */
void early_console_show_cursor(void) {
    console_show_cursor();
}

/**
 * Hide early cursor
 */
void early_console_hide_cursor(void) {
    console_hide_cursor();
}

/**
 * Scroll early console
 */
void early_console_scroll(void) {
    console_scroll();
}

/**
 * Move to next line on early console
 */
void early_console_newline(void) {
    console_newline();
}

/**
 * Backspace on early console
 */
void early_console_backspace(void) {
    console_backspace();
}

/**
 * Tab on early console
 */
void early_console_tab(void) {
    console_tab();
}

/**
 * Save early cursor position
 */
void early_console_save_cursor(void) {
    console_save_cursor();
}

/**
 * Restore early cursor position
 */
void early_console_restore_cursor(void) {
    console_restore_cursor();
}

/**
 * Print decimal number on early console
 *
 * @param n Number to print
 */
void early_console_print_dec(u32 n) {
    console_print_dec(n);
}

/**
 * Print hexadecimal number on early console
 *
 * @param n Number to print
 */
void early_console_print_hex(u32 n) {
    console_print_hex(n);
}

/**
 * Print binary number on early console
 *
 * @param n Number to print
 */
void early_console_print_bin(u32 n) {
    console_print_bin(n);
}

/**
 * Print octal number on early console
 *
 * @param n Number to print
 */
void early_console_print_oct(u32 n) {
    console_print_oct(n);
}

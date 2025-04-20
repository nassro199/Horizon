/**
 * console.h - Horizon kernel console definitions
 * 
 * This file contains definitions for the kernel console.
 */

#ifndef _HORIZON_CONSOLE_H
#define _HORIZON_CONSOLE_H

#include <horizon/types.h>

/* Console colors */
#define CONSOLE_BLACK         0x0
#define CONSOLE_BLUE          0x1
#define CONSOLE_GREEN         0x2
#define CONSOLE_CYAN          0x3
#define CONSOLE_RED           0x4
#define CONSOLE_MAGENTA       0x5
#define CONSOLE_BROWN         0x6
#define CONSOLE_LIGHT_GRAY    0x7
#define CONSOLE_DARK_GRAY     0x8
#define CONSOLE_LIGHT_BLUE    0x9
#define CONSOLE_LIGHT_GREEN   0xA
#define CONSOLE_LIGHT_CYAN    0xB
#define CONSOLE_LIGHT_RED     0xC
#define CONSOLE_LIGHT_MAGENTA 0xD
#define CONSOLE_YELLOW        0xE
#define CONSOLE_WHITE         0xF

/* Console attributes */
#define CONSOLE_ATTR(fg, bg)  ((bg) << 4 | (fg))

/* Default console attributes */
#define CONSOLE_DEFAULT_ATTR  CONSOLE_ATTR(CONSOLE_LIGHT_GRAY, CONSOLE_BLACK)
#define CONSOLE_ERROR_ATTR    CONSOLE_ATTR(CONSOLE_LIGHT_RED, CONSOLE_BLACK)
#define CONSOLE_WARNING_ATTR  CONSOLE_ATTR(CONSOLE_YELLOW, CONSOLE_BLACK)
#define CONSOLE_INFO_ATTR     CONSOLE_ATTR(CONSOLE_LIGHT_GREEN, CONSOLE_BLACK)
#define CONSOLE_DEBUG_ATTR    CONSOLE_ATTR(CONSOLE_LIGHT_CYAN, CONSOLE_BLACK)

/* Console functions */
void console_init(void);
void console_clear(void);
void console_putchar(char c);
void console_write(const char *str);
void console_write_attr(const char *str, u8 attr);
void console_write_color(const char *str, u8 fg, u8 bg);
void console_printf(const char *fmt, ...);
void console_printf_attr(u8 attr, const char *fmt, ...);
void console_printf_color(u8 fg, u8 bg, const char *fmt, ...);
void console_set_attr(u8 attr);
void console_set_color(u8 fg, u8 bg);
u8 console_get_attr(void);
void console_get_color(u8 *fg, u8 *bg);
void console_set_cursor(u32 x, u32 y);
void console_get_cursor(u32 *x, u32 *y);
void console_show_cursor(void);
void console_hide_cursor(void);
void console_scroll(void);
void console_newline(void);
void console_backspace(void);
void console_tab(void);
void console_save_cursor(void);
void console_restore_cursor(void);
void console_print_dec(u32 n);
void console_print_hex(u32 n);
void console_print_bin(u32 n);
void console_print_oct(u32 n);

/* Early console functions */
void early_console_init(void);
void early_console_clear(void);
void early_console_putchar(char c);
void early_console_write(const char *str);
void early_console_write_attr(const char *str, u8 attr);
void early_console_write_color(const char *str, u8 fg, u8 bg);
void early_console_set_attr(u8 attr);
void early_console_set_color(u8 fg, u8 bg);
u8 early_console_get_attr(void);
void early_console_get_color(u8 *fg, u8 *bg);
void early_console_set_cursor(u32 x, u32 y);
void early_console_get_cursor(u32 *x, u32 *y);
void early_console_show_cursor(void);
void early_console_hide_cursor(void);
void early_console_scroll(void);
void early_console_newline(void);
void early_console_backspace(void);
void early_console_tab(void);
void early_console_save_cursor(void);
void early_console_restore_cursor(void);
void early_console_print_dec(u32 n);
void early_console_print_hex(u32 n);
void early_console_print_bin(u32 n);
void early_console_print_oct(u32 n);

#endif /* _HORIZON_CONSOLE_H */

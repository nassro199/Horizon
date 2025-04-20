/**
 * keyboard.c - Keyboard driver
 * 
 * This file contains the implementation of the keyboard driver.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/device.h>
#include <horizon/fs.h>
#include <asm/io.h>
#include <asm/interrupt.h>

/* Keyboard ports */
#define KEYBOARD_DATA_PORT      0x60
#define KEYBOARD_STATUS_PORT    0x64
#define KEYBOARD_COMMAND_PORT   0x64

/* Keyboard commands */
#define KEYBOARD_CMD_LED        0xED
#define KEYBOARD_CMD_ECHO       0xEE
#define KEYBOARD_CMD_SCANCODE   0xF0
#define KEYBOARD_CMD_IDENTIFY   0xF2
#define KEYBOARD_CMD_TYPEMATIC  0xF3
#define KEYBOARD_CMD_ENABLE     0xF4
#define KEYBOARD_CMD_RESET      0xFF

/* Keyboard status */
#define KEYBOARD_STATUS_OUTPUT_FULL     0x01
#define KEYBOARD_STATUS_INPUT_FULL      0x02
#define KEYBOARD_STATUS_SYSTEM_FLAG     0x04
#define KEYBOARD_STATUS_COMMAND_DATA    0x08
#define KEYBOARD_STATUS_UNLOCKED        0x10
#define KEYBOARD_STATUS_MOUSE_OUTPUT    0x20
#define KEYBOARD_STATUS_TIMEOUT         0x40
#define KEYBOARD_STATUS_PARITY_ERROR    0x80

/* Keyboard LEDs */
#define KEYBOARD_LED_SCROLL_LOCK    0x01
#define KEYBOARD_LED_NUM_LOCK       0x02
#define KEYBOARD_LED_CAPS_LOCK      0x04

/* Keyboard modifiers */
#define KEYBOARD_MOD_SHIFT          0x01
#define KEYBOARD_MOD_CTRL           0x02
#define KEYBOARD_MOD_ALT            0x04
#define KEYBOARD_MOD_CAPS_LOCK      0x08
#define KEYBOARD_MOD_NUM_LOCK       0x10
#define KEYBOARD_MOD_SCROLL_LOCK    0x20

/* Keyboard state */
static u8 keyboard_modifiers = 0;
static u8 keyboard_leds = 0;

/* Scancode set 1 to ASCII mapping (US layout) */
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Scancode set 1 to ASCII mapping with shift (US layout) */
static const char scancode_to_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* External function to print a character */
extern void vga_putchar(char c);

/* Keyboard interrupt handler */
static void keyboard_handler(interrupt_frame_t *frame) {
    /* Read the scancode */
    u8 scancode = inb(KEYBOARD_DATA_PORT);
    
    /* Handle the scancode */
    if (scancode & 0x80) {
        /* Key release */
        scancode &= 0x7F;
        
        /* Handle modifier keys */
        switch (scancode) {
            case 0x2A: /* Left shift */
            case 0x36: /* Right shift */
                keyboard_modifiers &= ~KEYBOARD_MOD_SHIFT;
                break;
            case 0x1D: /* Control */
                keyboard_modifiers &= ~KEYBOARD_MOD_CTRL;
                break;
            case 0x38: /* Alt */
                keyboard_modifiers &= ~KEYBOARD_MOD_ALT;
                break;
        }
    } else {
        /* Key press */
        
        /* Handle modifier keys */
        switch (scancode) {
            case 0x2A: /* Left shift */
            case 0x36: /* Right shift */
                keyboard_modifiers |= KEYBOARD_MOD_SHIFT;
                break;
            case 0x1D: /* Control */
                keyboard_modifiers |= KEYBOARD_MOD_CTRL;
                break;
            case 0x38: /* Alt */
                keyboard_modifiers |= KEYBOARD_MOD_ALT;
                break;
            case 0x3A: /* Caps lock */
                keyboard_modifiers ^= KEYBOARD_MOD_CAPS_LOCK;
                keyboard_leds ^= KEYBOARD_LED_CAPS_LOCK;
                /* Update LEDs */
                outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_LED);
                outb(KEYBOARD_DATA_PORT, keyboard_leds);
                break;
            case 0x45: /* Num lock */
                keyboard_modifiers ^= KEYBOARD_MOD_NUM_LOCK;
                keyboard_leds ^= KEYBOARD_LED_NUM_LOCK;
                /* Update LEDs */
                outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_LED);
                outb(KEYBOARD_DATA_PORT, keyboard_leds);
                break;
            case 0x46: /* Scroll lock */
                keyboard_modifiers ^= KEYBOARD_MOD_SCROLL_LOCK;
                keyboard_leds ^= KEYBOARD_LED_SCROLL_LOCK;
                /* Update LEDs */
                outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_LED);
                outb(KEYBOARD_DATA_PORT, keyboard_leds);
                break;
            default:
                /* Regular key */
                if (scancode < sizeof(scancode_to_ascii)) {
                    char c;
                    
                    /* Apply modifiers */
                    if (keyboard_modifiers & KEYBOARD_MOD_SHIFT) {
                        c = scancode_to_ascii_shift[scancode];
                    } else {
                        c = scancode_to_ascii[scancode];
                    }
                    
                    /* Apply caps lock */
                    if (keyboard_modifiers & KEYBOARD_MOD_CAPS_LOCK) {
                        if (c >= 'a' && c <= 'z') {
                            c = c - 'a' + 'A';
                        } else if (c >= 'A' && c <= 'Z') {
                            c = c - 'A' + 'a';
                        }
                    }
                    
                    /* Print the character */
                    if (c != 0) {
                        vga_putchar(c);
                    }
                }
                break;
        }
    }
}

/* Initialize the keyboard */
void keyboard_init(void) {
    /* Register the keyboard interrupt handler */
    interrupt_register_handler(IRQ_KEYBOARD, keyboard_handler);
    
    /* Enable the keyboard */
    outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_ENABLE);
}

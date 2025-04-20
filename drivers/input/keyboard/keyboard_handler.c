/**
 * keyboard_handler.c - Keyboard handler for console input
 * 
 * This file contains the implementation of a keyboard handler for console input.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/input.h>
#include <horizon/input/keyboard.h>

/* External function to print a character */
extern void vga_putchar(char c);

/* External function to process a character in the shell */
extern void shell_process_char(char c);

/* Keyboard state */
static u8 keyboard_modifiers = 0;
static u8 keyboard_leds = 0;

/* Modifier flags */
#define MOD_SHIFT       0x01
#define MOD_CTRL        0x02
#define MOD_ALT         0x04
#define MOD_CAPS_LOCK   0x08
#define MOD_NUM_LOCK    0x10
#define MOD_SCROLL_LOCK 0x20

/* Key to ASCII mapping (US layout) */
static const char keycode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Key to ASCII mapping with shift (US layout) */
static const char keycode_to_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Keyboard handler connect method */
static int keyboard_handler_connect(input_handler_t *handler, input_dev_t *dev) {
    /* Nothing to do */
    return 0;
}

/* Keyboard handler disconnect method */
static void keyboard_handler_disconnect(input_handler_t *handler, input_dev_t *dev) {
    /* Nothing to do */
}

/* Keyboard handler event method */
static void keyboard_handler_event(input_handler_t *handler, input_dev_t *dev, u16 type, u16 code, s32 value) {
    /* Handle key events */
    if (type == EV_KEY) {
        if (value == 1) {
            /* Key press */
            
            /* Handle modifier keys */
            switch (code) {
                case KEY_LEFTSHIFT:
                case KEY_RIGHTSHIFT:
                    keyboard_modifiers |= MOD_SHIFT;
                    break;
                case KEY_LEFTCTRL:
                    keyboard_modifiers |= MOD_CTRL;
                    break;
                case KEY_LEFTALT:
                    keyboard_modifiers |= MOD_ALT;
                    break;
                case KEY_CAPSLOCK:
                    keyboard_modifiers ^= MOD_CAPS_LOCK;
                    keyboard_leds ^= (1 << LED_CAPSL);
                    input_event(dev, EV_LED, 0, keyboard_leds);
                    break;
                case KEY_NUMLOCK:
                    keyboard_modifiers ^= MOD_NUM_LOCK;
                    keyboard_leds ^= (1 << LED_NUML);
                    input_event(dev, EV_LED, 0, keyboard_leds);
                    break;
                case KEY_SCROLLLOCK:
                    keyboard_modifiers ^= MOD_SCROLL_LOCK;
                    keyboard_leds ^= (1 << LED_SCROLLL);
                    input_event(dev, EV_LED, 0, keyboard_leds);
                    break;
                default:
                    /* Regular key */
                    if (code < sizeof(keycode_to_ascii)) {
                        char c;
                        
                        /* Apply modifiers */
                        if (keyboard_modifiers & MOD_SHIFT) {
                            c = keycode_to_ascii_shift[code];
                        } else {
                            c = keycode_to_ascii[code];
                        }
                        
                        /* Apply caps lock */
                        if (keyboard_modifiers & MOD_CAPS_LOCK) {
                            if (c >= 'a' && c <= 'z') {
                                c = c - 'a' + 'A';
                            } else if (c >= 'A' && c <= 'Z') {
                                c = c - 'A' + 'a';
                            }
                        }
                        
                        /* Process the character */
                        if (c != 0) {
                            shell_process_char(c);
                        }
                    }
                    break;
            }
        } else if (value == 0) {
            /* Key release */
            
            /* Handle modifier keys */
            switch (code) {
                case KEY_LEFTSHIFT:
                case KEY_RIGHTSHIFT:
                    keyboard_modifiers &= ~MOD_SHIFT;
                    break;
                case KEY_LEFTCTRL:
                    keyboard_modifiers &= ~MOD_CTRL;
                    break;
                case KEY_LEFTALT:
                    keyboard_modifiers &= ~MOD_ALT;
                    break;
            }
        }
    }
}

/* Keyboard handler */
static input_handler_t keyboard_handler = {
    .name = "keyboard-handler",
    .event_types = (1 << EV_KEY) | (1 << EV_LED),
    .next = NULL,
    .connect = keyboard_handler_connect,
    .disconnect = keyboard_handler_disconnect,
    .event = keyboard_handler_event
};

/* Initialize the keyboard handler */
void keyboard_handler_init(void) {
    /* Register the keyboard handler */
    input_register_handler(&keyboard_handler);
}

/**
 * keyboard.c - Horizon kernel keyboard driver
 *
 * This file contains the implementation of the keyboard driver.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/input.h>
#include <horizon/device.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <asm/io.h>
#include <asm/interrupt.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

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

/* Keyboard device */
static input_device_t keyboard_dev;

/* Scancode set 1 to key code mapping */
static const u8 scancode_to_keycode[] = {
    KEY_RESERVED, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
    KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I,
    KEY_O, KEY_P, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_ENTER, KEY_LEFTCTRL, KEY_A, KEY_S, KEY_D, KEY_F,
    KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_GRAVE, KEY_LEFTSHIFT,
    KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH,
    KEY_RIGHTSHIFT, KEY_KPASTERISK, KEY_LEFTALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4,
    KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK, KEY_KP7, KEY_KP8,
    KEY_KP9, KEY_KPMINUS, KEY_KP4, KEY_KP5, KEY_KP6, KEY_KPPLUS, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP0,
    KEY_KPDOT
};

/* External function to print a character */
extern void vga_putchar(char c);

/* Keyboard event handler */
static int keyboard_event(input_device_t *dev, input_event_t *event) {
    if (dev == NULL || event == NULL) {
        return -1;
    }

    /* Handle LED events */
    if (event->type == EV_LED) {
        u8 leds = 0;

        if (event->value & (1 << LED_SCROLLL)) {
            leds |= KEYBOARD_LED_SCROLL_LOCK;
        }

        if (event->value & (1 << LED_NUML)) {
            leds |= KEYBOARD_LED_NUM_LOCK;
        }

        if (event->value & (1 << LED_CAPSL)) {
            leds |= KEYBOARD_LED_CAPS_LOCK;
        }

        /* Update LEDs */
        outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_LED);
        outb(KEYBOARD_DATA_PORT, leds);
    }

    return 0;
}

/* Keyboard interrupt handler */
static void keyboard_handler(interrupt_frame_t *frame) {
    /* Read the scancode */
    u8 scancode = inb(KEYBOARD_DATA_PORT);

    /* Handle the scancode */
    if (scancode < sizeof(scancode_to_keycode)) {
        u8 keycode = scancode_to_keycode[scancode];

        /* Send key press event */
        if (scancode & 0x80) {
            /* Key release */
            input_event(&keyboard_dev, EV_KEY, keycode, 0);
        } else {
            /* Key press */
            input_event(&keyboard_dev, EV_KEY, keycode, 1);
        }
    }
}

/* Open the keyboard device */
static int keyboard_open(input_device_t *dev) {
    if (dev == NULL) {
        return -1;
    }

    /* Enable the keyboard */
    outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_ENABLE);

    return 0;
}

/* Close the keyboard device */
static int keyboard_close(input_device_t *dev) {
    if (dev == NULL) {
        return -1;
    }

    return 0;
}

/* Flush the keyboard device */
static int keyboard_flush(input_device_t *dev) {
    if (dev == NULL) {
        return -1;
    }

    /* Flush the keyboard buffer */
    while (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL) {
        inb(KEYBOARD_DATA_PORT);
    }

    return 0;
}

/* Initialize the keyboard */
void keyboard_init(void) {
    /* Initialize the keyboard device */
    strcpy(keyboard_dev.name, "keyboard");
    keyboard_dev.event_types = (1 << EV_KEY) | (1 << EV_LED);

    /* Set supported keys */
    for (u32 i = 0; i < KEY_MAX; i++) {
        keyboard_dev.key_bits[i / 32] |= (1 << (i % 32));
    }

    /* Set device methods */
    keyboard_dev.open = keyboard_open;
    keyboard_dev.close = keyboard_close;
    keyboard_dev.flush = keyboard_flush;
    keyboard_dev.event = keyboard_event;

    /* Initialize the device structure */
    strcpy(keyboard_dev.dev.name, "keyboard");
    keyboard_dev.dev.class = NULL; /* Will be set by input_register_device */
    keyboard_dev.dev.private_data = &keyboard_dev;

    /* Register the keyboard device */
    input_register_device(&keyboard_dev);

    /* Register the keyboard interrupt handler */
    interrupt_register_handler(IRQ_KEYBOARD, keyboard_handler);

    /* Enable the keyboard */
    outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_ENABLE);
}

/* Initialize the keyboard interrupt handler */
void keyboard_handler_init(void) {
    /* Register the keyboard interrupt handler */
    interrupt_register_handler(IRQ_KEYBOARD, keyboard_handler);
}

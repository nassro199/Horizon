/**
 * tty.c - Horizon kernel TTY subsystem implementation
 * 
 * This file contains the implementation of the TTY subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/tty.h>
#include <horizon/char.h>
#include <horizon/input.h>
#include <horizon/device.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* List of TTY devices */
static list_head_t tty_list;

/* Character device operations */
static int tty_char_open(char_device_t *dev, u32 flags);
static int tty_char_close(char_device_t *dev);
static ssize_t tty_char_read(char_device_t *dev, void *buf, size_t count);
static ssize_t tty_char_write(char_device_t *dev, const void *buf, size_t count);
static int tty_char_ioctl(char_device_t *dev, u32 request, void *arg);
static int tty_char_flush(char_device_t *dev);

/* Character device operations structure */
static char_device_ops_t tty_char_ops = {
    .open = tty_char_open,
    .close = tty_char_close,
    .read = tty_char_read,
    .write = tty_char_write,
    .ioctl = tty_char_ioctl,
    .seek = NULL,
    .flush = tty_char_flush
};

/* Input handler operations */
static int tty_input_connect(input_handler_t *handler, input_device_t *dev);
static void tty_input_disconnect(input_handler_t *handler, input_device_t *dev);
static int tty_input_event(input_handler_t *handler, input_device_t *dev, input_event_t *event);

/* Initialize the TTY subsystem */
void tty_init(void) {
    /* Initialize the TTY list */
    list_init(&tty_list);
}

/* Register a TTY device */
int tty_register(tty_t *tty) {
    if (tty == NULL) {
        return -1;
    }
    
    /* Check if the TTY already exists */
    tty_t *existing = tty_get(tty->name);
    if (existing != NULL) {
        return -1;
    }
    
    /* Initialize the character device */
    tty->char_dev.ops = &tty_char_ops;
    tty->char_dev.private_data = tty;
    
    /* Initialize the input handler */
    strcpy(tty->input_handler.name, tty->name);
    tty->input_handler.event_types = (1 << EV_KEY);
    tty->input_handler.connect = tty_input_connect;
    tty->input_handler.disconnect = tty_input_disconnect;
    tty->input_handler.event = tty_input_event;
    
    /* Add the TTY to the list */
    tty->next = NULL;
    
    list_head_t *pos;
    tty_t *last = NULL;
    
    list_for_each(pos, &tty_list) {
        last = list_entry(pos, tty_t, char_dev.device.driver_list);
    }
    
    if (last == NULL) {
        list_add(&tty->char_dev.device.driver_list, &tty_list);
    } else {
        last->next = tty;
        list_add_tail(&tty->char_dev.device.driver_list, &tty_list);
    }
    
    /* Register the character device */
    int result = char_register_device(&tty->char_dev);
    
    if (result < 0) {
        /* Remove the TTY from the list */
        list_del(&tty->char_dev.device.driver_list);
        
        if (last != NULL) {
            last->next = NULL;
        }
        
        return result;
    }
    
    /* Register the input handler */
    result = input_register_handler(&tty->input_handler);
    
    if (result < 0) {
        /* Unregister the character device */
        char_unregister_device(&tty->char_dev);
        
        /* Remove the TTY from the list */
        list_del(&tty->char_dev.device.driver_list);
        
        if (last != NULL) {
            last->next = NULL;
        }
        
        return result;
    }
    
    return 0;
}

/* Unregister a TTY device */
int tty_unregister(tty_t *tty) {
    if (tty == NULL) {
        return -1;
    }
    
    /* Unregister the input handler */
    input_unregister_handler(&tty->input_handler);
    
    /* Unregister the character device */
    char_unregister_device(&tty->char_dev);
    
    /* Remove the TTY from the list */
    list_del(&tty->char_dev.device.driver_list);
    
    /* Update the linked list */
    tty_t *prev = NULL;
    tty_t *current = list_entry(tty_list.next, tty_t, char_dev.device.driver_list);
    
    while (current != NULL) {
        if (current == tty) {
            if (prev == NULL) {
                /* First TTY in the list */
                list_entry(tty_list.next, tty_t, char_dev.device.driver_list) = current->next;
            } else {
                /* Not the first TTY in the list */
                prev->next = current->next;
            }
            
            break;
        }
        
        prev = current;
        current = current->next;
    }
    
    return 0;
}

/* Get a TTY device by name */
tty_t *tty_get(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    
    /* Iterate over the TTY list */
    list_head_t *pos;
    
    list_for_each(pos, &tty_list) {
        tty_t *tty = list_entry(pos, tty_t, char_dev.device.driver_list);
        
        if (strcmp(tty->name, name) == 0) {
            return tty;
        }
    }
    
    return NULL;
}

/* Open a TTY device */
int tty_open(tty_t *tty, u32 flags) {
    if (tty == NULL) {
        return -1;
    }
    
    /* Set the TTY flags */
    tty->flags = flags;
    
    /* Initialize the buffers */
    tty->input_head = 0;
    tty->input_tail = 0;
    tty->output_head = 0;
    tty->output_tail = 0;
    
    return 0;
}

/* Close a TTY device */
int tty_close(tty_t *tty) {
    if (tty == NULL) {
        return -1;
    }
    
    /* Flush the buffers */
    tty_flush_input(tty);
    tty_flush_output(tty);
    
    return 0;
}

/* Read from a TTY device */
ssize_t tty_read(tty_t *tty, void *buf, size_t count) {
    if (tty == NULL || buf == NULL) {
        return -1;
    }
    
    /* Check if the input buffer is empty */
    if (tty->input_head == tty->input_tail) {
        return 0;
    }
    
    /* Read from the input buffer */
    size_t bytes_read = 0;
    char *cbuf = (char *)buf;
    
    while (bytes_read < count && tty->input_head != tty->input_tail) {
        cbuf[bytes_read++] = tty->input_buffer[tty->input_tail++];
        
        if (tty->input_tail >= TTY_BUFFER_SIZE) {
            tty->input_tail = 0;
        }
    }
    
    return bytes_read;
}

/* Write to a TTY device */
ssize_t tty_write(tty_t *tty, const void *buf, size_t count) {
    if (tty == NULL || buf == NULL) {
        return -1;
    }
    
    /* Write to the output buffer */
    size_t bytes_written = 0;
    const char *cbuf = (const char *)buf;
    
    while (bytes_written < count) {
        /* Check if the output buffer is full */
        u32 next_head = (tty->output_head + 1) % TTY_BUFFER_SIZE;
        
        if (next_head == tty->output_tail) {
            /* Output buffer is full */
            break;
        }
        
        /* Add the character to the output buffer */
        tty->output_buffer[tty->output_head] = cbuf[bytes_written++];
        tty->output_head = next_head;
        
        /* Write the character to the device */
        if (tty->write_char != NULL) {
            tty->write_char(tty, cbuf[bytes_written - 1]);
        }
    }
    
    return bytes_written;
}

/* Perform an I/O control operation on a TTY device */
int tty_ioctl(tty_t *tty, u32 request, void *arg) {
    if (tty == NULL) {
        return -1;
    }
    
    /* Handle the request */
    switch (request) {
        case 0: /* Get flags */
            if (arg != NULL) {
                *(u32 *)arg = tty->flags;
            }
            break;
        
        case 1: /* Set flags */
            if (arg != NULL) {
                tty->flags = *(u32 *)arg;
            }
            break;
        
        default:
            return -1;
    }
    
    return 0;
}

/* Add a character to the TTY input buffer */
int tty_input(tty_t *tty, char c) {
    if (tty == NULL) {
        return -1;
    }
    
    /* Check if the input buffer is full */
    u32 next_head = (tty->input_head + 1) % TTY_BUFFER_SIZE;
    
    if (next_head == tty->input_tail) {
        /* Input buffer is full */
        return -1;
    }
    
    /* Add the character to the input buffer */
    tty->input_buffer[tty->input_head] = c;
    tty->input_head = next_head;
    
    /* Echo the character if enabled */
    if (tty->flags & TTY_FLAG_ECHO) {
        tty_output(tty, c);
    }
    
    return 0;
}

/* Add a character to the TTY output buffer */
int tty_output(tty_t *tty, char c) {
    if (tty == NULL) {
        return -1;
    }
    
    /* Check if the output buffer is full */
    u32 next_head = (tty->output_head + 1) % TTY_BUFFER_SIZE;
    
    if (next_head == tty->output_tail) {
        /* Output buffer is full */
        return -1;
    }
    
    /* Add the character to the output buffer */
    tty->output_buffer[tty->output_head] = c;
    tty->output_head = next_head;
    
    /* Write the character to the device */
    if (tty->write_char != NULL) {
        tty->write_char(tty, c);
    }
    
    return 0;
}

/* Flush the TTY input buffer */
int tty_flush_input(tty_t *tty) {
    if (tty == NULL) {
        return -1;
    }
    
    /* Reset the input buffer */
    tty->input_head = 0;
    tty->input_tail = 0;
    
    return 0;
}

/* Flush the TTY output buffer */
int tty_flush_output(tty_t *tty) {
    if (tty == NULL) {
        return -1;
    }
    
    /* Reset the output buffer */
    tty->output_head = 0;
    tty->output_tail = 0;
    
    return 0;
}

/* Character device operations */

/* Open a TTY character device */
static int tty_char_open(char_device_t *dev, u32 flags) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Get the TTY device */
    tty_t *tty = (tty_t *)dev->private_data;
    
    if (tty == NULL) {
        return -1;
    }
    
    /* Open the TTY device */
    return tty_open(tty, flags);
}

/* Close a TTY character device */
static int tty_char_close(char_device_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Get the TTY device */
    tty_t *tty = (tty_t *)dev->private_data;
    
    if (tty == NULL) {
        return -1;
    }
    
    /* Close the TTY device */
    return tty_close(tty);
}

/* Read from a TTY character device */
static ssize_t tty_char_read(char_device_t *dev, void *buf, size_t count) {
    if (dev == NULL || buf == NULL) {
        return -1;
    }
    
    /* Get the TTY device */
    tty_t *tty = (tty_t *)dev->private_data;
    
    if (tty == NULL) {
        return -1;
    }
    
    /* Read from the TTY device */
    return tty_read(tty, buf, count);
}

/* Write to a TTY character device */
static ssize_t tty_char_write(char_device_t *dev, const void *buf, size_t count) {
    if (dev == NULL || buf == NULL) {
        return -1;
    }
    
    /* Get the TTY device */
    tty_t *tty = (tty_t *)dev->private_data;
    
    if (tty == NULL) {
        return -1;
    }
    
    /* Write to the TTY device */
    return tty_write(tty, buf, count);
}

/* Perform an I/O control operation on a TTY character device */
static int tty_char_ioctl(char_device_t *dev, u32 request, void *arg) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Get the TTY device */
    tty_t *tty = (tty_t *)dev->private_data;
    
    if (tty == NULL) {
        return -1;
    }
    
    /* Perform the I/O control operation */
    return tty_ioctl(tty, request, arg);
}

/* Flush a TTY character device */
static int tty_char_flush(char_device_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Get the TTY device */
    tty_t *tty = (tty_t *)dev->private_data;
    
    if (tty == NULL) {
        return -1;
    }
    
    /* Flush the TTY device */
    int result = tty_flush_input(tty);
    
    if (result < 0) {
        return result;
    }
    
    return tty_flush_output(tty);
}

/* Input handler operations */

/* Connect an input device to a TTY input handler */
static int tty_input_connect(input_handler_t *handler, input_device_t *dev) {
    if (handler == NULL || dev == NULL) {
        return -1;
    }
    
    /* Check if the device supports key events */
    if (!(dev->event_types & (1 << EV_KEY))) {
        return -1;
    }
    
    /* Get the TTY device */
    tty_t *tty = NULL;
    list_head_t *pos;
    
    list_for_each(pos, &tty_list) {
        tty_t *t = list_entry(pos, tty_t, char_dev.device.driver_list);
        
        if (&t->input_handler == handler) {
            tty = t;
            break;
        }
    }
    
    if (tty == NULL) {
        return -1;
    }
    
    /* Open the input device */
    return input_open(dev);
}

/* Disconnect an input device from a TTY input handler */
static void tty_input_disconnect(input_handler_t *handler, input_device_t *dev) {
    if (handler == NULL || dev == NULL) {
        return;
    }
    
    /* Close the input device */
    input_close(dev);
}

/* Handle an input event for a TTY input handler */
static int tty_input_event(input_handler_t *handler, input_device_t *dev, input_event_t *event) {
    if (handler == NULL || dev == NULL || event == NULL) {
        return -1;
    }
    
    /* Check if the event is a key event */
    if (event->type != EV_KEY) {
        return 0;
    }
    
    /* Get the TTY device */
    tty_t *tty = NULL;
    list_head_t *pos;
    
    list_for_each(pos, &tty_list) {
        tty_t *t = list_entry(pos, tty_t, char_dev.device.driver_list);
        
        if (&t->input_handler == handler) {
            tty = t;
            break;
        }
    }
    
    if (tty == NULL) {
        return -1;
    }
    
    /* Handle the key event */
    if (event->value == 1) {
        /* Key press */
        char c = 0;
        
        /* Convert the key code to a character */
        switch (event->code) {
            case KEY_A: c = 'a'; break;
            case KEY_B: c = 'b'; break;
            case KEY_C: c = 'c'; break;
            case KEY_D: c = 'd'; break;
            case KEY_E: c = 'e'; break;
            case KEY_F: c = 'f'; break;
            case KEY_G: c = 'g'; break;
            case KEY_H: c = 'h'; break;
            case KEY_I: c = 'i'; break;
            case KEY_J: c = 'j'; break;
            case KEY_K: c = 'k'; break;
            case KEY_L: c = 'l'; break;
            case KEY_M: c = 'm'; break;
            case KEY_N: c = 'n'; break;
            case KEY_O: c = 'o'; break;
            case KEY_P: c = 'p'; break;
            case KEY_Q: c = 'q'; break;
            case KEY_R: c = 'r'; break;
            case KEY_S: c = 's'; break;
            case KEY_T: c = 't'; break;
            case KEY_U: c = 'u'; break;
            case KEY_V: c = 'v'; break;
            case KEY_W: c = 'w'; break;
            case KEY_X: c = 'x'; break;
            case KEY_Y: c = 'y'; break;
            case KEY_Z: c = 'z'; break;
            case KEY_0: c = '0'; break;
            case KEY_1: c = '1'; break;
            case KEY_2: c = '2'; break;
            case KEY_3: c = '3'; break;
            case KEY_4: c = '4'; break;
            case KEY_5: c = '5'; break;
            case KEY_6: c = '6'; break;
            case KEY_7: c = '7'; break;
            case KEY_8: c = '8'; break;
            case KEY_9: c = '9'; break;
            case KEY_SPACE: c = ' '; break;
            case KEY_MINUS: c = '-'; break;
            case KEY_EQUAL: c = '='; break;
            case KEY_LEFTBRACE: c = '['; break;
            case KEY_RIGHTBRACE: c = ']'; break;
            case KEY_SEMICOLON: c = ';'; break;
            case KEY_APOSTROPHE: c = '\''; break;
            case KEY_GRAVE: c = '`'; break;
            case KEY_BACKSLASH: c = '\\'; break;
            case KEY_COMMA: c = ','; break;
            case KEY_DOT: c = '.'; break;
            case KEY_SLASH: c = '/'; break;
            case KEY_ENTER: c = '\n'; break;
            case KEY_BACKSPACE: c = '\b'; break;
            case KEY_TAB: c = '\t'; break;
            case KEY_ESC: c = 27; break;
            default: c = 0; break;
        }
        
        /* Add the character to the input buffer */
        if (c != 0) {
            tty_input(tty, c);
        }
    }
    
    return 0;
}

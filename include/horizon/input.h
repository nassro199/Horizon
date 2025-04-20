/**
 * input.h - Horizon kernel input subsystem definitions
 *
 * This file contains definitions for the input subsystem.
 */

#ifndef _KERNEL_INPUT_H
#define _KERNEL_INPUT_H

#include <horizon/types.h>
#include <horizon/device.h>

/* Input event types */
#define EV_KEY      0x01    /* Key event */
#define EV_REL      0x02    /* Relative movement event */
#define EV_ABS      0x03    /* Absolute movement event */
#define EV_MSC      0x04    /* Miscellaneous event */
#define EV_SW       0x05    /* Switch event */
#define EV_LED      0x11    /* LED event */
#define EV_SND      0x12    /* Sound event */
#define EV_REP      0x14    /* Repeat event */
#define EV_FF       0x15    /* Force feedback event */
#define EV_PWR      0x16    /* Power event */
#define EV_FF_STATUS 0x17   /* Force feedback status */
#define EV_MAX      0x1F    /* Maximum event type */

/* Input event structure */
typedef struct input_event {
    u32 time;       /* Event timestamp */
    u16 type;       /* Event type */
    u16 code;       /* Event code */
    s32 value;      /* Event value */
} input_event_t;

/* Key codes */
#define KEY_RESERVED    0
#define KEY_ESC         1
#define KEY_1           2
#define KEY_2           3
#define KEY_3           4
#define KEY_4           5
#define KEY_5           6
#define KEY_6           7
#define KEY_7           8
#define KEY_8           9
#define KEY_9           10
#define KEY_0           11
#define KEY_MINUS       12
#define KEY_EQUAL       13
#define KEY_BACKSPACE   14
#define KEY_TAB         15
#define KEY_Q           16
#define KEY_W           17
#define KEY_E           18
#define KEY_R           19
#define KEY_T           20
#define KEY_Y           21
#define KEY_U           22
#define KEY_I           23
#define KEY_O           24
#define KEY_P           25
#define KEY_LEFTBRACE   26
#define KEY_RIGHTBRACE  27
#define KEY_ENTER       28
#define KEY_LEFTCTRL    29
#define KEY_A           30
#define KEY_S           31
#define KEY_D           32
#define KEY_F           33
#define KEY_G           34
#define KEY_H           35
#define KEY_J           36
#define KEY_K           37
#define KEY_L           38
#define KEY_SEMICOLON   39
#define KEY_APOSTROPHE  40
#define KEY_GRAVE       41
#define KEY_LEFTSHIFT   42
#define KEY_BACKSLASH   43
#define KEY_Z           44
#define KEY_X           45
#define KEY_C           46
#define KEY_V           47
#define KEY_B           48
#define KEY_N           49
#define KEY_M           50
#define KEY_COMMA       51
#define KEY_DOT         52
#define KEY_SLASH       53
#define KEY_RIGHTSHIFT  54
#define KEY_KPASTERISK  55
#define KEY_LEFTALT     56
#define KEY_SPACE       57
#define KEY_CAPSLOCK    58
#define KEY_F1          59
#define KEY_F2          60
#define KEY_F3          61
#define KEY_F4          62
#define KEY_F5          63
#define KEY_F6          64
#define KEY_F7          65
#define KEY_F8          66
#define KEY_F9          67
#define KEY_F10         68
#define KEY_NUMLOCK     69
#define KEY_SCROLLLOCK  70
#define KEY_KP7         71
#define KEY_KP8         72
#define KEY_KP9         73
#define KEY_KPMINUS     74
#define KEY_KP4         75
#define KEY_KP5         76
#define KEY_KP6         77
#define KEY_KPPLUS      78
#define KEY_KP1         79
#define KEY_KP2         80
#define KEY_KP3         81
#define KEY_KP0         82
#define KEY_KPDOT       83
#define KEY_F11         87
#define KEY_F12         88
#define KEY_KPENTER     96
#define KEY_RIGHTCTRL   97
#define KEY_KPSLASH     98
#define KEY_RIGHTALT    100
#define KEY_HOME        102
#define KEY_UP          103
#define KEY_PAGEUP      104
#define KEY_LEFT        105
#define KEY_RIGHT       106
#define KEY_END         107
#define KEY_DOWN        108
#define KEY_PAGEDOWN    109
#define KEY_INSERT      110
#define KEY_DELETE      111
#define KEY_PAUSE       119
#define KEY_MAX         127

/* Input device structure */
typedef struct input_device {
    char name[64];              /* Device name */
    u32 event_types;            /* Supported event types */
    u32 key_bits[KEY_MAX / 32 + 1]; /* Supported keys */
    device_t dev;               /* Device structure */
    int (*open)(struct input_device *dev); /* Open function */
    int (*close)(struct input_device *dev); /* Close function */
    int (*flush)(struct input_device *dev); /* Flush function */
    int (*event)(struct input_device *dev, input_event_t *event); /* Event function */
} input_device_t;

/* Input handler structure */
typedef struct input_handler {
    char name[64];              /* Handler name */
    u32 event_types;            /* Handled event types */
    u32 key_bits[KEY_MAX / 32 + 1]; /* Handled keys */
    int (*connect)(struct input_handler *handler, input_device_t *dev); /* Connect function */
    void (*disconnect)(struct input_handler *handler, input_device_t *dev); /* Disconnect function */
    int (*event)(struct input_handler *handler, input_device_t *dev, input_event_t *event); /* Event function */
    struct input_handler *next; /* Next handler */
} input_handler_t;

/* Input functions */
void input_init(void);
int input_register_device(input_device_t *dev);
int input_unregister_device(input_device_t *dev);
int input_register_handler(input_handler_t *handler);
int input_unregister_handler(input_handler_t *handler);
int input_event(input_device_t *dev, u16 type, u16 code, s32 value);
int input_open(input_device_t *dev);
int input_close(input_device_t *dev);
int input_flush(input_device_t *dev);

#endif /* _KERNEL_INPUT_H */

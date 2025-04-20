/**
 * tty.h - Horizon kernel TTY subsystem definitions
 * 
 * This file contains definitions for the TTY subsystem.
 */

#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <horizon/types.h>
#include <horizon/char.h>
#include <horizon/input.h>

/* TTY flags */
#define TTY_FLAG_ECHO       0x0001  /* Echo input */
#define TTY_FLAG_ICANON     0x0002  /* Canonical mode */
#define TTY_FLAG_ISIG       0x0004  /* Enable signals */
#define TTY_FLAG_IEXTEN     0x0008  /* Enable extended input processing */
#define TTY_FLAG_OPOST      0x0010  /* Enable output processing */
#define TTY_FLAG_ONLCR      0x0020  /* Map NL to CR-NL on output */
#define TTY_FLAG_OCRNL      0x0040  /* Map CR to NL on output */
#define TTY_FLAG_ONOCR      0x0080  /* No CR output at column 0 */
#define TTY_FLAG_ONLRET     0x0100  /* NL performs CR function */
#define TTY_FLAG_OFILL      0x0200  /* Use fill characters for delay */
#define TTY_FLAG_OFDEL      0x0400  /* Fill is DEL, else NUL */
#define TTY_FLAG_NLDLY      0x0800  /* NL delay */
#define TTY_FLAG_CRDLY      0x1000  /* CR delay */
#define TTY_FLAG_TABDLY     0x2000  /* Horizontal tab delay */
#define TTY_FLAG_BSDLY      0x4000  /* Backspace delay */
#define TTY_FLAG_VTDLY      0x8000  /* Vertical tab delay */
#define TTY_FLAG_FFDLY      0x10000 /* Form feed delay */

/* TTY buffer size */
#define TTY_BUFFER_SIZE     1024

/* TTY structure */
typedef struct tty {
    char name[64];                  /* TTY name */
    u32 flags;                      /* TTY flags */
    char input_buffer[TTY_BUFFER_SIZE]; /* Input buffer */
    u32 input_head;                 /* Input buffer head */
    u32 input_tail;                 /* Input buffer tail */
    char output_buffer[TTY_BUFFER_SIZE]; /* Output buffer */
    u32 output_head;                /* Output buffer head */
    u32 output_tail;                /* Output buffer tail */
    char_device_t char_dev;         /* Character device */
    input_handler_t input_handler;  /* Input handler */
    void (*write_char)(struct tty *tty, char c); /* Write character function */
    void *private_data;             /* Private data */
    struct tty *next;               /* Next TTY in list */
} tty_t;

/* TTY functions */
void tty_init(void);
int tty_register(tty_t *tty);
int tty_unregister(tty_t *tty);
tty_t *tty_get(const char *name);
int tty_open(tty_t *tty, u32 flags);
int tty_close(tty_t *tty);
ssize_t tty_read(tty_t *tty, void *buf, size_t count);
ssize_t tty_write(tty_t *tty, const void *buf, size_t count);
int tty_ioctl(tty_t *tty, u32 request, void *arg);
int tty_input(tty_t *tty, char c);
int tty_output(tty_t *tty, char c);
int tty_flush_input(tty_t *tty);
int tty_flush_output(tty_t *tty);

#endif /* _KERNEL_TTY_H */

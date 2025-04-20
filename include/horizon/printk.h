/**
 * printk.h - Horizon kernel printing definitions
 * 
 * This file contains definitions for kernel printing.
 */

#ifndef _HORIZON_PRINTK_H
#define _HORIZON_PRINTK_H

#include <horizon/stdarg.h>

/* Kernel printing */
int printk(const char *fmt, ...);
int vprintk(const char *fmt, va_list args);

/* Log levels */
#define KERN_EMERG   "<0>"   /* System is unusable */
#define KERN_ALERT   "<1>"   /* Action must be taken immediately */
#define KERN_CRIT    "<2>"   /* Critical conditions */
#define KERN_ERR     "<3>"   /* Error conditions */
#define KERN_WARNING "<4>"   /* Warning conditions */
#define KERN_NOTICE  "<5>"   /* Normal but significant condition */
#define KERN_INFO    "<6>"   /* Informational */
#define KERN_DEBUG   "<7>"   /* Debug-level messages */
#define KERN_DEFAULT "<d>"   /* Default level */
#define KERN_CONT    "<c>"   /* Continuation of previous message */

#endif /* _HORIZON_PRINTK_H */

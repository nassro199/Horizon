/**
 * stdarg.h - Horizon kernel variable arguments definitions
 * 
 * This file contains definitions for variable arguments.
 */

#ifndef _HORIZON_STDARG_H
#define _HORIZON_STDARG_H

/* Variable arguments */
typedef __builtin_va_list va_list;

#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v, l)
#define va_copy(d, s) __builtin_va_copy(d, s)

#endif /* _HORIZON_STDARG_H */

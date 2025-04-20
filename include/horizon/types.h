/**
 * types.h - Common type definitions
 * 
 * This file contains common type definitions used throughout the kernel.
 */

#ifndef _TYPES_H
#define _TYPES_H

/* Basic types */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;

typedef u32                size_t;
typedef s32                ssize_t;

/* Boolean type */
typedef enum {
    false = 0,
    true = 1
} bool;

/* Error codes */
typedef enum {
    SUCCESS = 0,
    ERROR_GENERAL = -1,
    ERROR_NOMEM = -2,
    ERROR_IO = -3,
    ERROR_INVAL = -4,
    ERROR_NOENT = -5
} error_t;

#endif /* _TYPES_H */

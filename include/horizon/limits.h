/**
 * limits.h - Limits of integer types
 *
 * This file contains the limits of integer types.
 */

#ifndef _HORIZON_LIMITS_H
#define _HORIZON_LIMITS_H

/* Limits of integer types */
#define CHAR_BIT    8       /* Number of bits in a char */
#define SCHAR_MIN   (-128)  /* Minimum value of a signed char */
#define SCHAR_MAX   127     /* Maximum value of a signed char */
#define UCHAR_MAX   255     /* Maximum value of an unsigned char */
#define CHAR_MIN    SCHAR_MIN /* Minimum value of a char */
#define CHAR_MAX    SCHAR_MAX /* Maximum value of a char */
#define SHRT_MIN    (-32768) /* Minimum value of a short */
#define SHRT_MAX    32767   /* Maximum value of a short */
#define USHRT_MAX   65535   /* Maximum value of an unsigned short */
#define INT_MIN     (-2147483648) /* Minimum value of an int */
#define INT_MAX     2147483647 /* Maximum value of an int */
#define UINT_MAX    4294967295U /* Maximum value of an unsigned int */
#define LONG_MIN    (-2147483648L) /* Minimum value of a long */
#define LONG_MAX    2147483647L /* Maximum value of a long */
#define ULONG_MAX   4294967295UL /* Maximum value of an unsigned long */
#define LLONG_MIN   (-9223372036854775808LL) /* Minimum value of a long long */
#define LLONG_MAX   9223372036854775807LL /* Maximum value of a long long */
#define ULLONG_MAX  18446744073709551615ULL /* Maximum value of an unsigned long long */

#endif /* _HORIZON_LIMITS_H */

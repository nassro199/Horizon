/**
 * string.h - String manipulation functions
 * 
 * This file contains declarations for string manipulation functions.
 */

#ifndef _STRING_H
#define _STRING_H

#include <horizon/types.h>

/* String functions */
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);
char *strchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);

/* Memory functions */
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memset(void *s, int c, size_t n);

#endif /* _STRING_H */

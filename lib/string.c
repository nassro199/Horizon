/**
 * string.c - String manipulation functions
 * 
 * This file contains string manipulation functions.
 */

#include <horizon/types.h>

/* Copy a string */
char *strcpy(char *dest, const char *src) {
    char *d = dest;
    
    while ((*d++ = *src++) != '\0') {
        /* Empty loop body */
    }
    
    return dest;
}

/* Copy a string with a maximum length */
char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    
    while (n > 0 && (*d++ = *src++) != '\0') {
        n--;
    }
    
    while (n > 0) {
        *d++ = '\0';
        n--;
    }
    
    return dest;
}

/* Concatenate two strings */
char *strcat(char *dest, const char *src) {
    char *d = dest;
    
    /* Find the end of the destination string */
    while (*d != '\0') {
        d++;
    }
    
    /* Copy the source string */
    while ((*d++ = *src++) != '\0') {
        /* Empty loop body */
    }
    
    return dest;
}

/* Concatenate two strings with a maximum length */
char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    
    /* Find the end of the destination string */
    while (*d != '\0') {
        d++;
    }
    
    /* Copy the source string */
    while (n > 0 && (*d = *src) != '\0') {
        d++;
        src++;
        n--;
    }
    
    *d = '\0';
    
    return dest;
}

/* Compare two strings */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    
    return *s1 - *s2;
}

/* Compare two strings with a maximum length */
int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0 && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    
    if (n == 0) {
        return 0;
    }
    
    return *s1 - *s2;
}

/* Get the length of a string */
size_t strlen(const char *s) {
    const char *p = s;
    
    while (*p != '\0') {
        p++;
    }
    
    return p - s;
}

/* Find a character in a string */
char *strchr(const char *s, int c) {
    while (*s != '\0') {
        if (*s == c) {
            return (char *)s;
        }
        s++;
    }
    
    if (c == '\0') {
        return (char *)s;
    }
    
    return NULL;
}

/* Find a substring in a string */
char *strstr(const char *haystack, const char *needle) {
    size_t needle_len = strlen(needle);
    
    if (needle_len == 0) {
        return (char *)haystack;
    }
    
    while (*haystack != '\0') {
        if (*haystack == *needle && strncmp(haystack, needle, needle_len) == 0) {
            return (char *)haystack;
        }
        haystack++;
    }
    
    return NULL;
}

/* Copy memory */
void *memcpy(void *dest, const void *src, size_t n) {
    u8 *d = (u8 *)dest;
    const u8 *s = (const u8 *)src;
    
    while (n > 0) {
        *d++ = *s++;
        n--;
    }
    
    return dest;
}

/* Move memory */
void *memmove(void *dest, const void *src, size_t n) {
    u8 *d = (u8 *)dest;
    const u8 *s = (const u8 *)src;
    
    if (d < s) {
        /* Copy from start to end */
        while (n > 0) {
            *d++ = *s++;
            n--;
        }
    } else if (d > s) {
        /* Copy from end to start */
        d += n - 1;
        s += n - 1;
        
        while (n > 0) {
            *d-- = *s--;
            n--;
        }
    }
    
    return dest;
}

/* Compare memory */
int memcmp(const void *s1, const void *s2, size_t n) {
    const u8 *p1 = (const u8 *)s1;
    const u8 *p2 = (const u8 *)s2;
    
    while (n > 0) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
        n--;
    }
    
    return 0;
}

/* Set memory */
void *memset(void *s, int c, size_t n) {
    u8 *p = (u8 *)s;
    
    while (n > 0) {
        *p++ = c;
        n--;
    }
    
    return s;
}

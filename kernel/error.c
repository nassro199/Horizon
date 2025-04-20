/**
 * error.c - Horizon kernel error handling
 * 
 * This file contains the implementation of the error handling subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/error.h>
#include <horizon/string.h>
#include <horizon/stdarg.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Per-CPU error context */
__thread error_context_t *current_error_context = NULL;

/* Error message buffer */
#define ERROR_MESSAGE_SIZE 256
static char error_message[ERROR_MESSAGE_SIZE];

/**
 * Set error message
 * 
 * @param fmt Format string
 * @param ... Arguments
 */
void error_set_message(const char *fmt, ...) {
    va_list args;
    
    /* Check parameters */
    if (fmt == NULL) {
        error_message[0] = '\0';
        return;
    }
    
    /* Format message */
    va_start(args, fmt);
    vsnprintf(error_message, ERROR_MESSAGE_SIZE, fmt, args);
    va_end(args);
}

/**
 * Get error message
 * 
 * @return Error message
 */
const char *error_get_message(void) {
    return error_message;
}

/**
 * Clear error message
 */
void error_clear_message(void) {
    error_message[0] = '\0';
}

/**
 * Print error message
 * 
 * @param prefix Prefix string
 * @param error Error code
 */
void error_print(const char *prefix, int error) {
    /* Check parameters */
    if (prefix == NULL) {
        prefix = "Error";
    }
    
    /* Print error message */
    if (error_message[0] != '\0') {
        printk("%s: %s (%d)\n", prefix, error_message, error);
    } else {
        printk("%s: %s (%d)\n", prefix, strerror(error), error);
    }
    
    /* Clear error message */
    error_clear_message();
}

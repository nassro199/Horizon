/**
 * error.h - Horizon kernel error handling definitions
 * 
 * This file contains definitions for the error handling subsystem.
 */

#ifndef _HORIZON_ERROR_H
#define _HORIZON_ERROR_H

#include <horizon/types.h>

/* Error recovery callback */
typedef int (*error_recovery_t)(void *data, int error);

/* Error recovery context */
typedef struct error_context {
    error_recovery_t recovery;     /* Recovery function */
    void *data;                    /* Recovery data */
    struct error_context *prev;    /* Previous context */
} error_context_t;

/* Per-CPU error context */
extern __thread error_context_t *current_error_context;

/* Push error recovery context */
static inline void error_recovery_push(error_context_t *ctx, error_recovery_t recovery, void *data) {
    ctx->recovery = recovery;
    ctx->data = data;
    ctx->prev = current_error_context;
    current_error_context = ctx;
}

/* Pop error recovery context */
static inline void error_recovery_pop(error_context_t *ctx) {
    if (current_error_context == ctx) {
        current_error_context = ctx->prev;
    }
}

/* Recover from error */
static inline int error_recover(int error) {
    if (current_error_context != NULL && current_error_context->recovery != NULL) {
        return current_error_context->recovery(current_error_context->data, error);
    }
    return error;
}

/* Error recovery macro */
#define ERROR_RECOVERY(ctx, recovery, data, code) \
    do { \
        error_context_t ctx; \
        error_recovery_push(&ctx, recovery, data); \
        code; \
        error_recovery_pop(&ctx); \
    } while (0)

/* Error message functions */
void error_set_message(const char *fmt, ...);
const char *error_get_message(void);
void error_clear_message(void);
void error_print(const char *prefix, int error);

#endif /* _HORIZON_ERROR_H */

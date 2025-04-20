/**
 * completion.h - Horizon kernel completion definitions
 * 
 * This file contains definitions for completions.
 */

#ifndef _HORIZON_COMPLETION_H
#define _HORIZON_COMPLETION_H

#include <horizon/types.h>
#include <horizon/wait.h>

/* Completion structure */
typedef struct completion {
    unsigned int done;             /* Completion flag */
    wait_queue_head_t wait;        /* Wait queue */
} completion_t;

/* Initialize a completion */
#define COMPLETION_INITIALIZER(done) { (done), WAIT_QUEUE_HEAD_INITIALIZER }
#define COMPLETION_INITIALIZER_ONSTACK(done) { (done), WAIT_QUEUE_HEAD_INITIALIZER_ONSTACK }

/* Initialize a completion */
static inline void completion_init(struct completion *comp) {
    comp->done = 0;
    wait_queue_init(&comp->wait);
}

/* Mark a completion as done */
static inline void completion_complete(struct completion *comp) {
    comp->done++;
    wake_up_all(&comp->wait);
}

/* Mark a completion as done (all waiters) */
static inline void completion_complete_all(struct completion *comp) {
    comp->done = UINT_MAX;
    wake_up_all(&comp->wait);
}

/* Wait for a completion */
void completion_wait(struct completion *comp);

/* Wait for a completion with timeout */
int completion_wait_timeout(struct completion *comp, unsigned long timeout);

/* Try to wait for a completion */
int completion_wait_interruptible(struct completion *comp);

/* Try to wait for a completion with timeout */
int completion_wait_interruptible_timeout(struct completion *comp, unsigned long timeout);

#endif /* _HORIZON_COMPLETION_H */

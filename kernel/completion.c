/**
 * completion.c - Horizon kernel completion implementation
 * 
 * This file contains the implementation of completions.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/completion.h>
#include <horizon/wait.h>
#include <horizon/sched.h>

/* Maximum value for unsigned int */
#define UINT_MAX 0xFFFFFFFF

/**
 * Wait for a completion
 * 
 * @param comp Completion to wait for
 */
void completion_wait(struct completion *comp) {
    wait_event(comp->wait, comp->done > 0);
    
    /* Decrement completion count */
    spin_lock(&comp->wait.lock);
    comp->done--;
    spin_unlock(&comp->wait.lock);
}

/**
 * Wait for a completion with timeout
 * 
 * @param comp Completion to wait for
 * @param timeout Timeout in jiffies
 * @return Remaining jiffies, or 0 if timed out
 */
int completion_wait_timeout(struct completion *comp, unsigned long timeout) {
    int ret;
    
    ret = wait_event_timeout(comp->wait, comp->done > 0, timeout);
    if (ret == 0) {
        /* Timed out */
        return 0;
    }
    
    /* Decrement completion count */
    spin_lock(&comp->wait.lock);
    comp->done--;
    spin_unlock(&comp->wait.lock);
    
    return ret;
}

/**
 * Try to wait for a completion
 * 
 * @param comp Completion to wait for
 * @return 0 if completed, -ERESTARTSYS if interrupted
 */
int completion_wait_interruptible(struct completion *comp) {
    int ret;
    
    ret = wait_event_interruptible(comp->wait, comp->done > 0);
    if (ret != 0) {
        /* Interrupted */
        return ret;
    }
    
    /* Decrement completion count */
    spin_lock(&comp->wait.lock);
    comp->done--;
    spin_unlock(&comp->wait.lock);
    
    return 0;
}

/**
 * Try to wait for a completion with timeout
 * 
 * @param comp Completion to wait for
 * @param timeout Timeout in jiffies
 * @return Remaining jiffies, 0 if timed out, or -ERESTARTSYS if interrupted
 */
int completion_wait_interruptible_timeout(struct completion *comp, unsigned long timeout) {
    int ret;
    
    ret = wait_event_interruptible_timeout(comp->wait, comp->done > 0, timeout);
    if (ret <= 0) {
        /* Timed out or interrupted */
        return ret;
    }
    
    /* Decrement completion count */
    spin_lock(&comp->wait.lock);
    comp->done--;
    spin_unlock(&comp->wait.lock);
    
    return ret;
}

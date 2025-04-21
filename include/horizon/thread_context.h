/**
 * thread_context.h - Horizon kernel thread context definitions
 *
 * This file contains definitions for thread context.
 */

#ifndef _HORIZON_THREAD_CONTEXT_H
#define _HORIZON_THREAD_CONTEXT_H

#include <horizon/types.h>

/* Thread context structure */
typedef struct thread_context {
    u32 eip;                /* Instruction pointer */
    u32 esp;                /* Stack pointer */
    u32 ebp;                /* Base pointer */
    u32 ebx;                /* General purpose registers */
    u32 esi;
    u32 edi;
    u32 eflags;             /* Flags */
} thread_context_t;

#endif /* _HORIZON_THREAD_CONTEXT_H */

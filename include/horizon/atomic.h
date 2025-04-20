/*
 * atomic.h - Horizon kernel atomic operations
 *
 * This file contains definitions for atomic operations.
 */

#ifndef _HORIZON_ATOMIC_H
#define _HORIZON_ATOMIC_H

#include <horizon/types.h>

/* Atomic type */
typedef struct {
    volatile int counter;
} atomic_t;

/* Atomic64 type */
typedef struct {
    volatile long counter;
} atomic64_t;

/* Atomic operations */
#define ATOMIC_INIT(i)  { (i) }
#define atomic_read(v)  ((v)->counter)
#define atomic_set(v,i) (((v)->counter) = (i))

static inline void atomic_add(int i, atomic_t *v)
{
    v->counter += i;
}

static inline void atomic_sub(int i, atomic_t *v)
{
    v->counter -= i;
}

static inline void atomic_inc(atomic_t *v)
{
    v->counter++;
}

static inline void atomic_dec(atomic_t *v)
{
    v->counter--;
}

static inline int atomic_add_return(int i, atomic_t *v)
{
    v->counter += i;
    return v->counter;
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
    v->counter -= i;
    return v->counter;
}

static inline int atomic_inc_return(atomic_t *v)
{
    return atomic_add_return(1, v);
}

static inline int atomic_dec_return(atomic_t *v)
{
    return atomic_sub_return(1, v);
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
    int prev = v->counter;
    if (prev == old)
        v->counter = new;
    return prev;
}

static inline int atomic_xchg(atomic_t *v, int new)
{
    int prev = v->counter;
    v->counter = new;
    return prev;
}

static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
    int c, old;
    c = atomic_read(v);
    while (c != u && (old = atomic_cmpxchg(v, c, c + a)) != c)
        c = old;
    return c != u;
}

#define atomic_inc_not_zero(v) atomic_add_unless((v), 1, 0)

/* Atomic64 operations */
#define ATOMIC64_INIT(i) { (i) }
#define atomic64_read(v) ((v)->counter)
#define atomic64_set(v,i) (((v)->counter) = (i))

static inline void atomic64_add(long i, atomic64_t *v)
{
    v->counter += i;
}

static inline void atomic64_sub(long i, atomic64_t *v)
{
    v->counter -= i;
}

static inline void atomic64_inc(atomic64_t *v)
{
    v->counter++;
}

static inline void atomic64_dec(atomic64_t *v)
{
    v->counter--;
}

static inline long atomic64_add_return(long i, atomic64_t *v)
{
    v->counter += i;
    return v->counter;
}

static inline long atomic64_sub_return(long i, atomic64_t *v)
{
    v->counter -= i;
    return v->counter;
}

static inline long atomic64_inc_return(atomic64_t *v)
{
    return atomic64_add_return(1, v);
}

static inline long atomic64_dec_return(atomic64_t *v)
{
    return atomic64_sub_return(1, v);
}

static inline long atomic64_cmpxchg(atomic64_t *v, long old, long new)
{
    long prev = v->counter;
    if (prev == old)
        v->counter = new;
    return prev;
}

static inline long atomic64_xchg(atomic64_t *v, long new)
{
    long prev = v->counter;
    v->counter = new;
    return prev;
}

static inline long atomic64_add_unless(atomic64_t *v, long a, long u)
{
    long c, old;
    c = atomic64_read(v);
    while (c != u && (old = atomic64_cmpxchg(v, c, c + a)) != c)
        c = old;
    return c != u;
}

#define atomic64_inc_not_zero(v) atomic64_add_unless((v), 1, 0)

#endif /* _HORIZON_ATOMIC_H */

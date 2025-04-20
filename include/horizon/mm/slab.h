/**
 * slab.h - Horizon kernel slab allocator definitions
 * 
 * This file contains definitions for the slab allocator.
 */

#ifndef _HORIZON_MM_SLAB_H
#define _HORIZON_MM_SLAB_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/spinlock.h>

/* Slab flags */
#define SLAB_HWCACHE_ALIGN  0x00000001  /* Align on hardware cache lines */
#define SLAB_CACHE_DMA      0x00000002  /* Use DMA memory */
#define SLAB_PANIC          0x00000004  /* Panic on failure */
#define SLAB_DESTROY_BY_RCU 0x00000008  /* Defer freeing slabs to RCU */
#define SLAB_POISON         0x00000010  /* Poison objects */
#define SLAB_RED_ZONE       0x00000020  /* Add red zone */
#define SLAB_NOLEAKTRACE    0x00000040  /* Don't trace leaks */
#define SLAB_FAILSLAB       0x00000080  /* Fail slab alloc rather than wait */
#define SLAB_ACCOUNT        0x00000100  /* Account to memcg */
#define SLAB_NOTRACK        0x00000200  /* Don't track in kmem cache */
#define SLAB_RECLAIM_ACCOUNT 0x00000400 /* Allow reclaim */
#define SLAB_TEMPORARY      0x00000800  /* Temporary cache */

/* Slab object */
typedef struct slab_object {
    struct slab_object *next;      /* Next free object */
} slab_object_t;

/* Slab */
typedef struct slab {
    struct list_head list;         /* List of slabs */
    void *start;                   /* Start of slab */
    unsigned int inuse;            /* Number of objects in use */
    unsigned int free;             /* Number of free objects */
    slab_object_t *freelist;       /* Free object list */
} slab_t;

/* Slab cache */
typedef struct slab_cache {
    const char *name;              /* Cache name */
    struct list_head list;         /* List of caches */
    size_t size;                   /* Object size */
    size_t align;                  /* Object alignment */
    unsigned int flags;            /* Cache flags */
    unsigned int num;              /* Number of objects per slab */
    unsigned int total_objects;    /* Total number of objects */
    unsigned int total_slabs;      /* Total number of slabs */
    spinlock_t lock;               /* Cache lock */
    struct list_head slabs_full;   /* Full slabs */
    struct list_head slabs_partial; /* Partially full slabs */
    struct list_head slabs_free;   /* Free slabs */
    void (*ctor)(void *);          /* Object constructor */
    void (*dtor)(void *);          /* Object destructor */
} slab_cache_t;

/* Slab functions */
void slab_init(void);
slab_cache_t *slab_cache_create(const char *name, size_t size, size_t align, unsigned int flags, void (*ctor)(void *), void (*dtor)(void *));
void slab_cache_destroy(slab_cache_t *cache);
void *slab_cache_alloc(slab_cache_t *cache, unsigned int flags);
void slab_cache_free(slab_cache_t *cache, void *obj);
int slab_cache_shrink(slab_cache_t *cache);
size_t slab_cache_size(slab_cache_t *cache);
const char *slab_cache_name(slab_cache_t *cache);
void slab_cache_info(slab_cache_t *cache, unsigned int *total_objects, unsigned int *total_slabs);

/* General-purpose caches */
extern slab_cache_t *kmalloc_caches[13];

/* Allocate memory */
void *kmalloc(size_t size, unsigned int flags);
void *kzalloc(size_t size, unsigned int flags);
void *kcalloc(size_t n, size_t size, unsigned int flags);
void *krealloc(void *ptr, size_t size, unsigned int flags);
void kfree(void *ptr);

#endif /* _HORIZON_MM_SLAB_H */

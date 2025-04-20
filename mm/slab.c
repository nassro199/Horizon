/**
 * slab.c - Horizon kernel slab allocator implementation
 * 
 * This file contains the implementation of the slab allocator.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/slab.h>
#include <horizon/spinlock.h>
#include <horizon/list.h>
#include <horizon/string.h>
#include <horizon/errno.h>

/* Cache list */
static LIST_HEAD(cache_list);

/* Cache list lock */
static spinlock_t cache_list_lock = SPIN_LOCK_INITIALIZER;

/* General-purpose caches */
slab_cache_t *kmalloc_caches[13];

/* Cache sizes */
static const size_t kmalloc_sizes[] = {
    32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072
};

/**
 * Initialize slab allocator
 */
void slab_init(void) {
    int i;
    char name[32];
    
    /* Create general-purpose caches */
    for (i = 0; i < 13; i++) {
        snprintf(name, sizeof(name), "kmalloc-%lu", kmalloc_sizes[i]);
        kmalloc_caches[i] = slab_cache_create(name, kmalloc_sizes[i], 0, SLAB_HWCACHE_ALIGN, NULL, NULL);
        if (kmalloc_caches[i] == NULL) {
            kernel_panic("Failed to create kmalloc cache");
        }
    }
}

/**
 * Create a slab cache
 * 
 * @param name Cache name
 * @param size Object size
 * @param align Object alignment
 * @param flags Cache flags
 * @param ctor Object constructor
 * @param dtor Object destructor
 * @return Cache, or NULL on failure
 */
slab_cache_t *slab_cache_create(const char *name, size_t size, size_t align, unsigned int flags, void (*ctor)(void *), void (*dtor)(void *)) {
    slab_cache_t *cache;
    
    /* Check parameters */
    if (name == NULL || size == 0) {
        return NULL;
    }
    
    /* Adjust size and alignment */
    if (size < sizeof(slab_object_t)) {
        size = sizeof(slab_object_t);
    }
    if (align == 0) {
        align = sizeof(void *);
    }
    if (align < sizeof(void *)) {
        align = sizeof(void *);
    }
    size = (size + align - 1) & ~(align - 1);
    
    /* Allocate cache */
    cache = mm_alloc_pages(1, MEM_KERNEL | MEM_ZERO);
    if (cache == NULL) {
        if (flags & SLAB_PANIC) {
            kernel_panic("Failed to allocate slab cache");
        }
        return NULL;
    }
    
    /* Initialize cache */
    cache->name = name;
    cache->size = size;
    cache->align = align;
    cache->flags = flags;
    cache->ctor = ctor;
    cache->dtor = dtor;
    
    /* Calculate number of objects per slab */
    cache->num = (PAGE_SIZE - sizeof(slab_t)) / size;
    if (cache->num == 0) {
        cache->num = 1;
    }
    
    /* Initialize lists */
    INIT_LIST_HEAD(&cache->slabs_full);
    INIT_LIST_HEAD(&cache->slabs_partial);
    INIT_LIST_HEAD(&cache->slabs_free);
    
    /* Initialize lock */
    spin_lock_init(&cache->lock, name);
    
    /* Add to cache list */
    spin_lock(&cache_list_lock);
    list_add(&cache->list, &cache_list);
    spin_unlock(&cache_list_lock);
    
    return cache;
}

/**
 * Destroy a slab cache
 * 
 * @param cache Cache to destroy
 */
void slab_cache_destroy(slab_cache_t *cache) {
    slab_t *slab, *next;
    
    /* Check parameters */
    if (cache == NULL) {
        return;
    }
    
    /* Remove from cache list */
    spin_lock(&cache_list_lock);
    list_del(&cache->list);
    spin_unlock(&cache_list_lock);
    
    /* Free all slabs */
    spin_lock(&cache->lock);
    
    /* Free full slabs */
    list_for_each_entry_safe(slab, next, &cache->slabs_full, list) {
        list_del(&slab->list);
        mm_free_pages(slab->start, 1);
    }
    
    /* Free partial slabs */
    list_for_each_entry_safe(slab, next, &cache->slabs_partial, list) {
        list_del(&slab->list);
        mm_free_pages(slab->start, 1);
    }
    
    /* Free free slabs */
    list_for_each_entry_safe(slab, next, &cache->slabs_free, list) {
        list_del(&slab->list);
        mm_free_pages(slab->start, 1);
    }
    
    spin_unlock(&cache->lock);
    
    /* Free cache */
    mm_free_pages(cache, 1);
}

/**
 * Allocate a new slab
 * 
 * @param cache Cache to allocate for
 * @return Slab, or NULL on failure
 */
static slab_t *slab_alloc(slab_cache_t *cache) {
    slab_t *slab;
    slab_object_t *obj;
    void *start;
    int i;
    
    /* Allocate slab */
    start = mm_alloc_pages(1, MEM_KERNEL | MEM_ZERO);
    if (start == NULL) {
        return NULL;
    }
    
    /* Initialize slab */
    slab = (slab_t *)start;
    slab->start = start;
    slab->inuse = 0;
    slab->free = cache->num;
    slab->freelist = NULL;
    
    /* Initialize objects */
    for (i = 0; i < cache->num; i++) {
        obj = (slab_object_t *)((char *)start + sizeof(slab_t) + i * cache->size);
        obj->next = slab->freelist;
        slab->freelist = obj;
        
        /* Call constructor */
        if (cache->ctor != NULL) {
            cache->ctor(obj);
        }
    }
    
    /* Add to free list */
    list_add(&slab->list, &cache->slabs_free);
    
    /* Update statistics */
    cache->total_slabs++;
    cache->total_objects += cache->num;
    
    return slab;
}

/**
 * Allocate an object from a slab cache
 * 
 * @param cache Cache to allocate from
 * @param flags Allocation flags
 * @return Object, or NULL on failure
 */
void *slab_cache_alloc(slab_cache_t *cache, unsigned int flags) {
    slab_t *slab;
    slab_object_t *obj;
    
    /* Check parameters */
    if (cache == NULL) {
        return NULL;
    }
    
    /* Allocate object */
    spin_lock(&cache->lock);
    
    /* Check if there are any partial slabs */
    if (list_empty(&cache->slabs_partial)) {
        /* Check if there are any free slabs */
        if (list_empty(&cache->slabs_free)) {
            /* Allocate a new slab */
            slab = slab_alloc(cache);
            if (slab == NULL) {
                spin_unlock(&cache->lock);
                if (flags & MEM_PANIC) {
                    kernel_panic("Failed to allocate slab");
                }
                return NULL;
            }
        } else {
            /* Get a free slab */
            slab = list_first_entry(&cache->slabs_free, slab_t, list);
            list_del(&slab->list);
        }
        
        /* Add to partial list */
        list_add(&slab->list, &cache->slabs_partial);
    } else {
        /* Get a partial slab */
        slab = list_first_entry(&cache->slabs_partial, slab_t, list);
    }
    
    /* Get an object from the slab */
    obj = slab->freelist;
    slab->freelist = obj->next;
    slab->inuse++;
    slab->free--;
    
    /* Check if slab is now full */
    if (slab->free == 0) {
        list_del(&slab->list);
        list_add(&slab->list, &cache->slabs_full);
    }
    
    spin_unlock(&cache->lock);
    
    /* Clear object */
    if (flags & MEM_ZERO) {
        memset(obj, 0, cache->size);
    }
    
    return obj;
}

/**
 * Free an object to a slab cache
 * 
 * @param cache Cache to free to
 * @param obj Object to free
 */
void slab_cache_free(slab_cache_t *cache, void *obj) {
    slab_t *slab;
    slab_object_t *sobj;
    
    /* Check parameters */
    if (cache == NULL || obj == NULL) {
        return;
    }
    
    /* Get slab */
    slab = (slab_t *)((unsigned long)obj & PAGE_MASK);
    
    /* Call destructor */
    if (cache->dtor != NULL) {
        cache->dtor(obj);
    }
    
    /* Free object */
    spin_lock(&cache->lock);
    
    /* Add object to free list */
    sobj = (slab_object_t *)obj;
    sobj->next = slab->freelist;
    slab->freelist = sobj;
    slab->inuse--;
    slab->free++;
    
    /* Check if slab was full */
    if (slab->free == 1) {
        list_del(&slab->list);
        list_add(&slab->list, &cache->slabs_partial);
    }
    
    /* Check if slab is now empty */
    if (slab->free == cache->num) {
        list_del(&slab->list);
        list_add(&slab->list, &cache->slabs_free);
    }
    
    spin_unlock(&cache->lock);
}

/**
 * Shrink a slab cache
 * 
 * @param cache Cache to shrink
 * @return Number of pages freed
 */
int slab_cache_shrink(slab_cache_t *cache) {
    slab_t *slab, *next;
    int freed = 0;
    
    /* Check parameters */
    if (cache == NULL) {
        return 0;
    }
    
    /* Free empty slabs */
    spin_lock(&cache->lock);
    
    list_for_each_entry_safe(slab, next, &cache->slabs_free, list) {
        list_del(&slab->list);
        mm_free_pages(slab->start, 1);
        cache->total_slabs--;
        cache->total_objects -= cache->num;
        freed++;
    }
    
    spin_unlock(&cache->lock);
    
    return freed;
}

/**
 * Get slab cache size
 * 
 * @param cache Cache to get size of
 * @return Object size
 */
size_t slab_cache_size(slab_cache_t *cache) {
    /* Check parameters */
    if (cache == NULL) {
        return 0;
    }
    
    return cache->size;
}

/**
 * Get slab cache name
 * 
 * @param cache Cache to get name of
 * @return Cache name
 */
const char *slab_cache_name(slab_cache_t *cache) {
    /* Check parameters */
    if (cache == NULL) {
        return NULL;
    }
    
    return cache->name;
}

/**
 * Get slab cache information
 * 
 * @param cache Cache to get information about
 * @param total_objects Pointer to store total objects
 * @param total_slabs Pointer to store total slabs
 */
void slab_cache_info(slab_cache_t *cache, unsigned int *total_objects, unsigned int *total_slabs) {
    /* Check parameters */
    if (cache == NULL) {
        return;
    }
    
    /* Get information */
    spin_lock(&cache->lock);
    if (total_objects != NULL) {
        *total_objects = cache->total_objects;
    }
    if (total_slabs != NULL) {
        *total_slabs = cache->total_slabs;
    }
    spin_unlock(&cache->lock);
}

/**
 * Allocate memory
 * 
 * @param size Size to allocate
 * @param flags Allocation flags
 * @return Memory, or NULL on failure
 */
void *kmalloc(size_t size, unsigned int flags) {
    int i;
    
    /* Check parameters */
    if (size == 0) {
        return NULL;
    }
    
    /* Find appropriate cache */
    for (i = 0; i < 13; i++) {
        if (size <= kmalloc_sizes[i]) {
            return slab_cache_alloc(kmalloc_caches[i], flags);
        }
    }
    
    /* Size too large, allocate pages */
    size_t pages = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;
    return mm_alloc_pages(pages, flags);
}

/**
 * Allocate zeroed memory
 * 
 * @param size Size to allocate
 * @param flags Allocation flags
 * @return Memory, or NULL on failure
 */
void *kzalloc(size_t size, unsigned int flags) {
    return kmalloc(size, flags | MEM_ZERO);
}

/**
 * Allocate array
 * 
 * @param n Number of elements
 * @param size Element size
 * @param flags Allocation flags
 * @return Memory, or NULL on failure
 */
void *kcalloc(size_t n, size_t size, unsigned int flags) {
    size_t total;
    
    /* Check parameters */
    if (n == 0 || size == 0) {
        return NULL;
    }
    
    /* Check for overflow */
    if (size > SIZE_MAX / n) {
        return NULL;
    }
    
    /* Allocate memory */
    total = n * size;
    return kzalloc(total, flags);
}

/**
 * Reallocate memory
 * 
 * @param ptr Memory to reallocate
 * @param size New size
 * @param flags Allocation flags
 * @return Memory, or NULL on failure
 */
void *krealloc(void *ptr, size_t size, unsigned int flags) {
    void *new;
    size_t old_size;
    
    /* Check parameters */
    if (ptr == NULL) {
        return kmalloc(size, flags);
    }
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    /* Allocate new memory */
    new = kmalloc(size, flags);
    if (new == NULL) {
        return NULL;
    }
    
    /* Copy data */
    old_size = 0; /* TODO: Get old size */
    if (old_size > size) {
        old_size = size;
    }
    memcpy(new, ptr, old_size);
    
    /* Free old memory */
    kfree(ptr);
    
    return new;
}

/**
 * Free memory
 * 
 * @param ptr Memory to free
 */
void kfree(void *ptr) {
    /* Check parameters */
    if (ptr == NULL) {
        return;
    }
    
    /* TODO: Determine if memory is from slab or pages */
    /* For now, assume slab */
    /* This would be implemented by storing metadata with the allocation */
    
    /* Free memory */
    slab_cache_free(kmalloc_caches[0], ptr);
}

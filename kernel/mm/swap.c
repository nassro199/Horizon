/**
 * swap.c - Horizon kernel swap implementation
 *
 * This file contains the implementation of the swap subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/mm/vmm.h>
#include <horizon/mm/pmm.h>
#include <horizon/mm/page.h>
#include <horizon/mm/swap.h>
#include <horizon/mm/swap_policy.h>
#include <horizon/mm/swap_compress.h>
#include <horizon/mm/swap_priority.h>
#include <horizon/mm/swap_monitor.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/fs.h>
#include <horizon/task.h>

/* Seek constants */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of swap areas */
#define MAX_SWAP_AREAS 8

/* Swap area structure */
typedef struct swap_area {
    char path[256];                /* Swap file path */
    file_t *file;                  /* Swap file */
    u32 size;                      /* Swap size in pages */
    u32 used;                      /* Used pages */
    u32 *bitmap;                   /* Bitmap of used pages */
} swap_area_t;

/* Swap areas */
static swap_area_t swap_areas[MAX_SWAP_AREAS];
static int swap_area_count = 0;

/* Swap statistics */
static u64 swap_in_count = 0;
static u64 swap_out_count = 0;
static u64 swap_in_bytes = 0;
static u64 swap_out_bytes = 0;

/* Swap lock */
static spinlock_t swap_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize the swap subsystem
 */
void swap_init(void) {
    /* Initialize the swap areas */
    memset(swap_areas, 0, sizeof(swap_areas));
    swap_area_count = 0;

    printk(KERN_INFO "SWAP: Initialized swap subsystem\n");
}

/**
 * Add a swap area
 *
 * @param path Swap file path
 * @param size Swap size in bytes
 * @return 0 on success, negative error code on failure
 */
int swap_add(const char *path, u32 size) {
    /* Check parameters */
    if (path == NULL || size == 0) {
        return -EINVAL;
    }

    /* Lock the swap */
    spin_lock(&swap_lock);

    /* Check if there is space for a new swap area */
    if (swap_area_count >= MAX_SWAP_AREAS) {
        /* No space for a new swap area */
        spin_unlock(&swap_lock);
        return -ENOMEM;
    }

    /* Open the swap file */
    file_t *file = fs_open(path, FILE_OPEN_READ | FILE_OPEN_WRITE);

    if (file == NULL) {
        /* Failed to open the swap file */
        spin_unlock(&swap_lock);
        return -ENOENT;
    }

    /* Calculate the swap size in pages */
    u32 pages = size / PAGE_SIZE;

    /* Allocate the bitmap */
    u32 *bitmap = kmalloc(sizeof(u32) * ((pages + 31) / 32), MEM_KERNEL | MEM_ZERO);

    if (bitmap == NULL) {
        /* Failed to allocate the bitmap */
        fs_close(file);
        spin_unlock(&swap_lock);
        return -ENOMEM;
    }

    /* Initialize the swap area */
    swap_area_t *area = &swap_areas[swap_area_count];
    strncpy(area->path, path, 255);
    area->path[255] = '\0';
    area->file = file;
    area->size = pages;
    area->used = 0;
    area->bitmap = bitmap;

    /* Increment the swap area count */
    swap_area_count++;

    /* Unlock the swap */
    spin_unlock(&swap_lock);

    printk(KERN_INFO "SWAP: Added swap area '%s' with %u pages\n", path, pages);

    return 0;
}

/**
 * Remove a swap area
 *
 * @param path Swap file path
 * @return 0 on success, negative error code on failure
 */
int swap_remove(const char *path) {
    /* Check parameters */
    if (path == NULL) {
        return -EINVAL;
    }

    /* Lock the swap */
    spin_lock(&swap_lock);

    /* Find the swap area */
    int i;
    for (i = 0; i < swap_area_count; i++) {
        if (strcmp(swap_areas[i].path, path) == 0) {
            /* Found the swap area */
            break;
        }
    }

    /* Check if the swap area was found */
    if (i == swap_area_count) {
        /* Swap area not found */
        spin_unlock(&swap_lock);
        return -ENOENT;
    }

    /* Check if the swap area is in use */
    if (swap_areas[i].used > 0) {
        /* Swap area is in use */
        spin_unlock(&swap_lock);
        return -EBUSY;
    }

    /* Close the swap file */
    fs_close(swap_areas[i].file);

    /* Free the bitmap */
    kfree(swap_areas[i].bitmap);

    /* Remove the swap area */
    for (int j = i; j < swap_area_count - 1; j++) {
        swap_areas[j] = swap_areas[j + 1];
    }

    /* Decrement the swap area count */
    swap_area_count--;

    /* Unlock the swap */
    spin_unlock(&swap_lock);

    printk(KERN_INFO "SWAP: Removed swap area '%s'\n", path);

    return 0;
}

/**
 * Allocate a swap entry
 *
 * @return Swap entry, or 0 on failure
 */
u32 swap_alloc(void) {
    /* Lock the swap */
    spin_lock(&swap_lock);

    /* Find a free swap entry */
    for (int i = 0; i < swap_area_count; i++) {
        /* No need to lock the swap area in this implementation */

        /* Check if the swap area is full */
        if (swap_areas[i].used >= swap_areas[i].size) {
            /* Swap area is full */
            continue;
        }

        /* Find a free page */
        for (u32 j = 0; j < swap_areas[i].size; j++) {
            /* Check if the page is free */
            if ((swap_areas[i].bitmap[j / 32] & (1 << (j % 32))) == 0) {
                /* Found a free page */
                swap_areas[i].bitmap[j / 32] |= (1 << (j % 32));
                swap_areas[i].used++;

                /* No need to unlock the swap area in this implementation */

                /* Unlock the swap */
                spin_unlock(&swap_lock);

                /* Return the swap entry */
                return (i << 24) | j;
            }
        }

        /* No need to unlock the swap area in this implementation */
    }

    /* Unlock the swap */
    spin_unlock(&swap_lock);

    /* No free swap entry found */
    return 0;
}

/**
 * Free a swap entry
 *
 * @param entry Swap entry
 * @return 0 on success, negative error code on failure
 */
int swap_free(u32 entry) {
    /* Check parameters */
    if (entry == 0) {
        return -EINVAL;
    }

    /* Get the swap area index */
    u32 area_index = (entry >> 24) & 0xFF;

    /* Get the page index */
    u32 page_index = entry & 0xFFFFFF;

    /* Check if the swap area index is valid */
    if (area_index >= swap_area_count) {
        return -EINVAL;
    }

    /* Check if the page index is valid */
    if (page_index >= swap_areas[area_index].size) {
        /* Invalid page index */
        return -EINVAL;
    }

    /* Check if the page is allocated */
    if ((swap_areas[area_index].bitmap[page_index / 32] & (1 << (page_index % 32))) == 0) {
        /* Page is not allocated */
        return -EINVAL;
    }

    /* Free the page */
    swap_areas[area_index].bitmap[page_index / 32] &= ~(1 << (page_index % 32));
    swap_areas[area_index].used--;

    return 0;
}

/**
 * Write a page to swap
 *
 * @param entry Swap entry
 * @param data Data to write
 * @return 0 on success, negative error code on failure
 */
int swap_write(u32 entry, void *data) {
    /* Check parameters */
    if (entry == 0 || data == NULL) {
        return -EINVAL;
    }

    /* Get the swap area index */
    u32 area_index = (entry >> 24) & 0xFF;

    /* Get the page index */
    u32 page_index = entry & 0xFFFFFF;

    /* Check if the swap area index is valid */
    if (area_index >= swap_area_count) {
        return -EINVAL;
    }

    /* Check if the page index is valid */
    if (page_index >= swap_areas[area_index].size) {
        /* Invalid page index */
        return -EINVAL;
    }

    /* Check if the page is allocated */
    if ((swap_areas[area_index].bitmap[page_index / 32] & (1 << (page_index % 32))) == 0) {
        /* Page is not allocated */
        return -EINVAL;
    }

    /* Allocate a compression buffer */
    void *compress_buffer = kmalloc(PAGE_SIZE * 2, MEM_KERNEL);
    if (compress_buffer == NULL) {
        return -ENOMEM;
    }

    /* Compress the page */
    ssize_t compressed_size = swap_compress_page(data, compress_buffer, PAGE_SIZE, PAGE_SIZE * 2);
    if (compressed_size < 0) {
        /* Compression failed, use the original data */
        kfree(compress_buffer);
        compressed_size = PAGE_SIZE;
        compress_buffer = data;
    }

    /* Seek to the page */
    fs_seek(swap_areas[area_index].file, page_index * PAGE_SIZE, SEEK_SET);

    /* Write the compressed size */
    ssize_t ret = fs_write(swap_areas[area_index].file, &compressed_size, sizeof(ssize_t));
    if (ret != sizeof(ssize_t)) {
        if (compress_buffer != data) {
            kfree(compress_buffer);
        }
        return -EIO;
    }

    /* Write the compressed data */
    ret = fs_write(swap_areas[area_index].file, compress_buffer, compressed_size);

    /* Free the compression buffer if it's not the original data */
    if (compress_buffer != data) {
        kfree(compress_buffer);
    }

    /* Check if the write was successful */
    if (ret != compressed_size) {
        return -EIO;
    }

    /* Update the statistics */
    spin_lock(&swap_lock);
    swap_out_count++;
    swap_out_bytes += PAGE_SIZE;
    spin_unlock(&swap_lock);

    /* Update the swap monitoring */
    swap_monitor_update(0, 1);

    return 0;
}

/**
 * Read a page from swap
 *
 * @param entry Swap entry
 * @param data Buffer to read into
 * @return 0 on success, negative error code on failure
 */
int swap_read(u32 entry, void *data) {
    /* Check parameters */
    if (entry == 0 || data == NULL) {
        return -EINVAL;
    }

    /* Get the swap area index */
    u32 area_index = (entry >> 24) & 0xFF;

    /* Get the page index */
    u32 page_index = entry & 0xFFFFFF;

    /* Check if the swap area index is valid */
    if (area_index >= swap_area_count) {
        return -EINVAL;
    }

    /* Check if the page index is valid */
    if (page_index >= swap_areas[area_index].size) {
        /* Invalid page index */
        return -EINVAL;
    }

    /* Check if the page is allocated */
    if ((swap_areas[area_index].bitmap[page_index / 32] & (1 << (page_index % 32))) == 0) {
        /* Page is not allocated */
        return -EINVAL;
    }

    /* Seek to the page */
    fs_seek(swap_areas[area_index].file, page_index * PAGE_SIZE, SEEK_SET);

    /* Read the compressed size */
    ssize_t compressed_size;
    ssize_t ret = fs_read(swap_areas[area_index].file, &compressed_size, sizeof(ssize_t));
    if (ret != sizeof(ssize_t)) {
        return -EIO;
    }

    /* Check if the page is compressed */
    if (compressed_size == PAGE_SIZE) {
        /* Page is not compressed, read it directly */
        ret = fs_read(swap_areas[area_index].file, data, PAGE_SIZE);

        /* Check if the read was successful */
        if (ret != PAGE_SIZE) {
            return -EIO;
        }
    } else {
        /* Page is compressed, allocate a buffer */
        void *compress_buffer = kmalloc(compressed_size, MEM_KERNEL);
        if (compress_buffer == NULL) {
            return -ENOMEM;
        }

        /* Read the compressed data */
        ret = fs_read(swap_areas[area_index].file, compress_buffer, compressed_size);

        /* Check if the read was successful */
        if (ret != compressed_size) {
            kfree(compress_buffer);
            return -EIO;
        }

        /* Decompress the data */
        ret = swap_decompress_page(compress_buffer, data, compressed_size, PAGE_SIZE);

        /* Free the compression buffer */
        kfree(compress_buffer);

        /* Check if the decompression was successful */
        if (ret != PAGE_SIZE) {
            return -EIO;
        }
    }

    /* Update the statistics */
    spin_lock(&swap_lock);
    swap_in_count++;
    swap_in_bytes += PAGE_SIZE;
    spin_unlock(&swap_lock);

    /* Update the swap monitoring */
    swap_monitor_update(1, 0);

    return 0;
}

/**
 * Swap out a page
 *
 * @param task Task to swap out from
 * @param addr Address to swap out
 * @return 0 on success, negative error code on failure
 */
int swap_out_page(task_struct_t *task, u32 addr) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL) {
        return -EINVAL;
    }

    /* Align the address to a page boundary */
    addr &= ~(PAGE_SIZE - 1);

    /* Get the page */
    page_t *page = vmm_get_page(task->mm, addr);

    /* Check if the page was found */
    if (page == NULL) {
        /* No page found */
        return -EFAULT;
    }

    /* Check if the page is already swapped out */
    if (task->mm->swap_map != NULL && task->mm->swap_map[addr / PAGE_SIZE] != 0) {
        /* Page is already swapped out */
        return -EINVAL;
    }

    /* Check the page priority */
    swap_priority_t priority = swap_priority_get(task, addr);

    /* Skip high priority pages if possible */
    if (priority == SWAP_PRIORITY_HIGH) {
        /* Try to find a lower priority page */
        int found = swap_priority_scan_low(task, 1);
        if (found > 0) {
            /* Found a lower priority page, skip this one */
            return -EAGAIN;
        }

        found = swap_priority_scan_medium(task, 1);
        if (found > 0) {
            /* Found a medium priority page, skip this one */
            return -EAGAIN;
        }
    } else if (priority == SWAP_PRIORITY_MEDIUM) {
        /* Try to find a lower priority page */
        int found = swap_priority_scan_low(task, 1);
        if (found > 0) {
            /* Found a lower priority page, skip this one */
            return -EAGAIN;
        }
    }

    /* Allocate a swap entry */
    u32 swap_entry = swap_alloc();

    if (swap_entry == 0) {
        /* Failed to allocate a swap entry */
        return -ENOMEM;
    }

    /* Write the page to swap */
    int ret = swap_write(swap_entry, pmm_page_to_virt(page));

    if (ret < 0) {
        /* Failed to write the page to swap */
        swap_free(swap_entry);
        return ret;
    }

    /* Check if the task has a swap map */
    if (task->mm->swap_map == NULL) {
        /* Allocate a swap map */
        task->mm->swap_map = kmalloc(sizeof(u32) * (task->mm->total_vm), MEM_KERNEL | MEM_ZERO);

        if (task->mm->swap_map == NULL) {
            /* Failed to allocate a swap map */
            swap_free(swap_entry);
            return -ENOMEM;
        }
    }

    /* Set the swap entry */
    task->mm->swap_map[addr / PAGE_SIZE] = swap_entry;

    /* Unmap the page */
    ret = vmm_unmap_page(task->mm, addr);

    if (ret < 0) {
        /* Failed to unmap the page */
        task->mm->swap_map[addr / PAGE_SIZE] = 0;
        swap_free(swap_entry);
        return ret;
    }

    /* Free the page */
    page_free(page, 0);

    /* Update the swap statistics */
    if (task->mm->swap_used == 0) {
        task->mm->swap_used = 1;
    } else {
        task->mm->swap_used++;
    }

    return 0;
}

/**
 * Swap in a page
 *
 * @param task Task to swap in to
 * @param addr Address to swap in
 * @return 0 on success, negative error code on failure
 */
int swap_in_page(task_struct_t *task, u32 addr) {
    /* Check parameters */
    if (task == NULL || task->mm == NULL) {
        return -EINVAL;
    }

    /* Align the address to a page boundary */
    addr &= ~(PAGE_SIZE - 1);

    /* Check if the task has a swap map */
    if (task->mm->swap_map == NULL) {
        /* No swap map */
        return -EINVAL;
    }

    /* Get the swap entry */
    u32 swap_entry = task->mm->swap_map[addr / PAGE_SIZE];

    /* Check if the swap entry is valid */
    if (swap_entry == 0) {
        /* No swap entry */
        return -EINVAL;
    }

    /* Allocate a page */
    page_t *page = page_alloc(0);

    if (page == NULL) {
        /* Failed to allocate a page */
        return -ENOMEM;
    }

    /* Read the page from swap */
    int ret = swap_read(swap_entry, pmm_page_to_virt(page));

    if (ret < 0) {
        /* Failed to read the page from swap */
        page_free(page, 0);
        return ret;
    }

    /* Find the virtual memory area */
    vm_area_struct_t *vma = vmm_find_vma(task->mm, addr);

    /* Check if the virtual memory area was found */
    if (vma == NULL) {
        /* No virtual memory area found */
        page_free(page, 0);
        return -EFAULT;
    }

    /* Map the page */
    ret = vmm_map_page(task->mm, addr, page, vma->vm_flags);

    if (ret < 0) {
        /* Failed to map the page */
        page_free(page, 0);
        return ret;
    }

    /* Clear the swap entry */
    task->mm->swap_map[addr / PAGE_SIZE] = 0;

    /* Free the swap entry */
    swap_free(swap_entry);

    /* Update the swap statistics */
    if (task->mm->swap_used > 0) {
        task->mm->swap_used--;
    }

    /* Prefetch nearby pages */
    swap_policy_prefetch(task, addr, 4);

    return 0;
}

/**
 * Print swap statistics
 */
void swap_print_stats(void) {
    /* Print the statistics */
    printk(KERN_INFO "SWAP: Swap areas: %d\n", swap_area_count);

    for (int i = 0; i < swap_area_count; i++) {
        printk(KERN_INFO "SWAP: Area %d: %s, %u/%u pages used\n", i, swap_areas[i].path, swap_areas[i].used, swap_areas[i].size);
    }

    printk(KERN_INFO "SWAP: Swap in: %llu pages, %llu bytes\n", swap_in_count, swap_in_bytes);
    printk(KERN_INFO "SWAP: Swap out: %llu pages, %llu bytes\n", swap_out_count, swap_out_bytes);
}

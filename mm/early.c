/**
 * early.c - Horizon kernel early memory management implementation
 * 
 * This file contains the implementation of early memory management.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm/early.h>
#include <horizon/multiboot.h>
#include <horizon/string.h>
#include <horizon/console.h>

/* Memory regions */
static memory_region_t *memory_regions = NULL;
static u32 memory_region_count = 0;

/* Total and available memory */
static u64 total_memory = 0;
static u64 available_memory = 0;

/* Early memory allocator */
static early_allocator_t early_allocator;

/**
 * Initialize early memory management
 */
void early_mm_init(void) {
    early_console_print("Initializing early memory management...\n");
    
    /* Parse multiboot memory map */
    multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)multiboot_info->mmap_addr;
    u32 mmap_length = multiboot_info->mmap_length;
    
    while ((u32)entry < (u32)multiboot_info->mmap_addr + mmap_length) {
        /* Add memory region */
        early_mm_add_region(entry->addr, entry->len, entry->type);
        
        /* Move to the next entry */
        entry = (multiboot_mmap_entry_t *)((u32)entry + entry->size + 4);
    }
    
    /* Reserve kernel memory */
    extern u32 _kernel_start;
    extern u32 _kernel_end;
    early_mm_reserve_kernel(&_kernel_start, &_kernel_end);
    
    /* Reserve modules memory */
    early_mm_reserve_modules();
    
    /* Initialize early memory allocator */
    memory_region_t *region = memory_regions;
    while (region != NULL) {
        if (region->type == MEMORY_REGION_AVAILABLE && region->start >= 0x100000) {
            /* Found a suitable region */
            early_mm_init_allocator((void *)(u32)region->start, (void *)(u32)(region->start + region->size));
            break;
        }
        region = region->next;
    }
    
    /* Print memory regions */
    early_mm_print_regions();
    
    early_console_print("Early memory management initialized\n");
    early_console_print("Total memory: ");
    early_console_print_dec((u32)(total_memory / 1024 / 1024));
    early_console_print(" MB\n");
    early_console_print("Available memory: ");
    early_console_print_dec((u32)(available_memory / 1024 / 1024));
    early_console_print(" MB\n");
}

/**
 * Add a memory region
 * 
 * @param start Start address
 * @param size Size in bytes
 * @param type Region type
 */
void early_mm_add_region(u64 start, u64 size, u32 type) {
    /* Allocate memory region */
    memory_region_t *region = (memory_region_t *)early_mm_allocator_alloc(sizeof(memory_region_t), 8);
    if (region == NULL) {
        early_console_print("Failed to allocate memory region\n");
        return;
    }
    
    /* Initialize memory region */
    region->start = start;
    region->size = size;
    region->type = type;
    region->next = NULL;
    
    /* Add to list */
    if (memory_regions == NULL) {
        memory_regions = region;
    } else {
        memory_region_t *curr = memory_regions;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = region;
    }
    
    /* Update memory counters */
    total_memory += size;
    if (type == MEMORY_REGION_AVAILABLE) {
        available_memory += size;
    }
    
    memory_region_count++;
}

/**
 * Get memory regions
 * 
 * @return Memory regions
 */
memory_region_t *early_mm_get_regions(void) {
    return memory_regions;
}

/**
 * Print memory regions
 */
void early_mm_print_regions(void) {
    early_console_print("Memory regions:\n");
    
    memory_region_t *region = memory_regions;
    while (region != NULL) {
        early_console_print("  ");
        early_console_print_hex((u32)(region->start >> 32));
        early_console_print_hex((u32)region->start);
        early_console_print(" - ");
        early_console_print_hex((u32)((region->start + region->size) >> 32));
        early_console_print_hex((u32)(region->start + region->size));
        early_console_print(" (");
        
        /* Print region type */
        switch (region->type) {
            case MEMORY_REGION_AVAILABLE:
                early_console_print("Available");
                break;
            case MEMORY_REGION_RESERVED:
                early_console_print("Reserved");
                break;
            case MEMORY_REGION_ACPI:
                early_console_print("ACPI Reclaim");
                break;
            case MEMORY_REGION_NVS:
                early_console_print("ACPI NVS");
                break;
            case MEMORY_REGION_BADRAM:
                early_console_print("Bad RAM");
                break;
            case MEMORY_REGION_KERNEL:
                early_console_print("Kernel");
                break;
            case MEMORY_REGION_MODULES:
                early_console_print("Modules");
                break;
            case MEMORY_REGION_BOOTLOADER:
                early_console_print("Bootloader");
                break;
            default:
                early_console_print("Unknown");
                break;
        }
        
        early_console_print(", ");
        early_console_print_dec((u32)(region->size / 1024));
        early_console_print(" KB)\n");
        
        region = region->next;
    }
    
    early_console_print("Total memory regions: ");
    early_console_print_dec(memory_region_count);
    early_console_print("\n");
}

/**
 * Allocate memory
 * 
 * @param size Size in bytes
 * @param align Alignment
 * @return Allocated memory, or NULL on failure
 */
void *early_mm_alloc(u32 size, u32 align) {
    return early_mm_allocator_alloc(size, align);
}

/**
 * Free memory
 * 
 * @param addr Memory address
 */
void early_mm_free(void *addr) {
    early_mm_allocator_free(addr);
}

/**
 * Get total memory
 * 
 * @return Total memory in bytes
 */
u64 early_mm_get_total_memory(void) {
    return total_memory;
}

/**
 * Get available memory
 * 
 * @return Available memory in bytes
 */
u64 early_mm_get_available_memory(void) {
    return available_memory;
}

/**
 * Reserve kernel memory
 * 
 * @param start Kernel start address
 * @param end Kernel end address
 */
void early_mm_reserve_kernel(void *start, void *end) {
    u64 kernel_start = (u64)(u32)start;
    u64 kernel_size = (u64)(u32)end - (u64)(u32)start;
    
    early_console_print("Reserving kernel memory: ");
    early_console_print_hex((u32)start);
    early_console_print(" - ");
    early_console_print_hex((u32)end);
    early_console_print(" (");
    early_console_print_dec((u32)(kernel_size / 1024));
    early_console_print(" KB)\n");
    
    /* Find the memory region containing the kernel */
    memory_region_t *region = memory_regions;
    while (region != NULL) {
        if (region->type == MEMORY_REGION_AVAILABLE &&
            kernel_start >= region->start &&
            kernel_start + kernel_size <= region->start + region->size) {
            /* Found the region */
            
            /* Check if we need to split the region */
            if (kernel_start > region->start) {
                /* Create a new region before the kernel */
                u64 before_size = kernel_start - region->start;
                early_mm_add_region(region->start, before_size, MEMORY_REGION_AVAILABLE);
            }
            
            /* Check if we need to split the region */
            if (kernel_start + kernel_size < region->start + region->size) {
                /* Create a new region after the kernel */
                u64 after_start = kernel_start + kernel_size;
                u64 after_size = region->start + region->size - after_start;
                early_mm_add_region(after_start, after_size, MEMORY_REGION_AVAILABLE);
            }
            
            /* Update the region */
            region->start = kernel_start;
            region->size = kernel_size;
            region->type = MEMORY_REGION_KERNEL;
            
            /* Update available memory */
            available_memory -= kernel_size;
            
            return;
        }
        region = region->next;
    }
    
    early_console_print("Failed to reserve kernel memory\n");
}

/**
 * Reserve modules memory
 */
void early_mm_reserve_modules(void) {
    u32 mods_count = multiboot_get_mods_count();
    if (mods_count == 0) {
        return;
    }
    
    early_console_print("Reserving modules memory:\n");
    
    for (u32 i = 0; i < mods_count; i++) {
        multiboot_module_t *mod = multiboot_get_mod(i);
        if (mod == NULL) {
            continue;
        }
        
        u64 mod_start = (u64)mod->mod_start;
        u64 mod_size = (u64)mod->mod_end - (u64)mod->mod_start;
        
        early_console_print("  Module ");
        early_console_print_dec(i);
        early_console_print(": ");
        early_console_print_hex(mod->mod_start);
        early_console_print(" - ");
        early_console_print_hex(mod->mod_end);
        early_console_print(" (");
        early_console_print_dec((u32)(mod_size / 1024));
        early_console_print(" KB)\n");
        
        /* Find the memory region containing the module */
        memory_region_t *region = memory_regions;
        while (region != NULL) {
            if (region->type == MEMORY_REGION_AVAILABLE &&
                mod_start >= region->start &&
                mod_start + mod_size <= region->start + region->size) {
                /* Found the region */
                
                /* Check if we need to split the region */
                if (mod_start > region->start) {
                    /* Create a new region before the module */
                    u64 before_size = mod_start - region->start;
                    early_mm_add_region(region->start, before_size, MEMORY_REGION_AVAILABLE);
                }
                
                /* Check if we need to split the region */
                if (mod_start + mod_size < region->start + region->size) {
                    /* Create a new region after the module */
                    u64 after_start = mod_start + mod_size;
                    u64 after_size = region->start + region->size - after_start;
                    early_mm_add_region(after_start, after_size, MEMORY_REGION_AVAILABLE);
                }
                
                /* Update the region */
                region->start = mod_start;
                region->size = mod_size;
                region->type = MEMORY_REGION_MODULES;
                
                /* Update available memory */
                available_memory -= mod_size;
                
                break;
            }
            region = region->next;
        }
    }
}

/**
 * Initialize early memory allocator
 * 
 * @param start Start address
 * @param end End address
 */
void early_mm_init_allocator(void *start, void *end) {
    early_console_print("Initializing early memory allocator: ");
    early_console_print_hex((u32)start);
    early_console_print(" - ");
    early_console_print_hex((u32)end);
    early_console_print("\n");
    
    early_allocator.start = start;
    early_allocator.end = end;
    early_allocator.current = start;
}

/**
 * Allocate memory from early memory allocator
 * 
 * @param size Size in bytes
 * @param align Alignment
 * @return Allocated memory, or NULL on failure
 */
void *early_mm_allocator_alloc(u32 size, u32 align) {
    /* Check if allocator is initialized */
    if (early_allocator.start == NULL) {
        /* Use a static buffer for early allocations */
        static u8 early_buffer[4096];
        static u32 early_buffer_offset = 0;
        
        /* Align the offset */
        early_buffer_offset = (early_buffer_offset + align - 1) & ~(align - 1);
        
        /* Check if we have enough space */
        if (early_buffer_offset + size > sizeof(early_buffer)) {
            return NULL;
        }
        
        /* Allocate memory */
        void *ptr = &early_buffer[early_buffer_offset];
        early_buffer_offset += size;
        
        return ptr;
    }
    
    /* Align the current address */
    u32 current = (u32)early_allocator.current;
    current = (current + align - 1) & ~(align - 1);
    
    /* Check if we have enough space */
    if (current + size > (u32)early_allocator.end) {
        return NULL;
    }
    
    /* Allocate memory */
    void *ptr = (void *)current;
    early_allocator.current = (void *)(current + size);
    
    /* Clear memory */
    memset(ptr, 0, size);
    
    return ptr;
}

/**
 * Free memory from early memory allocator
 * 
 * @param addr Memory address
 */
void early_mm_allocator_free(void *addr) {
    /* Early memory allocator doesn't support freeing */
}

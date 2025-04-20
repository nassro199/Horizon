/**
 * multiboot.c - Multiboot specification implementation
 * 
 * This file contains the implementation of the Multiboot Specification.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/multiboot.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/console.h>

/* Multiboot information */
static multiboot_info_t *multiboot_info = NULL;
static u32 multiboot_magic = 0;

/* Memory information */
static u32 mem_lower = 0;
static u32 mem_upper = 0;
static u64 total_memory = 0;

/* Command line */
static char *cmdline = NULL;

/* Boot loader name */
static char *bootloader = NULL;

/* Modules */
static multiboot_module_t *modules = NULL;
static u32 modules_count = 0;

/* Memory map */
static multiboot_mmap_entry_t *mmap = NULL;
static u32 mmap_length = 0;

/**
 * Initialize multiboot
 * 
 * @param magic Multiboot magic number
 * @param mbi Multiboot information structure
 */
void multiboot_init(u32 magic, multiboot_info_t *mbi) {
    /* Check magic number */
    if (magic != 0x2BADB002) {
        early_console_print("Invalid multiboot magic number: 0x");
        early_console_print_hex(magic);
        early_console_print("\n");
        kernel_panic("Invalid multiboot magic number");
    }

    /* Store multiboot information */
    multiboot_info = mbi;
    multiboot_magic = magic;

    /* Parse memory information */
    if (mbi->flags & MULTIBOOT_INFO_FLAG_MEM) {
        mem_lower = mbi->mem_lower;
        mem_upper = mbi->mem_upper;
        total_memory = (u64)mem_lower + (u64)mem_upper;
        total_memory *= 1024; /* Convert from KB to bytes */
    }

    /* Parse command line */
    if (mbi->flags & MULTIBOOT_INFO_FLAG_CMDLINE) {
        cmdline = (char *)mbi->cmdline;
    }

    /* Parse boot loader name */
    if (mbi->flags & MULTIBOOT_INFO_FLAG_LOADER) {
        bootloader = (char *)mbi->boot_loader_name;
    }

    /* Parse modules */
    if (mbi->flags & MULTIBOOT_INFO_FLAG_MODS) {
        modules = (multiboot_module_t *)mbi->mods_addr;
        modules_count = mbi->mods_count;
    }

    /* Parse memory map */
    if (mbi->flags & MULTIBOOT_INFO_FLAG_MMAP) {
        mmap = (multiboot_mmap_entry_t *)mbi->mmap_addr;
        mmap_length = mbi->mmap_length;
    }
}

/**
 * Get command line
 * 
 * @return Command line, or NULL if not available
 */
const char *multiboot_get_cmdline(void) {
    return cmdline;
}

/**
 * Get boot loader name
 * 
 * @return Boot loader name, or NULL if not available
 */
const char *multiboot_get_bootloader(void) {
    return bootloader;
}

/**
 * Get lower memory size
 * 
 * @return Lower memory size in KB
 */
u32 multiboot_get_mem_lower(void) {
    return mem_lower;
}

/**
 * Get upper memory size
 * 
 * @return Upper memory size in KB
 */
u32 multiboot_get_mem_upper(void) {
    return mem_upper;
}

/**
 * Get total memory size
 * 
 * @return Total memory size in bytes
 */
u64 multiboot_get_total_memory(void) {
    return total_memory;
}

/**
 * Get modules count
 * 
 * @return Modules count
 */
u32 multiboot_get_mods_count(void) {
    return modules_count;
}

/**
 * Get module
 * 
 * @param index Module index
 * @return Module, or NULL if not available
 */
multiboot_module_t *multiboot_get_mod(u32 index) {
    if (index >= modules_count) {
        return NULL;
    }
    return &modules[index];
}

/**
 * Get module command line
 * 
 * @param index Module index
 * @return Module command line, or NULL if not available
 */
const char *multiboot_get_mod_cmdline(u32 index) {
    multiboot_module_t *mod = multiboot_get_mod(index);
    if (mod == NULL) {
        return NULL;
    }
    return (const char *)mod->cmdline;
}

/**
 * Parse memory map
 * 
 * This function parses the memory map and initializes the physical memory manager.
 */
void multiboot_parse_mmap(void) {
    if (mmap == NULL) {
        early_console_print("No memory map available\n");
        return;
    }

    early_console_print("Memory map:\n");

    /* Parse memory map */
    multiboot_mmap_entry_t *entry = mmap;
    u32 entry_count = 0;
    
    while ((u32)entry < (u32)mmap + mmap_length) {
        /* Print memory map entry */
        early_console_print("  ");
        early_console_print_hex((u32)(entry->addr >> 32));
        early_console_print_hex((u32)entry->addr);
        early_console_print(" - ");
        early_console_print_hex((u32)((entry->addr + entry->len) >> 32));
        early_console_print_hex((u32)(entry->addr + entry->len));
        early_console_print(" (");
        
        /* Print memory type */
        switch (entry->type) {
            case MULTIBOOT_MEMORY_AVAILABLE:
                early_console_print("Available");
                /* Initialize physical memory region */
                mm_init_region((void *)(u32)entry->addr, (u32)entry->len);
                break;
            case MULTIBOOT_MEMORY_RESERVED:
                early_console_print("Reserved");
                break;
            case MULTIBOOT_MEMORY_ACPI_RECLAIM:
                early_console_print("ACPI Reclaim");
                break;
            case MULTIBOOT_MEMORY_NVS:
                early_console_print("ACPI NVS");
                break;
            case MULTIBOOT_MEMORY_BADRAM:
                early_console_print("Bad RAM");
                break;
            default:
                early_console_print("Unknown");
                break;
        }
        
        early_console_print(")\n");
        
        /* Move to the next entry */
        entry = (multiboot_mmap_entry_t *)((u32)entry + entry->size + 4);
        entry_count++;
    }
    
    early_console_print("Total memory map entries: ");
    early_console_print_dec(entry_count);
    early_console_print("\n");
}

/**
 * Print multiboot information
 */
void multiboot_print_info(void) {
    early_console_print("Multiboot Information:\n");
    
    /* Print memory information */
    if (multiboot_info->flags & MULTIBOOT_INFO_FLAG_MEM) {
        early_console_print("  Memory: Lower = ");
        early_console_print_dec(mem_lower);
        early_console_print(" KB, Upper = ");
        early_console_print_dec(mem_upper);
        early_console_print(" KB, Total = ");
        early_console_print_dec((u32)(total_memory / 1024));
        early_console_print(" KB\n");
    }
    
    /* Print command line */
    if (multiboot_info->flags & MULTIBOOT_INFO_FLAG_CMDLINE) {
        early_console_print("  Command Line: ");
        early_console_print(cmdline);
        early_console_print("\n");
    }
    
    /* Print boot loader name */
    if (multiboot_info->flags & MULTIBOOT_INFO_FLAG_LOADER) {
        early_console_print("  Boot Loader: ");
        early_console_print(bootloader);
        early_console_print("\n");
    }
    
    /* Print modules */
    if (multiboot_info->flags & MULTIBOOT_INFO_FLAG_MODS) {
        early_console_print("  Modules: ");
        early_console_print_dec(modules_count);
        early_console_print("\n");
        
        for (u32 i = 0; i < modules_count; i++) {
            early_console_print("    Module ");
            early_console_print_dec(i);
            early_console_print(": ");
            early_console_print_hex(modules[i].mod_start);
            early_console_print(" - ");
            early_console_print_hex(modules[i].mod_end);
            early_console_print(" (");
            early_console_print((char *)modules[i].cmdline);
            early_console_print(")\n");
        }
    }
    
    /* Print memory map */
    if (multiboot_info->flags & MULTIBOOT_INFO_FLAG_MMAP) {
        early_console_print("  Memory Map: ");
        early_console_print_dec(mmap_length);
        early_console_print(" bytes\n");
    }
    
    /* Print drives */
    if (multiboot_info->flags & MULTIBOOT_INFO_FLAG_DRIVES) {
        early_console_print("  Drives: ");
        early_console_print_dec(multiboot_info->drives_length);
        early_console_print(" bytes\n");
    }
    
    /* Print config table */
    if (multiboot_info->flags & MULTIBOOT_INFO_FLAG_CONFIG) {
        early_console_print("  Config Table: ");
        early_console_print_hex(multiboot_info->config_table);
        early_console_print("\n");
    }
    
    /* Print APM table */
    if (multiboot_info->flags & MULTIBOOT_INFO_FLAG_APM) {
        early_console_print("  APM Table: ");
        early_console_print_hex(multiboot_info->apm_table);
        early_console_print("\n");
    }
    
    /* Print VBE info */
    if (multiboot_info->flags & MULTIBOOT_INFO_FLAG_VBE) {
        early_console_print("  VBE: Control Info = ");
        early_console_print_hex(multiboot_info->vbe_control_info);
        early_console_print(", Mode Info = ");
        early_console_print_hex(multiboot_info->vbe_mode_info);
        early_console_print(", Mode = ");
        early_console_print_hex(multiboot_info->vbe_mode);
        early_console_print("\n");
    }
    
    /* Print framebuffer info */
    if (multiboot_info->flags & MULTIBOOT_INFO_FLAG_FRAMEBUFFER) {
        early_console_print("  Framebuffer: Address = ");
        early_console_print_hex((u32)(multiboot_info->framebuffer_addr >> 32));
        early_console_print_hex((u32)multiboot_info->framebuffer_addr);
        early_console_print(", Width = ");
        early_console_print_dec(multiboot_info->framebuffer_width);
        early_console_print(", Height = ");
        early_console_print_dec(multiboot_info->framebuffer_height);
        early_console_print(", BPP = ");
        early_console_print_dec(multiboot_info->framebuffer_bpp);
        early_console_print("\n");
    }
}

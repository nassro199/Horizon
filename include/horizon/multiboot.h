/**
 * multiboot.h - Multiboot specification definitions
 * 
 * This file contains definitions for the Multiboot Specification.
 * See https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
 */

#ifndef _HORIZON_MULTIBOOT_H
#define _HORIZON_MULTIBOOT_H

#include <horizon/types.h>

/* Multiboot header magic value */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

/* Multiboot header flags */
#define MULTIBOOT_HEADER_FLAG_ALIGN     0x00000001  /* Align modules on page boundaries */
#define MULTIBOOT_HEADER_FLAG_MEMINFO   0x00000002  /* Provide memory map */
#define MULTIBOOT_HEADER_FLAG_VIDMODE   0x00000004  /* Provide video mode */
#define MULTIBOOT_HEADER_FLAG_AOUT      0x00010000  /* Load address info */

/* Multiboot info flags */
#define MULTIBOOT_INFO_FLAG_MEM         0x00000001  /* Memory info available */
#define MULTIBOOT_INFO_FLAG_BOOTDEV     0x00000002  /* Boot device info available */
#define MULTIBOOT_INFO_FLAG_CMDLINE     0x00000004  /* Command line available */
#define MULTIBOOT_INFO_FLAG_MODS        0x00000008  /* Modules available */
#define MULTIBOOT_INFO_FLAG_AOUT_SYMS   0x00000010  /* a.out symbol table available */
#define MULTIBOOT_INFO_FLAG_ELF_SHDR    0x00000020  /* ELF section header table available */
#define MULTIBOOT_INFO_FLAG_MMAP        0x00000040  /* Memory map available */
#define MULTIBOOT_INFO_FLAG_DRIVES      0x00000080  /* Drive info available */
#define MULTIBOOT_INFO_FLAG_CONFIG      0x00000100  /* Config table available */
#define MULTIBOOT_INFO_FLAG_LOADER      0x00000200  /* Boot loader name available */
#define MULTIBOOT_INFO_FLAG_APM         0x00000400  /* APM table available */
#define MULTIBOOT_INFO_FLAG_VBE         0x00000800  /* VBE info available */
#define MULTIBOOT_INFO_FLAG_FRAMEBUFFER 0x00001000  /* Framebuffer info available */

/* Multiboot memory map entry types */
#define MULTIBOOT_MEMORY_AVAILABLE      1  /* Available memory */
#define MULTIBOOT_MEMORY_RESERVED       2  /* Reserved memory */
#define MULTIBOOT_MEMORY_ACPI_RECLAIM   3  /* ACPI reclaimable memory */
#define MULTIBOOT_MEMORY_NVS            4  /* ACPI NVS memory */
#define MULTIBOOT_MEMORY_BADRAM         5  /* Bad RAM */

/* Multiboot module alignment */
#define MULTIBOOT_MOD_ALIGN             0x00001000  /* 4KB alignment */

/* Multiboot header structure */
typedef struct multiboot_header {
    u32 magic;                    /* Magic number */
    u32 flags;                    /* Flags */
    u32 checksum;                 /* Checksum */
    /* Address fields (valid if MULTIBOOT_HEADER_FLAG_AOUT is set) */
    u32 header_addr;              /* Header address */
    u32 load_addr;                /* Load address */
    u32 load_end_addr;            /* Load end address */
    u32 bss_end_addr;             /* BSS end address */
    u32 entry_addr;               /* Entry point address */
    /* Video mode fields (valid if MULTIBOOT_HEADER_FLAG_VIDMODE is set) */
    u32 mode_type;                /* Video mode type */
    u32 width;                    /* Video width */
    u32 height;                   /* Video height */
    u32 depth;                    /* Video depth */
} __attribute__((packed)) multiboot_header_t;

/* Multiboot information structure */
typedef struct multiboot_info {
    u32 flags;                    /* Multiboot info flags */
    /* Memory info (valid if MULTIBOOT_INFO_FLAG_MEM is set) */
    u32 mem_lower;                /* Amount of lower memory (KB) */
    u32 mem_upper;                /* Amount of upper memory (KB) */
    /* Boot device info (valid if MULTIBOOT_INFO_FLAG_BOOTDEV is set) */
    u32 boot_device;              /* Boot device */
    /* Command line (valid if MULTIBOOT_INFO_FLAG_CMDLINE is set) */
    u32 cmdline;                  /* Command line address */
    /* Modules (valid if MULTIBOOT_INFO_FLAG_MODS is set) */
    u32 mods_count;               /* Number of modules */
    u32 mods_addr;                /* Module address */
    /* Symbol tables (valid if MULTIBOOT_INFO_FLAG_AOUT_SYMS or MULTIBOOT_INFO_FLAG_ELF_SHDR is set) */
    union {
        struct {
            u32 tabsize;          /* Symbol table size */
            u32 strsize;          /* String table size */
            u32 addr;             /* Symbol table address */
            u32 reserved;         /* Reserved */
        } aout_sym;
        struct {
            u32 num;              /* Number of section headers */
            u32 size;             /* Section header size */
            u32 addr;             /* Section header address */
            u32 shndx;            /* Section header string index */
        } elf_sec;
    } syms;
    /* Memory map (valid if MULTIBOOT_INFO_FLAG_MMAP is set) */
    u32 mmap_length;              /* Memory map length */
    u32 mmap_addr;                /* Memory map address */
    /* Drives (valid if MULTIBOOT_INFO_FLAG_DRIVES is set) */
    u32 drives_length;            /* Drives length */
    u32 drives_addr;              /* Drives address */
    /* Config table (valid if MULTIBOOT_INFO_FLAG_CONFIG is set) */
    u32 config_table;             /* Config table address */
    /* Boot loader name (valid if MULTIBOOT_INFO_FLAG_LOADER is set) */
    u32 boot_loader_name;         /* Boot loader name address */
    /* APM table (valid if MULTIBOOT_INFO_FLAG_APM is set) */
    u32 apm_table;                /* APM table address */
    /* VBE info (valid if MULTIBOOT_INFO_FLAG_VBE is set) */
    u32 vbe_control_info;         /* VBE control info */
    u32 vbe_mode_info;            /* VBE mode info */
    u16 vbe_mode;                 /* VBE mode */
    u16 vbe_interface_seg;        /* VBE interface segment */
    u16 vbe_interface_off;        /* VBE interface offset */
    u16 vbe_interface_len;        /* VBE interface length */
    /* Framebuffer info (valid if MULTIBOOT_INFO_FLAG_FRAMEBUFFER is set) */
    u64 framebuffer_addr;         /* Framebuffer address */
    u32 framebuffer_pitch;        /* Framebuffer pitch */
    u32 framebuffer_width;        /* Framebuffer width */
    u32 framebuffer_height;       /* Framebuffer height */
    u8 framebuffer_bpp;           /* Framebuffer bits per pixel */
    u8 framebuffer_type;          /* Framebuffer type */
    union {
        struct {
            u32 framebuffer_palette_addr;     /* Palette address */
            u16 framebuffer_palette_num_colors; /* Number of colors */
        } palette;
        struct {
            u8 framebuffer_red_field_position;   /* Red field position */
            u8 framebuffer_red_mask_size;        /* Red mask size */
            u8 framebuffer_green_field_position; /* Green field position */
            u8 framebuffer_green_mask_size;      /* Green mask size */
            u8 framebuffer_blue_field_position;  /* Blue field position */
            u8 framebuffer_blue_mask_size;       /* Blue mask size */
        } rgb;
    } color_info;
} __attribute__((packed)) multiboot_info_t;

/* Multiboot memory map entry */
typedef struct multiboot_mmap_entry {
    u32 size;                     /* Size of this entry */
    u64 addr;                     /* Base address */
    u64 len;                      /* Length */
    u32 type;                     /* Type */
} __attribute__((packed)) multiboot_mmap_entry_t;

/* Multiboot module entry */
typedef struct multiboot_module {
    u32 mod_start;                /* Module start address */
    u32 mod_end;                  /* Module end address */
    u32 cmdline;                  /* Module command line */
    u32 reserved;                 /* Reserved */
} __attribute__((packed)) multiboot_module_t;

/* Multiboot drive entry */
typedef struct multiboot_drive {
    u32 size;                     /* Size of this entry */
    u8 drive_number;              /* Drive number */
    u8 drive_mode;                /* Drive mode */
    u16 drive_cylinders;          /* Number of cylinders */
    u8 drive_heads;               /* Number of heads */
    u8 drive_sectors;             /* Number of sectors per track */
    u16 drive_ports[0];           /* Drive ports */
} __attribute__((packed)) multiboot_drive_t;

/* Multiboot APM table */
typedef struct multiboot_apm_info {
    u16 version;                  /* APM version */
    u16 cseg;                     /* APM 32-bit code segment */
    u32 offset;                   /* Offset of entry point */
    u16 cseg_16;                  /* APM 16-bit code segment */
    u16 dseg;                     /* APM data segment */
    u16 flags;                    /* APM flags */
    u16 cseg_len;                 /* APM 32-bit code segment length */
    u16 cseg_16_len;              /* APM 16-bit code segment length */
    u16 dseg_len;                 /* APM data segment length */
} __attribute__((packed)) multiboot_apm_info_t;

/* Multiboot VBE info */
typedef struct multiboot_vbe_info {
    u8 signature[4];              /* VBE signature */
    u16 version;                  /* VBE version */
    u32 oem_string;               /* OEM string */
    u32 capabilities;             /* Capabilities */
    u32 video_modes;              /* Video modes */
    u16 video_memory;             /* Video memory in 64KB blocks */
    u16 software_rev;             /* Software revision */
    u32 vendor;                   /* Vendor */
    u32 product_name;             /* Product name */
    u32 product_rev;              /* Product revision */
    u8 reserved[222];             /* Reserved */
    u8 oem_data[256];             /* OEM data */
} __attribute__((packed)) multiboot_vbe_info_t;

/* Multiboot VBE mode info */
typedef struct multiboot_vbe_mode_info {
    u16 attributes;               /* Mode attributes */
    u8 window_a;                  /* Window A attributes */
    u8 window_b;                  /* Window B attributes */
    u16 granularity;              /* Window granularity */
    u16 window_size;              /* Window size */
    u16 segment_a;                /* Window A start segment */
    u16 segment_b;                /* Window B start segment */
    u32 window_func;              /* Window function pointer */
    u16 pitch;                    /* Bytes per scan line */
    u16 width;                    /* Width in pixels */
    u16 height;                   /* Height in pixels */
    u8 w_char;                    /* Character cell width */
    u8 y_char;                    /* Character cell height */
    u8 planes;                    /* Number of memory planes */
    u8 bpp;                       /* Bits per pixel */
    u8 banks;                     /* Number of banks */
    u8 memory_model;              /* Memory model type */
    u8 bank_size;                 /* Bank size in KB */
    u8 image_pages;               /* Number of image pages */
    u8 reserved0;                 /* Reserved */
    u8 red_mask;                  /* Red mask size */
    u8 red_position;              /* Red field position */
    u8 green_mask;                /* Green mask size */
    u8 green_position;            /* Green field position */
    u8 blue_mask;                 /* Blue mask size */
    u8 blue_position;             /* Blue field position */
    u8 reserved_mask;             /* Reserved mask size */
    u8 reserved_position;         /* Reserved field position */
    u8 direct_color_attributes;   /* Direct color mode attributes */
    u32 framebuffer;              /* Physical address of framebuffer */
    u32 off_screen_mem;           /* Off-screen memory address */
    u16 off_screen_mem_size;      /* Off-screen memory size in KB */
    u8 reserved1[206];            /* Reserved */
} __attribute__((packed)) multiboot_vbe_mode_info_t;

/* Multiboot functions */
void multiboot_init(u32 magic, multiboot_info_t *mbi);
const char *multiboot_get_cmdline(void);
const char *multiboot_get_bootloader(void);
u32 multiboot_get_mem_lower(void);
u32 multiboot_get_mem_upper(void);
u32 multiboot_get_mods_count(void);
multiboot_module_t *multiboot_get_mod(u32 index);
const char *multiboot_get_mod_cmdline(u32 index);
void multiboot_parse_mmap(void);
void multiboot_print_info(void);

#endif /* _HORIZON_MULTIBOOT_H */

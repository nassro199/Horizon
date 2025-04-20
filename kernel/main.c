/**
 * main.c - Horizon kernel main entry point
 *
 * This file contains the kernel main entry point.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Multiboot header structure */
typedef struct multiboot_info {
    u32 flags;                  /* Multiboot info flags */
    u32 mem_lower;              /* Amount of lower memory */
    u32 mem_upper;              /* Amount of upper memory */
    u32 boot_device;            /* Boot device */
    u32 cmdline;                /* Command line */
    u32 mods_count;             /* Number of modules */
    u32 mods_addr;              /* Module address */
    u32 syms[4];                /* Symbol table info */
    u32 mmap_length;            /* Memory map length */
    u32 mmap_addr;              /* Memory map address */
    u32 drives_length;          /* Drives info length */
    u32 drives_addr;            /* Drives info address */
    u32 config_table;           /* Configuration table */
    u32 boot_loader_name;       /* Boot loader name */
    u32 apm_table;              /* APM table */
    u32 vbe_control_info;       /* VBE control info */
    u32 vbe_mode_info;          /* VBE mode info */
    u16 vbe_mode;               /* VBE mode */
    u16 vbe_interface_seg;      /* VBE interface segment */
    u16 vbe_interface_off;      /* VBE interface offset */
    u16 vbe_interface_len;      /* VBE interface length */
} multiboot_info_t;

/* Memory map entry structure */
typedef struct memory_map {
    u32 size;                   /* Size of this entry */
    u64 base_addr;              /* Base address */
    u64 length;                 /* Region length */
    u32 type;                   /* Region type */
} __attribute__((packed)) memory_map_t;

/* External functions */
extern void arch_setup(void);
extern void interrupt_init(void);
extern void vga_init(void);
extern void keyboard_init(void);
extern void keyboard_handler_init(void);
extern void shell_init(void);
extern void capability_init(void);
extern void uhci_driver_init(void);

/* Global variables */
static multiboot_info_t *multiboot_info = NULL;
static u32 total_memory = 0;

/* Early console print function */
static void early_print(const char *str) {
    /* This assumes VGA text mode is already set up by the bootloader */
    static u16 *vga_buffer = (u16 *)0xB8000;
    static u32 pos = 0;

    for (u32 i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            /* Move to the next line */
            pos = (pos / 80 + 1) * 80;
        } else {
            /* Print the character */
            vga_buffer[pos++] = (u16)str[i] | 0x0700;
        }

        /* Handle scrolling */
        if (pos >= 80 * 25) {
            /* Move everything up one line */
            for (u32 j = 0; j < 80 * 24; j++) {
                vga_buffer[j] = vga_buffer[j + 80];
            }

            /* Clear the last line */
            for (u32 j = 80 * 24; j < 80 * 25; j++) {
                vga_buffer[j] = 0x0720; /* Space with white on black */
            }

            /* Adjust position */
            pos = 80 * 24;
        }
    }
}

/* Parse the multiboot information */
static void parse_multiboot_info(multiboot_info_t *mbi) {
    /* Store the multiboot info pointer */
    multiboot_info = mbi;

    /* Print multiboot info */
    early_print("Multiboot information:\n");

    /* Check memory info */
    if (mbi->flags & 0x1) {
        early_print("Memory: Lower = ");
        /* Convert to string and print */
        char buf[16];
        u32 val = mbi->mem_lower;
        u32 pos = 0;
        do {
            buf[pos++] = '0' + (val % 10);
            val /= 10;
        } while (val > 0);
        for (u32 i = 0; i < pos; i++) {
            char c[2] = { buf[pos - i - 1], '\0' };
            early_print(c);
        }
        early_print(" KB, Upper = ");

        val = mbi->mem_upper;
        pos = 0;
        do {
            buf[pos++] = '0' + (val % 10);
            val /= 10;
        } while (val > 0);
        for (u32 i = 0; i < pos; i++) {
            char c[2] = { buf[pos - i - 1], '\0' };
            early_print(c);
        }
        early_print(" KB\n");

        /* Calculate total memory */
        total_memory = mbi->mem_lower + mbi->mem_upper;
    }

    /* Check memory map */
    if (mbi->flags & 0x40) {
        early_print("Memory map available\n");

        /* Parse memory map */
        memory_map_t *mmap = (memory_map_t *)mbi->mmap_addr;
        while ((u32)mmap < mbi->mmap_addr + mbi->mmap_length) {
            /* Process memory map entry */
            if (mmap->type == 1) {
                /* Available memory */
                /* This would be used to initialize the physical memory manager */
            }

            /* Move to the next entry */
            mmap = (memory_map_t *)((u32)mmap + mmap->size + 4);
        }
    }
}

/* External functions */
extern void kernel_init(void);

/* Kernel main entry point */
void kernel_main(u32 magic, multiboot_info_t *mbi) {
    /* Check multiboot magic number */
    if (magic != 0x2BADB002) {
        /* Invalid magic number, halt the system */
        for (;;) {
            __asm__ volatile("hlt");
        }
    }

    /* Parse multiboot information */
    parse_multiboot_info(mbi);

    /* Architecture-specific setup */
    early_print("Initializing architecture-specific features...\n");
    arch_setup();

    /* Initialize interrupts */
    early_print("Initializing interrupt system...\n");
    interrupt_init();

    /* Initialize the VGA console */
    early_print("Initializing VGA console...\n");
    vga_init();

    /* Initialize the kernel */
    early_print("Initializing Horizon kernel subsystems...\n");
    kernel_init();

    /* Enable interrupts */
    early_print("Enabling interrupts...\n");
    __asm__ volatile("sti");

    early_print("Horizon kernel initialization complete.\n");
    early_print("Welcome to Horizon OS!\n");

    /* Main loop */
    for (;;) {
        /* Halt the CPU until an interrupt occurs */
        __asm__ volatile("hlt");
    }
}

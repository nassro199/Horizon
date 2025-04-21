/**
 * elf.c - Horizon kernel ELF loader implementation
 *
 * This file contains the implementation of the ELF (Executable and Linkable Format) loader.
 * The implementation is compatible with Linux.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/elf.h>
#include <horizon/mm.h>
#include <horizon/vmm.h>
#include <horizon/fs.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Check if an ELF header is valid */
int elf_check_header(Elf32_Ehdr *ehdr) {
    if (ehdr == NULL) {
        return -1;
    }

    /* Check ELF magic number */
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        return -1;
    }

    /* Check ELF class */
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        return -1;
    }

    /* Check ELF data encoding */
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        return -1;
    }

    /* Check ELF version */
    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
        return -1;
    }

    /* Check ELF type */
    if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
        return -1;
    }

    /* Check ELF machine */
    if (ehdr->e_machine != EM_386) {
        return -1;
    }

    /* Check ELF version */
    if (ehdr->e_version != EV_CURRENT) {
        return -1;
    }

    return 0;
}

/* Load an ELF file from a path */
int elf_load(const char *path, Elf32_Addr *entry) {
    if (path == NULL || entry == NULL) {
        return -1;
    }

    /* Open the file */
    file_t *file = fs_open(path, FILE_OPEN_READ);

    if (file == NULL) {
        return -1;
    }

    /* Get the file size */
    size_t size = file->size;

    if (size == 0) {
        fs_close(file);
        return -1;
    }

    /* Allocate memory for the file */
    void *buffer = kmalloc(size, MEM_KERNEL);

    if (buffer == NULL) {
        fs_close(file);
        return -1;
    }

    /* Read the file */
    if (fs_read(file, buffer, size) != size) {
        kfree(buffer);
        fs_close(file);
        return -1;
    }

    /* Close the file */
    fs_close(file);

    /* Load the ELF file */
    int result = elf_load_file(buffer, size, entry);

    /* Free the buffer */
    kfree(buffer);

    return result;
}

/* Load an ELF file from memory */
int elf_load_file(void *file, size_t size, Elf32_Addr *entry) {
    if (file == NULL || size == 0 || entry == NULL) {
        return -1;
    }

    /* Get the ELF header */
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)file;

    /* Check the ELF header */
    if (elf_check_header(ehdr) < 0) {
        return -1;
    }

    /* Set the entry point */
    *entry = ehdr->e_entry;

    /* Get the program headers */
    Elf32_Phdr *phdr = (Elf32_Phdr *)((u8 *)file + ehdr->e_phoff);

    /* Load each program segment */
    for (u32 i = 0; i < ehdr->e_phnum; i++) {
        /* Skip non-loadable segments */
        if (phdr[i].p_type != PT_LOAD) {
            continue;
        }

        /* Get the segment address and size */
        void *vaddr = (void *)phdr[i].p_vaddr;
        size_t memsz = phdr[i].p_memsz;
        size_t filesz = phdr[i].p_filesz;

        /* Allocate memory for the segment */
        void *segment = vmm_alloc_pages(NULL, vaddr, (memsz + PAGE_SIZE - 1) / PAGE_SIZE, 0);

        if (segment == NULL) {
            return -1;
        }

        /* Copy the segment data */
        memcpy(segment, (u8 *)file + phdr[i].p_offset, filesz);

        /* Zero the rest of the segment */
        if (memsz > filesz) {
            memset((u8 *)segment + filesz, 0, memsz - filesz);
        }

        /* Set the segment permissions */
        u32 flags = 0;

        if (phdr[i].p_flags & PF_R) {
            flags |= PROT_READ;
        }

        if (phdr[i].p_flags & PF_W) {
            flags |= PROT_WRITE;
        }

        if (phdr[i].p_flags & PF_X) {
            flags |= PROT_EXEC;
        }

        /* Create a virtual memory area for the segment */
        vm_area_struct_t *vma = vmm_create_vma(NULL, vaddr, memsz, flags);

        if (vma == NULL) {
            vmm_free_pages(NULL, vaddr, (memsz + PAGE_SIZE - 1) / PAGE_SIZE);
            return -1;
        }
    }

    return 0;
}

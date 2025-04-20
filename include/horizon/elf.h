/**
 * elf.h - Horizon kernel ELF format definitions
 * 
 * This file contains definitions for the ELF (Executable and Linkable Format) format.
 * The definitions are compatible with Linux.
 */

#ifndef _KERNEL_ELF_H
#define _KERNEL_ELF_H

#include <horizon/types.h>

/* ELF types */
typedef u32 Elf32_Addr;   /* Program address */
typedef u16 Elf32_Half;   /* Half word */
typedef u32 Elf32_Off;    /* File offset */
typedef s32 Elf32_Sword;  /* Signed word */
typedef u32 Elf32_Word;   /* Unsigned word */

/* ELF header */
#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT]; /* ELF identification */
    Elf32_Half    e_type;             /* Object file type */
    Elf32_Half    e_machine;          /* Machine type */
    Elf32_Word    e_version;          /* Object file version */
    Elf32_Addr    e_entry;            /* Entry point address */
    Elf32_Off     e_phoff;            /* Program header offset */
    Elf32_Off     e_shoff;            /* Section header offset */
    Elf32_Word    e_flags;            /* Processor-specific flags */
    Elf32_Half    e_ehsize;           /* ELF header size */
    Elf32_Half    e_phentsize;        /* Size of program header entry */
    Elf32_Half    e_phnum;            /* Number of program header entries */
    Elf32_Half    e_shentsize;        /* Size of section header entry */
    Elf32_Half    e_shnum;            /* Number of section header entries */
    Elf32_Half    e_shstrndx;         /* Section name string table index */
} Elf32_Ehdr;

/* ELF identification */
#define EI_MAG0         0  /* File identification */
#define EI_MAG1         1  /* File identification */
#define EI_MAG2         2  /* File identification */
#define EI_MAG3         3  /* File identification */
#define EI_CLASS        4  /* File class */
#define EI_DATA         5  /* Data encoding */
#define EI_VERSION      6  /* File version */
#define EI_OSABI        7  /* OS/ABI identification */
#define EI_ABIVERSION   8  /* ABI version */
#define EI_PAD          9  /* Start of padding bytes */

/* ELF magic number */
#define ELFMAG0         0x7F  /* e_ident[EI_MAG0] */
#define ELFMAG1         'E'   /* e_ident[EI_MAG1] */
#define ELFMAG2         'L'   /* e_ident[EI_MAG2] */
#define ELFMAG3         'F'   /* e_ident[EI_MAG3] */
#define ELFMAG          "\177ELF"

/* ELF file class */
#define ELFCLASSNONE    0  /* Invalid class */
#define ELFCLASS32      1  /* 32-bit objects */
#define ELFCLASS64      2  /* 64-bit objects */

/* ELF data encoding */
#define ELFDATANONE     0  /* Invalid data encoding */
#define ELFDATA2LSB     1  /* 2's complement, little endian */
#define ELFDATA2MSB     2  /* 2's complement, big endian */

/* ELF object file type */
#define ET_NONE         0  /* No file type */
#define ET_REL          1  /* Relocatable file */
#define ET_EXEC         2  /* Executable file */
#define ET_DYN          3  /* Shared object file */
#define ET_CORE         4  /* Core file */
#define ET_LOPROC       0xFF00  /* Processor-specific */
#define ET_HIPROC       0xFFFF  /* Processor-specific */

/* ELF machine type */
#define EM_NONE         0  /* No machine */
#define EM_M32          1  /* AT&T WE 32100 */
#define EM_SPARC        2  /* SPARC */
#define EM_386          3  /* Intel 80386 */
#define EM_68K          4  /* Motorola 68000 */
#define EM_88K          5  /* Motorola 88000 */
#define EM_860          7  /* Intel 80860 */
#define EM_MIPS         8  /* MIPS RS3000 */
#define EM_MIPS_RS4_BE  10 /* MIPS RS4000 Big Endian */
#define EM_PPC          20 /* PowerPC */
#define EM_PPC64        21 /* PowerPC 64-bit */
#define EM_ARM          40 /* ARM */
#define EM_X86_64       62 /* AMD x86-64 */

/* ELF version */
#define EV_NONE         0  /* Invalid version */
#define EV_CURRENT      1  /* Current version */

/* ELF program header */
typedef struct {
    Elf32_Word    p_type;    /* Segment type */
    Elf32_Off     p_offset;  /* Segment file offset */
    Elf32_Addr    p_vaddr;   /* Segment virtual address */
    Elf32_Addr    p_paddr;   /* Segment physical address */
    Elf32_Word    p_filesz;  /* Segment size in file */
    Elf32_Word    p_memsz;   /* Segment size in memory */
    Elf32_Word    p_flags;   /* Segment flags */
    Elf32_Word    p_align;   /* Segment alignment */
} Elf32_Phdr;

/* ELF program header types */
#define PT_NULL         0  /* Unused entry */
#define PT_LOAD         1  /* Loadable segment */
#define PT_DYNAMIC      2  /* Dynamic linking information */
#define PT_INTERP       3  /* Interpreter pathname */
#define PT_NOTE         4  /* Auxiliary information */
#define PT_SHLIB        5  /* Reserved */
#define PT_PHDR         6  /* Program header table */
#define PT_TLS          7  /* Thread-Local Storage template */
#define PT_LOOS         0x60000000  /* OS-specific */
#define PT_HIOS         0x6FFFFFFF  /* OS-specific */
#define PT_LOPROC       0x70000000  /* Processor-specific */
#define PT_HIPROC       0x7FFFFFFF  /* Processor-specific */

/* ELF program header flags */
#define PF_X            0x1  /* Execute */
#define PF_W            0x2  /* Write */
#define PF_R            0x4  /* Read */
#define PF_MASKOS       0x0FF00000  /* OS-specific */
#define PF_MASKPROC     0xF0000000  /* Processor-specific */

/* ELF section header */
typedef struct {
    Elf32_Word    sh_name;      /* Section name */
    Elf32_Word    sh_type;      /* Section type */
    Elf32_Word    sh_flags;     /* Section flags */
    Elf32_Addr    sh_addr;      /* Section virtual address */
    Elf32_Off     sh_offset;    /* Section file offset */
    Elf32_Word    sh_size;      /* Section size in bytes */
    Elf32_Word    sh_link;      /* Link to another section */
    Elf32_Word    sh_info;      /* Additional section information */
    Elf32_Word    sh_addralign; /* Section alignment */
    Elf32_Word    sh_entsize;   /* Entry size if section holds table */
} Elf32_Shdr;

/* ELF section header types */
#define SHT_NULL        0  /* Section header table entry unused */
#define SHT_PROGBITS    1  /* Program data */
#define SHT_SYMTAB      2  /* Symbol table */
#define SHT_STRTAB      3  /* String table */
#define SHT_RELA        4  /* Relocation entries with addends */
#define SHT_HASH        5  /* Symbol hash table */
#define SHT_DYNAMIC     6  /* Dynamic linking information */
#define SHT_NOTE        7  /* Notes */
#define SHT_NOBITS      8  /* Program space with no data (bss) */
#define SHT_REL         9  /* Relocation entries, no addends */
#define SHT_SHLIB       10 /* Reserved */
#define SHT_DYNSYM      11 /* Dynamic linker symbol table */
#define SHT_INIT_ARRAY  14 /* Array of constructors */
#define SHT_FINI_ARRAY  15 /* Array of destructors */
#define SHT_PREINIT_ARRAY 16 /* Array of pre-constructors */
#define SHT_GROUP       17 /* Section group */
#define SHT_SYMTAB_SHNDX 18 /* Extended section indices */
#define SHT_NUM         19 /* Number of defined types */
#define SHT_LOOS        0x60000000 /* Start OS-specific */
#define SHT_HIOS        0x6FFFFFFF /* End OS-specific */
#define SHT_LOPROC      0x70000000 /* Start processor-specific */
#define SHT_HIPROC      0x7FFFFFFF /* End processor-specific */
#define SHT_LOUSER      0x80000000 /* Start of application-specific */
#define SHT_HIUSER      0xFFFFFFFF /* End of application-specific */

/* ELF section header flags */
#define SHF_WRITE       0x1 /* Writable */
#define SHF_ALLOC       0x2 /* Occupies memory during execution */
#define SHF_EXECINSTR   0x4 /* Executable */
#define SHF_MERGE       0x10 /* Might be merged */
#define SHF_STRINGS     0x20 /* Contains nul-terminated strings */
#define SHF_INFO_LINK   0x40 /* 'sh_info' contains SHT index */
#define SHF_LINK_ORDER  0x80 /* Preserve order after combining */
#define SHF_OS_NONCONFORMING 0x100 /* Non-standard OS specific handling required */
#define SHF_GROUP       0x200 /* Section is member of a group */
#define SHF_TLS         0x400 /* Section hold thread-local data */
#define SHF_MASKOS      0x0FF00000 /* OS-specific */
#define SHF_MASKPROC    0xF0000000 /* Processor-specific */

/* ELF symbol table entry */
typedef struct {
    Elf32_Word    st_name;  /* Symbol name */
    Elf32_Addr    st_value; /* Symbol value */
    Elf32_Word    st_size;  /* Symbol size */
    unsigned char st_info;  /* Symbol type and binding */
    unsigned char st_other; /* Symbol visibility */
    Elf32_Half    st_shndx; /* Section index */
} Elf32_Sym;

/* ELF symbol bindings */
#define STB_LOCAL       0  /* Local symbol */
#define STB_GLOBAL      1  /* Global symbol */
#define STB_WEAK        2  /* Weak symbol */
#define STB_LOOS        10 /* OS-specific */
#define STB_HIOS        12 /* OS-specific */
#define STB_LOPROC      13 /* Processor-specific */
#define STB_HIPROC      15 /* Processor-specific */

/* ELF symbol types */
#define STT_NOTYPE      0  /* Symbol type is unspecified */
#define STT_OBJECT      1  /* Symbol is a data object */
#define STT_FUNC        2  /* Symbol is a code object */
#define STT_SECTION     3  /* Symbol associated with a section */
#define STT_FILE        4  /* Symbol's name is file name */
#define STT_COMMON      5  /* Symbol is a common data object */
#define STT_TLS         6  /* Symbol is thread-local data object */
#define STT_LOOS        10 /* OS-specific */
#define STT_HIOS        12 /* OS-specific */
#define STT_LOPROC      13 /* Processor-specific */
#define STT_HIPROC      15 /* Processor-specific */

/* ELF symbol info macros */
#define ELF32_ST_BIND(i)    ((i) >> 4)
#define ELF32_ST_TYPE(i)    ((i) & 0xF)
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xF))

/* ELF relocation entry */
typedef struct {
    Elf32_Addr    r_offset; /* Address */
    Elf32_Word    r_info;   /* Relocation type and symbol index */
} Elf32_Rel;

/* ELF relocation entry with addend */
typedef struct {
    Elf32_Addr    r_offset; /* Address */
    Elf32_Word    r_info;   /* Relocation type and symbol index */
    Elf32_Sword   r_addend; /* Addend */
} Elf32_Rela;

/* ELF relocation info macros */
#define ELF32_R_SYM(i)      ((i) >> 8)
#define ELF32_R_TYPE(i)     ((unsigned char)(i))
#define ELF32_R_INFO(s, t)  (((s) << 8) + (unsigned char)(t))

/* ELF dynamic section entry */
typedef struct {
    Elf32_Sword   d_tag;    /* Dynamic entry type */
    union {
        Elf32_Word d_val;   /* Integer value */
        Elf32_Addr d_ptr;   /* Address value */
    } d_un;
} Elf32_Dyn;

/* ELF dynamic entry types */
#define DT_NULL         0  /* Marks end of dynamic section */
#define DT_NEEDED       1  /* Name of needed library */
#define DT_PLTRELSZ     2  /* Size in bytes of PLT relocs */
#define DT_PLTGOT       3  /* Processor defined value */
#define DT_HASH         4  /* Address of symbol hash table */
#define DT_STRTAB       5  /* Address of string table */
#define DT_SYMTAB       6  /* Address of symbol table */
#define DT_RELA         7  /* Address of Rela relocs */
#define DT_RELASZ       8  /* Total size of Rela relocs */
#define DT_RELAENT      9  /* Size of one Rela reloc */
#define DT_STRSZ        10 /* Size of string table */
#define DT_SYMENT       11 /* Size of one symbol table entry */
#define DT_INIT         12 /* Address of init function */
#define DT_FINI         13 /* Address of termination function */
#define DT_SONAME       14 /* Name of shared object */
#define DT_RPATH        15 /* Library search path (deprecated) */
#define DT_SYMBOLIC     16 /* Start symbol search here */
#define DT_REL          17 /* Address of Rel relocs */
#define DT_RELSZ        18 /* Total size of Rel relocs */
#define DT_RELENT       19 /* Size of one Rel reloc */
#define DT_PLTREL       20 /* Type of reloc in PLT */
#define DT_DEBUG        21 /* For debugging; unspecified */
#define DT_TEXTREL      22 /* Reloc might modify .text */
#define DT_JMPREL       23 /* Address of PLT relocs */
#define DT_BIND_NOW     24 /* Process relocations of object */
#define DT_INIT_ARRAY   25 /* Array with addresses of init fct */
#define DT_FINI_ARRAY   26 /* Array with addresses of fini fct */
#define DT_INIT_ARRAYSZ 27 /* Size in bytes of DT_INIT_ARRAY */
#define DT_FINI_ARRAYSZ 28 /* Size in bytes of DT_FINI_ARRAY */
#define DT_RUNPATH      29 /* Library search path */
#define DT_FLAGS        30 /* Flags for the object being loaded */
#define DT_ENCODING     32 /* Start of encoded range */
#define DT_PREINIT_ARRAY 32 /* Array with addresses of preinit fct */
#define DT_PREINIT_ARRAYSZ 33 /* Size in bytes of DT_PREINIT_ARRAY */
#define DT_LOOS         0x6000000D /* Start of OS-specific */
#define DT_HIOS         0x6ffff000 /* End of OS-specific */
#define DT_LOPROC       0x70000000 /* Start of processor-specific */
#define DT_HIPROC       0x7FFFFFFF /* End of processor-specific */

/* ELF functions */
int elf_check_header(Elf32_Ehdr *ehdr);
int elf_load(const char *path, Elf32_Addr *entry);
int elf_load_file(void *file, size_t size, Elf32_Addr *entry);

#endif /* _KERNEL_ELF_H */

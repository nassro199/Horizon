/**
 * exec.c - Horizon kernel process execution implementation
 * 
 * This file contains the implementation of the process execution subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/process.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/fs/vfs.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Initialize the process execution subsystem */
void process_exec_init(void) {
    /* Initialize the process execution subsystem */
}

/* Execute a program */
int process_execve(const char *filename, char *const argv[], char *const envp[]) {
    /* Check if the parameters are valid */
    if (filename == NULL) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Open the file */
    file_t *file;
    int error = file_open(filename, O_RDONLY, 0, &file);
    
    if (error) {
        return error;
    }
    
    /* Check if the file is executable */
    if (!S_ISREG(file->f_inode->i_mode) || !(file->f_inode->i_mode & S_IXUSR)) {
        file_close(file);
        return -1;
    }
    
    /* Read the file header */
    char header[4];
    ssize_t bytes = file_read(file, header, sizeof(header));
    
    if (bytes != sizeof(header)) {
        file_close(file);
        return -1;
    }
    
    /* Check if the file is an ELF file */
    if (header[0] != 0x7F || header[1] != 'E' || header[2] != 'L' || header[3] != 'F') {
        file_close(file);
        return -1;
    }
    
    /* Free the old memory */
    vmm_free_mm(task->mm);
    
    /* Create a new memory context */
    task->mm = vmm_create_mm();
    
    if (task->mm == NULL) {
        file_close(file);
        return -1;
    }
    
    /* Load the program */
    error = process_load_elf(task, file);
    
    if (error) {
        vmm_free_mm(task->mm);
        file_close(file);
        return error;
    }
    
    /* Close the file */
    file_close(file);
    
    /* Set the arguments */
    process_set_args(task, argv, envp);
    
    /* Reset the signal handlers */
    process_reset_signals(task);
    
    /* Start the program */
    process_start(task);
    
    return 0;
}

/* Load an ELF file */
int process_load_elf(task_struct_t *task, file_t *file) {
    /* Check if the parameters are valid */
    if (task == NULL || file == NULL) {
        return -1;
    }
    
    /* Read the ELF header */
    Elf32_Ehdr header;
    ssize_t bytes = file_read(file, (char *)&header, sizeof(header));
    
    if (bytes != sizeof(header)) {
        return -1;
    }
    
    /* Check if the ELF header is valid */
    if (header.e_ident[EI_MAG0] != ELFMAG0 || header.e_ident[EI_MAG1] != ELFMAG1 ||
        header.e_ident[EI_MAG2] != ELFMAG2 || header.e_ident[EI_MAG3] != ELFMAG3) {
        return -1;
    }
    
    /* Check if the ELF class is valid */
    if (header.e_ident[EI_CLASS] != ELFCLASS32) {
        return -1;
    }
    
    /* Check if the ELF data encoding is valid */
    if (header.e_ident[EI_DATA] != ELFDATA2LSB) {
        return -1;
    }
    
    /* Check if the ELF version is valid */
    if (header.e_ident[EI_VERSION] != EV_CURRENT) {
        return -1;
    }
    
    /* Check if the ELF type is valid */
    if (header.e_type != ET_EXEC) {
        return -1;
    }
    
    /* Check if the ELF machine is valid */
    if (header.e_machine != EM_386) {
        return -1;
    }
    
    /* Check if the ELF version is valid */
    if (header.e_version != EV_CURRENT) {
        return -1;
    }
    
    /* Set the entry point */
    task->entry = (void *)header.e_entry;
    
    /* Load the program headers */
    for (int i = 0; i < header.e_phnum; i++) {
        /* Seek to the program header */
        file_seek(file, header.e_phoff + i * header.e_phentsize, SEEK_SET);
        
        /* Read the program header */
        Elf32_Phdr phdr;
        bytes = file_read(file, (char *)&phdr, sizeof(phdr));
        
        if (bytes != sizeof(phdr)) {
            return -1;
        }
        
        /* Check if the program header is loadable */
        if (phdr.p_type != PT_LOAD) {
            continue;
        }
        
        /* Calculate the memory protection */
        int prot = 0;
        
        if (phdr.p_flags & PF_R) {
            prot |= PROT_READ;
        }
        
        if (phdr.p_flags & PF_W) {
            prot |= PROT_WRITE;
        }
        
        if (phdr.p_flags & PF_X) {
            prot |= PROT_EXEC;
        }
        
        /* Allocate memory for the segment */
        void *addr = vmm_mmap(task->mm, (void *)phdr.p_vaddr, phdr.p_memsz, prot, MAP_FIXED | MAP_PRIVATE, NULL, 0, NULL);
        
        if (addr == NULL) {
            return -1;
        }
        
        /* Seek to the segment data */
        file_seek(file, phdr.p_offset, SEEK_SET);
        
        /* Read the segment data */
        bytes = file_read(file, (char *)addr, phdr.p_filesz);
        
        if (bytes != phdr.p_filesz) {
            return -1;
        }
        
        /* Zero the rest of the segment */
        if (phdr.p_memsz > phdr.p_filesz) {
            memset((char *)addr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz);
        }
    }
    
    return 0;
}

/* Set the arguments */
void process_set_args(task_struct_t *task, char *const argv[], char *const envp[]) {
    /* Check if the parameters are valid */
    if (task == NULL) {
        return;
    }
    
    /* Count the arguments */
    int argc = 0;
    
    if (argv != NULL) {
        while (argv[argc] != NULL) {
            argc++;
        }
    }
    
    /* Count the environment variables */
    int envc = 0;
    
    if (envp != NULL) {
        while (envp[envc] != NULL) {
            envc++;
        }
    }
    
    /* Allocate memory for the arguments */
    char **new_argv = kmalloc((argc + 1) * sizeof(char *), MEM_KERNEL | MEM_ZERO);
    
    if (new_argv == NULL) {
        return;
    }
    
    /* Copy the arguments */
    for (int i = 0; i < argc; i++) {
        /* Allocate memory for the argument */
        new_argv[i] = kmalloc(strlen(argv[i]) + 1, MEM_KERNEL | MEM_ZERO);
        
        if (new_argv[i] == NULL) {
            /* Free the previous arguments */
            for (int j = 0; j < i; j++) {
                kfree(new_argv[j]);
            }
            
            kfree(new_argv);
            return;
        }
        
        /* Copy the argument */
        strcpy(new_argv[i], argv[i]);
    }
    
    /* Set the last argument to NULL */
    new_argv[argc] = NULL;
    
    /* Allocate memory for the environment variables */
    char **new_envp = kmalloc((envc + 1) * sizeof(char *), MEM_KERNEL | MEM_ZERO);
    
    if (new_envp == NULL) {
        /* Free the arguments */
        for (int i = 0; i < argc; i++) {
            kfree(new_argv[i]);
        }
        
        kfree(new_argv);
        return;
    }
    
    /* Copy the environment variables */
    for (int i = 0; i < envc; i++) {
        /* Allocate memory for the environment variable */
        new_envp[i] = kmalloc(strlen(envp[i]) + 1, MEM_KERNEL | MEM_ZERO);
        
        if (new_envp[i] == NULL) {
            /* Free the previous environment variables */
            for (int j = 0; j < i; j++) {
                kfree(new_envp[j]);
            }
            
            /* Free the arguments */
            for (int j = 0; j < argc; j++) {
                kfree(new_argv[j]);
            }
            
            kfree(new_envp);
            kfree(new_argv);
            return;
        }
        
        /* Copy the environment variable */
        strcpy(new_envp[i], envp[i]);
    }
    
    /* Set the last environment variable to NULL */
    new_envp[envc] = NULL;
    
    /* Free the old arguments */
    if (task->argv != NULL) {
        for (int i = 0; task->argv[i] != NULL; i++) {
            kfree(task->argv[i]);
        }
        
        kfree(task->argv);
    }
    
    /* Free the old environment variables */
    if (task->envp != NULL) {
        for (int i = 0; task->envp[i] != NULL; i++) {
            kfree(task->envp[i]);
        }
        
        kfree(task->envp);
    }
    
    /* Set the new arguments */
    task->argc = argc;
    task->argv = new_argv;
    
    /* Set the new environment variables */
    task->envc = envc;
    task->envp = new_envp;
}

/* Reset the signal handlers */
void process_reset_signals(task_struct_t *task) {
    /* Check if the parameters are valid */
    if (task == NULL) {
        return;
    }
    
    /* Reset the signal handlers */
    for (int i = 0; i < _NSIG; i++) {
        task->sigaction[i].sa_handler = SIG_DFL;
        task->sigaction[i].sa_flags = 0;
        memset(&task->sigaction[i].sa_mask, 0, sizeof(sigset_t));
    }
    
    /* Reset the signal mask */
    memset(&task->sigmask, 0, sizeof(sigset_t));
    
    /* Reset the pending signals */
    memset(&task->sigpending, 0, sizeof(sigset_t));
}

/* Start the program */
void process_start(task_struct_t *task) {
    /* Check if the parameters are valid */
    if (task == NULL) {
        return;
    }
    
    /* Set the instruction pointer */
    task->regs.eip = (u32)task->entry;
    
    /* Set the stack pointer */
    task->regs.esp = (u32)task->stack + TASK_STACK_SIZE;
    
    /* Set the flags */
    task->regs.eflags = 0x202; /* IF = 1, IOPL = 0 */
    
    /* Set the segment registers */
    task->regs.cs = 0x1B; /* User code segment */
    task->regs.ds = 0x23; /* User data segment */
    task->regs.es = 0x23; /* User data segment */
    task->regs.fs = 0x23; /* User data segment */
    task->regs.gs = 0x23; /* User data segment */
    task->regs.ss = 0x23; /* User data segment */
    
    /* Set the task state */
    task->state = TASK_RUNNING;
}

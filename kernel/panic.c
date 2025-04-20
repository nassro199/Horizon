/**
 * panic.c - Horizon kernel panic handling
 * 
 * This file contains the implementation of the kernel panic handling.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/string.h>
#include <horizon/task.h>
#include <horizon/console.h>

/* Kernel panic information */
typedef struct panic_info {
    const char *message;           /* Panic message */
    const char *file;              /* Source file */
    int line;                      /* Source line */
    void *caller;                  /* Caller address */
    struct task_struct *task;      /* Current task */
    unsigned long flags;           /* CPU flags */
    int cpu;                       /* CPU number */
} panic_info_t;

/* Last panic information */
static panic_info_t last_panic;

/* Panic in progress flag */
static int panic_in_progress = 0;

/**
 * Convert an integer to a string
 * 
 * @param value Value to convert
 * @param str String buffer
 * @param base Base (10 for decimal, 16 for hex)
 * @return String length
 */
static int int_to_str(unsigned long value, char *str, int base) {
    int i = 0;
    int j;
    char tmp;
    
    /* Handle 0 */
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return 1;
    }
    
    /* Convert to string (in reverse) */
    while (value > 0) {
        int digit = value % base;
        str[i++] = digit < 10 ? '0' + digit : 'A' + digit - 10;
        value /= base;
    }
    
    /* Add null terminator */
    str[i] = '\0';
    
    /* Reverse the string */
    for (j = 0; j < i / 2; j++) {
        tmp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = tmp;
    }
    
    return i;
}

/**
 * Kernel panic implementation
 * 
 * @param message Panic message
 * @param file Source file
 * @param line Source line
 * @param caller Caller address
 */
void __kernel_panic(const char *message, const char *file, int line, void *caller) {
    char buf[16];
    
    /* Check if panic is already in progress */
    if (panic_in_progress) {
        /* Double panic, just halt */
        for (;;) {
            __asm__ volatile("cli; hlt");
        }
    }
    
    /* Set panic in progress flag */
    panic_in_progress = 1;
    
    /* Save panic information */
    last_panic.message = message;
    last_panic.file = file;
    last_panic.line = line;
    last_panic.caller = caller;
    last_panic.task = current;
    last_panic.cpu = 0; /* TODO: Get current CPU */
    
    /* Disable interrupts */
    __asm__ volatile("pushf; pop %0; cli" : "=r" (last_panic.flags));
    
    /* Print panic message */
    console_write("\n\n");
    console_write("KERNEL PANIC: ");
    console_write(message);
    console_write("\n");
    
    /* Print file and line */
    console_write("At: ");
    console_write(file);
    console_write(":");
    int_to_str(line, buf, 10);
    console_write(buf);
    console_write("\n");
    
    /* Print caller address */
    console_write("Caller: 0x");
    int_to_str((unsigned long)caller, buf, 16);
    console_write(buf);
    console_write("\n");
    
    /* Print current task */
    if (last_panic.task != NULL) {
        console_write("Task: ");
        console_write(last_panic.task->comm);
        console_write(" (PID: ");
        int_to_str(last_panic.task->pid, buf, 10);
        console_write(buf);
        console_write(")\n");
    }
    
    console_write("\nSystem halted.\n");
    
    /* Halt the system */
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

/**
 * Get the last panic information
 * 
 * @return Last panic information
 */
const panic_info_t *kernel_panic_info(void) {
    return &last_panic;
}

/**
 * Check if panic is in progress
 * 
 * @return 1 if panic is in progress, 0 if not
 */
int kernel_panic_in_progress(void) {
    return panic_in_progress;
}

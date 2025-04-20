/**
 * shell.c - Simple shell for testing
 * 
 * This file contains a simple shell for testing the kernel.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>

/* External functions */
extern void vga_puts(const char *str);
extern void vga_set_color(u8 fg, u8 bg);

/* Shell buffer */
#define SHELL_BUFFER_SIZE 256
static char shell_buffer[SHELL_BUFFER_SIZE];
static u32 shell_buffer_pos = 0;

/* Shell commands */
typedef void (*shell_command_func_t)(int argc, char *argv[]);

typedef struct shell_command {
    const char *name;
    const char *help;
    shell_command_func_t func;
} shell_command_t;

/* Forward declarations for shell commands */
static void cmd_help(int argc, char *argv[]);
static void cmd_echo(int argc, char *argv[]);
static void cmd_clear(int argc, char *argv[]);
static void cmd_version(int argc, char *argv[]);

/* Shell command table */
static const shell_command_t shell_commands[] = {
    { "help", "Display help information", cmd_help },
    { "echo", "Display a message", cmd_echo },
    { "clear", "Clear the screen", cmd_clear },
    { "version", "Display kernel version", cmd_version },
    { NULL, NULL, NULL }
};

/* Help command */
static void cmd_help(int argc, char *argv[]) {
    vga_puts("Available commands:\n");
    
    for (u32 i = 0; shell_commands[i].name != NULL; i++) {
        vga_puts("  ");
        vga_puts(shell_commands[i].name);
        vga_puts(" - ");
        vga_puts(shell_commands[i].help);
        vga_puts("\n");
    }
}

/* Echo command */
static void cmd_echo(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        vga_puts(argv[i]);
        if (i < argc - 1) {
            vga_puts(" ");
        }
    }
    vga_puts("\n");
}

/* Clear command */
static void cmd_clear(int argc, char *argv[]) {
    /* This would call the VGA clear function */
    vga_puts("\033[2J\033[H");
}

/* Version command */
static void cmd_version(int argc, char *argv[]) {
    vga_puts("Horizon Kernel v");
    
    /* Convert version numbers to strings */
    char version[32];
    int pos = 0;
    
    /* Major version */
    int v = KERNEL_VERSION_MAJOR;
    if (v >= 10) {
        version[pos++] = '0' + (v / 10);
    }
    version[pos++] = '0' + (v % 10);
    version[pos++] = '.';
    
    /* Minor version */
    v = KERNEL_VERSION_MINOR;
    if (v >= 10) {
        version[pos++] = '0' + (v / 10);
    }
    version[pos++] = '0' + (v % 10);
    version[pos++] = '.';
    
    /* Patch version */
    v = KERNEL_VERSION_PATCH;
    if (v >= 10) {
        version[pos++] = '0' + (v / 10);
    }
    version[pos++] = '0' + (v % 10);
    version[pos] = '\0';
    
    vga_puts(version);
    vga_puts("\n");
}

/* Parse a command line */
static void shell_parse(char *line) {
    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    
    /* Check if the line is empty */
    if (*line == '\0') {
        return;
    }
    
    /* Parse arguments */
    int argc = 0;
    char *argv[16];
    
    /* Parse the command name */
    argv[argc++] = line;
    
    /* Parse the arguments */
    while (*line != '\0') {
        if (*line == ' ' || *line == '\t') {
            *line = '\0';
            line++;
            
            /* Skip whitespace */
            while (*line == ' ' || *line == '\t') {
                line++;
            }
            
            /* Check if we reached the end */
            if (*line == '\0') {
                break;
            }
            
            /* Add the argument */
            if (argc < 16) {
                argv[argc++] = line;
            }
        } else {
            line++;
        }
    }
    
    /* Find the command */
    for (u32 i = 0; shell_commands[i].name != NULL; i++) {
        if (strcmp(argv[0], shell_commands[i].name) == 0) {
            /* Execute the command */
            shell_commands[i].func(argc, argv);
            return;
        }
    }
    
    /* Command not found */
    vga_puts("Unknown command: ");
    vga_puts(argv[0]);
    vga_puts("\n");
}

/* Process a character */
void shell_process_char(char c) {
    /* Handle special characters */
    if (c == '\n') {
        /* Execute the command */
        vga_puts("\n");
        shell_buffer[shell_buffer_pos] = '\0';
        shell_parse(shell_buffer);
        shell_buffer_pos = 0;
        vga_puts("$ ");
    } else if (c == '\b') {
        /* Backspace */
        if (shell_buffer_pos > 0) {
            shell_buffer_pos--;
            vga_puts("\b \b");
        }
    } else if (c >= ' ' && c <= '~') {
        /* Regular character */
        if (shell_buffer_pos < SHELL_BUFFER_SIZE - 1) {
            shell_buffer[shell_buffer_pos++] = c;
            /* Echo the character */
            char str[2] = { c, '\0' };
            vga_puts(str);
        }
    }
}

/* Initialize the shell */
void shell_init(void) {
    /* Clear the buffer */
    shell_buffer_pos = 0;
    
    /* Print the welcome message */
    vga_puts("Welcome to the Horizon Kernel Shell\n");
    vga_puts("Type 'help' for a list of commands\n");
    vga_puts("$ ");
}

/* Simple string comparison */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    
    return *s1 - *s2;
}

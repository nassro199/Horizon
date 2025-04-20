/**
 * boot.c - Horizon kernel boot implementation
 * 
 * This file contains the implementation of the boot process.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/boot.h>
#include <horizon/console.h>
#include <horizon/time.h>
#include <horizon/string.h>

/* Boot stage names */
const char *boot_stage_names[] = {
    "Early Boot",
    "Memory Initialization",
    "Console Initialization",
    "ACPI Initialization",
    "Interrupt Initialization",
    "Timer Initialization",
    "Scheduler Initialization",
    "Filesystem Initialization",
    "Driver Initialization",
    "Network Initialization",
    "Userspace Initialization",
    "Boot Complete"
};

/* Boot state */
static u32 boot_stage = BOOT_STAGE_EARLY;
static u32 boot_progress_percent = 0;
static u64 boot_start_time = 0;
static u64 boot_stage_start_time[BOOT_STAGE_COMPLETE + 1];
static u32 boot_splash_enabled = 0;
static u32 boot_log_enabled = 1;

/**
 * Initialize boot
 */
void boot_init(void) {
    /* Initialize boot start time */
    boot_start_time = get_timestamp();
    
    /* Initialize boot stage start times */
    for (u32 i = 0; i <= BOOT_STAGE_COMPLETE; i++) {
        boot_stage_start_time[i] = 0;
    }
    boot_stage_start_time[BOOT_STAGE_EARLY] = boot_start_time;
    
    /* Print boot header */
    early_console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_write_color("  _    _            _                   ____   _____ \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(" | |  | |          (_)                 / __ \\ / ____|\n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(" | |__| | ___  _ __ _ _______  _ __   | |  | | (___  \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(" |  __  |/ _ \\| '__| |_  / _ \\| '_ \\  | |  | |\\___ \\ \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(" | |  | | (_) | |  | |/ / (_) | | | | | |__| |____) |\n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(" |_|  |_|\\___/|_|  |_/___\\___/|_| |_|  \\____/|_____/ \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_write_color("                   Horizon OS v0.1                   \n", CONSOLE_YELLOW, CONSOLE_BLACK);
    early_console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Log boot start */
    boot_log("Horizon OS v0.1 booting...");
    boot_log_stage(BOOT_STAGE_EARLY);
}

/**
 * Set boot stage
 * 
 * @param stage Boot stage
 */
void boot_set_stage(u32 stage) {
    if (stage > BOOT_STAGE_COMPLETE) {
        stage = BOOT_STAGE_COMPLETE;
    }
    
    /* Set boot stage */
    boot_stage = stage;
    boot_stage_start_time[stage] = get_timestamp();
    
    /* Log boot stage */
    boot_log_stage(stage);
    
    /* Print boot stage */
    early_console_write_color("[ ", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_write_color(boot_stage_names[stage], CONSOLE_LIGHT_GREEN, CONSOLE_BLACK);
    early_console_write_color(" ]\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Update boot progress */
    boot_progress((stage * 100) / BOOT_STAGE_COMPLETE);
}

/**
 * Get boot stage
 * 
 * @return Boot stage
 */
u32 boot_get_stage(void) {
    return boot_stage;
}

/**
 * Set boot progress
 * 
 * @param percent Progress percentage
 */
void boot_progress(u32 percent) {
    if (percent > 100) {
        percent = 100;
    }
    
    /* Set boot progress */
    boot_progress_percent = percent;
    
    /* Log boot progress */
    boot_log_progress(percent);
    
    /* Print boot progress */
    early_console_write_color("Progress: [", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Print progress bar */
    for (u32 i = 0; i < 50; i++) {
        if (i < (percent / 2)) {
            early_console_write_color("=", CONSOLE_LIGHT_GREEN, CONSOLE_BLACK);
        } else {
            early_console_write_color(" ", CONSOLE_WHITE, CONSOLE_BLACK);
        }
    }
    
    early_console_write_color("] ", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_print_dec(percent);
    early_console_write_color("%\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Update boot splash */
    if (boot_splash_enabled) {
        boot_splash_update(percent);
    }
}

/**
 * Print boot message
 * 
 * @param message Message to print
 */
void boot_message(const char *message) {
    /* Print message */
    early_console_write_color("* ", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_write_color(message, CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Log message */
    boot_log(message);
}

/**
 * Print boot error
 * 
 * @param message Error message
 */
void boot_error(const char *message) {
    /* Print error */
    early_console_write_color("ERROR: ", CONSOLE_LIGHT_RED, CONSOLE_BLACK);
    early_console_write_color(message, CONSOLE_LIGHT_RED, CONSOLE_BLACK);
    early_console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Log error */
    char log_message[256];
    snprintf(log_message, sizeof(log_message), "ERROR: %s", message);
    boot_log(log_message);
}

/**
 * Print boot warning
 * 
 * @param message Warning message
 */
void boot_warning(const char *message) {
    /* Print warning */
    early_console_write_color("WARNING: ", CONSOLE_YELLOW, CONSOLE_BLACK);
    early_console_write_color(message, CONSOLE_YELLOW, CONSOLE_BLACK);
    early_console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Log warning */
    char log_message[256];
    snprintf(log_message, sizeof(log_message), "WARNING: %s", message);
    boot_log(log_message);
}

/**
 * Print boot info
 * 
 * @param message Info message
 */
void boot_info(const char *message) {
    /* Print info */
    early_console_write_color("INFO: ", CONSOLE_LIGHT_GREEN, CONSOLE_BLACK);
    early_console_write_color(message, CONSOLE_LIGHT_GREEN, CONSOLE_BLACK);
    early_console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Log info */
    char log_message[256];
    snprintf(log_message, sizeof(log_message), "INFO: %s", message);
    boot_log(log_message);
}

/**
 * Print boot debug
 * 
 * @param message Debug message
 */
void boot_debug(const char *message) {
    /* Print debug */
    early_console_write_color("DEBUG: ", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(message, CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Log debug */
    char log_message[256];
    snprintf(log_message, sizeof(log_message), "DEBUG: %s", message);
    boot_log(log_message);
}

/**
 * Show boot splash
 */
void boot_splash(void) {
    /* Enable boot splash */
    boot_splash_enabled = 1;
    
    /* Clear console */
    early_console_clear();
    
    /* Print boot splash */
    early_console_write_color("\n\n\n\n\n\n\n\n", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_write_color("  _    _            _                   ____   _____ \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(" | |  | |          (_)                 / __ \\ / ____|\n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(" | |__| | ___  _ __ _ _______  _ __   | |  | | (___  \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(" |  __  |/ _ \\| '__| |_  / _ \\| '_ \\  | |  | |\\___ \\ \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(" | |  | | (_) | |  | |/ / (_) | | | | | |__| |____) |\n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color(" |_|  |_|\\___/|_|  |_/___\\___/|_| |_|  \\____/|_____/ \n", CONSOLE_LIGHT_CYAN, CONSOLE_BLACK);
    early_console_write_color("\n", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_write_color("                   Horizon OS v0.1                   \n", CONSOLE_YELLOW, CONSOLE_BLACK);
    early_console_write_color("\n\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Print boot stage */
    early_console_write_color("Stage: ", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_write_color(boot_stage_names[boot_stage], CONSOLE_LIGHT_GREEN, CONSOLE_BLACK);
    early_console_write_color("\n\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Print progress bar */
    early_console_write_color("Progress: [", CONSOLE_WHITE, CONSOLE_BLACK);
    
    for (u32 i = 0; i < 50; i++) {
        if (i < (boot_progress_percent / 2)) {
            early_console_write_color("=", CONSOLE_LIGHT_GREEN, CONSOLE_BLACK);
        } else {
            early_console_write_color(" ", CONSOLE_WHITE, CONSOLE_BLACK);
        }
    }
    
    early_console_write_color("] ", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_print_dec(boot_progress_percent);
    early_console_write_color("%\n", CONSOLE_WHITE, CONSOLE_BLACK);
}

/**
 * Update boot splash
 * 
 * @param percent Progress percentage
 */
void boot_splash_update(u32 percent) {
    if (!boot_splash_enabled) {
        return;
    }
    
    /* Save cursor position */
    early_console_save_cursor();
    
    /* Set cursor position */
    early_console_set_cursor(0, 13);
    
    /* Print boot stage */
    early_console_write_color("Stage: ", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_write_color(boot_stage_names[boot_stage], CONSOLE_LIGHT_GREEN, CONSOLE_BLACK);
    early_console_write_color("                                        \n\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Print progress bar */
    early_console_write_color("Progress: [", CONSOLE_WHITE, CONSOLE_BLACK);
    
    for (u32 i = 0; i < 50; i++) {
        if (i < (percent / 2)) {
            early_console_write_color("=", CONSOLE_LIGHT_GREEN, CONSOLE_BLACK);
        } else {
            early_console_write_color(" ", CONSOLE_WHITE, CONSOLE_BLACK);
        }
    }
    
    early_console_write_color("] ", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_print_dec(percent);
    early_console_write_color("%  ", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Restore cursor position */
    early_console_restore_cursor();
}

/**
 * End boot splash
 */
void boot_splash_end(void) {
    /* Disable boot splash */
    boot_splash_enabled = 0;
    
    /* Clear console */
    early_console_clear();
}

/**
 * Get boot time
 * 
 * @return Boot time
 */
u64 boot_get_time(void) {
    return get_timestamp();
}

/**
 * Get boot start time
 * 
 * @return Boot start time
 */
u64 boot_get_start_time(void) {
    return boot_start_time;
}

/**
 * Get boot elapsed time
 * 
 * @return Boot elapsed time
 */
u64 boot_get_elapsed_time(void) {
    return get_timestamp() - boot_start_time;
}

/**
 * Print boot time
 */
void boot_print_time(void) {
    u64 elapsed = boot_get_elapsed_time();
    u32 ms = (u32)(elapsed / 1000);
    u32 s = ms / 1000;
    ms %= 1000;
    
    /* Print boot time */
    early_console_write_color("Boot time: ", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_print_dec(s);
    early_console_write_color(".", CONSOLE_WHITE, CONSOLE_BLACK);
    early_console_print_dec(ms);
    early_console_write_color(" seconds\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    /* Print stage times */
    early_console_write_color("Stage times:\n", CONSOLE_WHITE, CONSOLE_BLACK);
    
    for (u32 i = 0; i <= boot_stage; i++) {
        if (boot_stage_start_time[i] != 0) {
            u64 stage_elapsed;
            
            if (i == boot_stage) {
                stage_elapsed = get_timestamp() - boot_stage_start_time[i];
            } else {
                stage_elapsed = boot_stage_start_time[i + 1] - boot_stage_start_time[i];
            }
            
            u32 stage_ms = (u32)(stage_elapsed / 1000);
            u32 stage_s = stage_ms / 1000;
            stage_ms %= 1000;
            
            early_console_write_color("  ", CONSOLE_WHITE, CONSOLE_BLACK);
            early_console_write_color(boot_stage_names[i], CONSOLE_LIGHT_GREEN, CONSOLE_BLACK);
            early_console_write_color(": ", CONSOLE_WHITE, CONSOLE_BLACK);
            early_console_print_dec(stage_s);
            early_console_write_color(".", CONSOLE_WHITE, CONSOLE_BLACK);
            early_console_print_dec(stage_ms);
            early_console_write_color(" seconds\n", CONSOLE_WHITE, CONSOLE_BLACK);
        }
    }
    
    /* Log boot time */
    boot_log_time();
}

/**
 * Log boot message
 * 
 * @param message Message to log
 */
void boot_log(const char *message) {
    if (!boot_log_enabled) {
        return;
    }
    
    /* TODO: Implement boot log */
}

/**
 * Log boot stage
 * 
 * @param stage Boot stage
 */
void boot_log_stage(u32 stage) {
    if (!boot_log_enabled) {
        return;
    }
    
    /* Log boot stage */
    char log_message[256];
    snprintf(log_message, sizeof(log_message), "Boot stage: %s", boot_stage_names[stage]);
    boot_log(log_message);
}

/**
 * Log boot progress
 * 
 * @param percent Progress percentage
 */
void boot_log_progress(u32 percent) {
    if (!boot_log_enabled) {
        return;
    }
    
    /* Log boot progress */
    char log_message[256];
    snprintf(log_message, sizeof(log_message), "Boot progress: %u%%", percent);
    boot_log(log_message);
}

/**
 * Log boot time
 */
void boot_log_time(void) {
    if (!boot_log_enabled) {
        return;
    }
    
    /* Log boot time */
    u64 elapsed = boot_get_elapsed_time();
    u32 ms = (u32)(elapsed / 1000);
    u32 s = ms / 1000;
    ms %= 1000;
    
    char log_message[256];
    snprintf(log_message, sizeof(log_message), "Boot time: %u.%03u seconds", s, ms);
    boot_log(log_message);
    
    /* Log stage times */
    for (u32 i = 0; i <= boot_stage; i++) {
        if (boot_stage_start_time[i] != 0) {
            u64 stage_elapsed;
            
            if (i == boot_stage) {
                stage_elapsed = get_timestamp() - boot_stage_start_time[i];
            } else {
                stage_elapsed = boot_stage_start_time[i + 1] - boot_stage_start_time[i];
            }
            
            u32 stage_ms = (u32)(stage_elapsed / 1000);
            u32 stage_s = stage_ms / 1000;
            stage_ms %= 1000;
            
            snprintf(log_message, sizeof(log_message), "Stage time: %s: %u.%03u seconds", boot_stage_names[i], stage_s, stage_ms);
            boot_log(log_message);
        }
    }
}

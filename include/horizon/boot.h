/**
 * boot.h - Horizon kernel boot definitions
 * 
 * This file contains definitions for the boot process.
 */

#ifndef _HORIZON_BOOT_H
#define _HORIZON_BOOT_H

#include <horizon/types.h>

/* Boot stage constants */
#define BOOT_STAGE_EARLY          0  /* Early boot stage */
#define BOOT_STAGE_MEMORY         1  /* Memory initialization */
#define BOOT_STAGE_CONSOLE        2  /* Console initialization */
#define BOOT_STAGE_ACPI           3  /* ACPI initialization */
#define BOOT_STAGE_INTERRUPTS     4  /* Interrupt initialization */
#define BOOT_STAGE_TIMER          5  /* Timer initialization */
#define BOOT_STAGE_SCHEDULER      6  /* Scheduler initialization */
#define BOOT_STAGE_FILESYSTEM     7  /* Filesystem initialization */
#define BOOT_STAGE_DRIVERS        8  /* Driver initialization */
#define BOOT_STAGE_NETWORK        9  /* Network initialization */
#define BOOT_STAGE_USERSPACE      10 /* Userspace initialization */
#define BOOT_STAGE_COMPLETE       11 /* Boot complete */

/* Boot stage names */
extern const char *boot_stage_names[];

/* Boot progress functions */
void boot_init(void);
void boot_set_stage(u32 stage);
u32 boot_get_stage(void);
void boot_progress(u32 percent);
void boot_message(const char *message);
void boot_error(const char *message);
void boot_warning(const char *message);
void boot_info(const char *message);
void boot_debug(const char *message);
void boot_splash(void);
void boot_splash_update(u32 percent);
void boot_splash_end(void);
u64 boot_get_time(void);
u64 boot_get_start_time(void);
u64 boot_get_elapsed_time(void);
void boot_print_time(void);
void boot_log(const char *message);
void boot_log_stage(u32 stage);
void boot_log_progress(u32 percent);
void boot_log_time(void);

#endif /* _HORIZON_BOOT_H */

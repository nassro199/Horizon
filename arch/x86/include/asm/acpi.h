/**
 * acpi.h - x86 ACPI definitions
 * 
 * This file contains x86-specific ACPI definitions.
 */

#ifndef _ASM_ACPI_H
#define _ASM_ACPI_H

#include <horizon/types.h>

/* ACPI sleep states */
#define ACPI_STATE_S0      0  /* Working */
#define ACPI_STATE_S1      1  /* Sleeping with processor context maintained */
#define ACPI_STATE_S2      2  /* Sleeping with processor context lost */
#define ACPI_STATE_S3      3  /* Sleeping with processor and memory context lost */
#define ACPI_STATE_S4      4  /* Hibernation */
#define ACPI_STATE_S5      5  /* Soft off */

/* ACPI functions */
int acpi_init(void);
int acpi_power_off(void);
int acpi_reboot(void);
int acpi_sleep(unsigned int state);
int acpi_hibernate(void);
int acpi_resume(void);
int acpi_thaw(void);

#endif /* _ASM_ACPI_H */

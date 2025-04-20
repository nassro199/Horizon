/**
 * power.c - x86 power management implementation
 * 
 * This file contains the x86-specific power management implementation.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/power.h>
#include <asm/io.h>
#include <asm/acpi.h>

/**
 * Power off system
 */
void arch_power_off(void) {
    /* Try ACPI power off */
    if (acpi_power_off() == 0) {
        /* ACPI power off succeeded */
        return;
    }
    
    /* Try APM power off */
    outw(0xB004, 0x0001);
    
    /* Try keyboard controller power off */
    outb(0x64, 0xFE);
    
    /* Halt system */
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

/**
 * Reboot system
 */
void arch_power_reboot(void) {
    /* Try ACPI reboot */
    if (acpi_reboot() == 0) {
        /* ACPI reboot succeeded */
        return;
    }
    
    /* Try keyboard controller reboot */
    outb(0x64, 0xFE);
    
    /* Try PCI reboot */
    outb(0xCF9, 0x06);
    
    /* Try triple fault */
    __asm__ volatile("lidt %0; int $3" : : "m" (0));
    
    /* Halt system */
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

/**
 * Enter sleep state
 * 
 * @param state Sleep state
 * @return 0 on success, negative error code on failure
 */
int arch_power_sleep(unsigned int state) {
    /* Try ACPI sleep */
    return acpi_sleep(state);
}

/**
 * Enter hibernation state
 * 
 * @return 0 on success, negative error code on failure
 */
int arch_power_hibernate(void) {
    /* Try ACPI hibernation */
    return acpi_hibernate();
}

/**
 * Resume from sleep state
 * 
 * @return 0 on success, negative error code on failure
 */
int arch_power_resume(void) {
    /* Try ACPI resume */
    return acpi_resume();
}

/**
 * Resume from hibernation state
 * 
 * @return 0 on success, negative error code on failure
 */
int arch_power_thaw(void) {
    /* Try ACPI thaw */
    return acpi_thaw();
}

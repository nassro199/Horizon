/**
 * acpi.c - Horizon kernel ACPI implementation
 * 
 * This file contains the implementation of ACPI.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/acpi.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/console.h>
#include <horizon/io.h>

/* ACPI tables */
static acpi_rsdp_t *rsdp = NULL;
static acpi_rsdt_t *rsdt = NULL;
static acpi_xsdt_t *xsdt = NULL;
static acpi_fadt_t *fadt = NULL;
static acpi_madt_t *madt = NULL;
static acpi_hpet_t *hpet = NULL;

/* ACPI version */
static int acpi_version = 0;

/* ACPI enabled flag */
static int acpi_enabled = 0;

/**
 * Initialize ACPI
 */
void acpi_init(void) {
    early_console_print("Initializing ACPI...\n");
    
    /* Find RSDP */
    rsdp = acpi_find_rsdp();
    if (rsdp == NULL) {
        early_console_print("ACPI: RSDP not found\n");
        return;
    }
    
    /* Print RSDP information */
    early_console_print("ACPI: RSDP found at 0x");
    early_console_print_hex((u32)rsdp);
    early_console_print("\n");
    early_console_print("ACPI: OEM ID: ");
    char oem_id[7];
    memcpy(oem_id, rsdp->oem_id, 6);
    oem_id[6] = '\0';
    early_console_print(oem_id);
    early_console_print("\n");
    early_console_print("ACPI: Revision: ");
    early_console_print_dec(rsdp->revision);
    early_console_print("\n");
    
    /* Set ACPI version */
    acpi_version = rsdp->revision;
    
    /* Get RSDT/XSDT */
    if (acpi_version >= 2 && rsdp->xsdt_address != 0) {
        /* ACPI 2.0+ with XSDT */
        xsdt = (acpi_xsdt_t *)(u32)rsdp->xsdt_address;
        early_console_print("ACPI: XSDT found at 0x");
        early_console_print_hex((u32)xsdt);
        early_console_print("\n");
    } else {
        /* ACPI 1.0 with RSDT */
        rsdt = (acpi_rsdt_t *)rsdp->rsdt_address;
        early_console_print("ACPI: RSDT found at 0x");
        early_console_print_hex((u32)rsdt);
        early_console_print("\n");
    }
    
    /* Parse FADT */
    fadt = (acpi_fadt_t *)acpi_find_table(ACPI_FADT_SIGNATURE);
    if (fadt != NULL) {
        acpi_parse_fadt();
    }
    
    /* Parse MADT */
    madt = (acpi_madt_t *)acpi_find_table(ACPI_MADT_SIGNATURE);
    if (madt != NULL) {
        acpi_parse_madt();
    }
    
    /* Parse HPET */
    hpet = (acpi_hpet_t *)acpi_find_table(ACPI_HPET_SIGNATURE);
    if (hpet != NULL) {
        acpi_parse_hpet();
    }
    
    /* Enable ACPI */
    acpi_enable();
    
    early_console_print("ACPI initialized\n");
}

/**
 * Find RSDP
 * 
 * @return RSDP, or NULL if not found
 */
acpi_rsdp_t *acpi_find_rsdp(void) {
    /* Search in EBDA */
    u32 ebda = *((u16 *)0x40E) << 4;
    if (ebda) {
        for (u32 addr = ebda; addr < ebda + 1024; addr += 16) {
            if (memcmp((void *)addr, ACPI_RSDP_SIGNATURE, 8) == 0) {
                return (acpi_rsdp_t *)addr;
            }
        }
    }
    
    /* Search in BIOS area */
    for (u32 addr = 0xE0000; addr < 0x100000; addr += 16) {
        if (memcmp((void *)addr, ACPI_RSDP_SIGNATURE, 8) == 0) {
            return (acpi_rsdp_t *)addr;
        }
    }
    
    return NULL;
}

/**
 * Validate ACPI table checksum
 * 
 * @param table Table to validate
 * @param length Table length
 * @return 1 if valid, 0 if invalid
 */
static int acpi_validate_table(void *table, u32 length) {
    u8 sum = 0;
    for (u32 i = 0; i < length; i++) {
        sum += ((u8 *)table)[i];
    }
    return sum == 0;
}

/**
 * Find ACPI table
 * 
 * @param signature Table signature
 * @return Table, or NULL if not found
 */
acpi_table_header_t *acpi_find_table(const char *signature) {
    if (xsdt != NULL) {
        /* Search in XSDT */
        u32 entries = (xsdt->header.length - sizeof(acpi_table_header_t)) / sizeof(u64);
        for (u32 i = 0; i < entries; i++) {
            acpi_table_header_t *table = (acpi_table_header_t *)(u32)xsdt->tables[i];
            if (memcmp(table->signature, signature, 4) == 0) {
                if (acpi_validate_table(table, table->length)) {
                    return table;
                }
            }
        }
    } else if (rsdt != NULL) {
        /* Search in RSDT */
        u32 entries = (rsdt->header.length - sizeof(acpi_table_header_t)) / sizeof(u32);
        for (u32 i = 0; i < entries; i++) {
            acpi_table_header_t *table = (acpi_table_header_t *)rsdt->tables[i];
            if (memcmp(table->signature, signature, 4) == 0) {
                if (acpi_validate_table(table, table->length)) {
                    return table;
                }
            }
        }
    }
    
    return NULL;
}

/**
 * Parse MADT
 */
void acpi_parse_madt(void) {
    early_console_print("ACPI: Parsing MADT...\n");
    
    /* Print MADT information */
    early_console_print("ACPI: Local APIC address: 0x");
    early_console_print_hex(madt->local_apic_address);
    early_console_print("\n");
    early_console_print("ACPI: APIC flags: 0x");
    early_console_print_hex(madt->flags);
    early_console_print("\n");
    
    /* Parse MADT entries */
    u32 offset = 0;
    while (offset < madt->header.length - sizeof(acpi_table_header_t) - 8) {
        acpi_madt_entry_header_t *entry = (acpi_madt_entry_header_t *)(madt->entries + offset);
        
        switch (entry->type) {
            case ACPI_MADT_TYPE_LOCAL_APIC: {
                acpi_madt_local_apic_t *local_apic = (acpi_madt_local_apic_t *)entry;
                early_console_print("ACPI: Local APIC: Processor ");
                early_console_print_dec(local_apic->acpi_processor_id);
                early_console_print(", APIC ID ");
                early_console_print_dec(local_apic->apic_id);
                early_console_print(", Flags 0x");
                early_console_print_hex(local_apic->flags);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_IO_APIC: {
                acpi_madt_io_apic_t *io_apic = (acpi_madt_io_apic_t *)entry;
                early_console_print("ACPI: I/O APIC: ID ");
                early_console_print_dec(io_apic->id);
                early_console_print(", Address 0x");
                early_console_print_hex(io_apic->address);
                early_console_print(", GSI Base ");
                early_console_print_dec(io_apic->global_system_interrupt_base);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE: {
                acpi_madt_interrupt_override_t *interrupt_override = (acpi_madt_interrupt_override_t *)entry;
                early_console_print("ACPI: Interrupt Override: Bus ");
                early_console_print_dec(interrupt_override->bus);
                early_console_print(", Source ");
                early_console_print_dec(interrupt_override->source);
                early_console_print(", GSI ");
                early_console_print_dec(interrupt_override->global_system_interrupt);
                early_console_print(", Flags 0x");
                early_console_print_hex(interrupt_override->flags);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_NMI_SOURCE: {
                acpi_madt_nmi_source_t *nmi_source = (acpi_madt_nmi_source_t *)entry;
                early_console_print("ACPI: NMI Source: Flags 0x");
                early_console_print_hex(nmi_source->flags);
                early_console_print(", GSI ");
                early_console_print_dec(nmi_source->global_system_interrupt);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_LOCAL_APIC_NMI: {
                acpi_madt_local_apic_nmi_t *local_apic_nmi = (acpi_madt_local_apic_nmi_t *)entry;
                early_console_print("ACPI: Local APIC NMI: Processor ");
                early_console_print_dec(local_apic_nmi->acpi_processor_id);
                early_console_print(", Flags 0x");
                early_console_print_hex(local_apic_nmi->flags);
                early_console_print(", LINT ");
                early_console_print_dec(local_apic_nmi->lint);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE: {
                acpi_madt_local_apic_override_t *local_apic_override = (acpi_madt_local_apic_override_t *)entry;
                early_console_print("ACPI: Local APIC Override: Address 0x");
                early_console_print_hex((u32)local_apic_override->address);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_IO_SAPIC: {
                acpi_madt_io_sapic_t *io_sapic = (acpi_madt_io_sapic_t *)entry;
                early_console_print("ACPI: I/O SAPIC: ID ");
                early_console_print_dec(io_sapic->id);
                early_console_print(", GSI Base ");
                early_console_print_dec(io_sapic->global_system_interrupt_base);
                early_console_print(", Address 0x");
                early_console_print_hex((u32)io_sapic->address);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_LOCAL_SAPIC: {
                acpi_madt_local_sapic_t *local_sapic = (acpi_madt_local_sapic_t *)entry;
                early_console_print("ACPI: Local SAPIC: Processor ");
                early_console_print_dec(local_sapic->acpi_processor_id);
                early_console_print(", ID ");
                early_console_print_dec(local_sapic->local_sapic_id);
                early_console_print(", EID ");
                early_console_print_dec(local_sapic->local_sapic_eid);
                early_console_print(", UID ");
                early_console_print_dec(local_sapic->acpi_processor_uid);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_PLATFORM_INTERRUPT_SOURCE: {
                acpi_madt_platform_interrupt_source_t *platform_interrupt_source = (acpi_madt_platform_interrupt_source_t *)entry;
                early_console_print("ACPI: Platform Interrupt Source: Flags 0x");
                early_console_print_hex(platform_interrupt_source->flags);
                early_console_print(", Type ");
                early_console_print_dec(platform_interrupt_source->interrupt_type);
                early_console_print(", Processor ");
                early_console_print_dec(platform_interrupt_source->processor_id);
                early_console_print(", EID ");
                early_console_print_dec(platform_interrupt_source->processor_eid);
                early_console_print(", Vector ");
                early_console_print_dec(platform_interrupt_source->io_sapic_vector);
                early_console_print(", GSI ");
                early_console_print_dec(platform_interrupt_source->global_system_interrupt);
                early_console_print(", Source Flags 0x");
                early_console_print_hex(platform_interrupt_source->platform_interrupt_source_flags);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_LOCAL_X2APIC: {
                acpi_madt_local_x2apic_t *local_x2apic = (acpi_madt_local_x2apic_t *)entry;
                early_console_print("ACPI: Local x2APIC: ID ");
                early_console_print_dec(local_x2apic->x2apic_id);
                early_console_print(", Flags 0x");
                early_console_print_hex(local_x2apic->flags);
                early_console_print(", UID ");
                early_console_print_dec(local_x2apic->acpi_processor_uid);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_LOCAL_X2APIC_NMI: {
                acpi_madt_local_x2apic_nmi_t *local_x2apic_nmi = (acpi_madt_local_x2apic_nmi_t *)entry;
                early_console_print("ACPI: Local x2APIC NMI: Flags 0x");
                early_console_print_hex(local_x2apic_nmi->flags);
                early_console_print(", UID ");
                early_console_print_dec(local_x2apic_nmi->acpi_processor_uid);
                early_console_print(", LINT ");
                early_console_print_dec(local_x2apic_nmi->lint);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_GENERIC_INTERRUPT: {
                acpi_madt_generic_interrupt_t *generic_interrupt = (acpi_madt_generic_interrupt_t *)entry;
                early_console_print("ACPI: Generic Interrupt: Interface ");
                early_console_print_dec(generic_interrupt->cpu_interface_number);
                early_console_print(", UID ");
                early_console_print_dec(generic_interrupt->acpi_processor_uid);
                early_console_print(", Flags 0x");
                early_console_print_hex(generic_interrupt->flags);
                early_console_print("\n");
                break;
            }
            case ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR: {
                acpi_madt_generic_distributor_t *generic_distributor = (acpi_madt_generic_distributor_t *)entry;
                early_console_print("ACPI: Generic Distributor: ID ");
                early_console_print_dec(generic_distributor->gic_id);
                early_console_print(", Address 0x");
                early_console_print_hex((u32)generic_distributor->base_address);
                early_console_print(", GSI Base ");
                early_console_print_dec(generic_distributor->global_irq_base);
                early_console_print(", Version ");
                early_console_print_dec(generic_distributor->version);
                early_console_print("\n");
                break;
            }
            default:
                early_console_print("ACPI: Unknown MADT entry type: ");
                early_console_print_dec(entry->type);
                early_console_print("\n");
                break;
        }
        
        offset += entry->length;
    }
}

/**
 * Parse FADT
 */
void acpi_parse_fadt(void) {
    early_console_print("ACPI: Parsing FADT...\n");
    
    /* Print FADT information */
    early_console_print("ACPI: FADT Revision: ");
    early_console_print_dec(fadt->header.revision);
    early_console_print("\n");
    early_console_print("ACPI: DSDT Address: 0x");
    early_console_print_hex(fadt->dsdt);
    early_console_print("\n");
    early_console_print("ACPI: SCI Interrupt: ");
    early_console_print_dec(fadt->sci_int);
    early_console_print("\n");
    early_console_print("ACPI: SMI Command: 0x");
    early_console_print_hex(fadt->smi_cmd);
    early_console_print("\n");
    early_console_print("ACPI: ACPI Enable: 0x");
    early_console_print_hex(fadt->acpi_enable);
    early_console_print("\n");
    early_console_print("ACPI: ACPI Disable: 0x");
    early_console_print_hex(fadt->acpi_disable);
    early_console_print("\n");
    early_console_print("ACPI: PM1a Control Block: 0x");
    early_console_print_hex(fadt->pm1a_cnt_blk);
    early_console_print("\n");
    early_console_print("ACPI: PM1b Control Block: 0x");
    early_console_print_hex(fadt->pm1b_cnt_blk);
    early_console_print("\n");
    early_console_print("ACPI: PM1 Control Length: ");
    early_console_print_dec(fadt->pm1_cnt_len);
    early_console_print("\n");
    early_console_print("ACPI: PM Timer Block: 0x");
    early_console_print_hex(fadt->pm_tmr_blk);
    early_console_print("\n");
    early_console_print("ACPI: PM Timer Length: ");
    early_console_print_dec(fadt->pm_tmr_len);
    early_console_print("\n");
    early_console_print("ACPI: Reset Register: 0x");
    early_console_print_hex(fadt->reset_reg[0]);
    early_console_print("\n");
    early_console_print("ACPI: Reset Value: 0x");
    early_console_print_hex(fadt->reset_value);
    early_console_print("\n");
}

/**
 * Parse HPET
 */
void acpi_parse_hpet(void) {
    early_console_print("ACPI: Parsing HPET...\n");
    
    /* Print HPET information */
    early_console_print("ACPI: HPET ID: 0x");
    early_console_print_hex(hpet->id);
    early_console_print("\n");
    early_console_print("ACPI: HPET Address: 0x");
    early_console_print_hex(hpet->address[0]);
    early_console_print("\n");
    early_console_print("ACPI: HPET Sequence: ");
    early_console_print_dec(hpet->sequence);
    early_console_print("\n");
    early_console_print("ACPI: HPET Minimum Tick: ");
    early_console_print_dec(hpet->minimum_tick);
    early_console_print("\n");
    early_console_print("ACPI: HPET Flags: 0x");
    early_console_print_hex(hpet->flags);
    early_console_print("\n");
}

/**
 * Enable ACPI
 */
void acpi_enable(void) {
    if (acpi_enabled) {
        return;
    }
    
    early_console_print("ACPI: Enabling ACPI...\n");
    
    /* Check if ACPI is already enabled */
    if (inw(fadt->pm1a_cnt_blk) & 1) {
        early_console_print("ACPI: ACPI already enabled\n");
        acpi_enabled = 1;
        return;
    }
    
    /* Enable ACPI */
    if (fadt->smi_cmd != 0 && fadt->acpi_enable != 0) {
        outb(fadt->smi_cmd, fadt->acpi_enable);
        
        /* Wait for ACPI to be enabled */
        for (int i = 0; i < 300; i++) {
            if (inw(fadt->pm1a_cnt_blk) & 1) {
                early_console_print("ACPI: ACPI enabled\n");
                acpi_enabled = 1;
                return;
            }
            io_wait();
        }
        
        early_console_print("ACPI: Failed to enable ACPI\n");
    } else {
        early_console_print("ACPI: No SMI command or ACPI enable value\n");
    }
}

/**
 * Disable ACPI
 */
void acpi_disable(void) {
    if (!acpi_enabled) {
        return;
    }
    
    early_console_print("ACPI: Disabling ACPI...\n");
    
    /* Check if ACPI is already disabled */
    if (!(inw(fadt->pm1a_cnt_blk) & 1)) {
        early_console_print("ACPI: ACPI already disabled\n");
        acpi_enabled = 0;
        return;
    }
    
    /* Disable ACPI */
    if (fadt->smi_cmd != 0 && fadt->acpi_disable != 0) {
        outb(fadt->smi_cmd, fadt->acpi_disable);
        
        /* Wait for ACPI to be disabled */
        for (int i = 0; i < 300; i++) {
            if (!(inw(fadt->pm1a_cnt_blk) & 1)) {
                early_console_print("ACPI: ACPI disabled\n");
                acpi_enabled = 0;
                return;
            }
            io_wait();
        }
        
        early_console_print("ACPI: Failed to disable ACPI\n");
    } else {
        early_console_print("ACPI: No SMI command or ACPI disable value\n");
    }
}

/**
 * Shutdown system
 * 
 * @return 0 on success, -1 on failure
 */
int acpi_shutdown(void) {
    if (!acpi_enabled) {
        return -1;
    }
    
    early_console_print("ACPI: Shutting down...\n");
    
    /* Set SLP_TYP and SLP_EN */
    u16 pm1a_cnt = inw(fadt->pm1a_cnt_blk);
    pm1a_cnt &= ~(7 << 10);
    pm1a_cnt |= (5 << 10);
    pm1a_cnt |= (1 << 13);
    outw(fadt->pm1a_cnt_blk, pm1a_cnt);
    
    /* If PM1b control block exists, set it too */
    if (fadt->pm1b_cnt_blk != 0) {
        u16 pm1b_cnt = inw(fadt->pm1b_cnt_blk);
        pm1b_cnt &= ~(7 << 10);
        pm1b_cnt |= (5 << 10);
        pm1b_cnt |= (1 << 13);
        outw(fadt->pm1b_cnt_blk, pm1b_cnt);
    }
    
    /* Wait for shutdown */
    for (;;) {
        __asm__ volatile("hlt");
    }
    
    return 0;
}

/**
 * Reboot system
 * 
 * @return 0 on success, -1 on failure
 */
int acpi_reboot(void) {
    if (!acpi_enabled) {
        return -1;
    }
    
    early_console_print("ACPI: Rebooting...\n");
    
    /* Check if reset register is supported */
    if (fadt->header.revision >= 2 && fadt->reset_reg[0] != 0) {
        /* Use reset register */
        switch (fadt->reset_reg[0] & 0xC0) {
            case 0x00: /* Memory mapped */
                *((volatile u8 *)(u32)fadt->reset_reg[0]) = fadt->reset_value;
                break;
            case 0x40: /* I/O port */
                outb(fadt->reset_reg[0] & 0xFFFF, fadt->reset_value);
                break;
            case 0x80: /* PCI configuration space */
                /* Not implemented */
                break;
        }
    } else {
        /* Use keyboard controller */
        u8 good = 0x02;
        while (good & 0x02) {
            good = inb(0x64);
        }
        outb(0x64, 0xFE);
    }
    
    /* Wait for reboot */
    for (;;) {
        __asm__ volatile("hlt");
    }
    
    return 0;
}

/**
 * Sleep system
 * 
 * @param state Sleep state (1-5)
 * @return 0 on success, -1 on failure
 */
int acpi_sleep(u8 state) {
    if (!acpi_enabled || state < 1 || state > 5) {
        return -1;
    }
    
    early_console_print("ACPI: Sleeping (S");
    early_console_print_dec(state);
    early_console_print(")...\n");
    
    /* Set SLP_TYP and SLP_EN */
    u16 pm1a_cnt = inw(fadt->pm1a_cnt_blk);
    pm1a_cnt &= ~(7 << 10);
    pm1a_cnt |= (state << 10);
    pm1a_cnt |= (1 << 13);
    outw(fadt->pm1a_cnt_blk, pm1a_cnt);
    
    /* If PM1b control block exists, set it too */
    if (fadt->pm1b_cnt_blk != 0) {
        u16 pm1b_cnt = inw(fadt->pm1b_cnt_blk);
        pm1b_cnt &= ~(7 << 10);
        pm1b_cnt |= (state << 10);
        pm1b_cnt |= (1 << 13);
        outw(fadt->pm1b_cnt_blk, pm1b_cnt);
    }
    
    /* Wait for sleep */
    for (;;) {
        __asm__ volatile("hlt");
    }
    
    return 0;
}

/**
 * Wake system
 * 
 * @return 0 on success, -1 on failure
 */
int acpi_wake(void) {
    if (!acpi_enabled) {
        return -1;
    }
    
    early_console_print("ACPI: Waking up...\n");
    
    /* Reset sleep type */
    u16 pm1a_cnt = inw(fadt->pm1a_cnt_blk);
    pm1a_cnt &= ~(7 << 10);
    outw(fadt->pm1a_cnt_blk, pm1a_cnt);
    
    /* If PM1b control block exists, reset it too */
    if (fadt->pm1b_cnt_blk != 0) {
        u16 pm1b_cnt = inw(fadt->pm1b_cnt_blk);
        pm1b_cnt &= ~(7 << 10);
        outw(fadt->pm1b_cnt_blk, pm1b_cnt);
    }
    
    return 0;
}

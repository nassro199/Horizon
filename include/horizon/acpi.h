/**
 * acpi.h - Horizon kernel ACPI definitions
 * 
 * This file contains definitions for ACPI.
 */

#ifndef _HORIZON_ACPI_H
#define _HORIZON_ACPI_H

#include <horizon/types.h>

/* ACPI signature constants */
#define ACPI_RSDP_SIGNATURE       "RSD PTR "
#define ACPI_RSDT_SIGNATURE       "RSDT"
#define ACPI_XSDT_SIGNATURE       "XSDT"
#define ACPI_FADT_SIGNATURE       "FACP"
#define ACPI_DSDT_SIGNATURE       "DSDT"
#define ACPI_SSDT_SIGNATURE       "SSDT"
#define ACPI_MADT_SIGNATURE       "APIC"
#define ACPI_HPET_SIGNATURE       "HPET"
#define ACPI_MCFG_SIGNATURE       "MCFG"
#define ACPI_SRAT_SIGNATURE       "SRAT"
#define ACPI_SLIT_SIGNATURE       "SLIT"
#define ACPI_BERT_SIGNATURE       "BERT"
#define ACPI_CPEP_SIGNATURE       "CPEP"
#define ACPI_ECDT_SIGNATURE       "ECDT"
#define ACPI_EINJ_SIGNATURE       "EINJ"
#define ACPI_ERST_SIGNATURE       "ERST"
#define ACPI_HEST_SIGNATURE       "HEST"
#define ACPI_MSCT_SIGNATURE       "MSCT"
#define ACPI_SBST_SIGNATURE       "SBST"
#define ACPI_SLIC_SIGNATURE       "SLIC"
#define ACPI_SPCR_SIGNATURE       "SPCR"
#define ACPI_SPMI_SIGNATURE       "SPMI"
#define ACPI_TCPA_SIGNATURE       "TCPA"
#define ACPI_UEFI_SIGNATURE       "UEFI"
#define ACPI_WAET_SIGNATURE       "WAET"
#define ACPI_WDAT_SIGNATURE       "WDAT"
#define ACPI_WDRT_SIGNATURE       "WDRT"
#define ACPI_WSPT_SIGNATURE       "WSPT"

/* ACPI RSDP structure */
typedef struct acpi_rsdp {
    char signature[8];            /* "RSD PTR " */
    u8 checksum;                  /* Checksum of the first 20 bytes */
    char oem_id[6];               /* OEM ID */
    u8 revision;                  /* ACPI revision */
    u32 rsdt_address;             /* Physical address of RSDT */
    u32 length;                   /* Length of the table in bytes */
    u64 xsdt_address;             /* Physical address of XSDT */
    u8 extended_checksum;         /* Checksum of the entire table */
    u8 reserved[3];               /* Reserved */
} __attribute__((packed)) acpi_rsdp_t;

/* ACPI table header */
typedef struct acpi_table_header {
    char signature[4];            /* Table signature */
    u32 length;                   /* Length of the table in bytes */
    u8 revision;                  /* ACPI revision */
    u8 checksum;                  /* Checksum of the entire table */
    char oem_id[6];               /* OEM ID */
    char oem_table_id[8];         /* OEM table ID */
    u32 oem_revision;             /* OEM revision */
    u32 creator_id;               /* Creator ID */
    u32 creator_revision;         /* Creator revision */
} __attribute__((packed)) acpi_table_header_t;

/* ACPI RSDT structure */
typedef struct acpi_rsdt {
    acpi_table_header_t header;   /* Table header */
    u32 tables[];                 /* Table pointers */
} __attribute__((packed)) acpi_rsdt_t;

/* ACPI XSDT structure */
typedef struct acpi_xsdt {
    acpi_table_header_t header;   /* Table header */
    u64 tables[];                 /* Table pointers */
} __attribute__((packed)) acpi_xsdt_t;

/* ACPI FADT structure */
typedef struct acpi_fadt {
    acpi_table_header_t header;   /* Table header */
    u32 firmware_ctrl;            /* Physical address of FACS */
    u32 dsdt;                     /* Physical address of DSDT */
    u8 reserved;                  /* Reserved */
    u8 preferred_pm_profile;      /* Preferred power management profile */
    u16 sci_int;                  /* System control interrupt */
    u32 smi_cmd;                  /* SMI command port */
    u8 acpi_enable;               /* Value to write to SMI_CMD to enable ACPI */
    u8 acpi_disable;              /* Value to write to SMI_CMD to disable ACPI */
    u8 s4bios_req;                /* Value to write to SMI_CMD to enter S4BIOS state */
    u8 pstate_cnt;                /* Value to write to SMI_CMD to assume processor performance state control responsibility */
    u32 pm1a_evt_blk;             /* Port address of PM1a event register block */
    u32 pm1b_evt_blk;             /* Port address of PM1b event register block */
    u32 pm1a_cnt_blk;             /* Port address of PM1a control register block */
    u32 pm1b_cnt_blk;             /* Port address of PM1b control register block */
    u32 pm2_cnt_blk;              /* Port address of PM2 control register block */
    u32 pm_tmr_blk;               /* Port address of PM timer register block */
    u32 gpe0_blk;                 /* Port address of general purpose event 0 register block */
    u32 gpe1_blk;                 /* Port address of general purpose event 1 register block */
    u8 pm1_evt_len;               /* Byte length of PM1 event register block */
    u8 pm1_cnt_len;               /* Byte length of PM1 control register block */
    u8 pm2_cnt_len;               /* Byte length of PM2 control register block */
    u8 pm_tmr_len;                /* Byte length of PM timer register block */
    u8 gpe0_blk_len;              /* Byte length of general purpose event 0 register block */
    u8 gpe1_blk_len;              /* Byte length of general purpose event 1 register block */
    u8 gpe1_base;                 /* Offset within the ACPI general purpose event model where GPE1 events start */
    u8 cst_cnt;                   /* Value to write to SMI_CMD to indicate C state notification */
    u16 p_lvl2_lat;               /* Worst case latency to enter C2 state in microseconds */
    u16 p_lvl3_lat;               /* Worst case latency to enter C3 state in microseconds */
    u16 flush_size;               /* Cache line size in bytes */
    u16 flush_stride;             /* Number of flush strides */
    u8 duty_offset;               /* Bit location of duty cycle field in P_CNT register */
    u8 duty_width;                /* Bit width of duty cycle field in P_CNT register */
    u8 day_alrm;                  /* RTC CMOS RAM index to day-of-month alarm value */
    u8 mon_alrm;                  /* RTC CMOS RAM index to month-of-year alarm value */
    u8 century;                   /* RTC CMOS RAM index to century value */
    u16 iapc_boot_arch;           /* IA-PC boot architecture flags */
    u8 reserved2;                 /* Reserved */
    u32 flags;                    /* Fixed feature flags */
    /* Extended fields */
    u32 reset_reg[3];             /* Reset register */
    u8 reset_value;               /* Value to write to the reset register */
    u8 reserved3[3];              /* Reserved */
    u64 x_firmware_ctrl;          /* 64-bit physical address of FACS */
    u64 x_dsdt;                   /* 64-bit physical address of DSDT */
    /* More extended fields */
    u32 x_pm1a_evt_blk[3];        /* Extended PM1a event register block */
    u32 x_pm1b_evt_blk[3];        /* Extended PM1b event register block */
    u32 x_pm1a_cnt_blk[3];        /* Extended PM1a control register block */
    u32 x_pm1b_cnt_blk[3];        /* Extended PM1b control register block */
    u32 x_pm2_cnt_blk[3];         /* Extended PM2 control register block */
    u32 x_pm_tmr_blk[3];          /* Extended PM timer register block */
    u32 x_gpe0_blk[3];            /* Extended general purpose event 0 register block */
    u32 x_gpe1_blk[3];            /* Extended general purpose event 1 register block */
} __attribute__((packed)) acpi_fadt_t;

/* ACPI MADT structure */
typedef struct acpi_madt {
    acpi_table_header_t header;   /* Table header */
    u32 local_apic_address;       /* Physical address of local APIC */
    u32 flags;                    /* APIC flags */
    u8 entries[];                 /* APIC entries */
} __attribute__((packed)) acpi_madt_t;

/* ACPI MADT entry types */
#define ACPI_MADT_TYPE_LOCAL_APIC         0
#define ACPI_MADT_TYPE_IO_APIC            1
#define ACPI_MADT_TYPE_INTERRUPT_OVERRIDE 2
#define ACPI_MADT_TYPE_NMI_SOURCE         3
#define ACPI_MADT_TYPE_LOCAL_APIC_NMI     4
#define ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE 5
#define ACPI_MADT_TYPE_IO_SAPIC           6
#define ACPI_MADT_TYPE_LOCAL_SAPIC        7
#define ACPI_MADT_TYPE_PLATFORM_INTERRUPT_SOURCE 8
#define ACPI_MADT_TYPE_LOCAL_X2APIC       9
#define ACPI_MADT_TYPE_LOCAL_X2APIC_NMI   10
#define ACPI_MADT_TYPE_GENERIC_INTERRUPT  11
#define ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR 12
#define ACPI_MADT_TYPE_GENERIC_MSI_FRAME  13
#define ACPI_MADT_TYPE_GENERIC_REDISTRIBUTOR 14
#define ACPI_MADT_TYPE_GENERIC_TRANSLATOR 15

/* ACPI MADT entry header */
typedef struct acpi_madt_entry_header {
    u8 type;                      /* Entry type */
    u8 length;                    /* Entry length */
} __attribute__((packed)) acpi_madt_entry_header_t;

/* ACPI MADT local APIC entry */
typedef struct acpi_madt_local_apic {
    acpi_madt_entry_header_t header; /* Entry header */
    u8 acpi_processor_id;         /* ACPI processor ID */
    u8 apic_id;                   /* APIC ID */
    u32 flags;                    /* Flags */
} __attribute__((packed)) acpi_madt_local_apic_t;

/* ACPI MADT I/O APIC entry */
typedef struct acpi_madt_io_apic {
    acpi_madt_entry_header_t header; /* Entry header */
    u8 id;                        /* I/O APIC ID */
    u8 reserved;                  /* Reserved */
    u32 address;                  /* I/O APIC address */
    u32 global_system_interrupt_base; /* Global system interrupt base */
} __attribute__((packed)) acpi_madt_io_apic_t;

/* ACPI MADT interrupt override entry */
typedef struct acpi_madt_interrupt_override {
    acpi_madt_entry_header_t header; /* Entry header */
    u8 bus;                       /* Bus */
    u8 source;                    /* Source */
    u32 global_system_interrupt;  /* Global system interrupt */
    u16 flags;                    /* Flags */
} __attribute__((packed)) acpi_madt_interrupt_override_t;

/* ACPI MADT NMI source entry */
typedef struct acpi_madt_nmi_source {
    acpi_madt_entry_header_t header; /* Entry header */
    u16 flags;                    /* Flags */
    u32 global_system_interrupt;  /* Global system interrupt */
} __attribute__((packed)) acpi_madt_nmi_source_t;

/* ACPI MADT local APIC NMI entry */
typedef struct acpi_madt_local_apic_nmi {
    acpi_madt_entry_header_t header; /* Entry header */
    u8 acpi_processor_id;         /* ACPI processor ID */
    u16 flags;                    /* Flags */
    u8 lint;                      /* Local interrupt */
} __attribute__((packed)) acpi_madt_local_apic_nmi_t;

/* ACPI MADT local APIC address override entry */
typedef struct acpi_madt_local_apic_override {
    acpi_madt_entry_header_t header; /* Entry header */
    u16 reserved;                 /* Reserved */
    u64 address;                  /* Local APIC address */
} __attribute__((packed)) acpi_madt_local_apic_override_t;

/* ACPI MADT I/O SAPIC entry */
typedef struct acpi_madt_io_sapic {
    acpi_madt_entry_header_t header; /* Entry header */
    u8 id;                        /* I/O SAPIC ID */
    u8 reserved;                  /* Reserved */
    u32 global_system_interrupt_base; /* Global system interrupt base */
    u64 address;                  /* I/O SAPIC address */
} __attribute__((packed)) acpi_madt_io_sapic_t;

/* ACPI MADT local SAPIC entry */
typedef struct acpi_madt_local_sapic {
    acpi_madt_entry_header_t header; /* Entry header */
    u8 acpi_processor_id;         /* ACPI processor ID */
    u8 local_sapic_id;            /* Local SAPIC ID */
    u8 local_sapic_eid;           /* Local SAPIC EID */
    u8 reserved[3];               /* Reserved */
    u32 flags;                    /* Flags */
    u32 acpi_processor_uid;       /* ACPI processor UID */
    char acpi_processor_uid_string[]; /* ACPI processor UID string */
} __attribute__((packed)) acpi_madt_local_sapic_t;

/* ACPI MADT platform interrupt source entry */
typedef struct acpi_madt_platform_interrupt_source {
    acpi_madt_entry_header_t header; /* Entry header */
    u16 flags;                    /* Flags */
    u8 interrupt_type;            /* Interrupt type */
    u8 processor_id;              /* Processor ID */
    u8 processor_eid;             /* Processor EID */
    u8 io_sapic_vector;           /* I/O SAPIC vector */
    u32 global_system_interrupt;  /* Global system interrupt */
    u32 platform_interrupt_source_flags; /* Platform interrupt source flags */
} __attribute__((packed)) acpi_madt_platform_interrupt_source_t;

/* ACPI MADT local x2APIC entry */
typedef struct acpi_madt_local_x2apic {
    acpi_madt_entry_header_t header; /* Entry header */
    u16 reserved;                 /* Reserved */
    u32 x2apic_id;                /* x2APIC ID */
    u32 flags;                    /* Flags */
    u32 acpi_processor_uid;       /* ACPI processor UID */
} __attribute__((packed)) acpi_madt_local_x2apic_t;

/* ACPI MADT local x2APIC NMI entry */
typedef struct acpi_madt_local_x2apic_nmi {
    acpi_madt_entry_header_t header; /* Entry header */
    u16 flags;                    /* Flags */
    u32 acpi_processor_uid;       /* ACPI processor UID */
    u8 lint;                      /* Local interrupt */
    u8 reserved[3];               /* Reserved */
} __attribute__((packed)) acpi_madt_local_x2apic_nmi_t;

/* ACPI MADT generic interrupt entry */
typedef struct acpi_madt_generic_interrupt {
    acpi_madt_entry_header_t header; /* Entry header */
    u16 reserved;                 /* Reserved */
    u32 cpu_interface_number;     /* CPU interface number */
    u32 acpi_processor_uid;       /* ACPI processor UID */
    u32 flags;                    /* Flags */
    u32 parking_protocol_version; /* Parking protocol version */
    u32 performance_interrupt;    /* Performance interrupt */
    u64 parked_address;           /* Parked address */
    u64 base_address;             /* Base address */
    u64 gicv_base_address;        /* GICV base address */
    u64 gich_base_address;        /* GICH base address */
    u32 vgic_interrupt;           /* VGIC interrupt */
    u64 gicr_base_address;        /* GICR base address */
    u64 mpidr;                    /* MPIDR */
    u8 efficiency_class;          /* Efficiency class */
    u8 reserved2[3];              /* Reserved */
} __attribute__((packed)) acpi_madt_generic_interrupt_t;

/* ACPI MADT generic distributor entry */
typedef struct acpi_madt_generic_distributor {
    acpi_madt_entry_header_t header; /* Entry header */
    u16 reserved;                 /* Reserved */
    u32 gic_id;                   /* GIC ID */
    u64 base_address;             /* Base address */
    u32 global_irq_base;          /* Global IRQ base */
    u8 version;                   /* Version */
    u8 reserved2[3];              /* Reserved */
} __attribute__((packed)) acpi_madt_generic_distributor_t;

/* ACPI MADT generic MSI frame entry */
typedef struct acpi_madt_generic_msi_frame {
    acpi_madt_entry_header_t header; /* Entry header */
    u16 reserved;                 /* Reserved */
    u32 msi_frame_id;             /* MSI frame ID */
    u64 base_address;             /* Base address */
    u32 flags;                    /* Flags */
    u16 spi_count;                /* SPI count */
    u16 spi_base;                 /* SPI base */
} __attribute__((packed)) acpi_madt_generic_msi_frame_t;

/* ACPI MADT generic redistributor entry */
typedef struct acpi_madt_generic_redistributor {
    acpi_madt_entry_header_t header; /* Entry header */
    u16 reserved;                 /* Reserved */
    u64 base_address;             /* Base address */
    u32 length;                   /* Length */
} __attribute__((packed)) acpi_madt_generic_redistributor_t;

/* ACPI MADT generic translator entry */
typedef struct acpi_madt_generic_translator {
    acpi_madt_entry_header_t header; /* Entry header */
    u16 reserved;                 /* Reserved */
    u32 translation_id;           /* Translation ID */
    u64 base_address;             /* Base address */
    u32 reserved2;                /* Reserved */
} __attribute__((packed)) acpi_madt_generic_translator_t;

/* ACPI HPET structure */
typedef struct acpi_hpet {
    acpi_table_header_t header;   /* Table header */
    u32 id;                       /* Hardware ID */
    u32 address[3];               /* Base address */
    u8 sequence;                  /* Sequence number */
    u16 minimum_tick;             /* Minimum tick */
    u8 flags;                     /* Flags */
} __attribute__((packed)) acpi_hpet_t;

/* ACPI functions */
void acpi_init(void);
acpi_rsdp_t *acpi_find_rsdp(void);
acpi_table_header_t *acpi_find_table(const char *signature);
void acpi_parse_madt(void);
void acpi_parse_fadt(void);
void acpi_parse_hpet(void);
void acpi_enable(void);
void acpi_disable(void);
int acpi_shutdown(void);
int acpi_reboot(void);
int acpi_sleep(u8 state);
int acpi_wake(void);

#endif /* _HORIZON_ACPI_H */

/**
 * smp.h - x86 SMP definitions
 * 
 * This file contains x86-specific SMP definitions.
 */

#ifndef _ASM_SMP_H
#define _ASM_SMP_H

#include <horizon/types.h>

/* Local APIC registers */
#define LAPIC_ID            0x20    /* ID Register */
#define LAPIC_VER           0x30    /* Version Register */
#define LAPIC_TPR           0x80    /* Task Priority Register */
#define LAPIC_APR           0x90    /* Arbitration Priority Register */
#define LAPIC_PPR           0xA0    /* Processor Priority Register */
#define LAPIC_EOI           0xB0    /* End of Interrupt Register */
#define LAPIC_RRD           0xC0    /* Remote Read Register */
#define LAPIC_LDR           0xD0    /* Logical Destination Register */
#define LAPIC_DFR           0xE0    /* Destination Format Register */
#define LAPIC_SVR           0xF0    /* Spurious Interrupt Vector Register */
#define LAPIC_ISR           0x100   /* In-Service Register */
#define LAPIC_TMR           0x180   /* Trigger Mode Register */
#define LAPIC_IRR           0x200   /* Interrupt Request Register */
#define LAPIC_ESR           0x280   /* Error Status Register */
#define LAPIC_ICR_LOW       0x300   /* Interrupt Command Register (low) */
#define LAPIC_ICR_HIGH      0x310   /* Interrupt Command Register (high) */
#define LAPIC_LVT_TIMER     0x320   /* Timer Local Vector Table Entry */
#define LAPIC_LVT_THERMAL   0x330   /* Thermal Local Vector Table Entry */
#define LAPIC_LVT_PERF      0x340   /* Performance Local Vector Table Entry */
#define LAPIC_LVT_LINT0     0x350   /* Local Interrupt 0 Vector Table Entry */
#define LAPIC_LVT_LINT1     0x360   /* Local Interrupt 1 Vector Table Entry */
#define LAPIC_LVT_ERROR     0x370   /* Error Vector Table Entry */
#define LAPIC_TIMER_ICR     0x380   /* Timer Initial Count Register */
#define LAPIC_TIMER_CCR     0x390   /* Timer Current Count Register */
#define LAPIC_TIMER_DCR     0x3E0   /* Timer Divide Configuration Register */

/* Local APIC flags */
#define LAPIC_SVR_ENABLE    0x00000100  /* APIC Enable */
#define LAPIC_LVT_MASKED    0x00010000  /* Interrupt Masked */
#define LAPIC_LVT_LEVEL     0x00008000  /* Level Triggered */
#define LAPIC_LVT_EDGE      0x00000000  /* Edge Triggered */
#define LAPIC_LVT_EXTINT    0x00000700  /* External Interrupt */
#define LAPIC_LVT_NMI       0x00000400  /* NMI */
#define LAPIC_LVT_SMI       0x00000200  /* SMI */
#define LAPIC_TIMER_PERIODIC 0x00020000 /* Periodic Timer */
#define LAPIC_TIMER_ONESHOT 0x00000000  /* One-Shot Timer */
#define LAPIC_TIMER_DCR_DIV1 0x0000000B /* Divide by 1 */
#define LAPIC_TIMER_DCR_DIV2 0x00000000 /* Divide by 2 */
#define LAPIC_TIMER_DCR_DIV4 0x00000001 /* Divide by 4 */
#define LAPIC_TIMER_DCR_DIV8 0x00000002 /* Divide by 8 */
#define LAPIC_TIMER_DCR_DIV16 0x00000003 /* Divide by 16 */
#define LAPIC_TIMER_DCR_DIV32 0x00000008 /* Divide by 32 */
#define LAPIC_TIMER_DCR_DIV64 0x00000009 /* Divide by 64 */
#define LAPIC_TIMER_DCR_DIV128 0x0000000A /* Divide by 128 */

/* ICR flags */
#define LAPIC_ICR_BUSY      0x00001000  /* Delivery Status */
#define LAPIC_ICR_FIXED     0x00000000  /* Fixed Delivery Mode */
#define LAPIC_ICR_LOWEST    0x00000100  /* Lowest Priority Delivery Mode */
#define LAPIC_ICR_SMI       0x00000200  /* SMI Delivery Mode */
#define LAPIC_ICR_NMI       0x00000400  /* NMI Delivery Mode */
#define LAPIC_ICR_INIT      0x00000500  /* INIT Delivery Mode */
#define LAPIC_ICR_STARTUP   0x00000600  /* Startup Delivery Mode */
#define LAPIC_ICR_PHYSICAL  0x00000000  /* Physical Destination Mode */
#define LAPIC_ICR_LOGICAL   0x00000800  /* Logical Destination Mode */
#define LAPIC_ICR_IDLE      0x00000000  /* Delivery Status: Idle */
#define LAPIC_ICR_PENDING   0x00001000  /* Delivery Status: Pending */
#define LAPIC_ICR_DEASSERT  0x00000000  /* Level: Deassert */
#define LAPIC_ICR_ASSERT    0x00004000  /* Level: Assert */
#define LAPIC_ICR_EDGE      0x00000000  /* Trigger Mode: Edge */
#define LAPIC_ICR_LEVEL     0x00008000  /* Trigger Mode: Level */
#define LAPIC_ICR_SELF      0x00040000  /* Destination Shorthand: Self */
#define LAPIC_ICR_ALL       0x00080000  /* Destination Shorthand: All */
#define LAPIC_ICR_ALL_EXCL  0x000C0000  /* Destination Shorthand: All Excluding Self */

/* I/O APIC registers */
#define IOAPIC_ID           0x00    /* ID Register */
#define IOAPIC_VER          0x01    /* Version Register */
#define IOAPIC_ARB          0x02    /* Arbitration Register */
#define IOAPIC_REDTBL       0x10    /* Redirection Table */

/* I/O APIC flags */
#define IOAPIC_INT_MASKED   0x00010000  /* Interrupt Masked */
#define IOAPIC_INT_LEVEL    0x00008000  /* Level Triggered */
#define IOAPIC_INT_EDGE     0x00000000  /* Edge Triggered */
#define IOAPIC_INT_PHYSICAL 0x00000000  /* Physical Destination Mode */
#define IOAPIC_INT_LOGICAL  0x00000800  /* Logical Destination Mode */
#define IOAPIC_INT_FIXED    0x00000000  /* Fixed Delivery Mode */
#define IOAPIC_INT_LOWEST   0x00000100  /* Lowest Priority Delivery Mode */
#define IOAPIC_INT_SMI      0x00000200  /* SMI Delivery Mode */
#define IOAPIC_INT_NMI      0x00000400  /* NMI Delivery Mode */
#define IOAPIC_INT_INIT     0x00000500  /* INIT Delivery Mode */
#define IOAPIC_INT_EXTINT   0x00000700  /* External Interrupt */

/* SMP functions */
int arch_smp_processor_id(void);
int arch_smp_boot_cpu(int cpu);
void arch_smp_send_ipi(int cpu, int vector);
void arch_smp_send_ipi_all(int vector);
void arch_smp_send_ipi_allbutself(int vector);
void arch_smp_init(void);
void arch_cpu_relax(void);
void arch_cpu_halt(void);

/* Local APIC functions */
u32 lapic_read(u32 reg);
void lapic_write(u32 reg, u32 value);
void lapic_init(void);

/* I/O APIC functions */
u32 ioapic_read(u8 reg);
void ioapic_write(u8 reg, u32 value);
void ioapic_init(void);

/* IPI handlers */
void smp_handle_invalidate_tlb_ipi(void);
void smp_handle_invalidate_page_ipi(void);

#endif /* _ASM_SMP_H */

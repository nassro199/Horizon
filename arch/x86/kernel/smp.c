/**
 * smp.c - x86 SMP implementation
 * 
 * This file contains the x86-specific SMP implementation.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/smp.h>
#include <horizon/spinlock.h>
#include <horizon/mm.h>
#include <horizon/sched.h>
#include <horizon/task.h>
#include <horizon/interrupt.h>
#include <horizon/errno.h>
#include <asm/io.h>
#include <asm/apic.h>
#include <asm/smp.h>

/* Local APIC base address */
static void *lapic_base = (void *)0xFEE00000;

/* I/O APIC base address */
static void *ioapic_base = (void *)0xFEC00000;

/* SMP boot flag */
static volatile int smp_boot_flag = 0;

/* SMP boot CPU ID */
static int smp_boot_cpu_id = 0;

/* SMP trampoline code */
extern void smp_trampoline_start(void);
extern void smp_trampoline_end(void);

/* IPI vectors */
#define IPI_VECTOR_BASE     0xF0
#define IPI_CALL_FUNC       (IPI_VECTOR_BASE + 0)
#define IPI_RESCHEDULE      (IPI_VECTOR_BASE + 1)
#define IPI_STOP            (IPI_VECTOR_BASE + 2)
#define IPI_INVALIDATE_TLB  (IPI_VECTOR_BASE + 3)
#define IPI_INVALIDATE_PAGE (IPI_VECTOR_BASE + 4)

/**
 * Get processor ID
 * 
 * @return Processor ID
 */
int arch_smp_processor_id(void) {
    /* Read APIC ID */
    return (lapic_read(LAPIC_ID) >> 24) & 0xFF;
}

/**
 * Boot a CPU
 * 
 * @param cpu CPU to boot
 * @return 0 on success, negative error code on failure
 */
int arch_smp_boot_cpu(int cpu) {
    /* Check parameters */
    if (cpu < 0 || cpu >= NR_CPUS) {
        return -EINVAL;
    }
    
    /* Check if CPU is already booted */
    if (cpu_isset(cpu, &cpu_online_mask)) {
        return 0;
    }
    
    /* Set boot flag */
    smp_boot_flag = 0;
    
    /* Set boot CPU ID */
    smp_boot_cpu_id = cpu;
    
    /* Copy trampoline code to low memory */
    void *trampoline = (void *)0x1000;
    memcpy(trampoline, smp_trampoline_start, (unsigned long)smp_trampoline_end - (unsigned long)smp_trampoline_start);
    
    /* Send INIT IPI */
    lapic_write(LAPIC_ICR_HIGH, cpu << 24);
    lapic_write(LAPIC_ICR_LOW, LAPIC_ICR_INIT | LAPIC_ICR_LEVEL);
    
    /* Wait for INIT IPI to be sent */
    while (lapic_read(LAPIC_ICR_LOW) & LAPIC_ICR_BUSY) {
        /* Wait */
    }
    
    /* Delay */
    udelay(10000);
    
    /* Send STARTUP IPI */
    lapic_write(LAPIC_ICR_HIGH, cpu << 24);
    lapic_write(LAPIC_ICR_LOW, LAPIC_ICR_STARTUP | (0x1000 >> 12));
    
    /* Wait for STARTUP IPI to be sent */
    while (lapic_read(LAPIC_ICR_LOW) & LAPIC_ICR_BUSY) {
        /* Wait */
    }
    
    /* Wait for CPU to boot */
    int timeout = 1000;
    while (smp_boot_flag == 0 && timeout > 0) {
        udelay(1000);
        timeout--;
    }
    
    /* Check if CPU booted */
    if (smp_boot_flag == 0) {
        /* CPU failed to boot */
        return -ETIMEDOUT;
    }
    
    return 0;
}

/**
 * Send IPI to a CPU
 * 
 * @param cpu CPU to send to
 * @param vector IPI vector
 */
void arch_smp_send_ipi(int cpu, int vector) {
    /* Check parameters */
    if (cpu < 0 || cpu >= NR_CPUS) {
        return;
    }
    
    /* Send IPI */
    lapic_write(LAPIC_ICR_HIGH, cpu << 24);
    lapic_write(LAPIC_ICR_LOW, LAPIC_ICR_FIXED | vector);
    
    /* Wait for IPI to be sent */
    while (lapic_read(LAPIC_ICR_LOW) & LAPIC_ICR_BUSY) {
        /* Wait */
    }
}

/**
 * Send IPI to all CPUs
 * 
 * @param vector IPI vector
 */
void arch_smp_send_ipi_all(int vector) {
    /* Send IPI */
    lapic_write(LAPIC_ICR_HIGH, 0);
    lapic_write(LAPIC_ICR_LOW, LAPIC_ICR_FIXED | LAPIC_ICR_ALL_EXCL | vector);
    
    /* Wait for IPI to be sent */
    while (lapic_read(LAPIC_ICR_LOW) & LAPIC_ICR_BUSY) {
        /* Wait */
    }
}

/**
 * Send IPI to all CPUs except self
 * 
 * @param vector IPI vector
 */
void arch_smp_send_ipi_allbutself(int vector) {
    /* Send IPI */
    lapic_write(LAPIC_ICR_HIGH, 0);
    lapic_write(LAPIC_ICR_LOW, LAPIC_ICR_FIXED | LAPIC_ICR_ALL_EXCL | vector);
    
    /* Wait for IPI to be sent */
    while (lapic_read(LAPIC_ICR_LOW) & LAPIC_ICR_BUSY) {
        /* Wait */
    }
}

/**
 * Initialize SMP
 */
void arch_smp_init(void) {
    /* Initialize local APIC */
    lapic_init();
    
    /* Initialize I/O APIC */
    ioapic_init();
    
    /* Register IPI handlers */
    interrupt_register_handler(IPI_CALL_FUNC, smp_handle_call_function_ipi);
    interrupt_register_handler(IPI_RESCHEDULE, smp_handle_reschedule_ipi);
    interrupt_register_handler(IPI_STOP, smp_handle_stop_ipi);
    interrupt_register_handler(IPI_INVALIDATE_TLB, smp_handle_invalidate_tlb_ipi);
    interrupt_register_handler(IPI_INVALIDATE_PAGE, smp_handle_invalidate_page_ipi);
}

/**
 * CPU relax
 */
void arch_cpu_relax(void) {
    __asm__ volatile("pause");
}

/**
 * CPU halt
 */
void arch_cpu_halt(void) {
    __asm__ volatile("hlt");
}

/**
 * Read local APIC register
 * 
 * @param reg Register offset
 * @return Register value
 */
u32 lapic_read(u32 reg) {
    return *(volatile u32 *)((u8 *)lapic_base + reg);
}

/**
 * Write local APIC register
 * 
 * @param reg Register offset
 * @param value Register value
 */
void lapic_write(u32 reg, u32 value) {
    *(volatile u32 *)((u8 *)lapic_base + reg) = value;
}

/**
 * Initialize local APIC
 */
void lapic_init(void) {
    /* Enable local APIC */
    lapic_write(LAPIC_SVR, LAPIC_SVR_ENABLE | 0xFF);
    
    /* Set task priority to accept all interrupts */
    lapic_write(LAPIC_TPR, 0);
    
    /* Configure LINT0 as ExtINT */
    lapic_write(LAPIC_LVT_LINT0, LAPIC_LVT_MASKED | LAPIC_LVT_EXTINT);
    
    /* Configure LINT1 as NMI */
    lapic_write(LAPIC_LVT_LINT1, LAPIC_LVT_NMI);
    
    /* Configure error register */
    lapic_write(LAPIC_LVT_ERROR, 0xFF);
    
    /* Configure timer */
    lapic_write(LAPIC_LVT_TIMER, LAPIC_LVT_MASKED | LAPIC_TIMER_PERIODIC | 0xFE);
    lapic_write(LAPIC_TIMER_DCR, LAPIC_TIMER_DCR_DIV1);
    lapic_write(LAPIC_TIMER_ICR, 10000000);
}

/**
 * Read I/O APIC register
 * 
 * @param reg Register offset
 * @return Register value
 */
u32 ioapic_read(u8 reg) {
    *(volatile u32 *)ioapic_base = reg;
    return *(volatile u32 *)((u8 *)ioapic_base + 0x10);
}

/**
 * Write I/O APIC register
 * 
 * @param reg Register offset
 * @param value Register value
 */
void ioapic_write(u8 reg, u32 value) {
    *(volatile u32 *)ioapic_base = reg;
    *(volatile u32 *)((u8 *)ioapic_base + 0x10) = value;
}

/**
 * Initialize I/O APIC
 */
void ioapic_init(void) {
    int i;
    
    /* Get number of IRQs */
    u32 num_irqs = (ioapic_read(IOAPIC_VER) >> 16) & 0xFF;
    
    /* Disable all IRQs */
    for (i = 0; i <= num_irqs; i++) {
        ioapic_write(IOAPIC_REDTBL + i * 2, IOAPIC_INT_MASKED);
        ioapic_write(IOAPIC_REDTBL + i * 2 + 1, 0);
    }
}

/**
 * Handle TLB invalidation IPI
 */
void smp_handle_invalidate_tlb_ipi(void) {
    /* Invalidate TLB */
    __asm__ volatile("movl %%cr3, %%eax; movl %%eax, %%cr3" : : : "eax");
}

/**
 * Handle page invalidation IPI
 */
void smp_handle_invalidate_page_ipi(void) {
    /* Invalidate page */
    /* This would be implemented with architecture-specific code */
}

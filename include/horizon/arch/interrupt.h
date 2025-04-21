/**
 * arch/interrupt.h - Horizon kernel architecture-specific interrupt handling definitions
 * 
 * This file contains architecture-specific definitions for the interrupt handling subsystem.
 */

#ifndef _HORIZON_ARCH_INTERRUPT_H
#define _HORIZON_ARCH_INTERRUPT_H

#include <horizon/types.h>
#include <horizon/interrupt.h>

/* Architecture-specific interrupt initialization */
void arch_interrupt_init(void);

/* Architecture-specific interrupt setup */
int arch_interrupt_setup(unsigned int irq, unsigned int flow_type);

/* Architecture-specific interrupt enable */
void arch_interrupt_enable(unsigned int irq);

/* Architecture-specific interrupt disable */
void arch_interrupt_disable(unsigned int irq);

/* Architecture-specific interrupt acknowledge */
void arch_interrupt_ack(unsigned int irq);

/* Architecture-specific interrupt end of interrupt */
void arch_interrupt_eoi(unsigned int irq);

/* Architecture-specific interrupt set affinity */
int arch_interrupt_set_affinity(unsigned int irq, const struct cpumask *dest);

/* Architecture-specific interrupt entry point */
void arch_interrupt_entry(struct interrupt_frame *frame);

/* Architecture-specific interrupt exit point */
void arch_interrupt_exit(struct interrupt_frame *frame);

/* Architecture-specific interrupt save flags */
void arch_interrupt_save_flags(unsigned long *flags);

/* Architecture-specific interrupt restore flags */
void arch_interrupt_restore_flags(unsigned long flags);

/* Architecture-specific interrupt enable all */
void arch_interrupt_enable_all(void);

/* Architecture-specific interrupt disable all */
void arch_interrupt_disable_all(void);

/* Architecture-specific interrupt descriptor table setup */
void arch_interrupt_setup_idt(void);

/* Architecture-specific interrupt vector setup */
void arch_interrupt_setup_vector(unsigned int vector, void (*handler)(void));

/* Architecture-specific interrupt controller setup */
void arch_interrupt_setup_controller(void);

/* Architecture-specific interrupt controller initialization */
void arch_interrupt_init_controller(void);

/* Architecture-specific interrupt controller shutdown */
void arch_interrupt_shutdown_controller(void);

/* Architecture-specific interrupt controller enable */
void arch_interrupt_enable_controller(void);

/* Architecture-specific interrupt controller disable */
void arch_interrupt_disable_controller(void);

/* Architecture-specific interrupt controller mask */
void arch_interrupt_mask_controller(void);

/* Architecture-specific interrupt controller unmask */
void arch_interrupt_unmask_controller(void);

/* Architecture-specific interrupt controller end of interrupt */
void arch_interrupt_eoi_controller(void);

/* Architecture-specific interrupt controller set type */
int arch_interrupt_set_type_controller(unsigned int irq, unsigned int flow_type);

/* Architecture-specific interrupt controller set affinity */
int arch_interrupt_set_affinity_controller(unsigned int irq, const struct cpumask *dest);

#endif /* _HORIZON_ARCH_INTERRUPT_H */

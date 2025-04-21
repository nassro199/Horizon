/**
 * interrupt.h - Horizon kernel interrupt handling definitions
 *
 * This file contains definitions for the interrupt handling subsystem.
 * The definitions are compatible with Linux.
 */

#ifndef _HORIZON_INTERRUPT_H
#define _HORIZON_INTERRUPT_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/cpumask.h>

/* Interrupt flags */
#define IRQF_TRIGGER_NONE       0x00000000  /* No trigger */
#define IRQF_TRIGGER_RISING     0x00000001  /* Rising edge trigger */
#define IRQF_TRIGGER_FALLING    0x00000002  /* Falling edge trigger */
#define IRQF_TRIGGER_HIGH       0x00000004  /* High level trigger */
#define IRQF_TRIGGER_LOW        0x00000008  /* Low level trigger */
#define IRQF_TRIGGER_MASK       0x0000000f  /* Trigger mask */
#define IRQF_SHARED             0x00000080  /* Shared IRQ */
#define IRQF_PROBE_SHARED       0x00000100  /* Probe for shared IRQ */
#define IRQF_TIMER              0x00000200  /* Timer interrupt */
#define IRQF_PERCPU             0x00000400  /* Per-CPU interrupt */
#define IRQF_NOBALANCING        0x00000800  /* No IRQ balancing */
#define IRQF_IRQPOLL            0x00001000  /* IRQ polling */
#define IRQF_ONESHOT            0x00002000  /* One-shot interrupt */
#define IRQF_NO_SUSPEND         0x00004000  /* Do not suspend */
#define IRQF_FORCE_RESUME       0x00008000  /* Force resume */
#define IRQF_NO_THREAD          0x00010000  /* Do not thread */
#define IRQF_EARLY_RESUME       0x00020000  /* Early resume */
#define IRQF_COND_SUSPEND       0x00040000  /* Conditional suspend */

/* Interrupt status */
#define IRQ_NONE                0  /* Interrupt was not from this device */
#define IRQ_HANDLED             1  /* Interrupt was handled by this device */
#define IRQ_WAKE_THREAD         2  /* Interrupt should wake the handler thread */

/* Interrupt types */
#define IRQ_TYPE_NONE           0  /* No interrupt */
#define IRQ_TYPE_EDGE_RISING    1  /* Rising edge triggered */
#define IRQ_TYPE_EDGE_FALLING   2  /* Falling edge triggered */
#define IRQ_TYPE_EDGE_BOTH      3  /* Rising and falling edge triggered */
#define IRQ_TYPE_LEVEL_HIGH     4  /* High level triggered */
#define IRQ_TYPE_LEVEL_LOW      5  /* Low level triggered */

/* Interrupt priority levels */
#define INT_PRIO_HIGHEST        0  /* Highest priority */
#define INT_PRIO_HIGH           1  /* High priority */
#define INT_PRIO_NORMAL         2  /* Normal priority */
#define INT_PRIO_LOW            3  /* Low priority */
#define INT_PRIO_LOWEST         4  /* Lowest priority */
#define INT_PRIO_DEFAULT        INT_PRIO_NORMAL  /* Default priority */

/* Interrupt frame */
typedef struct interrupt_frame {
    unsigned long ip;              /* Instruction pointer */
    unsigned long cs;              /* Code segment */
    unsigned long flags;           /* Flags */
    unsigned long sp;              /* Stack pointer */
    unsigned long ss;              /* Stack segment */
} interrupt_frame_t;

/* Interrupt handler */
typedef struct interrupt_handler {
    void (*handler)(struct interrupt_frame *);  /* Handler function */
    unsigned int irq;              /* IRQ number */
    const char *name;              /* Handler name */
    unsigned long flags;           /* Handler flags */
    void *dev_id;                  /* Device ID */
    struct interrupt_handler *next; /* Next handler */
} interrupt_handler_t;

/* Interrupt controller */
typedef struct interrupt_controller {
    const char *name;              /* Controller name */
    int (*startup)(unsigned int irq);  /* Start up an interrupt */
    void (*shutdown)(unsigned int irq);  /* Shut down an interrupt */
    void (*enable)(unsigned int irq);  /* Enable an interrupt */
    void (*disable)(unsigned int irq);  /* Disable an interrupt */
    void (*ack)(unsigned int irq);  /* Acknowledge an interrupt */
    void (*mask)(unsigned int irq);  /* Mask an interrupt */
    void (*unmask)(unsigned int irq);  /* Unmask an interrupt */
    void (*eoi)(unsigned int irq);  /* End of interrupt */
    int (*set_type)(unsigned int irq, unsigned int flow_type);  /* Set interrupt type */
    int (*set_affinity)(unsigned int irq, const struct cpumask *dest);  /* Set interrupt affinity */
    struct interrupt_controller *next;  /* Next controller */
} interrupt_controller_t;

/* Interrupt descriptor */
typedef struct interrupt_desc {
    unsigned int irq;              /* IRQ number */
    unsigned int status;           /* IRQ status */
    unsigned int depth;            /* Disable depth */
    unsigned int priority;         /* Interrupt priority */
    unsigned int handler_count;    /* Handler count */
    struct interrupt_handler *handlers;  /* Handlers */
    struct interrupt_controller *controller;  /* Controller */
} interrupt_desc_t;

/* Interrupt functions */
void interrupt_init(void);
int interrupt_register_handler(unsigned int irq, void (*handler)(struct interrupt_frame *));
int interrupt_unregister_handler(unsigned int irq, void (*handler)(struct interrupt_frame *));
int interrupt_register_controller(struct interrupt_controller *controller);
int interrupt_unregister_controller(struct interrupt_controller *controller);
int interrupt_enable(unsigned int irq);
int interrupt_disable(unsigned int irq);
int interrupt_set_type(unsigned int irq, unsigned int flow_type);
int interrupt_set_priority(unsigned int irq, unsigned int priority);
unsigned int interrupt_get_priority(unsigned int irq);
int interrupt_set_affinity(unsigned int irq, const struct cpumask *dest);
void interrupt_handle(unsigned int irq, struct interrupt_frame *frame);
void interrupt_dispatch(struct interrupt_frame *frame);
void interrupt_eoi(unsigned int irq);
int interrupt_in_interrupt(void);
void interrupt_enable_all(void);
void interrupt_disable_all(void);
void interrupt_save_flags(unsigned long *flags);
void interrupt_restore_flags(unsigned long flags);
int interrupt_defer_work(void (*func)(void *data), void *data);
void check_deferred_work(void);

/* Interrupt service routine */
#define ISR(irq) void isr_##irq(struct interrupt_frame *frame)

/* Interrupt request */
#define IRQ(irq) void irq_##irq(struct interrupt_frame *frame)

/* Interrupt descriptor table */
extern void *interrupt_descriptor_table[];

/* Interrupt vector table */
extern void *interrupt_vector_table[];

/* Interrupt handlers */
extern struct interrupt_handler *interrupt_handlers[];

/* Interrupt descriptors */
extern struct interrupt_desc interrupt_descs[];

/* Current interrupt */
extern unsigned int current_interrupt;

/* Interrupt nesting level */
extern unsigned int interrupt_nesting_level;

#endif /* _HORIZON_INTERRUPT_H */

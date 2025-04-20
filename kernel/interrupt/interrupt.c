/**
 * interrupt.c - Horizon kernel interrupt handling
 * 
 * This file contains the implementation of the interrupt handling subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/interrupt.h>
#include <horizon/spinlock.h>
#include <horizon/errno.h>

/* Interrupt descriptors */
static struct interrupt_desc interrupt_descs[256];

/* Interrupt lock */
static spinlock_t interrupt_lock = SPIN_LOCK_INITIALIZER;

/* Current interrupt */
unsigned int current_interrupt = 0;

/* Interrupt nesting level */
unsigned int interrupt_nesting_level = 0;

/**
 * Initialize the interrupt subsystem
 */
void interrupt_init(void) {
    int i;
    
    /* Initialize interrupt descriptors */
    for (i = 0; i < 256; i++) {
        interrupt_descs[i].irq = i;
        interrupt_descs[i].status = 0;
        interrupt_descs[i].depth = 0;
        interrupt_descs[i].priority = INT_PRIO_DEFAULT;
        interrupt_descs[i].handler_count = 0;
        interrupt_descs[i].handlers = NULL;
        interrupt_descs[i].controller = NULL;
    }
    
    /* Initialize architecture-specific interrupt handling */
    arch_interrupt_init();
}

/**
 * Register an interrupt handler
 * 
 * @param irq IRQ number
 * @param handler Handler function
 * @return 0 on success, negative error code on failure
 */
int interrupt_register_handler(unsigned int irq, void (*handler)(struct interrupt_frame *)) {
    struct interrupt_handler *new_handler;
    
    /* Check parameters */
    if (irq >= 256 || handler == NULL) {
        return -EINVAL;
    }
    
    /* Allocate handler */
    new_handler = kmalloc(sizeof(struct interrupt_handler), MEM_KERNEL);
    if (new_handler == NULL) {
        return -ENOMEM;
    }
    
    /* Initialize handler */
    new_handler->handler = handler;
    new_handler->irq = irq;
    new_handler->name = NULL;
    new_handler->flags = 0;
    new_handler->dev_id = NULL;
    
    /* Add handler to list */
    spin_lock(&interrupt_lock);
    new_handler->next = interrupt_descs[irq].handlers;
    interrupt_descs[irq].handlers = new_handler;
    interrupt_descs[irq].handler_count++;
    spin_unlock(&interrupt_lock);
    
    /* Enable the interrupt */
    interrupt_enable(irq);
    
    return 0;
}

/**
 * Unregister an interrupt handler
 * 
 * @param irq IRQ number
 * @param handler Handler function
 * @return 0 on success, negative error code on failure
 */
int interrupt_unregister_handler(unsigned int irq, void (*handler)(struct interrupt_frame *)) {
    struct interrupt_handler *curr, *prev;
    
    /* Check parameters */
    if (irq >= 256 || handler == NULL) {
        return -EINVAL;
    }
    
    /* Find handler */
    spin_lock(&interrupt_lock);
    prev = NULL;
    curr = interrupt_descs[irq].handlers;
    while (curr != NULL) {
        if (curr->handler == handler) {
            /* Remove handler from list */
            if (prev == NULL) {
                interrupt_descs[irq].handlers = curr->next;
            } else {
                prev->next = curr->next;
            }
            interrupt_descs[irq].handler_count--;
            
            /* Free handler */
            kfree(curr);
            
            /* Disable the interrupt if no more handlers */
            if (interrupt_descs[irq].handler_count == 0) {
                interrupt_disable(irq);
            }
            
            spin_unlock(&interrupt_lock);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    spin_unlock(&interrupt_lock);
    
    /* Handler not found */
    return -ENOENT;
}

/**
 * Register an interrupt controller
 * 
 * @param controller Controller structure
 * @return 0 on success, negative error code on failure
 */
int interrupt_register_controller(struct interrupt_controller *controller) {
    /* Check parameters */
    if (controller == NULL) {
        return -EINVAL;
    }
    
    /* TODO: Implement controller registration */
    
    return 0;
}

/**
 * Unregister an interrupt controller
 * 
 * @param controller Controller structure
 * @return 0 on success, negative error code on failure
 */
int interrupt_unregister_controller(struct interrupt_controller *controller) {
    /* Check parameters */
    if (controller == NULL) {
        return -EINVAL;
    }
    
    /* TODO: Implement controller unregistration */
    
    return 0;
}

/**
 * Enable an interrupt
 * 
 * @param irq IRQ number
 * @return 0 on success, negative error code on failure
 */
int interrupt_enable(unsigned int irq) {
    /* Check parameters */
    if (irq >= 256) {
        return -EINVAL;
    }
    
    /* Enable the interrupt */
    spin_lock(&interrupt_lock);
    if (interrupt_descs[irq].depth > 0) {
        interrupt_descs[irq].depth--;
    }
    if (interrupt_descs[irq].depth == 0) {
        if (interrupt_descs[irq].controller != NULL && interrupt_descs[irq].controller->enable != NULL) {
            interrupt_descs[irq].controller->enable(irq);
        }
    }
    spin_unlock(&interrupt_lock);
    
    return 0;
}

/**
 * Disable an interrupt
 * 
 * @param irq IRQ number
 * @return 0 on success, negative error code on failure
 */
int interrupt_disable(unsigned int irq) {
    /* Check parameters */
    if (irq >= 256) {
        return -EINVAL;
    }
    
    /* Disable the interrupt */
    spin_lock(&interrupt_lock);
    if (interrupt_descs[irq].depth == 0) {
        if (interrupt_descs[irq].controller != NULL && interrupt_descs[irq].controller->disable != NULL) {
            interrupt_descs[irq].controller->disable(irq);
        }
    }
    interrupt_descs[irq].depth++;
    spin_unlock(&interrupt_lock);
    
    return 0;
}

/**
 * Set the interrupt type
 * 
 * @param irq IRQ number
 * @param flow_type Flow type
 * @return 0 on success, negative error code on failure
 */
int interrupt_set_type(unsigned int irq, unsigned int flow_type) {
    /* Check parameters */
    if (irq >= 256) {
        return -EINVAL;
    }
    
    /* Set the interrupt type */
    spin_lock(&interrupt_lock);
    if (interrupt_descs[irq].controller != NULL && interrupt_descs[irq].controller->set_type != NULL) {
        int ret = interrupt_descs[irq].controller->set_type(irq, flow_type);
        spin_unlock(&interrupt_lock);
        return ret;
    }
    spin_unlock(&interrupt_lock);
    
    return -ENOSYS;
}

/**
 * Set the interrupt priority
 * 
 * @param irq IRQ number
 * @param priority Priority level
 * @return 0 on success, negative error code on failure
 */
int interrupt_set_priority(unsigned int irq, unsigned int priority) {
    /* Check parameters */
    if (irq >= 256 || priority > INT_PRIO_LOWEST) {
        return -EINVAL;
    }
    
    /* Set the interrupt priority */
    spin_lock(&interrupt_lock);
    interrupt_descs[irq].priority = priority;
    spin_unlock(&interrupt_lock);
    
    return 0;
}

/**
 * Get the interrupt priority
 * 
 * @param irq IRQ number
 * @return Priority level, or INT_PRIO_DEFAULT on error
 */
unsigned int interrupt_get_priority(unsigned int irq) {
    unsigned int priority;
    
    /* Check parameters */
    if (irq >= 256) {
        return INT_PRIO_DEFAULT;
    }
    
    /* Get the interrupt priority */
    spin_lock(&interrupt_lock);
    priority = interrupt_descs[irq].priority;
    spin_unlock(&interrupt_lock);
    
    return priority;
}

/**
 * Set the interrupt affinity
 * 
 * @param irq IRQ number
 * @param dest CPU mask
 * @return 0 on success, negative error code on failure
 */
int interrupt_set_affinity(unsigned int irq, const struct cpumask *dest) {
    /* Check parameters */
    if (irq >= 256 || dest == NULL) {
        return -EINVAL;
    }
    
    /* Set the interrupt affinity */
    spin_lock(&interrupt_lock);
    if (interrupt_descs[irq].controller != NULL && interrupt_descs[irq].controller->set_affinity != NULL) {
        int ret = interrupt_descs[irq].controller->set_affinity(irq, dest);
        spin_unlock(&interrupt_lock);
        return ret;
    }
    spin_unlock(&interrupt_lock);
    
    return -ENOSYS;
}

/**
 * Handle an interrupt
 * 
 * @param irq IRQ number
 * @param frame Interrupt frame
 */
void interrupt_handle(unsigned int irq, struct interrupt_frame *frame) {
    struct interrupt_handler *handler;
    
    /* Check parameters */
    if (irq >= 256 || frame == NULL) {
        return;
    }
    
    /* Save current interrupt */
    unsigned int prev_interrupt = current_interrupt;
    current_interrupt = irq;
    
    /* Call handlers */
    spin_lock(&interrupt_lock);
    handler = interrupt_descs[irq].handlers;
    while (handler != NULL) {
        handler->handler(frame);
        handler = handler->next;
    }
    spin_unlock(&interrupt_lock);
    
    /* Restore current interrupt */
    current_interrupt = prev_interrupt;
}

/**
 * Dispatch an interrupt
 * 
 * @param frame Interrupt frame
 */
void interrupt_dispatch(struct interrupt_frame *frame) {
    /* Check parameters */
    if (frame == NULL) {
        return;
    }
    
    /* Dispatch the interrupt */
    if (frame->ip < 32) {
        /* Exception */
        interrupt_handle(frame->ip, frame);
    } else if (frame->ip < 48) {
        /* IRQ */
        interrupt_handle(frame->ip, frame);
        interrupt_eoi(frame->ip - 32);
    } else {
        /* Software interrupt */
        interrupt_handle(frame->ip, frame);
    }
}

/**
 * End of interrupt
 * 
 * @param irq IRQ number
 */
void interrupt_eoi(unsigned int irq) {
    /* Check parameters */
    if (irq >= 16) {
        return;
    }
    
    /* Send EOI */
    spin_lock(&interrupt_lock);
    if (interrupt_descs[irq + 32].controller != NULL && interrupt_descs[irq + 32].controller->eoi != NULL) {
        interrupt_descs[irq + 32].controller->eoi(irq);
    }
    spin_unlock(&interrupt_lock);
}

/**
 * Check if we're in an interrupt context
 * 
 * @return 1 if in interrupt, 0 if not
 */
int interrupt_in_interrupt(void) {
    return interrupt_nesting_level > 0;
}

/**
 * Enable all interrupts
 */
void interrupt_enable_all(void) {
    __asm__ volatile("sti");
}

/**
 * Disable all interrupts
 */
void interrupt_disable_all(void) {
    __asm__ volatile("cli");
}

/**
 * Save interrupt flags
 * 
 * @param flags Pointer to store flags
 */
void interrupt_save_flags(unsigned long *flags) {
    __asm__ volatile("pushf; pop %0" : "=r" (*flags));
}

/**
 * Restore interrupt flags
 * 
 * @param flags Flags to restore
 */
void interrupt_restore_flags(unsigned long flags) {
    __asm__ volatile("push %0; popf" : : "r" (flags));
}

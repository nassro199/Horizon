/**
 * timer.c - Horizon kernel x86 timer implementation
 *
 * This file contains the implementation of the x86 timer subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/timer.h>
#include <horizon/interrupt.h>
#include <horizon/io.h>

/* PIT (Programmable Interval Timer) ports */
#define PIT_CHANNEL0     0x40    /* Channel 0 data port */
#define PIT_CHANNEL1     0x41    /* Channel 1 data port */
#define PIT_CHANNEL2     0x42    /* Channel 2 data port */
#define PIT_COMMAND      0x43    /* Command register */

/* PIT commands */
#define PIT_CMD_CHANNEL0 0x00    /* Select channel 0 */
#define PIT_CMD_LATCH    0x00    /* Latch counter value command */
#define PIT_CMD_ACCESS   0x30    /* Access mode: low byte then high byte */
#define PIT_CMD_MODE3    0x06    /* Mode 3: square wave generator */
#define PIT_CMD_BINARY   0x00    /* 16-bit binary counter */

/* PIT frequency */
#define PIT_FREQUENCY    1193182 /* PIT input frequency in Hz */

/* Timer IRQ */
#define TIMER_IRQ        0       /* IRQ 0 */

/* Timer frequency */
static u32 timer_frequency = 0;

/* Timer IRQ handler */
static void timer_irq_handler(struct interrupt_frame *frame) {
    /* Call the timer tick handler */
    timer_tick();
}

/**
 * Initialize the architecture-specific timer
 * 
 * @param frequency Timer frequency in Hz
 */
void arch_timer_init(u32 frequency) {
    u32 divisor;

    /* Set the timer frequency */
    timer_frequency = frequency;

    /* Calculate the divisor */
    divisor = PIT_FREQUENCY / frequency;
    if (divisor > 65535) {
        divisor = 65535;
    }

    /* Set the PIT mode */
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_ACCESS | PIT_CMD_MODE3 | PIT_CMD_BINARY);

    /* Set the PIT frequency */
    outb(PIT_CHANNEL0, divisor & 0xFF);           /* Low byte */
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);    /* High byte */

    /* Register the timer IRQ handler */
    interrupt_register_handler(TIMER_IRQ, timer_irq_handler);
}

/**
 * Start the architecture-specific timer
 */
void arch_timer_start(void) {
    /* Enable the timer IRQ */
    interrupt_enable(TIMER_IRQ);
}

/**
 * Stop the architecture-specific timer
 */
void arch_timer_stop(void) {
    /* Disable the timer IRQ */
    interrupt_disable(TIMER_IRQ);
}

/**
 * Read the architecture-specific timer
 * 
 * @return Timer value
 */
u64 arch_timer_read(void) {
    u32 count;

    /* Latch the counter value */
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_LATCH);

    /* Read the counter value */
    count = inb(PIT_CHANNEL0);           /* Low byte */
    count |= inb(PIT_CHANNEL0) << 8;     /* High byte */

    return count;
}

/**
 * Set the architecture-specific timer frequency
 * 
 * @param frequency Timer frequency in Hz
 */
void arch_timer_set_frequency(u32 frequency) {
    /* Stop the timer */
    arch_timer_stop();

    /* Initialize the timer with the new frequency */
    arch_timer_init(frequency);

    /* Start the timer */
    arch_timer_start();
}

/**
 * Get the architecture-specific timer frequency
 * 
 * @return Timer frequency in Hz
 */
u32 arch_timer_get_frequency(void) {
    return timer_frequency;
}

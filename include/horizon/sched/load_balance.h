/**
 * load_balance.h - Horizon kernel load balancing definitions
 * 
 * This file contains definitions for the CPU load balancing subsystem.
 */

#ifndef _HORIZON_SCHED_LOAD_BALANCE_H
#define _HORIZON_SCHED_LOAD_BALANCE_H

#include <horizon/types.h>

/* Initialize the load balancing subsystem */
void load_balance_init(void);

/* Enable or disable load balancing */
int load_balance_enable(int enable);

/* Set the load balancing interval */
int load_balance_set_interval(u64 interval);

/* Set the load balancing threshold */
int load_balance_set_threshold(u64 threshold);

/* Check if load balancing is needed */
int load_balance_needed(void);

/* Check if there is an imbalance between CPUs */
int load_balance_check_imbalance(void);

/* Balance the load between CPUs */
int load_balance_run(void);

/* Print load balancing statistics */
void load_balance_print_stats(void);

#endif /* _HORIZON_SCHED_LOAD_BALANCE_H */

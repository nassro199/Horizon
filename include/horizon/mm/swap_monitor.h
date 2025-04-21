/**
 * swap_monitor.h - Horizon kernel swap monitoring definitions
 * 
 * This file contains definitions for the swap monitoring subsystem.
 */

#ifndef _HORIZON_MM_SWAP_MONITOR_H
#define _HORIZON_MM_SWAP_MONITOR_H

#include <horizon/types.h>

/* Swap monitoring statistics structure */
typedef struct swap_monitor_stats {
    int enabled;            /* Monitoring is enabled */
    int auto_adjust;        /* Auto-adjustment is enabled */
    u64 interval;           /* Monitoring interval in milliseconds */
    u64 threshold;          /* Swap threshold in percent */
    u64 start_time;         /* Monitoring start time */
    u64 last_time;          /* Last update time */
    u64 count;              /* Number of updates */
    u64 swap_in_total;      /* Total pages swapped in */
    u64 swap_out_total;     /* Total pages swapped out */
    u64 swap_in_rate;       /* Pages swapped in per second */
    u64 swap_out_rate;      /* Pages swapped out per second */
    u64 swap_in_peak;       /* Peak pages swapped in per second */
    u64 swap_out_peak;      /* Peak pages swapped out per second */
    u64 swap_in_last;       /* Pages swapped in during last interval */
    u64 swap_out_last;      /* Pages swapped out during last interval */
    u64 swap_pressure;      /* Swap pressure in percent */
} swap_monitor_stats_t;

/* Initialize the swap monitoring subsystem */
void swap_monitor_init(void);

/* Start swap monitoring */
int swap_monitor_start(void);

/* Stop swap monitoring */
int swap_monitor_stop(void);

/* Set the monitoring interval */
int swap_monitor_set_interval(u64 interval);

/* Set the swap threshold */
int swap_monitor_set_threshold(u64 threshold);

/* Enable or disable auto-adjustment */
int swap_monitor_set_auto_adjust(int enable);

/* Update swap monitoring */
void swap_monitor_update(u64 swap_in, u64 swap_out);

/* Adjust swap parameters based on monitoring */
void swap_monitor_adjust(void);

/* Get swap monitoring statistics */
int swap_monitor_get_stats(swap_monitor_stats_t *stats);

/* Print swap monitoring statistics */
void swap_monitor_print_stats(void);

#endif /* _HORIZON_MM_SWAP_MONITOR_H */

/**
 * config.h - Horizon kernel configuration
 *
 * This file contains kernel configuration options.
 */

#ifndef _HORIZON_CONFIG_H
#define _HORIZON_CONFIG_H

/* Number of CPUs */
#define CONFIG_NR_CPUS 8

/* Maximum number of processes */
#define CONFIG_MAX_PROCESSES 1024

/* Maximum number of threads per process */
#define CONFIG_MAX_THREADS 64

/* Maximum number of file descriptors per process */
#define CONFIG_MAX_FILES 1024

/* Maximum number of memory regions per process */
#define CONFIG_MAX_MEMORY_REGIONS 64

/* Maximum number of semaphores */
#define CONFIG_MAX_SEMAPHORES 1024

/* Maximum number of message queues */
#define CONFIG_MAX_MSGQUEUES 1024

/* Maximum number of shared memory segments */
#define CONFIG_MAX_SHMSEGS 1024

/* Maximum number of timers */
#define CONFIG_MAX_TIMERS 1024

/* Maximum number of signals */
#define CONFIG_MAX_SIGNALS 64

/* Maximum number of network connections */
#define CONFIG_MAX_CONNECTIONS 1024

/* Maximum number of network interfaces */
#define CONFIG_MAX_INTERFACES 16

/* Maximum number of network routes */
#define CONFIG_MAX_ROUTES 1024

/* Maximum number of network protocols */
#define CONFIG_MAX_PROTOCOLS 16

/* Maximum number of network sockets */
#define CONFIG_MAX_SOCKETS 1024

/* Maximum number of network socket options */
#define CONFIG_MAX_SOCKET_OPTIONS 16

/* Maximum number of network socket types */
#define CONFIG_MAX_SOCKET_TYPES 16

/* Maximum number of network socket families */
#define CONFIG_MAX_SOCKET_FAMILIES 16

/* Maximum number of network socket protocols */
#define CONFIG_MAX_SOCKET_PROTOCOLS 16

/* Maximum number of network socket addresses */
#define CONFIG_MAX_SOCKET_ADDRESSES 16

/* Maximum number of network socket operations */
#define CONFIG_MAX_SOCKET_OPERATIONS 16

/* Maximum number of network socket options */
#define CONFIG_MAX_SOCKET_OPTIONS 16

/* Maximum number of network socket types */
#define CONFIG_MAX_SOCKET_TYPES 16

/* Maximum number of network socket families */
#define CONFIG_MAX_SOCKET_FAMILIES 16

/* Maximum number of network socket protocols */
#define CONFIG_MAX_SOCKET_PROTOCOLS 16

/* Maximum number of network socket addresses */
#define CONFIG_MAX_SOCKET_ADDRESSES 16

/* Maximum number of network socket operations */
#define CONFIG_MAX_SOCKET_OPERATIONS 16

#endif /* _HORIZON_CONFIG_H */

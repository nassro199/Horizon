/**
 * mprotect.h - Horizon kernel memory protection definitions
 * 
 * This file contains definitions for memory protection.
 */

#ifndef _HORIZON_MM_MPROTECT_H
#define _HORIZON_MM_MPROTECT_H

#include <horizon/types.h>

/* Memory protection flags */
#define PROT_NONE       0x00    /* No access */
#define PROT_READ       0x01    /* Read access */
#define PROT_WRITE      0x02    /* Write access */
#define PROT_EXEC       0x04    /* Execute access */
#define PROT_SEM        0x08    /* Special execute mode */
#define PROT_GROWSDOWN  0x10    /* Stack segment grows down */
#define PROT_GROWSUP    0x20    /* Stack segment grows up */

/* Memory protection functions */
int mprotect_init(void);
int mprotect_check(void *addr, size_t len, unsigned int prot);
int mprotect_set(void *addr, size_t len, unsigned int prot);
int mprotect_get(void *addr, unsigned int *prot);
int mprotect_lock(void *addr, size_t len);
int mprotect_unlock(void *addr, size_t len);

#endif /* _HORIZON_MM_MPROTECT_H */

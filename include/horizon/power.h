/**
 * power.h - Horizon kernel power management definitions
 * 
 * This file contains definitions for power management.
 */

#ifndef _HORIZON_POWER_H
#define _HORIZON_POWER_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/spinlock.h>

/* Power states */
#define POWER_STATE_ON       0  /* System is on */
#define POWER_STATE_SLEEP    1  /* System is sleeping */
#define POWER_STATE_SUSPEND  2  /* System is suspended */
#define POWER_STATE_HIBERNATE 3 /* System is hibernating */
#define POWER_STATE_OFF      4  /* System is off */

/* Power events */
#define POWER_EVENT_SUSPEND  0  /* System is suspending */
#define POWER_EVENT_RESUME   1  /* System is resuming */
#define POWER_EVENT_HIBERNATE 2 /* System is hibernating */
#define POWER_EVENT_THAW     3  /* System is thawing */
#define POWER_EVENT_SHUTDOWN 4  /* System is shutting down */
#define POWER_EVENT_REBOOT   5  /* System is rebooting */

/* Power device states */
#define POWER_DEV_ON         0  /* Device is on */
#define POWER_DEV_SLEEP      1  /* Device is sleeping */
#define POWER_DEV_SUSPEND    2  /* Device is suspended */
#define POWER_DEV_OFF        3  /* Device is off */

/* Power device flags */
#define POWER_DEV_WAKEUP     0x01  /* Device can wake up system */
#define POWER_DEV_AUTOSUSPEND 0x02 /* Device can auto-suspend */
#define POWER_DEV_NOSUSPEND  0x04  /* Device cannot suspend */

/* Power device */
typedef struct power_device {
    const char *name;              /* Device name */
    struct list_head list;         /* List of devices */
    unsigned int state;            /* Device state */
    unsigned int flags;            /* Device flags */
    int (*suspend)(struct power_device *dev);  /* Suspend device */
    int (*resume)(struct power_device *dev);   /* Resume device */
    int (*hibernate)(struct power_device *dev); /* Hibernate device */
    int (*thaw)(struct power_device *dev);     /* Thaw device */
    int (*shutdown)(struct power_device *dev); /* Shutdown device */
    int (*reboot)(struct power_device *dev);   /* Reboot device */
    void *data;                    /* Device data */
} power_device_t;

/* Power notifier */
typedef struct power_notifier {
    struct list_head list;         /* List of notifiers */
    int (*notify)(unsigned int event, void *data);  /* Notify function */
    void *data;                    /* Notifier data */
} power_notifier_t;

/* Power functions */
int power_init(void);
int power_register_device(power_device_t *dev);
int power_unregister_device(power_device_t *dev);
int power_register_notifier(power_notifier_t *notifier);
int power_unregister_notifier(power_notifier_t *notifier);
int power_suspend(void);
int power_resume(void);
int power_hibernate(void);
int power_thaw(void);
int power_shutdown(void);
int power_reboot(void);
int power_get_state(void);
int power_set_state(unsigned int state);
int power_device_suspend(power_device_t *dev);
int power_device_resume(power_device_t *dev);
int power_device_hibernate(power_device_t *dev);
int power_device_thaw(power_device_t *dev);
int power_device_shutdown(power_device_t *dev);
int power_device_reboot(power_device_t *dev);
int power_device_get_state(power_device_t *dev);
int power_device_set_state(power_device_t *dev, unsigned int state);
int power_device_can_wakeup(power_device_t *dev);
int power_device_set_wakeup(power_device_t *dev, int enable);
int power_device_can_autosuspend(power_device_t *dev);
int power_device_set_autosuspend(power_device_t *dev, int enable);
int power_device_can_suspend(power_device_t *dev);
int power_device_set_nosuspend(power_device_t *dev, int enable);

#endif /* _HORIZON_POWER_H */

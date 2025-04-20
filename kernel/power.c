/**
 * power.c - Horizon kernel power management implementation
 * 
 * This file contains the implementation of power management.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/power.h>
#include <horizon/spinlock.h>
#include <horizon/list.h>
#include <horizon/errno.h>

/* Power state */
static unsigned int power_state = POWER_STATE_ON;

/* Power lock */
static spinlock_t power_lock = SPIN_LOCK_INITIALIZER;

/* Power device list */
static LIST_HEAD(power_device_list);

/* Power notifier list */
static LIST_HEAD(power_notifier_list);

/**
 * Initialize power management
 * 
 * @return 0 on success, negative error code on failure
 */
int power_init(void) {
    /* Initialize lock */
    spin_lock_init(&power_lock, "power");
    
    return 0;
}

/**
 * Register a power device
 * 
 * @param dev Device to register
 * @return 0 on success, negative error code on failure
 */
int power_register_device(power_device_t *dev) {
    /* Check parameters */
    if (dev == NULL || dev->name == NULL) {
        return -EINVAL;
    }
    
    /* Register device */
    spin_lock(&power_lock);
    list_add(&dev->list, &power_device_list);
    spin_unlock(&power_lock);
    
    return 0;
}

/**
 * Unregister a power device
 * 
 * @param dev Device to unregister
 * @return 0 on success, negative error code on failure
 */
int power_unregister_device(power_device_t *dev) {
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Unregister device */
    spin_lock(&power_lock);
    list_del(&dev->list);
    spin_unlock(&power_lock);
    
    return 0;
}

/**
 * Register a power notifier
 * 
 * @param notifier Notifier to register
 * @return 0 on success, negative error code on failure
 */
int power_register_notifier(power_notifier_t *notifier) {
    /* Check parameters */
    if (notifier == NULL || notifier->notify == NULL) {
        return -EINVAL;
    }
    
    /* Register notifier */
    spin_lock(&power_lock);
    list_add(&notifier->list, &power_notifier_list);
    spin_unlock(&power_lock);
    
    return 0;
}

/**
 * Unregister a power notifier
 * 
 * @param notifier Notifier to unregister
 * @return 0 on success, negative error code on failure
 */
int power_unregister_notifier(power_notifier_t *notifier) {
    /* Check parameters */
    if (notifier == NULL) {
        return -EINVAL;
    }
    
    /* Unregister notifier */
    spin_lock(&power_lock);
    list_del(&notifier->list);
    spin_unlock(&power_lock);
    
    return 0;
}

/**
 * Notify power event
 * 
 * @param event Event to notify
 * @return 0 on success, negative error code on failure
 */
static int power_notify(unsigned int event) {
    power_notifier_t *notifier;
    int ret = 0;
    
    /* Notify all notifiers */
    spin_lock(&power_lock);
    list_for_each_entry(notifier, &power_notifier_list, list) {
        if (notifier->notify != NULL) {
            ret = notifier->notify(event, notifier->data);
            if (ret != 0) {
                break;
            }
        }
    }
    spin_unlock(&power_lock);
    
    return ret;
}

/**
 * Suspend system
 * 
 * @return 0 on success, negative error code on failure
 */
int power_suspend(void) {
    power_device_t *dev;
    int ret;
    
    /* Check if system is already suspended */
    if (power_state == POWER_STATE_SUSPEND) {
        return 0;
    }
    
    /* Notify suspend */
    ret = power_notify(POWER_EVENT_SUSPEND);
    if (ret != 0) {
        return ret;
    }
    
    /* Suspend all devices */
    spin_lock(&power_lock);
    list_for_each_entry(dev, &power_device_list, list) {
        if (dev->suspend != NULL && !(dev->flags & POWER_DEV_NOSUSPEND)) {
            ret = dev->suspend(dev);
            if (ret != 0) {
                /* Resume devices that were suspended */
                power_device_t *rdev;
                list_for_each_entry(rdev, &power_device_list, list) {
                    if (rdev == dev) {
                        break;
                    }
                    if (rdev->resume != NULL && !(rdev->flags & POWER_DEV_NOSUSPEND)) {
                        rdev->resume(rdev);
                    }
                }
                spin_unlock(&power_lock);
                power_notify(POWER_EVENT_RESUME);
                return ret;
            }
        }
    }
    
    /* Set power state */
    power_state = POWER_STATE_SUSPEND;
    
    spin_unlock(&power_lock);
    
    return 0;
}

/**
 * Resume system
 * 
 * @return 0 on success, negative error code on failure
 */
int power_resume(void) {
    power_device_t *dev;
    int ret;
    
    /* Check if system is suspended */
    if (power_state != POWER_STATE_SUSPEND) {
        return 0;
    }
    
    /* Resume all devices */
    spin_lock(&power_lock);
    list_for_each_entry_reverse(dev, &power_device_list, list) {
        if (dev->resume != NULL && !(dev->flags & POWER_DEV_NOSUSPEND)) {
            ret = dev->resume(dev);
            if (ret != 0) {
                spin_unlock(&power_lock);
                return ret;
            }
        }
    }
    
    /* Set power state */
    power_state = POWER_STATE_ON;
    
    spin_unlock(&power_lock);
    
    /* Notify resume */
    ret = power_notify(POWER_EVENT_RESUME);
    
    return ret;
}

/**
 * Hibernate system
 * 
 * @return 0 on success, negative error code on failure
 */
int power_hibernate(void) {
    power_device_t *dev;
    int ret;
    
    /* Check if system is already hibernating */
    if (power_state == POWER_STATE_HIBERNATE) {
        return 0;
    }
    
    /* Notify hibernate */
    ret = power_notify(POWER_EVENT_HIBERNATE);
    if (ret != 0) {
        return ret;
    }
    
    /* Hibernate all devices */
    spin_lock(&power_lock);
    list_for_each_entry(dev, &power_device_list, list) {
        if (dev->hibernate != NULL) {
            ret = dev->hibernate(dev);
            if (ret != 0) {
                /* Thaw devices that were hibernated */
                power_device_t *rdev;
                list_for_each_entry(rdev, &power_device_list, list) {
                    if (rdev == dev) {
                        break;
                    }
                    if (rdev->thaw != NULL) {
                        rdev->thaw(rdev);
                    }
                }
                spin_unlock(&power_lock);
                power_notify(POWER_EVENT_THAW);
                return ret;
            }
        }
    }
    
    /* Set power state */
    power_state = POWER_STATE_HIBERNATE;
    
    spin_unlock(&power_lock);
    
    return 0;
}

/**
 * Thaw system
 * 
 * @return 0 on success, negative error code on failure
 */
int power_thaw(void) {
    power_device_t *dev;
    int ret;
    
    /* Check if system is hibernating */
    if (power_state != POWER_STATE_HIBERNATE) {
        return 0;
    }
    
    /* Thaw all devices */
    spin_lock(&power_lock);
    list_for_each_entry_reverse(dev, &power_device_list, list) {
        if (dev->thaw != NULL) {
            ret = dev->thaw(dev);
            if (ret != 0) {
                spin_unlock(&power_lock);
                return ret;
            }
        }
    }
    
    /* Set power state */
    power_state = POWER_STATE_ON;
    
    spin_unlock(&power_lock);
    
    /* Notify thaw */
    ret = power_notify(POWER_EVENT_THAW);
    
    return ret;
}

/**
 * Shutdown system
 * 
 * @return 0 on success, negative error code on failure
 */
int power_shutdown(void) {
    power_device_t *dev;
    int ret;
    
    /* Notify shutdown */
    ret = power_notify(POWER_EVENT_SHUTDOWN);
    if (ret != 0) {
        return ret;
    }
    
    /* Shutdown all devices */
    spin_lock(&power_lock);
    list_for_each_entry(dev, &power_device_list, list) {
        if (dev->shutdown != NULL) {
            ret = dev->shutdown(dev);
            if (ret != 0) {
                spin_unlock(&power_lock);
                return ret;
            }
        }
    }
    
    /* Set power state */
    power_state = POWER_STATE_OFF;
    
    spin_unlock(&power_lock);
    
    /* Halt system */
    arch_power_off();
    
    /* Should not reach here */
    return -EINVAL;
}

/**
 * Reboot system
 * 
 * @return 0 on success, negative error code on failure
 */
int power_reboot(void) {
    power_device_t *dev;
    int ret;
    
    /* Notify reboot */
    ret = power_notify(POWER_EVENT_REBOOT);
    if (ret != 0) {
        return ret;
    }
    
    /* Reboot all devices */
    spin_lock(&power_lock);
    list_for_each_entry(dev, &power_device_list, list) {
        if (dev->reboot != NULL) {
            ret = dev->reboot(dev);
            if (ret != 0) {
                spin_unlock(&power_lock);
                return ret;
            }
        }
    }
    
    spin_unlock(&power_lock);
    
    /* Reboot system */
    arch_power_reboot();
    
    /* Should not reach here */
    return -EINVAL;
}

/**
 * Get power state
 * 
 * @return Power state
 */
int power_get_state(void) {
    return power_state;
}

/**
 * Set power state
 * 
 * @param state Power state
 * @return 0 on success, negative error code on failure
 */
int power_set_state(unsigned int state) {
    /* Check parameters */
    if (state > POWER_STATE_OFF) {
        return -EINVAL;
    }
    
    /* Set power state */
    switch (state) {
        case POWER_STATE_ON:
            if (power_state == POWER_STATE_SUSPEND) {
                return power_resume();
            } else if (power_state == POWER_STATE_HIBERNATE) {
                return power_thaw();
            }
            break;
        case POWER_STATE_SLEEP:
        case POWER_STATE_SUSPEND:
            return power_suspend();
        case POWER_STATE_HIBERNATE:
            return power_hibernate();
        case POWER_STATE_OFF:
            return power_shutdown();
    }
    
    return 0;
}

/**
 * Suspend a power device
 * 
 * @param dev Device to suspend
 * @return 0 on success, negative error code on failure
 */
int power_device_suspend(power_device_t *dev) {
    int ret;
    
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Check if device can suspend */
    if (dev->flags & POWER_DEV_NOSUSPEND) {
        return -ENOTSUP;
    }
    
    /* Suspend device */
    if (dev->suspend != NULL) {
        ret = dev->suspend(dev);
        if (ret != 0) {
            return ret;
        }
    }
    
    /* Set device state */
    dev->state = POWER_DEV_SUSPEND;
    
    return 0;
}

/**
 * Resume a power device
 * 
 * @param dev Device to resume
 * @return 0 on success, negative error code on failure
 */
int power_device_resume(power_device_t *dev) {
    int ret;
    
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Check if device is suspended */
    if (dev->state != POWER_DEV_SUSPEND) {
        return 0;
    }
    
    /* Resume device */
    if (dev->resume != NULL) {
        ret = dev->resume(dev);
        if (ret != 0) {
            return ret;
        }
    }
    
    /* Set device state */
    dev->state = POWER_DEV_ON;
    
    return 0;
}

/**
 * Hibernate a power device
 * 
 * @param dev Device to hibernate
 * @return 0 on success, negative error code on failure
 */
int power_device_hibernate(power_device_t *dev) {
    int ret;
    
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Hibernate device */
    if (dev->hibernate != NULL) {
        ret = dev->hibernate(dev);
        if (ret != 0) {
            return ret;
        }
    }
    
    /* Set device state */
    dev->state = POWER_DEV_OFF;
    
    return 0;
}

/**
 * Thaw a power device
 * 
 * @param dev Device to thaw
 * @return 0 on success, negative error code on failure
 */
int power_device_thaw(power_device_t *dev) {
    int ret;
    
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Check if device is hibernated */
    if (dev->state != POWER_DEV_OFF) {
        return 0;
    }
    
    /* Thaw device */
    if (dev->thaw != NULL) {
        ret = dev->thaw(dev);
        if (ret != 0) {
            return ret;
        }
    }
    
    /* Set device state */
    dev->state = POWER_DEV_ON;
    
    return 0;
}

/**
 * Shutdown a power device
 * 
 * @param dev Device to shutdown
 * @return 0 on success, negative error code on failure
 */
int power_device_shutdown(power_device_t *dev) {
    int ret;
    
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Shutdown device */
    if (dev->shutdown != NULL) {
        ret = dev->shutdown(dev);
        if (ret != 0) {
            return ret;
        }
    }
    
    /* Set device state */
    dev->state = POWER_DEV_OFF;
    
    return 0;
}

/**
 * Reboot a power device
 * 
 * @param dev Device to reboot
 * @return 0 on success, negative error code on failure
 */
int power_device_reboot(power_device_t *dev) {
    int ret;
    
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Reboot device */
    if (dev->reboot != NULL) {
        ret = dev->reboot(dev);
        if (ret != 0) {
            return ret;
        }
    }
    
    /* Set device state */
    dev->state = POWER_DEV_ON;
    
    return 0;
}

/**
 * Get power device state
 * 
 * @param dev Device to get state of
 * @return Device state, or negative error code on failure
 */
int power_device_get_state(power_device_t *dev) {
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    return dev->state;
}

/**
 * Set power device state
 * 
 * @param dev Device to set state of
 * @param state Device state
 * @return 0 on success, negative error code on failure
 */
int power_device_set_state(power_device_t *dev, unsigned int state) {
    /* Check parameters */
    if (dev == NULL || state > POWER_DEV_OFF) {
        return -EINVAL;
    }
    
    /* Set device state */
    switch (state) {
        case POWER_DEV_ON:
            if (dev->state == POWER_DEV_SUSPEND) {
                return power_device_resume(dev);
            } else if (dev->state == POWER_DEV_OFF) {
                return power_device_thaw(dev);
            }
            break;
        case POWER_DEV_SLEEP:
        case POWER_DEV_SUSPEND:
            return power_device_suspend(dev);
        case POWER_DEV_OFF:
            return power_device_hibernate(dev);
    }
    
    return 0;
}

/**
 * Check if power device can wake up system
 * 
 * @param dev Device to check
 * @return 1 if device can wake up system, 0 if not, negative error code on failure
 */
int power_device_can_wakeup(power_device_t *dev) {
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    return (dev->flags & POWER_DEV_WAKEUP) ? 1 : 0;
}

/**
 * Set power device wake up capability
 * 
 * @param dev Device to set
 * @param enable Enable flag
 * @return 0 on success, negative error code on failure
 */
int power_device_set_wakeup(power_device_t *dev, int enable) {
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Set wake up capability */
    if (enable) {
        dev->flags |= POWER_DEV_WAKEUP;
    } else {
        dev->flags &= ~POWER_DEV_WAKEUP;
    }
    
    return 0;
}

/**
 * Check if power device can auto-suspend
 * 
 * @param dev Device to check
 * @return 1 if device can auto-suspend, 0 if not, negative error code on failure
 */
int power_device_can_autosuspend(power_device_t *dev) {
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    return (dev->flags & POWER_DEV_AUTOSUSPEND) ? 1 : 0;
}

/**
 * Set power device auto-suspend capability
 * 
 * @param dev Device to set
 * @param enable Enable flag
 * @return 0 on success, negative error code on failure
 */
int power_device_set_autosuspend(power_device_t *dev, int enable) {
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Set auto-suspend capability */
    if (enable) {
        dev->flags |= POWER_DEV_AUTOSUSPEND;
    } else {
        dev->flags &= ~POWER_DEV_AUTOSUSPEND;
    }
    
    return 0;
}

/**
 * Check if power device can suspend
 * 
 * @param dev Device to check
 * @return 1 if device can suspend, 0 if not, negative error code on failure
 */
int power_device_can_suspend(power_device_t *dev) {
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    return (dev->flags & POWER_DEV_NOSUSPEND) ? 0 : 1;
}

/**
 * Set power device no-suspend capability
 * 
 * @param dev Device to set
 * @param enable Enable flag
 * @return 0 on success, negative error code on failure
 */
int power_device_set_nosuspend(power_device_t *dev, int enable) {
    /* Check parameters */
    if (dev == NULL) {
        return -EINVAL;
    }
    
    /* Set no-suspend capability */
    if (enable) {
        dev->flags |= POWER_DEV_NOSUSPEND;
    } else {
        dev->flags &= ~POWER_DEV_NOSUSPEND;
    }
    
    return 0;
}

/**
 * input.c - Input subsystem implementation
 * 
 * This file contains the implementation of the input subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/input.h>
#include <horizon/mm.h>

/* Input device list */
static input_dev_t *input_devices = NULL;

/* Input handler list */
static input_handler_t *input_handlers = NULL;

/* Initialize the input subsystem */
void input_init(void) {
    /* Initialize the device and handler lists */
    input_devices = NULL;
    input_handlers = NULL;
}

/* Register an input device */
int input_register_device(input_dev_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Add the device to the list */
    dev->next = input_devices;
    input_devices = dev;
    
    /* Connect the device to all matching handlers */
    input_handler_t *handler = input_handlers;
    while (handler != NULL) {
        if ((handler->event_types & dev->event_types) != 0) {
            handler->connect(handler, dev);
        }
        handler = handler->next;
    }
    
    return 0;
}

/* Unregister an input device */
int input_unregister_device(input_dev_t *dev) {
    if (dev == NULL) {
        return -1;
    }
    
    /* Disconnect the device from all handlers */
    input_handler_t *handler = input_handlers;
    while (handler != NULL) {
        if ((handler->event_types & dev->event_types) != 0) {
            handler->disconnect(handler, dev);
        }
        handler = handler->next;
    }
    
    /* Remove the device from the list */
    if (input_devices == dev) {
        input_devices = dev->next;
    } else {
        input_dev_t *prev = input_devices;
        while (prev != NULL && prev->next != dev) {
            prev = prev->next;
        }
        
        if (prev != NULL) {
            prev->next = dev->next;
        }
    }
    
    return 0;
}

/* Register an input handler */
int input_register_handler(input_handler_t *handler) {
    if (handler == NULL) {
        return -1;
    }
    
    /* Add the handler to the list */
    handler->next = input_handlers;
    input_handlers = handler;
    
    /* Connect the handler to all matching devices */
    input_dev_t *dev = input_devices;
    while (dev != NULL) {
        if ((handler->event_types & dev->event_types) != 0) {
            handler->connect(handler, dev);
        }
        dev = dev->next;
    }
    
    return 0;
}

/* Unregister an input handler */
int input_unregister_handler(input_handler_t *handler) {
    if (handler == NULL) {
        return -1;
    }
    
    /* Disconnect the handler from all devices */
    input_dev_t *dev = input_devices;
    while (dev != NULL) {
        if ((handler->event_types & dev->event_types) != 0) {
            handler->disconnect(handler, dev);
        }
        dev = dev->next;
    }
    
    /* Remove the handler from the list */
    if (input_handlers == handler) {
        input_handlers = handler->next;
    } else {
        input_handler_t *prev = input_handlers;
        while (prev != NULL && prev->next != handler) {
            prev = prev->next;
        }
        
        if (prev != NULL) {
            prev->next = handler->next;
        }
    }
    
    return 0;
}

/* Send an input event */
void input_event(input_dev_t *dev, u16 type, u16 code, s32 value) {
    if (dev == NULL) {
        return;
    }
    
    /* Call the device's event method */
    if (dev->event != NULL) {
        dev->event(dev, type, code, value);
    }
    
    /* Pass the event to all matching handlers */
    input_handler_t *handler = input_handlers;
    while (handler != NULL) {
        if ((handler->event_types & (1 << type)) != 0) {
            handler->event(handler, dev, type, code, value);
        }
        handler = handler->next;
    }
}

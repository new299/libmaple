#ifndef __DEVICE_H__
#define __DEVICE_H__

enum dev_state {
    UNINITIALIZED,
    SUSPENDED,
    RUNNING,
};

/* NOTE: Devices that are SUSPENDED can still wake the system up */

struct device {
    /* Performs one-time initialization, leaving the device SUSPENDED */
    int (*init)(void);

    /* Deconfigures the device, putting it in an ultra-low-power state */
    int (*deinit)(struct device *dev);

    /* Pauses the device, but maintains its state */
    int (*suspend)(struct device *dev);

    /* Pulls the device out of a suspended state */
    int (*resume)(struct device *dev);

    /* Printable name */
    const char *name;

    enum dev_state state;
};

/* Adds the device and places it in a SUSPENDED state */
int device_add(struct device *dev);

/* Moves the device from RUNNING to SUSPENDED */
int device_pause(struct device *dev);

/* Moves the device from SUSPENDED to RUNNING */
int device_resume(struct device *dev);

/* Removes the device, shutting it down */
int device_remove(struct device *dev);


int device_resume_all(void);
int device_pause_all(void);
int device_remove_all(void);


#endif /* __DEVICE_H__ */

#include "wirish.h"
#include "device.h"

#define MAX_DEV 32
static struct device *devices[MAX_DEV];
static int dev_ptr = 0;

/* Adds the device and places it in a SUSPENDED state */
int
device_add(struct device *dev)
{
    int res;
    res = dev->init();
    if (!res)
        devices[dev_ptr++] = dev;
    res = dev->init();

    /* If the device was added, mark it as suspended */
    if (!res)
        dev->state = SUSPENDED;
    return res;
}

/* Moves the device from RUNNING to SUSPENDED */
int
device_pause(struct device *dev)
{
    int res = 0;
    if (dev->state != SUSPENDED) {
        res = dev->suspend(dev);
        if (!res)
            dev->state = SUSPENDED;
    }
    return res;
}

/* Moves the device from SUSPENDED to RUNNING */
int
device_resume(struct device *dev)
{
    int res = 0;
    if (dev->state == UNINITIALIZED)
        return -1;
    if (dev->state == SUSPENDED) {
        res = dev->resume(dev);
        if (!res)
            dev->state = RUNNING;
    }
    return res;
}

/* Removes the device, shutting it down */
int
device_remove(struct device *dev)
{
    int res = 0;
    int i, j;
    if (dev->state == RUNNING) {
        res = device_pause(dev);
        if (res)
            return res;
    }

    res = dev->deinit(dev);
    if (res)
        return res;

    for (i=0; i<dev_ptr; i++) {
        if (devices[i] == dev) {
            for (j=i+1; j<dev_ptr; j++)
                devices[j-1] = devices[j];
            dev_ptr--;
        }
    }

    return 0;
}

int
device_resume_all(void)
{
    int i;
    int res = 0;
    for (i=0; i<dev_ptr; i++) {
        Serial1.print("Resuming "); Serial1.println(devices[i]->name);
        res |= device_resume(devices[i]);
    }
    return res;
}

int
device_pause_all(void)
{
    int i;
    int res = 0;
    for (i=dev_ptr-1; i>=0; i--) {
        Serial1.print("Suspending "); Serial1.println(devices[i]->name);
        res |= device_pause(devices[i]);
    }
    return res;
}

int
device_remove_all(void)
{
    int i;
    int res = 0;
    for (i=dev_ptr-1; i>=0; i--)
        res |= device_remove(devices[i]);
    return res;
}


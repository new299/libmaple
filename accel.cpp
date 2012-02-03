#include "wirish.h"



static int
accel_init(void)
{
    return 0;
}


static int
accel_resume(struct device *dev)
{
    return 0;
}


static int
accel_suspend(struct device *dev) {
    return 0;
}


static int
accel_deinit(struct device *dev)
{
    return 0;
}


struct device accel = {
    accel_init,
    accel_deinit,
    accel_suspend,
    accel_resume,
};

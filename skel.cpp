#include "wirish.h"



static int
skel_init(void)
{
    return 0;
}


static int
skel_resume(struct device *dev)
{
    return 0;
}


static int
skel_suspend(struct device *dev) {
    return 0;
}


static int
skel_deinit(struct device *dev)
{
    return 0;
}


struct device skel = {
    skel_init,
    skel_deinit,
    skel_suspend,
    skel_resume,
};

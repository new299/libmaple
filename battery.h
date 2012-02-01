#ifndef __BATTERY_H__
#define __BATTERY_H__

#include "device.h"


int BATTERY_state(struct device *dev);
int battery_is_low(void);
uint16 battery_level(void);


extern struct device battery;

#endif /* __BATTERY_H__ */

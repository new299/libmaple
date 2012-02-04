#ifndef __TIME_H__
#define __TIME_H__

#include "device.h"


void time_set(uint32 time);
uint32 time_dump_registers(void);

uint32 time_get(void);
uint32 time_hour(void);
uint32 time_minutes(void);
uint32 time_seconds(void);

extern struct device time;

#endif /* __TIME_H__ */

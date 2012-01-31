#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "device.h"

void buzzer_set_volume(int new_vol);
void buzzer_buzz_blocking(void);

extern struct device buzzer;

#endif /* __BUZZER_H__ */

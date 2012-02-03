#ifndef __POWER_H__
#define __POWER_H__

#include "device.h"

#define PWRSTATE_DOWN  0   // everything off, no logging; entered when battery is low
#define PWRSTATE_LOG   1   // system is on, listening to geiger and recording; but no UI
#define PWRSTATE_USER  2   // system is on, UI is fully active
#define PWRSTATE_BOOT  3   // during boot
#define PWRSTATE_OFF   4   // power is simply off, or cold reset
#define PWRSTATE_ERROR 5   // an error conditions state


int power_switch_state(void);

void power_set_debug(int level);
int power_set_state(int state);
int power_get_state(void);
int power_wfi(void);
int power_wfe(void);
int power_sleep(void);


/* Call this to see if the power state changed during an IRQ to actually
 * force the update to occur */
int power_needs_update(void);
void power_update(void);

extern struct device power;

#endif /* __POWER_H__ */

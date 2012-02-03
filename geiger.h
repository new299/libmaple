#ifndef __GEIGER_H__
#define __GEIGER_H__

#include "device.h"

#define PWRSTATE_DOWN  0   // everything off, no logging; entered when battery is low
#define PWRSTATE_LOG   1   // system is on, listening to geiger and recording; but no UI
#define PWRSTATE_USER  2   // system is on, UI is fully active
#define PWRSTATE_BOOT  3   // during boot
#define PWRSTATE_OFF   4   // power is simply off, or cold reset
#define PWRSTATE_ERROR 5   // an error conditions state


int geiger_state(struct device *dev);

int geiger_check_event(void); // polls for an event, for UI calls only

extern struct device geiger;

#endif /* __GEIGER_H__ */

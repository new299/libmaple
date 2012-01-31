// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "switch.h"


// for power control support
#include "pwr.h"
#include "scb.h"

#define MANUAL_WAKEUP_GPIO 18 // PC3

static int
switch_init(void)
{
    pinMode(MANUAL_WAKEUP_GPIO, INPUT);
    return 0;
}


static int
switch_suspend(struct device *dev) {
    return 0;
}


static int
switch_deinit(struct device *dev)
{
    return 0;
}

static int
switch_resume(struct device *dev)
{
    return 0;
}

int
switch_state(void)
{
    return digitalRead(MANUAL_WAKEUP_GPIO) == HIGH;
}

struct device back_switch = {
    switch_init,
    switch_deinit,
    switch_suspend,
    switch_resume,
};

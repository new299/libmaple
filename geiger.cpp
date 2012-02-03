// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "geiger.h"
#include "power.h"
#include <limits.h>

// for power control support
#include "pwr.h"
#include "scb.h"
#include "exti.h"
#include "gpio.h"

#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5

static uint32 eventCount = 0;

static void
geiger_rising(void)
{
    // for now, set to defaults but may want to lower clock rate so we're not burning battery
    // to run a CPU just while the buzzer does its thing
    rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSI_DIV_2, RCC_PLLMUL_9); 
    rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB2_HCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);
    
    Serial1.println("beep.");
    
    if (power_get_state() == PWRSTATE_LOG ) {
        // do some data logging stuff.
    } else {
        // assume we're in the power on state...
        // for now just count the events, being wary of overflow (god forbid you get 4 billion radiation events...
        if(eventCount < UINT_MAX) 
            eventCount++;
    }
}

int 
geiger_check_event(void) {
    uint32 retval = eventCount;
    if(eventCount) {
        eventCount = 0;
        return retval;
    } else {
        return 0;
    }
}

static int
geiger_init(void)
{
    pinMode(GEIGER_ON_GPIO, OUTPUT);
    digitalWrite(GEIGER_ON_GPIO, 1);
    delay_us(1000); // 1 ms for the geiger to settle

    pinMode(GEIGER_PULSE_GPIO, INPUT_PULLDOWN); // make it INPUT for production, this is for testing without a module

    attachInterrupt(GEIGER_PULSE_GPIO, geiger_rising, CHANGE);
    return 0;
}


static int
geiger_suspend(struct device *dev) {
    return 0;
}


static int
geiger_deinit(struct device *dev)
{
    digitalWrite(GEIGER_ON_GPIO, 0);
    detachInterrupt(GEIGER_PULSE_GPIO);
    return 0;
}

static int
geiger_resume(struct device *dev)
{
    return 0;
}

int
geiger_state(struct device *dev)
{
    return digitalRead(GEIGER_PULSE_GPIO) == HIGH;
}

struct device geiger = {
    geiger_init,
    geiger_deinit,
    geiger_suspend,
    geiger_resume,

    "Geiger detector",
};

// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "power.h"


// for power control support
#include "pwr.h"
#include "scb.h"

#define MAGPOWER_GPIO     41 // PA15
#define MAGSENSE_GPIO     29 // PB10
#define LIMIT_VREF_DAC    10 // PA4 -- should be DAC eventually, but GPIO initially to tied own
#define WAKEUP_GPIO       2


#define PWRSTATE_DOWN  0   // everything off, no logging; entered when battery is low
#define PWRSTATE_LOG   1   // system is on, listening to geiger and recording; but no UI
#define PWRSTATE_USER  2   // system is on, UI is fully active
#define PWRSTATE_BOOT  3   // during boot
#define PWRSTATE_OFF   4   // power is simply off, or cold reset
#define PWRSTATE_ERROR 5   // an error conditions state

// maximum range for battery, where the value is "full" and 
// 0 means the system should shut down
#define BATT_RANGE 16

static uint8 power_state = PWRSTATE_BOOT;
static uint8 last_power_state = PWRSTATE_OFF;
static uint8 debug;


static int
power_init(void)
{
    pinMode(WAKEUP_GPIO, INPUT);
    pinMode(MAGSENSE_GPIO, INPUT);

    // initially, turn off the hall effect sensor
    pinMode(MAGPOWER_GPIO, OUTPUT);
    digitalWrite(MAGPOWER_GPIO, 0);


    // as a hack, tie this low to reduce current consumption
    // until we hook it up to a proper DAC output
    pinMode(LIMIT_VREF_DAC, OUTPUT);
    digitalWrite(LIMIT_VREF_DAC, 0);

    return 0;
}

void
power_set_debug(int level)
{
    debug = level;
}

int power_sleep() {
    // sleep will wait not for pending ISRs to exit, immediate sleep
    SCB_BASE->SCR = 0;

    power_wfe(); // allow any event to wake up the CPU from sleep
    return 0;
}

static int
power_stop(struct device *dev) { 
    Serial1.println ("Stopping CPU.\n" ); // for debug only

    // hard code register values because the macros provided by libmaple are broken
    SCB_BASE->SCR = 0x4;     // set DEEPSLEEP
    PWR_BASE->CSR = 0x100;   // allow wakeup pin to wake me up
    PWR_BASE->CR = 4;        // clear the woken up flag
    PWR_BASE->CR = 1;        // set the low power regulator mode, unset PDDS, 2 for standby
    
    asm volatile (".code 16\n"
                  "wfi\n");
    return 0;
}

int
power_wfi(void)
{
    // request wait for interrupt (in-line assembly)
    asm volatile (".code 16\n"
                  "wfi\n");
    return 0;
}

int
power_wfe(void)
{
    // request wait for any event (in-line assembly)
    asm volatile (".code 16\n"
                  "wfe\n");
    return 0;
}

static int
power_deinit(struct device *dev)
{
    Serial1.println ("Putting CPU into standby.\n" );

    SCB_BASE->SCR = 0x4;     // set DEEPSLEEP
    PWR_BASE->CSR = 0x000;   // allow wakeup pin to wake me up
    PWR_BASE->CR = 4;        // clear the woken up flag
    PWR_BASE->CR = 2;        // set the low power regulator mode, unset PDDS, 2 for standby
    
    asm volatile (".code 16\n"
                  "wfi\n");
    return 0;
}

static int
power_resume(struct device *dev)
{
    return 0;
}


int
power_get_state(void)
{
    return power_state;
}

int
power_set_state(int state)
{
    if (state == power_state)
        return 0;

    last_power_state = power_state;
    power_state = state;

    return last_power_state;
}

int
power_needs_update(void)
{
    return power_state != last_power_state;
}

void
power_update(void)
{
    if (last_power_state == power_state)
        return;

    if (power_state == PWRSTATE_USER)
        device_resume_all();

    else if (power_state == PWRSTATE_LOG)
        device_pause_all();

    else if (power_state == PWRSTATE_DOWN)
        device_remove_all();

    last_power_state = power_state;
}



struct device power = {
    power_init,
    power_deinit, // called on hard power down
    power_stop,   // called on logging
    power_resume,
    "Power Management",
};

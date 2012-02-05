// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "power.h"
#include "switch.h"

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

/*
empirically, wirish does this:
AHBENR: 0x14   - SRAM clock enabled, FLITF clock enabled
APB1ENR: 0x1020403F - TIM2-7, SPI2, I2C1, PWR
APB2ENR: 0xEFFD - AFIO, IOPA-IOPG, ADC1, ADC2, TIM1, TIM8, USART1, ADC3
 */

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

void power_log_stop(void) {
    // hard code register values because the macros provided by libmaple are broken
    SCB_BASE->SCR = 0x4;     // set DEEPSLEEP
    PWR_BASE->CSR = 0x100;   // allow wakeup pin to wake me up
    PWR_BASE->CR = 4;        // clear the woken up flag
    PWR_BASE->CR = 1;        // set the low power regulator mode, unset PDDS, 2 for standby
    
    asm volatile (".code 16\n"
                  "wfi\n");
}

static int
power_stop(struct device *dev) { 
    uint32 gpioBkp[8];

    Serial1.println ("Stopping CPU.\n" ); // for debug only

    //backup GPIO state
    gpioBkp[0] = GPIOA->regs->CRL;
    gpioBkp[1] = GPIOA->regs->CRH;
    gpioBkp[2] = GPIOB->regs->CRL;
    gpioBkp[3] = GPIOB->regs->CRH;
    gpioBkp[4] = GPIOC->regs->CRL;
    gpioBkp[5] = GPIOC->regs->CRH;
    gpioBkp[6] = GPIOD->regs->CRL;
    gpioBkp[7] = GPIOD->regs->CRH;

    // force all GPIOs to inputs -- saves power
    GPIOA->regs->CRL = 0x44444444;
    GPIOA->regs->CRH = 0x44444444;
    GPIOB->regs->CRL = 0x44444444;
    GPIOB->regs->CRH = 0x44444444;
    GPIOC->regs->CRL = 0x44444424; // PC1 still an output, to drive regulators off
    GPIOC->regs->CRH = 0x44444444;
    GPIOD->regs->CRL = 0x44444444;
    GPIOD->regs->CRH = 0x44444444;

    // hard code register values because the macros provided by libmaple are broken
    SCB_BASE->SCR = 0x4;     // set DEEPSLEEP
    PWR_BASE->CSR = 0x100;   // allow wakeup pin to wake me up
    PWR_BASE->CR = 4;        // clear the woken up flag
    PWR_BASE->CR = 1;        // set the low power regulator mode, unset PDDS, 2 for standby
    
    asm volatile (".code 16\n"
                  "wfi\n");

    // restore GPIOs
    GPIOA->regs->CRL = gpioBkp[0];
    GPIOA->regs->CRH = gpioBkp[1];
    GPIOB->regs->CRL = gpioBkp[2];
    GPIOB->regs->CRH = gpioBkp[3];
    GPIOC->regs->CRL = gpioBkp[4];
    GPIOC->regs->CRH = gpioBkp[5];
    GPIOD->regs->CRL = gpioBkp[6];
    GPIOD->regs->CRH = gpioBkp[7];

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

void power_force_standby() {
    SCB_BASE->SCR = 0x4;     // set DEEPSLEEP
    PWR_BASE->CSR = 0x000;   // don't wakeup pin to wake me up
    PWR_BASE->CR = 4;        // clear the woken up flag
    PWR_BASE->CR = 2;        // set the low power regulator mode, unset PDDS, 2 for standby
    
    asm volatile (".code 16\n"
                  "wfi\n");
}

static int
power_deinit(struct device *dev)
{
    Serial1.println ("Putting CPU into standby.\n" );

    SCB_BASE->SCR = 0x4;     // set DEEPSLEEP
    PWR_BASE->CSR = 0x000;   // don't wakeup pin to wake me up
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

    if (power_state == PWRSTATE_USER) {
        if( switch_state(&back_switch) != 0 )  // something is borked in power code. this hack fixes it.
            // the problem is that despite the switch being in LOG state, something is really eager
            // about setting the power state to user. 
            device_resume_all();
        else
            power_state = PWRSTATE_LOG;
    }

    else if (power_state == PWRSTATE_LOG) {
        device_pause_all();
        if( switch_state(&back_switch) != 0 )  // something is borked in power code. this hack fixes it.
            device_resume_all();
    }

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

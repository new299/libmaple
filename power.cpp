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
static uint8 lastPowerState = PWRSTATE_OFF;
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
    SCB_BASE->SCR &= ~SCB_SCR_SLEEPONEXIT;

    power_wfe(); // allow any event to wake up the CPU from sleep
    return 0;
}

static int
power_stop(struct device *dev) { 
    Serial1.println ("Stopping CPU.\n" ); // for debug only

    // enable wake on interrupt
    PWR_BASE->CSR |= PWR_CSR_EWUP;

    // stop will wait for ISRs to exit
    SCB_BASE->SCR |= SCB_SCR_SLEEPONEXIT;
    
    /* Enter "Stop" mode */
    // clear wakup flag
    PWR_BASE->CR |= PWR_CR_CWUF;
    
    // set sleepdeep in cortex system control register
    SCB_BASE->SCR |= SCB_SCR_SLEEPDEEP;

    // select stop mode
    PWR_BASE->CR |= PWR_CR_PDDS | PWR_CR_LPDS;

    power_wfe(); 
    return 0;
}

static int
power_standby(struct device *dev) {
    // enable wake on interrupt
    PWR_BASE->CSR |= PWR_CSR_EWUP;

    // standby will not wait for ISRs to exit
    SCB_BASE->SCR &= ~SCB_SCR_SLEEPONEXIT;
    
    /* Enter "standby" mode */
    // clear wakup flag
    PWR_BASE->CR |= PWR_CR_CWUF;
    
    // set sleepdeep in cortex system control register
    SCB_BASE->SCR |= SCB_SCR_SLEEPDEEP;

    // select standby mode
    PWR_BASE->CR |= PWR_CR_PDDS;

    power_wfi(); // allow only interrupts to wake up the CPU from sleep
    return 0;
}

int
power_wfi(void)
{
    // request wait for interrupt (in-line assembly)
    asm volatile (
        "WFI\n\t" 
        );
    return 0;
}

int
power_wfe(void)
{
    // request wait for any event (in-line assembly)
    asm volatile (
        "WFE\n\t" 
        );
    return 0;
}

static int
power_deinit(struct device *dev)
{
    // disable wake on interrupt
    PWR_BASE->CSR &= ~PWR_CSR_EWUP;
    
    // set sleepdeep in cortex system control register
    SCB_BASE->SCR |= SCB_SCR_SLEEPDEEP;

    // select standby mode
    PWR_BASE->CR |= PWR_CR_PDDS;
    
    // clear wakup flag
    PWR_BASE->CSR &= ~PWR_CSR_EWUP;

    power_wfi();
    return 0;
}

static int
power_resume(struct device *dev)
{
    return 0;
}

#if 0
int
main(void)
{
    int t = 0;

    while (true) {
        switch(power_state) {
        case PWRSTATE_DOWN:  /////////// PWRSTATE_DOWN TEST STATUS: THIS CODE FUNCTIONS BUT NEEDS VALIDATION WITH AMMETER TO CONFIRM LOW POWER OPERATION.
            Serial1.println ( "Entering DOWN power_state." );
            while(1) {
                powerDown();

                // system resets when power is plugged in no matter what, so this is sort of irrelevant
                lastPowerState = PWRSTATE_DOWN;
                power_state = PWRSTATE_DOWN;
            }
            break;
        case PWRSTATE_LOG:   ////////// PWRSTATE_LOG TEST STATUS: THIS CODE IS UNTESTED
            if( isBattLow() ) {
                lastPowerState = PWRSTATE_LOG;
                power_state = PWRSTATE_DOWN;
                break;
            }
            
            if( lastPowerState != PWRSTATE_LOG ) {
                Serial1.println ( "Entering LOG power_state." );
                // we are just entering, so do things like turn off beeping, LED flashing, etc.
                prepSleep();

                // once it's all setup, re-enter the loop so we go into the next branch
                lastPowerState = PWRSTATE_LOG;
                power_state = PWRSTATE_LOG;
                break;
            } else {
                // first, we sleep and wait for an interrupt
                logStandby();

                // when we get here, we got a wakeup event
                // we'll wake up due to a switch or geiger event, so determine which and
                // then re-enter the loop
                gpio_init(GPIOC); // just init the bare minimum to read the GPIO
                pinMode(MANUAL_WAKEUP_GPIO, INPUT);

                // test code
                gpio_init(GPIOD); 
                pinMode(LED_GPIO, OUTPUT); 
                digitalWrite(LED_GPIO, 1);
                // end test code

                if( digitalRead(MANUAL_WAKEUP_GPIO) == HIGH ) {
                    init();  // need to clean up everything we shut down
                    setup_gpio();
                    touchInit = 0;  // can't assume anything about the touch interface
                    setup();

                    power_state = PWRSTATE_USER;
                    lastPowerState = PWRSTATE_LOG;
                    break;
                } else {
                    // this is a geiger event. for now, just make a beep and go back to sleep
                    // eventually, we'll want to log the vent with a timestamp to flash
                    short_init(); // special-case init for minimal operational parameters

                    setup_buzzer();
                    blockingBeep();

                    // TODO: put logging infos here...

                    power_state = PWRSTATE_LOG;
                    lastPowerState = PWRSTATE_LOG;
                    break;
                }
            }
            break;
        case PWRSTATE_USER:   ////////// PWRSTATE_LOG TEST STATUS: THIS CODE IS ROUTINELY USED FOR DEVELOPMENT AND IS LIGHTLY TESTED
            // check for events from the touchscreen
            if( lastPowerState != PWRSTATE_USER ) {
                Serial1.println ( "Entering USER power_state." );
                // setup anything specific to this state, i.e. turn on LED flashing and beeping on
                // radiation events
                setup_lcd();
                fill_oled(0); // eventually this can go away i think.
                /* Set up PB11 to be an IRQ that triggers cap_down */
                attachInterrupt(CAPTOUCH_GPIO, cap_down, FALLING);
                allowBeep = 1;
            }
            if( !touchInit ) {
                Serial1.println("Initializing captouch..." );
                setup_captouch();
                Serial1.println("Done.");
                delay(100);
            } else {
                if(touchService) {
                    touchStat = 0;
                    touchStat = mpr121Read(TCH_STATL);
                    touchStat |= mpr121Read(TCH_STATH) << 8;
                    touchService = 0;
                }
            }

            // call the event loop
            loop(t++);

            if( isBattLow() ) {
                power_state = PWRSTATE_DOWN;
                lastPowerState = PWRSTATE_USER;
                break;
            } else if( digitalRead(MANUAL_WAKEUP_GPIO) == HIGH ) {
                power_state = PWRSTATE_USER;
                lastPowerState = PWRSTATE_USER;
            } else {
                power_state = PWRSTATE_LOG;
                lastPowerState = PWRSTATE_USER;
            }
            break;
        case PWRSTATE_BOOT:   ////////// PWRSTATE_BOOT TEST STATUS: THIS CODE HAS BEEN LIGHTLY TESTED
            Serial1.begin(115200);
            Serial1.println(FIRMWARE_VERSION);
            
            Serial1.println ( "Entering BOOT power_state." );

            debug = 0;
            setup();
            allowBeep = 1;
            blockingBeep();

            // set up Flash, etc. and interrupt handlers for logging. At this point
            // we can start receiving radiation events
            setupLogging();

            if( digitalRead(MANUAL_WAKEUP_GPIO) == HIGH ) {
                power_state = PWRSTATE_USER;
            } else {
                power_state = PWRSTATE_LOG;
                touchInit = 0;
            }
            lastPowerState = PWRSTATE_BOOT;
            break;
        default:
            Serial1.println("Entering ERROR power_state." );
            power_state = PWRSTATE_BOOT;
            lastPowerState = PWRSTATE_ERROR;
        }
    }

    return 0;
}
#endif

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

    lastPowerState = power_state;
    power_state = state;

    if (state == PWRSTATE_USER)
        device_resume_all();

    else if (state == PWRSTATE_LOG)
        device_pause_all();

    else if (state == PWRSTATE_DOWN)
        device_remove_all();

    return lastPowerState;
}


struct device power = {
    power_init,
    power_deinit, // called on hard power down
    power_stop,   // called on logging
    power_resume,
    "Power Management",
};

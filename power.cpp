// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "power.h"


// for power control support
#include "pwr.h"
#include "scb.h"

#define CHG_STAT2_GPIO    44 // PC11
#define CHG_STAT1_GPIO    26 // PC10
#define MAGPOWER_GPIO     41 // PA15
#define MEASURE_FET_GPIO  45 // PC12
#define BATT_MEASURE_ADC  28 // PB1
#define MAGSENSE_GPIO     29 // PB10
#define LIMIT_VREF_DAC    10 // PA4 -- should be DAC eventually, but GPIO initially to tied own
#define CHG_TIMEREN_N_GPIO 37 // PC8
#define WAKEUP_GPIO       2

//#define CHARGE_GPIO 38


// "WASD" cluster as defined by physical arrangement of touch switches
#define W_KEY (1 << 3)
#define A_KEY (1 << 6)
#define S_KEY (1 << 4)
#define D_KEY (1 << 2)
#define Q_KEY (1 << 8)
#define E_KEY (1 << 0)

#define PWRSTATE_DOWN  0   // everything off, no logging; entered when battery is low
#define PWRSTATE_LOG   1   // system is on, listening to geiger and recording; but no UI
#define PWRSTATE_USER  2   // system is on, UI is fully active
#define PWRSTATE_BOOT  3   // during boot
#define PWRSTATE_OFF   4   // power is simply off, or cold reset
#define PWRSTATE_ERROR 5   // an error conditions state

// maximum range for battery, where the value is "full" and 
// 0 means the system should shut down
#define BATT_RANGE 16
// frequency of checking battery voltage during logging state
#define LOG_BATT_FREQ 20 

static uint8 power_state = PWRSTATE_BOOT;
static uint8 lastPowerState = PWRSTATE_OFF;
static uint8 dbg_batt = 0;


static int
power_init(void)
{
    pinMode(CHG_STAT2_GPIO, INPUT);
    pinMode(CHG_STAT1_GPIO, INPUT);
    pinMode(WAKEUP_GPIO, INPUT);
    pinMode(BATT_MEASURE_ADC, INPUT_ANALOG);
    pinMode(MAGSENSE_GPIO, INPUT);

    // setup and initialize the outputs
    // initially, don't measure battery voltage
    pinMode(MEASURE_FET_GPIO, OUTPUT);
    digitalWrite(MEASURE_FET_GPIO, 0);

    // initially, turn off the hall effect sensor
    pinMode(MAGPOWER_GPIO, OUTPUT);
    digitalWrite(MAGPOWER_GPIO, 0);


    // as a hack, tie this low to reduce current consumption
    // until we hook it up to a proper DAC output
    pinMode(LIMIT_VREF_DAC, OUTPUT);
    digitalWrite(LIMIT_VREF_DAC, 0);
    
    // initially, charge timer is enabled (active low)
    pinMode(CHG_TIMEREN_N_GPIO, OUTPUT);
    digitalWrite(CHG_TIMEREN_N_GPIO, 0);

    return 0;
}

void
power_set_debug(int level)
{
    dbg_batt = level;
}

// returns a calibrated ADC code for the current battery voltage
uint16
power_battery_level(void) {
    uint32 battVal;
    uint32 vrefVal;
    uint32 ratio;
    uint16 retcode = 0;

    uint32 cr2 = ADC1->regs->CR2;
    cr2 |= ADC_CR2_TSEREFE; // enable reference voltage only for this measurement
    ADC1->regs->CR2 = cr2;

    digitalWrite(MEASURE_FET_GPIO, 1);
    battVal = (uint32) analogRead(BATT_MEASURE_ADC) * 1000;
    digitalWrite(MEASURE_FET_GPIO, 0);

    vrefVal = (uint32) adc_read(ADC1, 17);

    cr2 &= ~ADC_CR2_TSEREFE; // power down reference to save battery power
    ADC1->regs->CR2 = cr2; 

    // calibrate
    // this is important because VDDA = VMCU which is proportional to battery voltage
    // VREF is independent of battery voltage, and is 1.2V +/- 3.4%
    // we want to indicate system should shut down at 3.1V; 4.2V is full
    // this is a ratio from 1750 (= 4.2V) to 1292 (=3.1V)
    ratio = battVal / vrefVal;
    if (dbg_batt) {
        Serial1.print( "BattVal: " );
        Serial1.println( battVal );
        Serial1.print( "VrefVal: " );
        Serial1.println( vrefVal );
        Serial1.print( "Raw ratio: " );
        Serial1.println( ratio );
    }
    if( ratio < 1292 )
        return 0;
    ratio = ratio - 1292; // should always be positive now due to test above

    retcode = ratio / (459 / BATT_RANGE);

    if (dbg_batt) {
        Serial1.print( "Rebased ratio: " );
        Serial1.println( ratio );
        Serial1.print( "Retcode: " );
        Serial1.println( retcode );
    }

    return retcode;
}


// power_is_battery_low should measure ADC and determine if the battery voltage is
// too low to continue operation. When that happens, we should immediately
// power down to prevent over-discharge of the battery.
int
power_is_battery_low(void)
{
    static uint32 count = 0;
    
    count++;

    if( power_state == PWRSTATE_LOG ) {   ////////// PWRSTATE_LOG TEST STATUS: THIS CODE IS UNTESTED
        if( (count % LOG_BATT_FREQ) == 0 ) {
            // only once every LOG_BATT_FREQ events do we actually measure the battery
            // this is to reduce power consumption
            gpio_init_all();
            afio_init();
            
            // init ADC
            rcc_set_prescaler(RCC_PRESCALER_ADC, RCC_ADCPRE_PCLK_DIV_6);
            adc_init(ADC1);

            // this is from "adcDefaultConfig" inside boards.cpp
            // lifted and modified here so *only* ADC1 is initialized
            // the default routine "does them all"
            adc_set_extsel(ADC1, ADC_SWSTART);
            adc_set_exttrig(ADC1, true);

            adc_enable(ADC1);
            adc_calibrate(ADC1);
            adc_set_sample_rate(ADC1, ADC_SMPR_55_5);

            // again, a minimal set of operations done to save power; these are lifted from
            // setup_gpio()
            pinMode(BATT_MEASURE_ADC, INPUT_ANALOG);
            pinMode(MEASURE_FET_GPIO, OUTPUT);
            digitalWrite(MEASURE_FET_GPIO, 0);
        } else {
            // on the fall-through just lie and assume battery isn't low. close enough.
            return 0;
        }
    }

    if( power_battery_level() <= 5 )  // normally 0, 5 for testing
        return 1;
    else
        return 0;
}

static int
power_suspend(struct device *dev) {
    /* Enter "Stop" mode */
    // clear wakup flag
    PWR_BASE->CR |= PWR_CR_CWUF;
    
    // set sleepdeep in cortex system control register
    SCB_BASE->SCR |= SCB_SCR_SLEEPDEEP;

    // select standby mode
    PWR_BASE->CR |= PWR_CR_PDDS | PWR_CR_LPDS;

    return 0;
}

int
power_wfi(void)
{
    // request wait for interrupt (in-line assembly)
    asm volatile (
        "WFI\n\t" // note for WFE, just replace this with WFE
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

            dbg_batt = 0;
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
    power_deinit,
    power_suspend,
    power_resume,
    "Power Management",
};

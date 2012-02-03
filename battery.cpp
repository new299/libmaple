// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "battery.h"
#include "power.h"

// for power control support
#include "pwr.h"
#include "scb.h"

#define MEASURE_FET_GPIO  45 // PC12
#define BATT_MEASURE_ADC  28 // PB1

#define CHG_TIMEREN_N_GPIO 37 // PC8

#define CHG_STAT2_GPIO    44 // PC11
#define CHG_STAT1_GPIO    26 // PC10

#define MANUAL_WAKEUP_GPIO 18 // PC3

// maximum range for battery, where the value is "full" and 
// 0 means the system should shut down
#define BATT_RANGE 16

// frequency of checking battery voltage during logging state
#define LOG_BATT_FREQ 20 

#define MEASURE_BATT_FREQ (1024*1024*5)

static HardwareTimer measure_timer(3);



static uint8 debug = 0;
static uint16 level;


void
battery_set_debug(int l)
{
    debug = l;
}


uint16
battery_level(void)
{
    return level;
}


// returns a calibrated ADC code for the current battery voltage
static uint16
battery_level_real(void)
{
    uint32 battVal;
    uint32 vrefVal;
    uint32 ratio;

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
    if (debug) {
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

    level = ratio / (459 / BATT_RANGE);

    if (debug) {
        Serial1.print( "Rebased ratio: " );
        Serial1.println( ratio );
        Serial1.print( "Retcode: " );
        Serial1.println( level );
    }

    return level;
}



// power_is_battery_low should measure ADC and determine if the battery voltage is
// too low to continue operation. When that happens, we should immediately
// power down to prevent over-discharge of the battery.
int
battery_is_low(void)
{
    static uint32 count = 0;

    count++;

    if( power_get_state() == PWRSTATE_LOG ) {   ////////// PWRSTATE_LOG TEST STATUS: THIS CODE IS UNTESTED
        if( (count % LOG_BATT_FREQ) == 0 ) {
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

            // now turn off the ADC
            rcc_reset_dev(ADC1->clk_id);
            rcc_clk_disable(ADC1->clk_id);
        } else {
            // on the fall-through just lie and assume battery isn't low. close enough.
            return 0;
        }
    }

    if( battery_level_real() <= 5 )  // normally 0, 5 for testing
        return 1;
    else
        return 0;
}



static void
take_measurement(void)
{
    static int c=0;

    if( debug ) 
        Serial1.begin(115200);

    if( battery_is_low() ) {
        if( debug ) {
            Serial1.println( "Battery is low, going into deep shutdown.\n" );
        }

        // this will call power_deinit, which is a hard powerdown
        device_remove( &power );
    } else {
        if( debug ) {
            Serial1.print("Battery measurement #"); Serial1.print(c++); Serial1.println( " is not low.\n" );
        }
        return;
    }
}



static int
battery_init(void)
{
    pinMode(CHG_STAT2_GPIO, INPUT);
    pinMode(CHG_STAT1_GPIO, INPUT);

    pinMode(BATT_MEASURE_ADC, INPUT_ANALOG);

    // setup and initialize the outputs
    // initially, don't measure battery voltage
    pinMode(MEASURE_FET_GPIO, OUTPUT);
    digitalWrite(MEASURE_FET_GPIO, 0);

    // initially, charge timer is enabled (active low)
    pinMode(CHG_TIMEREN_N_GPIO, OUTPUT);
    digitalWrite(CHG_TIMEREN_N_GPIO, 0);

    measure_timer.pause();
    measure_timer.setPeriod(MEASURE_BATT_FREQ);
    measure_timer.setChannel3Mode(TIMER_OUTPUT_COMPARE);
    measure_timer.setCompare(TIMER_CH3, 1);
    measure_timer.attachCompare3Interrupt(take_measurement);
    measure_timer.refresh();

    /* Battery is a low-level device that never gets suspended/resumed */
    measure_timer.resume();
    return 0;
}

static int
battery_resume(struct device *dev)
{
    Serial1.println ("Battery: Resuming timer." );
    measure_timer.resume();
    return 0;
}

static int
battery_suspend(struct device *dev) {
    Serial1.println ("Battery: Pausing timer." );
    measure_timer.pause();
    return 0;
}

static int
battery_deinit(struct device *dev)
{
    Serial1.println("De-init battery.");
    measure_timer.pause();
    detachInterrupt(TIMER_CH3);
    return 0;
}

struct device battery = {
    battery_init,
    battery_deinit,
    battery_suspend,
    battery_resume,

    "Battery Management",
};

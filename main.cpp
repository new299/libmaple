// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "captouch.h"
#include "power.h"
#include "tiles.h"
#include "OLED.h"
#include "log.h"

// for power control support
#include "pwr.h"
#include "scb.h"

#define LED_GPIO 25       // PD2

#define UART_CTS_GPIO     46 // PA12
#define UART_RTS_GPIO     47 // PA11
#define UART_TXD_GPIO     8 // PA10
#define UART_RXD_GPIO     7 // PA9

#define MEASURE_FET_GPIO  45 // PC12
#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5
#define BUZZER_PWM        24 // PB9

//#define CHARGE_GPIO 38

#define BUZZ_RATE  250  // in microseconds; set to 4kHz = 250us

// "WASD" cluster as defined by physical arrangement of touch switches
#define W_KEY (1 << 3)
#define A_KEY (1 << 6)
#define S_KEY (1 << 4)
#define D_KEY (1 << 2)
#define Q_KEY (1 << 8)
#define E_KEY (1 << 0)

#define FIRMWARE_VERSION "Safecast firmware v0.1 Jan 28 2012"

// frequency of checking battery voltage during logging state
#define LOG_BATT_FREQ 20 

uint8 allowBeep = 1;

HardwareTimer buzzTimer(4);
void blockingBeep(void);


static void
setup_gpio(void)
{
    // setup the inputs
    pinMode(UART_CTS_GPIO, INPUT);
    pinMode(UART_RTS_GPIO, INPUT);
    pinMode(UART_TXD_GPIO, INPUT);
    pinMode(UART_RXD_GPIO, INPUT);

    pinMode(GEIGER_PULSE_GPIO, INPUT);

    // initially, un-bias the buzzer
    pinMode(BUZZER_PWM, OUTPUT);
    digitalWrite(BUZZER_PWM, 0);

    pinMode(LED_GPIO, OUTPUT);  
    digitalWrite(LED_GPIO, 0);
}

void handler_buzz(void) {
    togglePin(BUZZER_PWM);
}

static void
setup_buzzer(void)
{
    pinMode(BUZZER_PWM, OUTPUT);
    // pause timer during setup
    buzzTimer.pause();
    //setup period
    buzzTimer.setPeriod(BUZZ_RATE);

    // setup interrupt on channel 4
    buzzTimer.setChannel4Mode(TIMER_OUTPUT_COMPARE);
    buzzTimer.setCompare(TIMER_CH4, 1); // interrupt one count after each update
    buzzTimer.attachCompare4Interrupt(handler_buzz);

    // refresh timer count, prescale, overflow
    buzzTimer.refresh();
    
    // start the timer counting
    //    buzzTimer.resume();
}

/* Single-call setup routine */
static void
setup()
{
    cap_init();
    power_init();
    log_init();

    setup_gpio();
    setup_buzzer();
}


static uint8 images[][128] = {
    #include "font.h"
    #include "alert.h"
};

static void fill_oled(int c) {
    // a test routine to fill the oled with a pattern
    int x, y, ptr;
//    uint16 data[8*8*2];

    // a little bit of oled
//    ptr = 0;
//    for (y=0; y<8; y++)
//        for (x=0; x<8; x++)
//            data[ptr++] = RGB16(x+c, (x+c)*(y+c), (y+c) * (((y/32)+1)*16));

    ptr = c;
    for (y=0; y<16; y++)
        for (x=0; x<16; x++)
            tile_set(x, y, images[256+0]);

    tile_set(1, 2, images[256+7]);
    tile_set(2, 2, images[256+6]);
    tile_set(3, 2, images[256+6]);
    tile_set(4, 2, images[256+6]);
    tile_set(5, 2, images[256+6]);
    tile_set(6, 2, images[256+6]);
    tile_set(7, 2, images[256+6]);
    tile_set(8, 2, images[256+6]);
    tile_set(9, 2, images[256+6]);
    tile_set(10, 2, images[256+6]);
    tile_set(11, 2, images[256+6]);
    tile_set(12, 2, images[256+6]);
    tile_set(13, 2, images[256+6]);
    tile_set(14, 2, images[256+8]);

    tile_set(1, 3, images[256+2]);
    tile_set(2, 3, images[122]);
    tile_set(3, 3, images['h'-'`'+64]);
    tile_set(4, 3, images['e'-'`']);
    tile_set(5, 3, images['l'-'`']);
    tile_set(6, 3, images['l'-'`']);
    tile_set(7, 3, images['o'-'`']);
    tile_set(8, 3, images[32]);
    tile_set(9, 3, images['t'-'`']);
    tile_set(10, 3, images['h'-'`']);
    tile_set(11, 3, images['e'-'`']);
    tile_set(12, 3, images['r'-'`']);
    tile_set(13, 3, images['e'-'`']);
    tile_set(14, 3, images[256+5]);

    tile_set(1, 4, images[256+3]);
    tile_set(2, 4, images[256+1]);
    tile_set(3, 4, images[256+1]);
    tile_set(4, 4, images[256+1]);
    tile_set(5, 4, images[256+1]);
    tile_set(6, 4, images[256+1]);
    tile_set(7, 4, images[256+1]);
    tile_set(8, 4, images[256+1]);
    tile_set(9, 4, images[256+1]);
    tile_set(10, 4, images[256+1]);
    tile_set(11, 4, images[256+1]);
    tile_set(12, 4, images[256+1]);
    tile_set(13, 4, images[256+1]);
    tile_set(14, 4, images[256+4]);
}


static void drawTiles(int t) {
    tile_draw(0, 9, images[(t+0)&0xff]);
    tile_draw(1, 9, images[(t+1)&0xff]);
    tile_draw(2, 9, images[(t+2)&0xff]);
    tile_draw(3, 9, images[(t+3)&0xff]);
    tile_draw(4, 9, images[(t+4)&0xff]);
    tile_draw(5, 9, images[(t+5)&0xff]);
    tile_draw(6, 9, images[(t+6)&0xff]);
    tile_draw(7, 9, images[(t+7)&0xff]);
    tile_draw(8, 9, images[(t+8)&0xff]);
    tile_draw(9, 9, images[(t+9)&0xff]);
    tile_draw(10, 9, images[(t+10)&0xff]);
    tile_draw(11, 9, images[(t+11)&0xff]);
    tile_draw(12, 9, images[(t+12)&0xff]);
    tile_draw(13, 9, images[(t+13)&0xff]);
    tile_draw(14, 9, images[(t+14)&0xff]);
    tile_draw(15, 9, images[(t+15)&0xff]);
}


/* Main loop */
static void
loop(unsigned int t)
{
    uint8 c;
    static int dbg_touch = 0;
    uint16 temp;

    if (dbg_touch)
        cap_debug();

    drawTiles(t);
    
    c = '\0';
    if( Serial1.available() ) {
        c = Serial1.read();
    }
    /*
    else if( touchStat ) {
        // pick just one of the touch states and turn it into a key press
        if( touchStat & W_KEY )
            c = 'W';
        if( touchStat & A_KEY )
            c = 'A';
        if( touchStat & S_KEY )
            c = 'S';
        if( touchStat & D_KEY )
            c = 'D';
        if( touchStat & Q_KEY )
            c = 'Q';
        if( touchStat & E_KEY )
            c = 'E';
        
        touchStat = 0;
    }
    */
    else {
        return;
    }
    // echo the character received
    Serial1.print( "safecast> " );
    Serial1.write(c);
    Serial1.println( "\r" );

    switch(c) {
    case '\0':
        break;
    case '1':
        dbg_touch = 1;
        break;
    case '!':
        dbg_touch = 0;
        break;
        /*
    case '2':
        Serial1.println("Resetting MPR121");
        mpr121Write(ELE_CFG, 0x00);
        delay(100);
        mpr121Write(ELE_CFG, 0x0C);   // Enables all 12 Electrodes
        delay(100);
        break;
    case '3':
        rel_thresh++;
        tou_thresh++;
        mpr121Write(ELE_CFG, 0x00);   // disable
        for( i = 0; i < 12; i++ ) {
            mpr121Write(ELE0_T + i * 2, tou_thresh);
            mpr121Write(ELE0_R + i * 2, rel_thresh);
        }
        mpr121Write(ELE_CFG, 0x0C);   // Enables
        break;
    case '4':
        rel_thresh--;
        tou_thresh--;
        mpr121Write(ELE_CFG, 0x00);   // disable
        for( i = 0; i < 12; i++ ) {
            mpr121Write(ELE0_T + i * 2, tou_thresh);
            mpr121Write(ELE0_R + i * 2, rel_thresh);
        }
        mpr121Write(ELE_CFG, 0x0C);   // Enables
        break;
        */
    case '5':
        power_set_debug(1);
        Serial1.println( "Turning on battery voltage debugging\n" );
        break;
    case '\%':
        Serial1.println( "Turning off battery voltage debugging\n" );
        power_set_debug(0);
        break;
    case 'v':
        temp = power_battery_level();
        Serial1.print("Battery voltage code: ");
        Serial1.println(temp);
        break;
    case '|':
        // use for validation only because it mucks with last power state tracking info
        Serial1.println("Forcing powerdown (use for validation only)\n" );
        power_set_state(PWRSTATE_DOWN);
        break;
    default:
        Serial1.println("?");
    }
    
}

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void
premain()
{

    init();
    delay(100);
}


void blockingBeep() {
    if( allowBeep ) {
        buzzTimer.resume();
        delay(50);
        buzzTimer.pause();
    }
}


int
main(void)
{
    int t = 0;

    Serial1.begin(115200);
    Serial1.println(FIRMWARE_VERSION);
            
    Serial1.println ( "Entering BOOT powerstate." );

    power_set_debug(0);
    setup();
    power_set_debug(1);
    blockingBeep();

    /* Determine whether the power switch is "on" or "off" */
    if (power_switch_state())
        power_set_state(PWRSTATE_USER);
    else
        power_set_state(PWRSTATE_LOG);


    /* All activity should take place in interrupts. */

    while (true) {
        if (power_get_state() == PWRSTATE_USER)
            loop(t);
        power_wfi();
    }

    #if 0
        switch(powerState) {
        case PWRSTATE_DOWN:  /////////// PWRSTATE_DOWN TEST STATUS: THIS CODE FUNCTIONS BUT NEEDS VALIDATION WITH AMMETER TO CONFIRM LOW POWER OPERATION.
            Serial1.println ( "Entering DOWN powerstate." );
            while(1) {
                powerDown();

                // system resets when power is plugged in no matter what, so this is sort of irrelevant
                power_set_state(PWRSTATE_DOWN);
            }
            break;
        case PWRSTATE_LOG:   ////////// PWRSTATE_LOG TEST STATUS: THIS CODE IS UNTESTED
            if( power_is_battery_low() ) {
                power_set_state(PWRSTATE_DOWN);
                break;
            }
            
            if( lastPowerState != PWRSTATE_LOG ) {
                Serial1.println ( "Entering LOG powerstate." );
                // we are just entering, so do things like turn off beeping, LED flashing, etc.
                prepSleep();

                // once it's all setup, re-enter the loop so we go into the next branch
                lastPowerState = PWRSTATE_LOG;
                powerState = PWRSTATE_LOG;
                break;
            }
            else {
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
                    setup();

                    powerState = PWRSTATE_USER;
                    lastPowerState = PWRSTATE_LOG;
                    break;
                } else {
                    // this is a geiger event. for now, just make a beep and go back to sleep
                    // eventually, we'll want to log the vent with a timestamp to flash
                    short_init(); // special-case init for minimal operational parameters

                    setup_buzzer();
                    blockingBeep();

                    // TODO: put logging infos here...

                    powerState = PWRSTATE_LOG;
                    lastPowerState = PWRSTATE_LOG;
                    break;
                }
            }
            break;
        case PWRSTATE_USER:   ////////// PWRSTATE_LOG TEST STATUS: THIS CODE IS ROUTINELY USED FOR DEVELOPMENT AND IS LIGHTLY TESTED
            // check for events from the touchscreen
            if( lastPowerState != PWRSTATE_USER ) {
                Serial1.println ( "Entering USER powerstate." );
                // setup anything specific to this state, i.e. turn on LED flashing and beeping on
                // radiation events
                oled_init();
                fill_oled(0); // eventually this can go away i think.
                /* Set up PB11 to be an IRQ that triggers cap_down */
                allowBeep = 1;
            }

            // call the event loop
            loop(t++);

            if( power_is_battery_low() ) {
                powerState = PWRSTATE_DOWN;
                lastPowerState = PWRSTATE_USER;
                break;
            } else if( digitalRead(MANUAL_WAKEUP_GPIO) == HIGH ) {
                powerState = PWRSTATE_USER;
                lastPowerState = PWRSTATE_USER;
            } else {
                powerState = PWRSTATE_LOG;
                lastPowerState = PWRSTATE_USER;
            }
            break;
        case PWRSTATE_BOOT:   ////////// PWRSTATE_BOOT TEST STATUS: THIS CODE HAS BEEN LIGHTLY TESTED
            Serial1.begin(115200);
            Serial1.println(FIRMWARE_VERSION);
            
            Serial1.println ( "Entering BOOT powerstate." );

            power_set_debug(0);
            setup();
            power_set_debug(1);
            blockingBeep();

            // set up Flash, etc. and interrupt handlers for logging. At this point
            // we can start receiving radiation events
            setupLogging();

            if( digitalRead(MANUAL_WAKEUP_GPIO) == HIGH ) {
                powerState = PWRSTATE_USER;
            } else {
                powerState = PWRSTATE_LOG;
            }
            lastPowerState = PWRSTATE_BOOT;
            break;
        default:
            Serial1.println("Entering ERROR powerstate." );
            powerState = PWRSTATE_BOOT;
            lastPowerState = PWRSTATE_ERROR;
        }
    }
#endif

    return 0;
}

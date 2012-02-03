// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "captouch.h"
#include "power.h"
#include "tiles.h"
#include "OLED.h"
#include "log.h"
#include "switch.h"
#include "buzzer.h"
#include "battery.h"
#include "accel.h"

// for power control support
#include "pwr.h"
#include "scb.h"

#define LED_GPIO 25       // PD2

#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5

#define FIRMWARE_VERSION "Safecast firmware v0.1 Jan 28 2012"

#define KEY_Q 0x01
#define KEY_W 0x02
#define KEY_E 0x04
#define KEY_A 0x08
#define KEY_S 0x10
#define KEY_D 0x20
static uint8 touch_pad;
static uint8 old_touch_pad;

const static uint8 images[][128] = {
    #include "font.h"
    #include "alert.h"
};


static void
setup_gpio(void)
{
    pinMode(GEIGER_PULSE_GPIO, INPUT);

    pinMode(LED_GPIO, OUTPUT);  
    digitalWrite(LED_GPIO, 0);
}

static void
on_keydown(char key)
{
    if (key == 'Q')
        touch_pad |= KEY_Q;

    if (key == 'W')
        touch_pad |= KEY_W;

    if (key == 'E')
        touch_pad |= KEY_E;

    if (key == 'A')
        touch_pad |= KEY_A;

    if (key == 'S')
        touch_pad |= KEY_S;

    if (key == 'D')
        touch_pad |= KEY_D;
}

static void
on_keyup(char key)
{
    if (key == 'Q')
        touch_pad &= ~KEY_Q;

    if (key == 'W')
        touch_pad &= ~KEY_W;

    if (key == 'E')
        touch_pad &= ~KEY_E;

    if (key == 'A')
        touch_pad &= ~KEY_A;

    if (key == 'S')
        touch_pad &= ~KEY_S;

    if (key == 'D')
        touch_pad &= ~KEY_D;
}

/* Single-call setup routine */
static void
setup(void)
{
    Serial1.println("Setting up GPIO...");
    setup_gpio();

    // add devices. Devices are called in reverse order during susped, and forward order in resume
    Serial1.println("Adding power...");
    device_add(&power);

    Serial1.println("Adding battery...");
    device_add(&battery);

    Serial1.println("Adding buzzer...");
    device_add(&buzzer);

    Serial1.println("Adding logger...");
    device_add(&logger);

    Serial1.println("Adding OLED...");
    device_add(&oled);

    Serial1.println("Adding back switch...");
    device_add(&back_switch);

    Serial1.println("Adding captouch...");
    device_add(&captouch);
    cap_setkeyup(on_keyup);
    cap_setkeydown(on_keydown);

    Serial1.println("Adding accelerometer...");
    device_add(&accel);

    Serial1.println("Done adding devices.");
}


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

    tile_set(1, 4, images[256+2]);
    tile_set(2, 4, images[32]);
    tile_set(3, 4, images[32]);
    tile_set(4, 4, images[32]);
    tile_set(5, 4, images[32]);
    tile_set(6, 4, images[32]);
    tile_set(7, 4, images[32]);
    tile_set(8, 4, images[32]);
    tile_set(9, 4, images[32]);
    tile_set(10, 4, images[32]);
    tile_set(11, 4, images[32]);
    tile_set(12, 4, images[32]);
    tile_set(13, 4, images[32]);
    tile_set(14, 4, images[256+5]);

    tile_set(1, 5, images[256+2]);
    tile_set(2, 5, images[32]);
    tile_set(3, 5, images[32]);
    tile_set(4, 5, images[32]);
    tile_set(5, 5, images[32]);
    tile_set(6, 5, images[32]);
    tile_set(7, 5, images[32]);
    tile_set(8, 5, images[32]);
    tile_set(9, 5, images[32]);
    tile_set(10, 5, images[32]);
    tile_set(11, 5, images[32]);
    tile_set(12, 5, images[32]);
    tile_set(13, 5, images[32]);
    tile_set(14, 5, images[256+5]);

    tile_set(1, 6, images[256+2]);
    tile_set(2, 6, images[32]);
    tile_set(3, 6, images[32]);
    tile_set(4, 6, images[32]);
    tile_set(5, 6, images[32]);
    tile_set(6, 6, images[32]);
    tile_set(7, 6, images[32]);
    tile_set(8, 6, images[32]);
    tile_set(9, 6, images[32]);
    tile_set(10, 6, images[32]);
    tile_set(11, 6, images[32]);
    tile_set(12, 6, images[32]);
    tile_set(13, 6, images[32]);
    tile_set(14, 6, images[256+5]);

    tile_set(1, 7, images[256+2]);
    tile_set(2, 7, images['x'-'`'+64]);
    tile_set(3, 7, images[58]);
    tile_set(4, 7, images[32]);
    tile_set(5, 7, images[32]);
    tile_set(6, 7, images[32]);
    tile_set(7, 7, images[32]);
    tile_set(8, 7, images[32]);
    tile_set(9, 7, images[32]);
    tile_set(10, 7, images[32]);
    tile_set(11, 7, images[32]);
    tile_set(12, 7, images[32]);
    tile_set(13, 7, images[32]);
    tile_set(14, 7, images[256+5]);

    tile_set(1, 8, images[256+2]);
    tile_set(2, 8, images['y'-'`'+64]);
    tile_set(3, 8, images[58]);
    tile_set(4, 8, images[32]);
    tile_set(5, 8, images[32]);
    tile_set(6, 8, images[32]);
    tile_set(7, 8, images[32]);
    tile_set(8, 8, images[32]);
    tile_set(9, 8, images[32]);
    tile_set(10, 8, images[32]);
    tile_set(11, 8, images[32]);
    tile_set(12, 8, images[32]);
    tile_set(13, 8, images[32]);
    tile_set(14, 8, images[256+5]);

    tile_set(1, 9, images[256+2]);
    tile_set(2, 9, images['z'-'`'+64]);
    tile_set(3, 9, images[58]);
    tile_set(4, 9, images[32]);
    tile_set(5, 9, images[32]);
    tile_set(6, 9, images[32]);
    tile_set(7, 9, images[32]);
    tile_set(8, 9, images[32]);
    tile_set(9, 9, images[32]);
    tile_set(10, 9, images[32]);
    tile_set(11, 9, images[32]);
    tile_set(12, 9, images[32]);
    tile_set(13, 9, images[32]);
    tile_set(14, 9, images[256+5]);

    tile_set(1, 6, images[256+2]);
    tile_set(2, 6, images[32]);
    tile_set(3, 6, images[32]);
    tile_set(4, 6, images[32]);
    tile_set(5, 6, images[32]);
    tile_set(6, 6, images[32]);
    tile_set(7, 6, images[32]);
    tile_set(8, 6, images[32]);
    tile_set(9, 6, images[32]);
    tile_set(10, 6, images[32]);
    tile_set(11, 6, images[32]);
    tile_set(12, 6, images[32]);
    tile_set(13, 6, images[32]);
    tile_set(14, 6, images[256+5]);


    tile_set(1, 10, images[256+3]);
    tile_set(2, 10, images[256+1]);
    tile_set(3, 10, images[256+1]);
    tile_set(4, 10, images[256+1]);
    tile_set(5, 10, images[256+1]);
    tile_set(6, 10, images[256+1]);
    tile_set(7, 10, images[256+1]);
    tile_set(8, 10, images[256+1]);
    tile_set(9, 10, images[256+1]);
    tile_set(10, 10, images[256+1]);
    tile_set(11, 10, images[256+1]);
    tile_set(12, 10, images[256+1]);
    tile_set(13, 10, images[256+1]);
    tile_set(14, 10, images[256+4]);
}


static void
draw_number(int x, int y, int n)
{
    {
        unsigned char buf[8 * sizeof(long long)];
        unsigned long i = 0;

        if (n < 0) {
            n = -n;
            tile_set(x++, y, images[45]);
        }

        if (n == 0) {
            tile_set(x++, y, images['0']);
        }

        else {
            while (n > 0) {
                buf[i++] = n % 10;
                n /= 10;
            }
            for (; x<=13 && i > 0; i--,x++)
                tile_set(x, y, images['0' + buf[i - 1]]);
        }

        /* Fill the rest of the line with space */
        for (; x<=13; x++)
            tile_set(x, y, images[32]);
    }
}


static void
drawTiles(int t) {
    draw_number(4, 4, t);

    tile_draw(0, 11, images[(t+0)&0xff]);
    tile_draw(1, 11, images[(t+1)&0xff]);
    tile_draw(2, 11, images[(t+2)&0xff]);
    tile_draw(3, 11, images[(t+3)&0xff]);
    tile_draw(4, 11, images[(t+4)&0xff]);
    tile_draw(5, 11, images[(t+5)&0xff]);
    tile_draw(6, 11, images[(t+6)&0xff]);
    tile_draw(7, 11, images[(t+7)&0xff]);
    tile_draw(8, 11, images[(t+8)&0xff]);
    tile_draw(9, 11, images[(t+9)&0xff]);
    tile_draw(10, 11, images[(t+10)&0xff]);
    tile_draw(11, 11, images[(t+11)&0xff]);
    tile_draw(12, 11, images[(t+12)&0xff]);
    tile_draw(13, 11, images[(t+13)&0xff]);
    tile_draw(14, 11, images[(t+14)&0xff]);
    tile_draw(15, 11, images[(t+15)&0xff]);
}

static void
update_keys(int keys)
{
    const static uint8 *q_tile = images['q'-'`'+64];
    const static uint8 *w_tile = images['w'-'`'+64];
    const static uint8 *e_tile = images['e'-'`'+64];
    const static uint8 *a_tile = images['a'-'`'+64];
    const static uint8 *s_tile = images['s'-'`'+64];
    const static uint8 *d_tile = images['d'-'`'+64];
    const static uint8 *blank  = images[32];
    if (keys & KEY_Q)
        tile_set(6, 5, q_tile);
    else
        tile_set(6, 5, blank);

    if (keys & KEY_W)
        tile_set(7, 5, w_tile);
    else
        tile_set(7, 5, blank);

    if (keys & KEY_E)
        tile_set(8, 5, e_tile);
    else
        tile_set(8, 5, blank);

    if (keys & KEY_A)
        tile_set(6, 6, a_tile);
    else
        tile_set(6, 6, blank);

    if (keys & KEY_S)
        tile_set(7, 6, s_tile);
    else
        tile_set(7, 6, blank);

    if (keys & KEY_D)
        tile_set(8, 6, d_tile);
    else
        tile_set(8, 6, blank);
}


/* Main loop */
static void
loop(unsigned int t)
{
    uint8 c;
    static int dbg_touch = 0;
    uint16 temp;
    signed int x, y, z;

    if (dbg_touch)
        cap_debug();
    if (cap_should_poll())
        cap_poll();

    drawTiles(t);
    
    if (accel_read_state(&x, &y, &z))
        Serial1.println("Unable to read accel value!");

    draw_number(4, 7, x);
    draw_number(4, 8, y);
    draw_number(4, 9, z);

    update_keys(touch_pad);

    c = '\0';
    if( Serial1.available() ) {
        c = Serial1.read();
    }
    else if(touch_pad != old_touch_pad) {
        // pick just one of the touch states and turn it into a key press
        if ( (touch_pad & KEY_W) && !(old_touch_pad & KEY_W) )
            c = 'W';
        if ( (touch_pad & KEY_A) && !(old_touch_pad & KEY_A) )
            c = 'A';
        if ( (touch_pad & KEY_S) && !(old_touch_pad & KEY_S) )
            c = 'S';
        if ( (touch_pad & KEY_D) && !(old_touch_pad & KEY_D) )
            c = 'D';
        if ( (touch_pad & KEY_Q) && !(old_touch_pad & KEY_Q) )
            c = 'Q';
        if ( (touch_pad & KEY_E) && !(old_touch_pad & KEY_E) )
            c = 'E';
        old_touch_pad = touch_pad;
    }
    else {
        return;
    }
    // echo the character received
    Serial1.print( "safecast> " );
    Serial1.println((char)c);

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
        temp = battery_level();
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
}


int
main(void)
{
    int t = 0;
    
    Serial1.begin(115200);
    Serial1.println(FIRMWARE_VERSION);
            
    Serial1.println("Entering BOOT powerstate.");


    power_set_debug(0);
    setup();
    power_set_debug(1);
    buzzer_buzz_blocking();

    battery_set_debug(1);

    /* Determine whether the power switch is "on" or "off" */
    if (switch_state(&back_switch))
        power_set_state(PWRSTATE_USER);
    else
        power_set_state(PWRSTATE_LOG);
    fill_oled(0); // eventually this can go away i think.


    /* All activity should take place in interrupts. */
    Serial1.println("Entering main loop...");
    while (true) {
        if (power_get_state() == PWRSTATE_USER)
            loop(t++);
        else
            Serial1.println(".");

        power_sleep(); // just stop clock to CPU core, don't shut down peripherals
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

// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "i2c.h"
#include "mpr121.h"
#include "OLED.h"
#include "tiles.h"

#define CAPTOUCH_ADDR 0x5A
#define CAPTOUCH_I2C I2C1
#define CAPTOUCH_GPIO 30

#define LED_GPIO 25       // PD2
#define MANUAL_WAKEUP_GPIO 18 // PC3
#define CHG_STAT2_GPIO    44 // PC11
#define CHG_STAT1_GPIO    26 // PC10
#define MAGPOWER_GPIO     41 // PA15
#define MEASURE_FET_GPIO  45 // PC12
#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5
#define BUZZER_PWM        24 // PB9
#define BATT_MEASURE_ADC  28 // PB1
#define MAGSENSE_GPIO     29 // PB10
#define LIMIT_VREF_DAC    10 // PA4 -- should be DAC eventually, but GPIO initially to tied own
#define CHG_TIMEREN_N_GPIO 37 // PC8
#define LED_PWR_ENA_GPIO  16 // PC1 // handled in OLED platform_init
#define WAKEUP_GPIO       2

#define UART_CTS_GPIO     46 // PA12
#define UART_RTS_GPIO     47 // PA11
#define UART_TXD_GPIO     8 // PA10
#define UART_RXD_GPIO     7 // PA9

//#define CHARGE_GPIO 38

#define BUZZ_RATE  250  // in microseconds; set to 4kHz = 250us

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

#define FIRMWARE_VERSION "Safecast firmware v0.1 Jan 28 2012"

static struct i2c_dev *i2c;
uint8 powerState = PWRSTATE_BOOT;
uint8 lastPowerState = PWRSTATE_OFF;
uint16 touchStat;
uint8 touchInit = 0;
uint8 touchService = 0;
uint16 touchList =  1 << 9 | 1 << 8 | 1 << 6 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 0;
uint8 allowBeep = 1;
//uint16 touchList =  0x3FF;

HardwareTimer buzzTimer(4);
void blockingBeep();

/*
static void
cap_read(void)
{
    return;
}
*/

static void
mpr121Write(uint8 addr, uint8 value)
{
    struct i2c_msg msg;
    uint8 bytes[2];
    int result;

    bytes[0] = addr;
    bytes[1] = value;

    msg.addr = CAPTOUCH_ADDR;
    msg.flags = 0;
    msg.length = sizeof(bytes);
    msg.xferred = 0;
    msg.data = bytes;

    result = i2c_master_xfer(i2c, &msg, 1, 100);
    if (!result) {
        Serial1.print(addr, 16); Serial1.print(" -> "); Serial1.print(value); Serial1.print("\r\n");
    }
    else {
        Serial1.print(addr, 16); Serial1.print(" err "); Serial1.print(result); Serial1.print("\r\n");
    }

    return;
}

static uint8
mpr121Read(uint8 addr)
{
    struct i2c_msg msgs[2];
    uint8 byte;

    byte = addr;
    msgs[0].addr   = msgs[1].addr   = CAPTOUCH_ADDR;
    msgs[0].length = msgs[1].length = sizeof(byte);
    msgs[0].data   = msgs[1].data   = &byte;
    msgs[0].flags = 0;
    msgs[1].flags = I2C_MSG_READ;
    i2c_master_xfer(i2c, msgs, 2, 100);
    return byte;
}

static void
cap_down(void)
{
    if( digitalRead(MANUAL_WAKEUP_GPIO) == LOW ) {
        touchInit = 0;
        return; // don't initiate service if the unit is powered down
    }

    if(touchInit) {
        touchService = 1; // flag that we're ready to be serviced

        toggleLED();
    }

    return;
}

static void
setup_captouch()
{
    i2c = CAPTOUCH_I2C;
    i2c_init(i2c);
    i2c_master_enable(i2c, 0);
    Serial1.print(".");

    mpr121Write(ELE_CFG, 0x00);   // disable electrodes for config
    delay(100);

    // Section A
    // This group controls filtering when data is > baseline.
    mpr121Write(MHD_R, 0x01);
    mpr121Write(NHD_R, 0x01);
    //    mpr121Write(NCL_R, 0x50);
    //    mpr121Write(FDL_R, 0x50);
    mpr121Write(NCL_R, 0x00);
    mpr121Write(FDL_R, 0x00);

    // Section B
    // This group controls filtering when data is < baseline.
    mpr121Write(MHD_F, 0x01);
    mpr121Write(NHD_F, 0x01);
    mpr121Write(NCL_F, 0xFF);
    //    mpr121Write(FDL_F, 0x52);
    mpr121Write(FDL_F, 0x02);

    // Section C
    // This group sets touch and release thresholds for each electrode
    mpr121Write(ELE0_T, TOU_THRESH);
    mpr121Write(ELE0_R, REL_THRESH);
    mpr121Write(ELE1_T, TOU_THRESH);
    mpr121Write(ELE1_R, REL_THRESH);
    mpr121Write(ELE2_T, TOU_THRESH);
    mpr121Write(ELE2_R, REL_THRESH);
    mpr121Write(ELE3_T, TOU_THRESH);
    mpr121Write(ELE3_R, REL_THRESH);
    mpr121Write(ELE4_T, TOU_THRESH);
    mpr121Write(ELE4_R, REL_THRESH);
    mpr121Write(ELE5_T, TOU_THRESH);
    mpr121Write(ELE5_R, REL_THRESH);
    mpr121Write(ELE6_T, TOU_THRESH);
    mpr121Write(ELE6_R, REL_THRESH);
    mpr121Write(ELE7_T, TOU_THRESH);
    mpr121Write(ELE7_R, REL_THRESH);
    mpr121Write(ELE8_T, TOU_THRESH);
    mpr121Write(ELE8_R, REL_THRESH);
    mpr121Write(ELE9_T, TOU_THRESH);
    mpr121Write(ELE9_R, REL_THRESH);
    mpr121Write(ELE10_T, TOU_THRESH);
    mpr121Write(ELE10_R, REL_THRESH);
    mpr121Write(ELE11_T, TOU_THRESH);
    mpr121Write(ELE11_R, REL_THRESH);

    // Section D
    // Set the Filter Configuration
    // Set ESI2
    mpr121Write(FIL_CFG, 0x03);  // set CDT to 32us, ESI (sampling interval) to 8 ms
    mpr121Write(AFE_CONF, 0x3F); // 6 samples, 63uA <-- will be overridden by auto-config i think

    // Section F
    mpr121Write(ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V
    mpr121Write(ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
    mpr121Write(ATO_CFGT, 0xB5);    // Target = 0.9*USL = 0xB5 @3.3V

    // mpr121Write(ATO_CFGU, 0xC0);  // VSL = (Vdd-0.7)/vdd*256 = 0xC0 @2.8V
    // mpr121Write(ATO_CFGL, 0x7D);  // LSL = 0.65*USL = 0x7D @2.8V
    // mpr121Write(ATO_CFGT, 0xB2);    // Target = 0.9*USL = 0xB2 @2.8V

    //    mpr121Write(ATO_CFGU, 0x9C);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @1.8V
    //    mpr121Write(ATO_CFGL, 0x65);  // LSL = 0.65*USL = 0x82 @1.8V
    //    mpr121Write(ATO_CFGT, 0x8C);    // Target = 0.9*USL = 0xB5 @1.8V

    // Enable Auto Config and auto Reconfig
    mpr121Write(ATO_CFG0, 0x3B); // must match AFE_CONF setting of 6 samples, retry enabled

    delay(100);

    // Section E
    // Electrode Configuration
    // Enable 6 Electrodes and set to run mode
    // Set ELE_CFG to 0x00 to return to standby mode
    mpr121Write(ELE_CFG, 0x0C);   // Enables all 12 Electrodes
    //mpr121Write(ELE_CFG, 0x06);     // Enable first 6 electrodes
    delay(100);

    touchInit = 1;
    return;
}

static void
setup_lcd(void)
{
    OLED_init();
    return;
}

static void
setup_gpio(void)
{
    // setup the inputs
    pinMode(UART_CTS_GPIO, INPUT);
    pinMode(UART_RTS_GPIO, INPUT);
    pinMode(UART_TXD_GPIO, INPUT);
    pinMode(UART_RXD_GPIO, INPUT);

    pinMode(MANUAL_WAKEUP_GPIO, INPUT);
    pinMode(CHG_STAT2_GPIO, INPUT);
    pinMode(CHG_STAT1_GPIO, INPUT);
    pinMode(GEIGER_PULSE_GPIO, INPUT);
    pinMode(BATT_MEASURE_ADC, INPUT_ANALOG);
    pinMode(MAGSENSE_GPIO, INPUT);
    pinMode(WAKEUP_GPIO, INPUT);
    pinMode(CAPTOUCH_GPIO, INPUT);  

    // setup and initialize the outputs
    // initially, don't measure battery voltage
    pinMode(MEASURE_FET_GPIO, OUTPUT);
    digitalWrite(MEASURE_FET_GPIO, 0);

    // initially, turn off the hall effect sensor
    pinMode(MAGPOWER_GPIO, OUTPUT);
    digitalWrite(MAGPOWER_GPIO, 0);

    // initially, un-bias the buzzer
    pinMode(BUZZER_PWM, OUTPUT);
    digitalWrite(BUZZER_PWM, 0);
    
    // as a hack, tie this low to reduce current consumption
    // until we hook it up to a proper DAC output
    pinMode(LIMIT_VREF_DAC, OUTPUT);
    digitalWrite(LIMIT_VREF_DAC, 0);
    
    // initially, charge timer is enabled (active low)
    pinMode(CHG_TIMEREN_N_GPIO, OUTPUT);
    digitalWrite(CHG_TIMEREN_N_GPIO, 0);

    // initially OLED is off
    pinMode(LED_PWR_ENA_GPIO, OUTPUT);
    digitalWrite(LED_PWR_ENA_GPIO, 0);

    pinMode(LED_GPIO, OUTPUT);  // hard coded guess for now
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
    Serial1.print("In setup()...");

    setup_buzzer();

    Serial1.println(" Done.\n");
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
    tile_set(2, 3, images['h'-'H']);
    tile_set(3, 3, images['e'-'H']);
    tile_set(4, 3, images['l'-'H']);
    tile_set(5, 3, images['l'-'H']);
    tile_set(6, 3, images['o'-'H']);
    tile_set(7, 3, images[' '-'H']);
    tile_set(8, 3, images['t'-'H']);
    tile_set(9, 3, images['h'-'H']);
    tile_set(10, 3, images['e'-'H']);
    tile_set(11, 3, images['r'-'H']);
    tile_set(12, 3, images['e'-'H']);
    tile_set(13, 3, images[' '-'H']);
    tile_set(14, 3, images[256+5]);
}

static void debug_touch(void) {
    uint8 bytes[2];
    uint16 temp;
    int i;

    bytes[0] = mpr121Read(TCH_STATL);
    bytes[1] = mpr121Read(TCH_STATH);

    Serial1.print("Values: [");
    Serial1.print(bytes[0]&(1<<0)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<2)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<3)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<4)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<6)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[1]&(1<<0)?"1":"0");

    Serial1.print(" ");
    Serial1.print(bytes[1]&(1<<1)?"1":"0");  // 9, a dummy electrode

    //    Serial1.print("]\r");
    Serial1.println("]\r");
    
    Serial1.print("OOR: ");
    temp = mpr121Read(TCH_OORL);
    temp |= mpr121Read(TCH_OORH) << 8;
    Serial1.print( temp, 16 );
    Serial1.println( "\r" );

    Serial1.print("FIL_CFG: ");
    temp = mpr121Read(FIL_CFG);
    Serial1.print( temp, 16 );
    Serial1.println( "\r" );

    Serial1.print("AFE_CONF: ");
    temp = mpr121Read(AFE_CONF);
    Serial1.print( temp, 16 );
    Serial1.println( "\r" );

    for( i = 0; i < 13; i++ ) {
        temp = 0;
        if( touchList & (1 << i) ) {
            temp = mpr121Read(ELE0_T + i * 2);
            Serial1.print( "   TT" );
            Serial1.print( i );
            Serial1.print( " " );
            Serial1.print( temp, 16 );

            temp = mpr121Read(ELE0_R + i * 2);
            Serial1.print( "   RT" );
            Serial1.print( i );
            Serial1.print( " " );
            Serial1.print( temp, 16 );
            
            temp = mpr121Read(0x5F + i);
            Serial1.print( "   CUR" );
            Serial1.print( i );
            Serial1.print( " " );
            Serial1.print( temp, 16 );
            
            temp = mpr121Read(0x1e + i);
            Serial1.print( "   ELEBASE" );
            Serial1.print( i );
            Serial1.print( " " );
            Serial1.print( temp << 2, 16 );

            temp |= mpr121Read(0x4 + i);
            temp |= (mpr121Read(0x5 + i) & 0x3) << 8;
            Serial1.print( "   ELEFILT" );
            Serial1.print( i );
            Serial1.print( " " );
            Serial1.print( temp, 16 );

            Serial1.println( "\r" );
            
            temp = 0;
        }
    }
    Serial1.print( "CHG TIME: ");
    for( i = 0; i < 5; i++ ) {
        temp = mpr121Read(0x6c + i);
        Serial1.print( temp, 16 );
        Serial1.print( " " );
    }
    Serial1.println( "\r" );
    Serial1.println( "\r" );

    delay(500);
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
    static int rel_thresh = 0xA;
    static int tou_thresh = 0xF;
    int i;

    if( dbg_touch ) {
        debug_touch();
    }

    drawTiles(t);
    
    c = '\0';
    if( Serial1.available() ) {
        c = Serial1.read();
    } else if( touchStat ) {
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
    } else {
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
    setup_gpio();
    delay(100);
    touchInit = 0;
}


void blockingBeep() {
    if( allowBeep ) {
        buzzTimer.resume();
        delay(50);
        buzzTimer.pause();
    }
}

// isBattLow should measure ADC and determine if the battery voltage is
// too low to continue operation. When that happens, we should immediately
// power down to prevent over-discharge of the battery.
int isBattLow() {
    // for now, we don't check
    return 0;
}

// this routine should set everything up for geiger pulse logging
void setupLogging() {
    return;
}

// this routine enters a sleep mode, with a wakeup set from the WAKEUP event from the geiger counter or switch
void logSleep() {
    return;
}

int
main(void)
{
    int t = 0;

    while (true) {
        switch(powerState) {
        case PWRSTATE_DOWN:
            Serial1.println ( "Entering DOWN powerstate." );
            // disable all interrupts and just turn the system off

            break;
        case PWRSTATE_LOG:
            if( isBattLow() ) {
                lastPowerState = PWRSTATE_LOG;
                powerState = PWRSTATE_DOWN;
                break;
            }
            
            if( lastPowerState != PWRSTATE_LOG ) {
                Serial1.println ( "Entering LOG powerstate." );
                // we are just entering, so do things like turn off beeping, LED flashing, etc.
                OLED_ShutDown();
                detachInterrupt(CAPTOUCH_GPIO);
                digitalWrite(LED_GPIO, 0);
                allowBeep = 0;

                // the interrupt handler will handle logging. that's already setup.

                // once it's all setup, re-enter the loop so we go into the next branch
                lastPowerState = PWRSTATE_LOG;
                powerState = PWRSTATE_LOG;
                break;
            } else {
                // first, we sleep and wait for an interrupt
                logSleep();

                // we'll wake up due to a switch or geiger event, so determine which and
                // then re-enter the loop
                if( digitalRead(MANUAL_WAKEUP_GPIO) == HIGH ) {
                    powerState = PWRSTATE_USER;
                    lastPowerState = PWRSTATE_LOG;
                    break;
                } else {
                    powerState = PWRSTATE_LOG;
                    lastPowerState = PWRSTATE_LOG;
                    break;
                }
            }
            break;
        case PWRSTATE_USER:
            // check for events from the touchscreen
            if( lastPowerState != PWRSTATE_USER ) {
                Serial1.println ( "Entering USER powerstate." );
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
        case PWRSTATE_BOOT:
            Serial1.begin(115200);
            Serial1.println(FIRMWARE_VERSION);
            
            Serial1.println ( "Entering BOOT powerstate." );

            setup();
            allowBeep = 1;
            blockingBeep();

            // set up Flash, etc. and interrupt handlers for logging. At this point
            // we can start receiving radiation events
            setupLogging();

            if( digitalRead(MANUAL_WAKEUP_GPIO) == HIGH ) {
                powerState = PWRSTATE_USER;
            } else {
                powerState = PWRSTATE_LOG;
                touchInit = 0;
            }
            lastPowerState = PWRSTATE_BOOT;
            break;
        default:
            Serial1.println("Entering ERROR powerstate." );
            powerState = PWRSTATE_BOOT;
            lastPowerState = PWRSTATE_ERROR;
        }
    }

    return 0;
}

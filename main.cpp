// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "i2c.h"
#include "mpr121.h"
#include "OLED.h"

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
//#define CHARGE_GPIO 38

#define BUZZ_RATE  250  // in microseconds; set to 4kHz = 250us

// "WASD" cluster as defined by physical arrangement of touch switches
#define W_KEY (1 << 3)
#define A_KEY (1 << 6)
#define S_KEY (1 << 4)
#define D_KEY (1 << 2)
#define Q_KEY (1 << 8)
#define E_KEY (1 << 0)

static struct i2c_dev *i2c;
uint16 touchStat;
uint8 touchInit = 0;
uint8 touchService = 0;
uint16 touchList =  1 << 9 | 1 << 8 | 1 << 6 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 0;
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

    // initially LED is off
    pinMode(LED_PWR_ENA_GPIO, OUTPUT);
    digitalWrite(LED_PWR_ENA_GPIO, 0);

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
    //delay(1500);
    Serial1.print("In setup()...");


    /* Set up the LED to blink  */
    pinMode(LED_GPIO, OUTPUT);  // hard coded guess for now

    setup_gpio();

    setup_lcd();

    setup_buzzer();

    /* Set up PB11 to be an IRQ that triggers cap_down */
    attachInterrupt(CAPTOUCH_GPIO, cap_down, CHANGE);

    Serial1.println(" Done.\n");
}


static uint8 images[][128] = {
    #include "font.h"
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
    for (y=0; y<128; y+=8)
        for (x=0; x<128; x+=8)
            OLED_draw_rect(x, y, 8, 8, images[(ptr++)&0xff]);
}

static void debug_touch(void) {
    uint8 bytes[2];
    static uint8 prevTouch[2];
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
    touchInit = 0;
}


void blockingBeep() {
    buzzTimer.resume();
    delay(50);
    buzzTimer.pause();
}

int
main(void)
{
    int t = 0;

    /* Send a message out USART2  */
    Serial1.begin(115200);
    //delay(1500);
    Serial1.println("Initialized.");

    setup();

    fill_oled(0);

    // left off: figure out power management...
    while (true) {
        if( digitalRead(MANUAL_WAKEUP_GPIO) == HIGH ) {
            //        blockingBeep();
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
            OLED_draw_rect(0, 48, 8, 8, images[(t+0)&0xff]);
            OLED_draw_rect(8, 48, 8, 8, images[(t+1)&0xff]);
            OLED_draw_rect(16, 48, 8, 8, images[(t+2)&0xff]);
            OLED_draw_rect(24, 48, 8, 8, images[(t+3)&0xff]);
            OLED_draw_rect(32, 48, 8, 8, images[(t+4)&0xff]);
            OLED_draw_rect(40, 48, 8, 8, images[(t+5)&0xff]);
            OLED_draw_rect(48, 48, 8, 8, images[(t+6)&0xff]);
            OLED_draw_rect(56, 48, 8, 8, images[(t+7)&0xff]);
            OLED_draw_rect(64, 48, 8, 8, images[(t+8)&0xff]);
            OLED_draw_rect(72, 48, 8, 8, images[(t+9)&0xff]);
            OLED_draw_rect(80, 48, 8, 8, images[(t+10)&0xff]);
            OLED_draw_rect(88, 48, 8, 8, images[(t+11)&0xff]);
            OLED_draw_rect(96, 48, 8, 8, images[(t+12)&0xff]);
            OLED_draw_rect(104, 48, 8, 8, images[(t+13)&0xff]);
            OLED_draw_rect(112, 48, 8, 8, images[(t+14)&0xff]);
            OLED_draw_rect(120, 48, 8, 8, images[(t+15)&0xff]);
            loop(t++);
        } else {
            touchInit = 0;
            delay(500);
            Serial1.println("power is off.\n");
        }
    }

    return 0;
}

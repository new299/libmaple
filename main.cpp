// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "i2c.h"
#include "mpr121.h"

#define CAPTOUCH_ADDR 0x5A
#define CAPTOUCH_I2C I2C1
#define CAPTOUCH_GPIO 30

#define LED_GPIO 25

#define CHARGE_GPIO 38

static struct i2c_dev *i2c;
extern void OLED_init(void);

/*
static void
cap_read(void)
{
    return;
}
*/

static void
cap_down(void)
{
    toggleLED();
    Serial1.println("Got toggle");
    return;
}


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
        Serial1.print(addr); Serial1.print(" -> "); Serial1.print(value); Serial1.print("\r\n");
    }
    else {
        Serial1.print(addr); Serial1.print(" err "); Serial1.print(result); Serial1.print("\r\n");
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
setup_i2c()
{
    i2c = CAPTOUCH_I2C;
    i2c_init(i2c);
    i2c_master_enable(i2c, 0);
    Serial1.print(".");
    pinMode(CAPTOUCH_GPIO, INPUT);  // hard coded guess for now

    // Section A
    // This group controls filtering when data is > baseline.
    mpr121Write(MHD_R, 0x01);
    mpr121Write(NHD_R, 0x01);
    mpr121Write(NCL_R, 0x50);
    mpr121Write(FDL_R, 0x50);

    // Section B
    // This group controls filtering when data is < baseline.
    mpr121Write(MHD_F, 0x01);
    mpr121Write(NHD_F, 0x01);
    mpr121Write(NCL_F, 0xFF);
    mpr121Write(FDL_F, 0x52);

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
    mpr121Write(FIL_CFG, 0x04);

    // Section E
    // Electrode Configuration
    // Enable 6 Electrodes and set to run mode
    // Set ELE_CFG to 0x00 to return to standby mode
    mpr121Write(ELE_CFG, 0x0C);   // Enables all 12 Electrodes
    //mpr121Write(ELE_CFG, 0x06);     // Enable first 6 electrodes

    // Section F
    // Enable Auto Config and auto Reconfig
    mpr121Write(ATO_CFG0, 0x0B);
    mpr121Write(ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V
    mpr121Write(ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
    mpr121Write(ATO_CFGT, 0xB5);    // Target = 0.9*USL = 0xB5 @3.3V


    return;
}

static void
setup_lcd(void)
{
    OLED_init();
    return;
}


/* Single-call setup routine */
static void
setup()
{
    //delay(1500);
    Serial1.print("In setup()...");


    /* Set up the LED to blink  */
    pinMode(LED_GPIO, OUTPUT);  // hard coded guess for now

    /* Set up battery charger */
    pinMode(CHARGE_GPIO, OUTPUT);
    digitalWrite(CHARGE_GPIO, 0);

    /* Must happen AFTER battery is set up */
    setup_lcd();
    //setup_i2c();

    /* Set up PB11 to be an IRQ that triggers cap_down */
    attachInterrupt(CAPTOUCH_GPIO, cap_down, CHANGE);

    Serial1.println(" Done.\n");
}


/* Main loop */
static void
loop(unsigned int t)
{
    uint8 bytes[2];
    bytes[0] = mpr121Read(0);
    bytes[1] = mpr121Read(1);
    
    Serial1.print("Values: [");
    Serial1.print(bytes[0]&(1<<0)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<1)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<2)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<3)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<4)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<5)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<6)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[0]&(1<<7)?"1":"0");
    Serial1.print(" ");

    Serial1.print(bytes[1]&(1<<0)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[1]&(1<<1)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[1]&(1<<2)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[1]&(1<<3)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[1]&(1<<4)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[1]&(1<<5)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[1]&(1<<6)?"1":"0");
    Serial1.print(" ");
    Serial1.print(bytes[1]&(1<<7)?"1":"0");
    Serial1.print("]\r");
    /*
    Serial1.print("Loop "); Serial1.print(t); Serial1.print("  I2C Banks:\r\n");
    for (i=0; i<0x1e; i++) {
        Serial1.print("    ");
        Serial1.print(i);
        Serial1.print(" = 0x");
        Serial1.print(mpr121Read(i), 16);
        Serial1.print("\r\n");
    }
    Serial1.print("\r\n");
    */

    delay(100);
}

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void
premain()
{
    init();
    /* Send a message out USART2  */
    Serial1.begin(115200);
    Serial1.print("In premain()...");
    //delay(1500);
    Serial1.println("Done.");
}



int
main(void)
{
    int t = 0;
    setup();

    while (true);
//        loop(t++);

    return 0;
}

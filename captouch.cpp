// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "captouch.h"
#include "mpr121.h"
#include "i2c.h"
#include "switch.h"

#define CAPTOUCH_ADDR 0x5A
#define CAPTOUCH_I2C I2C1
#define CAPTOUCH_GPIO 30

// "WASD" cluster as defined by physical arrangement of touch switches
static const char keys[] = "E.DWS.A.Q........";

static struct i2c_dev *i2c = CAPTOUCH_I2C;
static int should_poll;


static void (*on_keydown)(char key);
static void (*on_keyup)(char key);


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
cap_change(void)
{
    should_poll = 1;
}

int
cap_should_poll(void)
{
    return should_poll;
}


void
cap_poll(void)
{
    static int previous_board_state;
    int board_state;
    unsigned int key;

    board_state = mpr121Read(TCH_STATL);
    board_state |= mpr121Read(TCH_STATH) << 8;

    /* Go through and see what keys have changed */
    for (key=0; key<16; key++) {
        if ( (board_state&(1<<key)) != (previous_board_state&(1<<key))) {
            if ( (board_state&(1<<key))) {
                toggleLED();
                if (on_keydown)
                    on_keydown(keys[key]);
            }
            else if ( !(board_state&(1<<key)) ) {
                toggleLED();
                if (on_keyup)
                    on_keyup(keys[key]);
            }
        }
    }

    if (previous_board_state == board_state)
        Serial1.println("Got a cap_change event, but no change noted");

    previous_board_state = board_state;
    should_poll = 0;

    return;
}


int
cap_setkeydown(void (*new_keydown)(char key))
{
    on_keydown = new_keydown;
    return 0;
}

int
cap_setkeyup(void (*new_keyup)(char key))
{
    on_keyup = new_keyup;
    return 0;
}



void
cap_debug(void)
{
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
        /*
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
        */
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


static int
cap_init(void)
{
    return 0;
}

static int
cap_resume(struct device *dev)
{
    i2c_init(i2c);
    i2c_master_enable(i2c, 0);
    i2c->state = I2C_STATE_IDLE;

    should_poll = 1;

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

    pinMode(CAPTOUCH_GPIO, INPUT);  
    Serial1.println( "Attaching captouch interrupt\n" );
    attachInterrupt(CAPTOUCH_GPIO, cap_change, CHANGE);

    /* Read from the status registers to clear pending IRQs */
    mpr121Read(TCH_STATL);
    mpr121Read(TCH_STATH);

    return 0;
}

static int
cap_suspend(struct device *dev)
{
    detachInterrupt(CAPTOUCH_GPIO);
    should_poll = 0;

    // Disable MPR121 scanning, in case the chip is on
    if (switch_state(&back_switch))
        mpr121Write(ELE_CFG, 0x00);

    /* Shut down I2C */
    i2c_disable(i2c);

    return 0;
}

static int
cap_deinit(struct device *dev)
{
    return 0;
}



struct device captouch = {
    cap_init,
    cap_deinit,
    cap_suspend,
    cap_resume,
    "Capacitive Touchpad",
};


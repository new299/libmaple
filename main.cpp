// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "i2c.h"

#define PWM_PIN  2
static struct i2c_dev *i2c;

void setup() {
    /* Set up the LED to blink  */
    pinMode(25, OUTPUT);  // hard coded guess for now

#if 0
    /* Turn on PWM on pin PWM_PIN */
    pinMode(PWM_PIN, PWM);
    pwmWrite(PWM_PIN, 0x8000);
#endif

    /* Send a message out USART2  */
    Serial1.begin(115200);
    Serial1.println("Hello world!");
    i2c_init(i2c);
    i2c_master_enable(i2c, 0);

    /* Send a message out the usb virtual serial port  */
    //    SerialUSB.println("Hello!");
}

void loop() {
    #if 0
typedef struct i2c_msg {
    uint16 addr;                /**< Address */
#define I2C_MSG_READ            0x1
#define I2C_MSG_10BIT_ADDR      0x2
    uint16 flags;               /**< Bitwise OR of I2C_MSG_READ and
                                     I2C_MSG_10BIT_ADDR */
    uint16 length;              /**< Message length */
    uint16 xferred;             /**< Messages transferred */
    uint8 *data;                /**< Data */
} i2c_msg;
    #endif

    struct i2c_msg msgs[2];
    uint8 byte;
    int result;

    msgs[0].addr = 0x5A;
    msgs[0].flags = 0;
    msgs[0].length = 1;
    msgs[0].xferred = 0;
    byte = 0x00;
    msgs[0].data = &byte;

    msgs[1].addr = 0x5A;
    msgs[1].flags = I2C_MSG_READ;
    msgs[1].length = 1;
    msgs[1].xferred = 0;
    msgs[1].data = &byte;

    toggleLED();
    result = i2c_master_xfer(i2c, msgs, 2, 100);
    Serial1.print("I2C Result: ");
    Serial1.print(result);
    Serial1.print(" / ");
    Serial1.print(byte, 16);
    Serial1.print("\n");
    delay(500);
    Serial1.println("Hello world!");
}

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain() {
    init();
}

int main(void) {
    i2c = I2C1;
    setup();

    while (true) {
        loop();
    }

    return 0;
}

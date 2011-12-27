// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"

#define PWM_PIN  2

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

    /* Send a message out the usb virtual serial port  */
    //    SerialUSB.println("Hello!");
}

void loop() {
    toggleLED();
    delay(500);
    Serial1.println("Hello world!");
}

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain() {
    init();
}

int main(void) {
    setup();

    while (true) {
        loop();
    }

    return 0;
}

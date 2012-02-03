// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "led.h"
#include "power.h"

// for power control support
#include "pwr.h"
#include "scb.h"
#include "exti.h"
#include "gpio.h"

#define LED_GPIO 25       // PD2


void
led_set(int state)
{
    if(state) 
        digitalWrite(LED_GPIO, 1);
    else
        digitalWrite(LED_GPIO, 0);
}

void
led_toggle(void)
{
    togglePin(LED_GPIO);
}

static int
led_init(void)
{
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, 0);
    return 0;
}


static int
led_suspend(struct device *dev) {
    pinMode(LED_GPIO, INPUT); // hi-z it
    return 0;
}


static int
led_deinit(struct device *dev)
{
    pinMode(LED_GPIO, INPUT); // hi-z it
    return 0;
}

static int
led_resume(struct device *dev)
{
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, 0);
    return 0;
}

int
led_state(struct device *dev)
{
    return digitalRead(LED_GPIO) == HIGH;
}

struct device led = {
    led_init,
    led_deinit,
    led_suspend,
    led_resume,

    "Red LED",
};

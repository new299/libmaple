// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "switch.h"
#include "power.h"

// for power control support
#include "pwr.h"
#include "scb.h"
#include "exti.h"
#include "gpio.h"

#define MANUAL_WAKEUP_GPIO 18 // PC3

#define COMBO_WAKEUP_GPIO 2 // PA0


static void
switch_change(void)
{
    pinMode(MANUAL_WAKEUP_GPIO, INPUT);

    if (switch_state(&back_switch)) {
        rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSI_DIV_2, RCC_PLLMUL_9); 
        rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
        rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB2_HCLK_DIV_1);
        rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);
        power_set_state(PWRSTATE_USER);
    } else {
        power_set_state(PWRSTATE_LOG);
    }
}

static int
switch_init(void)
{
    pinMode(MANUAL_WAKEUP_GPIO, INPUT);
    pinMode(COMBO_WAKEUP_GPIO, INPUT);

    attachInterrupt(COMBO_WAKEUP_GPIO, switch_change, CHANGE);
    return 0;
}


static int
switch_suspend(struct device *dev) {
    return 0;
}


static int
switch_deinit(struct device *dev)
{
    detachInterrupt(COMBO_WAKEUP_GPIO);
    return 0;
}

static int
switch_resume(struct device *dev)
{
    return 0;
}

int
switch_state(struct device *dev)
{
    return digitalRead(MANUAL_WAKEUP_GPIO) == HIGH;
}

struct device back_switch = {
    switch_init,
    switch_deinit,
    switch_suspend,
    switch_resume,

    "Back Switch",
};

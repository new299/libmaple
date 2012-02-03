///// power testing stubs.

#include "wirish.h"
#include "flash.h"
#include "nvic.h"
#include "gpio.h"

// for power control support
#include "pwr.h"
#include "scb.h"
#include "power.h"

#define LED_GPIO 25       // PD2
#define MANUAL_WAKEUP_GPIO 18 // PC3

#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5

int delay_rate = 300000;
int doStop = 0;

int
power_stop_b() { 
    //    Serial1.println ("Stopping CPU.\n" ); // for debug only
#if 0
    // stop will not wait for ISRs to exit
    SCB_BASE->SCR &= ~SCB_SCR_SLEEPONEXIT;
    
    /* Enter "Stop" mode */
    // clear wakup flag
    PWR_BASE->CR |= BIT(PWR_CR_CWUF);

    // set sleepdeep in cortex system control register
    SCB_BASE->SCR |= SCB_SCR_SLEEPDEEP;

    // enable wakeup pin -- can't seem to get it to work any other way
    PWR_BASE->CSR |= BIT(PWR_CSR_EWUP);

    // select stop mode (clear standby bit)
    PWR_BASE->CR &= ~BIT(PWR_CR_PDDS);

    // put regulator in low power mode
    PWR_BASE->CR |= BIT(PWR_CR_LPDS);
#else
    SCB_BASE->SCR = 0x4;
    PWR_BASE->CSR = 0x100;
    PWR_BASE->CR = 4;
    //    PWR_BASE->CR = 2; // for standby only
    PWR_BASE->CR = 1;  // for stop and low power mode set
#endif

    asm volatile (".code 16\n"
                  "wfi\n");

    digitalWrite(LED_GPIO, 1);

    return 0;
}

static int switch_state()
{
    return digitalRead(MANUAL_WAKEUP_GPIO) == HIGH;
}

void switch_change_b(void)
{
    digitalWrite(LED_GPIO, 1);

    rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSI_DIV_2, RCC_PLLMUL_9); 
    rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB2_HCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);

#if 0    
    gpio_init_all();
    afio_init();

    pinMode(LED_GPIO, OUTPUT);  
    digitalWrite(LED_GPIO, 1);

    pinMode(MANUAL_WAKEUP_GPIO, INPUT);
#endif 
    delay_us(100);

    if (switch_state()) {
        delay_rate = 300000;
    }
    else {
        delay_rate = 100000;
        digitalWrite(LED_GPIO, 0);
        doStop = 1;
    }

    return;
}

void
static geiger_rising_b(void)
{
    // for now, set to defaults but may want to lower clock rate so we're not burning battery
    // to run a CPU just while the buzzer does its thing
    rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSI_DIV_2, RCC_PLLMUL_9); 
    rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB2_HCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);
    
    delay_us(100);
    Serial1.println("beep.");
    
}

// this version of pwr_test() tests stop and resume
void pwr_test() {
    uint32 temp;

    //    init();
    flash_enable_prefetch();
    flash_set_latency(FLASH_WAIT_STATE_1);

    rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSI_DIV_2, RCC_PLLMUL_9); 
    rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB2_HCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);


#ifdef VECT_TAB_FLASH
    nvic_init(USER_ADDR_ROM, 0);
#elif defined VECT_TAB_RAM
    nvic_init(USER_ADDR_RAM, 0);
#elif defined VECT_TAB_BASE
    nvic_init((uint32)0x08000000, 0);
#else
#error "You must select a base address for the vector table."
#endif
    
    gpio_init_all();
    afio_init();

    Serial1.begin(115200);

    pinMode(LED_GPIO, OUTPUT);  
    digitalWrite(LED_GPIO, 1);

    pinMode(MANUAL_WAKEUP_GPIO, INPUT);
    doStop = 0;
    attachInterrupt(MANUAL_WAKEUP_GPIO, switch_change_b, CHANGE);

    ////////
    //    pinMode(GEIGER_ON_GPIO, OUTPUT);
    //    digitalWrite(GEIGER_ON_GPIO, 1);
    //    delay_us(1000); // 1 ms for the geiger to settle
    //    temp = AFIO_BASE->MAPR & 0xF8FFFFFF;
    //    temp = 0x04000000;
    AFIO_BASE->MAPR = 0x04000000;
    //    pinMode(GEIGER_PULSE_GPIO, INPUT_PULLDOWN); // make it INPUT for production, this is for testing without a module
    delay_us(5000000);
    Serial1.println((AFIO_BASE->MAPR) >> 16,16);

    //    pinMode(42, INPUT_PULLDOWN);
    //    attachInterrupt(42, geiger_rising_b, RISING);
    pinMode(GEIGER_PULSE_GPIO, INPUT_PULLDOWN);
    attachInterrupt(GEIGER_PULSE_GPIO, geiger_rising_b, RISING);
    /////////

    while(1) {
        Serial1.print(".");
        delay_us( delay_rate );
        digitalWrite(LED_GPIO, 0);
        delay_us( delay_rate );
        digitalWrite(LED_GPIO, 1);
#if 1
        if( doStop ) {
            Serial1.println("Powering board off!");
            doStop = 0;
            digitalWrite(LED_GPIO, 0);
            power_stop_b();
            delay_us(1000);
            Serial1.println("Powering board on!");
        }
#else
        if( delay_rate == 100000 ) {
            asm volatile (".code 16\n"
                          //                          "nop\n");
                          "wfi\n");
        }
#endif
    }
}

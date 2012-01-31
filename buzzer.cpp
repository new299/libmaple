// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "wirish.h"
#include "buzzer.h"

#define BUZZER_PWM 24 // PB9
#define BUZZ_RATE  250  // in microseconds; set to 4kHz = 250us

static HardwareTimer buzzTimer(4);
static int volume = 1;


void
buzzer_set_volume(int new_vol)
{
    volume = new_vol;
}

void
buzzer_buzz_blocking(void)
{
    if (volume) {
        buzzTimer.resume();
        delay(50);
        buzzTimer.pause();
    }
}


static void
handler_buzz(void) {
    togglePin(BUZZER_PWM);
}




static int
buzzer_init(void)
{
    // initially, un-bias the buzzer
    pinMode(BUZZER_PWM, OUTPUT);
    digitalWrite(BUZZER_PWM, 0);

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

    return 0;
}


static int
buzzer_suspend(struct device *dev) {
    return 0;
}


static int
buzzer_deinit(struct device *dev)
{
    return 0;
}

static int
buzzer_resume(struct device *dev)
{
    return 0;
}

struct device buzzer = {
    buzzer_init,
    buzzer_deinit,
    buzzer_suspend,
    buzzer_resume,
    "Simple Buzzer",
};

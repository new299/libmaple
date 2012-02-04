#include "wirish.h"
#include "rtc.h"
#include "device.h"

#define SECS_PER_DAY (60*60*24)
#define DAY_SECONDS(x) (x-SECS_PER_DAY*(x/(SECS_PER_DAY)))

uint32
time_get(void)
{
    return rtc_get_time(RTC);
}

uint32
time_hour(void)
{
    static int last_hour;
    static int last_time;
    int time = rtc_get_time(RTC);

    if (time == last_time)
        return last_hour;

    int s = DAY_SECONDS(time);
    last_hour = s/60/60;
    last_time = time;
    return last_hour;
}

uint32
time_minutes(void)
{
    static int last_minutes;
    static int last_time;
    int time = rtc_get_time(RTC);

    if (time == last_time)
        return last_minutes;

    int s = DAY_SECONDS(time);
    last_minutes = (s-(60*60*(s/60/60)))/60;
    last_time = time;
    return last_minutes;
}

uint32
time_seconds(void)
{
    static int last_seconds;
    static int last_time;
    int time = rtc_get_time(RTC);

    if (time == last_time)
        return last_seconds;

    int s = DAY_SECONDS(time);
    last_seconds = (s-(60*(s/60)));
    last_time = time;
    return last_seconds;
}

void
time_set(uint32 time)
{
    rtc_set_time(RTC, time);
}

static int
time_init(void)
{
    rtc_init(RTC);
    return 0;
}

int
time_dump_registers(void)
{
    Serial1.println("Time registers:");
    Serial1.print("   CRH:  0x"); Serial1.println(RTC->regs->CRH, 16);
    Serial1.print("   CRL:  0x"); Serial1.println(RTC->regs->CRL, 16);
    Serial1.print("   PRLH: 0x"); Serial1.println(RTC->regs->PRLH, 16);
    Serial1.print("   PRLL: 0x"); Serial1.println(RTC->regs->PRLL, 16);
    Serial1.print("   DIVH: 0x"); Serial1.println(RTC->regs->DIVH, 16);
    Serial1.print("   DIVL: 0x"); Serial1.println(RTC->regs->DIVL, 16);
    Serial1.print("   CNTH: 0x"); Serial1.println(RTC->regs->CNTH, 16);
    Serial1.print("   CNTL: 0x"); Serial1.println(RTC->regs->CNTL, 16);
    Serial1.print("   ALRH: 0x"); Serial1.println(RTC->regs->ALRH, 16);
    Serial1.print("   ALRL: 0x"); Serial1.println(RTC->regs->ALRL, 16);
    return 0;
}


static int
time_resume(struct device *dev)
{
    return 0;
}


static int
time_suspend(struct device *dev) {
    return 0;
}


static int
time_deinit(struct device *dev)
{
    return 0;
}


struct device time = {
    time_init,
    time_deinit,
    time_suspend,
    time_resume,

    "Real Time Clock",
};

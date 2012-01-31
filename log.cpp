#include "wirish.h"
#include "log.h"
#include "pwr.h"

// this routine should set everything up for geiger pulse logging
static int
log_init(void)
{
    return 0;
}

static int
log_deinit(struct device *dev)
{
    return 0;
}


// this routine enter standby mode, with a wakeup set from the WAKEUP event from the geiger counter or switch
static int
log_standby(struct device *dev)
{
    // enable wake on interrupt
    PWR_BASE->CSR |= PWR_CSR_EWUP;
    return 0;
}

static int
log_resume(struct device *dev)
{
    return 0;
}

struct device logger = {
    log_init,
    log_deinit,
    log_standby,
    log_resume,
    
    "Flash Logger",
};

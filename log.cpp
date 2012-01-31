#include "wirish.h"
#include "log.h"
#include "pwr.h"

// this routine should set everything up for geiger pulse logging
void
log_init(void)
{
    return;
}

void
log_deinit(void)
{
    return;
}


// this routine enter standby mode, with a wakeup set from the WAKEUP event from the geiger counter or switch
void
log_standby(void)
{
    // enable wake on interrupt
    PWR_BASE->CSR |= PWR_CSR_EWUP;
    return;
}

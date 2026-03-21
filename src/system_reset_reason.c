#include <hal/nrf_power.h>
#include <zephyr/logging/log.h>
#include "system_reset_reason.h"

LOG_MODULE_REGISTER(RESET_REASON);

void log_reset_reason(){
    uint32_t reason = nrf_power_resetreas_get(NRF_POWER);
    if(reason ==0){
        LOG_INF("Reset occured unknown");
    }
    LOG_INF("Reset reason is %u",reason);
    if(reason & NRF_POWER_RESETREAS_RESETPIN_MASK){
        LOG_INF("Reset Cause: Reset pin");
    }

    if(reason & NRF_POWER_RESETREAS_DOG_MASK){
        LOG_INF("Reset Cause: WDT timeout");
    }

    if(reason & NRF_POWER_RESETREAS_SREQ_MASK){
        LOG_INF("Reset Cause: Software Reset");
    }
    if(reason & NRF_POWER_RESETREAS_LOCKUP_MASK){
        LOG_INF("Reset Cause: cpu lockup");
    }

    if(reason & NRF_POWER_RESETREAS_OFF_MASK){
        LOG_INF("Reset Cause: Wake from System OFF");
    }

    if(reason & NRF_POWER_RESETREAS_SREQ_MASK){
        LOG_INF("Reset Cause: Software Reset");
    }

    nrf_power_resetreas_clear(NRF_POWER, reason);
}
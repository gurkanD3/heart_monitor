#include "watchdog.h"
LOG_MODULE_REGISTER(watchdog_timer, LOG_LEVEL_INF);

#ifndef WDT_OPT
#define WDT_OPT 0
#endif

#ifndef WDT_MAX_WINDOW// consumer task has 10second sleep
#define WDT_MAX_WINDOW  20000U 
#endif

#ifndef WDT_MIN_WINDOW
#define WDT_MIN_WINDOW  0U
#endif


#ifndef WDT_ALLOW_CALLBACK
#define WDT_ALLOW_CALLBACK 1
#endif

static int wdt_channel_id;
const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog_timer));

#if WDT_ALLOW_CALLBACK
static void wdt_callback(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	sys_reboot(0);
	LOG_ERR("Watchdog timeout callback fired");
}
#endif /* WDT_ALLOW_CALLBACK */


void watchdog_configuration(){
    int err;

	if (!device_is_ready(wdt)) {
		LOG_ERR("%s: device not ready.\n", wdt->name);
		return ;
	}

	struct wdt_timeout_cfg wdt_config = {

		.flags = WDT_FLAG_RESET_SOC,
		.window.min = WDT_MIN_WINDOW,
		.window.max = WDT_MAX_WINDOW,
	};

#if WDT_ALLOW_CALLBACK
	
	wdt_config.callback = wdt_callback;

	LOG_INF("Watchdog callback enabled");
#else 
	wdt_config.callback = NULL;
	LOG_INF("Watchdog callback disabled, hardware reset only");
#endif 

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id == -ENOTSUP) {
		LOG_ERR("Watchdog callback/flags not supported, retrying without callback");
		wdt_config.callback = NULL;
		wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	}
	if (wdt_channel_id < 0) {
		LOG_ERR("Watchdog install error: %d", wdt_channel_id);
		return ;
	}
	LOG_INF("Watchdog timeout installed on channel %d, timeout=%u ms",
		wdt_channel_id, (unsigned int)WDT_MAX_WINDOW);

	err = wdt_setup(wdt, WDT_OPT);
	if (err < 0) {
		LOG_ERR("Watchdog setup error: %d", err);
		return ;
	}
	LOG_INF("Watchdog started");
}

void watchdog_feed(){
	int err = wdt_feed(wdt, wdt_channel_id);
	LOG_INF("FEED CALLED");
	if (err < 0) {
		LOG_ERR("Watchdog feed failed: %d", err);
	}
}

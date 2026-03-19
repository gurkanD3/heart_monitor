#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

extern void watchdog_configuration();
extern void watchdog_feed();

#endif
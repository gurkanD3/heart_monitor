#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "system_reset_reason.h"
#include "circular_buffer.h"
#include "watchdog.h"

LOG_MODULE_REGISTER(heart_monitor, LOG_LEVEL_INF);

#define EMA_ALPHA_MAX	256
#define THREAD_PRIORITY 4
#define PRODUCER_INTERVAL_MS 1000
#define CONSUMER_INTERVAL_MS 10000
#define EMA_ALPHA_FACTOR (64)

K_THREAD_STACK_DEFINE(producer_stack, 1024);
K_THREAD_STACK_DEFINE(consumer_stack, 1024);

static struct k_thread producer_thread;
static struct k_thread consumer_thread;

static k_tid_t producer_tid;
static k_tid_t consumer_tid;

K_MUTEX_DEFINE(buffer_mutex);
static atomic_t producer_wdt_check;
static atomic_t consumer_wdt_check;

//EMA=alpha⋅x+(1−alpha)⋅prev
// for easy divison I change ema range to 0-256
struct ema_filter { 
	uint32_t value;
	uint16_t alpha_factor; //based 256
	uint8_t initialized:1;
}ema={0,EMA_ALPHA_FACTOR,false};

static void producer_task(void *arg1, void *arg2, void *arg3);
static void consumer_task(void *arg1, void *arg2, void *arg3);

static uint8_t generate_random_sample(void)
{
	uint32_t value = sys_rand32_get();

	return (uint8_t)(44U + (value % (185U - 44U + 1U)));
}

static uint32_t ema_update(struct ema_filter *filter, uint8_t sample)
{
	uint32_t alpha = filter->alpha_factor;

	if (!filter->initialized) {
		filter->value = sample;
		filter->initialized = true;
		return filter->value;
	}

		/**
		 * calculation is alpha is a 256 based so
		 * alpha/256 so if we write every alpha to alpha/256
		 * now = alpha*sample/256 + (256-alpha/256)*prev
		 * ((alpha*sample)+(256-alpha)*prev))/256
		 */
	filter->value = ((alpha*sample)+(EMA_ALPHA_MAX-alpha)*filter->value)>>8; // 1>>8 = 256 easy division and faster

	return filter->value;
}

int main(void)
{       
	cb_err_t ret= circular_buffer_init();
	if (ret != CBUFFER_OK) {
		LOG_ERR("Failed to initialize circular buffer: %d", ret);
		return ret;
	}
	log_reset_reason();
	watchdog_configuration();
	if (producer_tid == NULL) {
		producer_tid = k_thread_create(&producer_thread, producer_stack,
					       K_THREAD_STACK_SIZEOF(producer_stack),
					       producer_task,
					       NULL, NULL, NULL,
					       K_PRIO_PREEMPT(THREAD_PRIORITY),
					       0, K_NO_WAIT);
	}

	if (consumer_tid == NULL) {
		consumer_tid = k_thread_create(&consumer_thread, consumer_stack,
					       K_THREAD_STACK_SIZEOF(consumer_stack),
					       consumer_task,
					       NULL, NULL, NULL,
					       K_PRIO_PREEMPT(THREAD_PRIORITY),
					       0, K_NO_WAIT);
	}

	while (true) {
		k_sleep(K_MSEC(1000));
		bool producer_heart_beat = atomic_get(&producer_wdt_check);
		bool consumer_heart_beat = atomic_get(&consumer_wdt_check);

		if (producer_heart_beat &&consumer_heart_beat) { // make sure all tasks are updated!
			watchdog_feed();
			atomic_cas(&producer_wdt_check,1,0);
			atomic_cas(&consumer_wdt_check,1,0);
		}
	}
}

static void producer_task(void *arg1, void *arg2, void *arg3)
{
	cb_err_t ret;
	uint8_t sample;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		sample = generate_random_sample();

		k_mutex_lock(&buffer_mutex, K_FOREVER);
		ret = circular_buffer_push(sample);
		k_mutex_unlock(&buffer_mutex);
		if (ret != CBUFFER_OK) {
			LOG_ERR("Failed to push sample: %d", ret);
		}
		atomic_set(&producer_wdt_check, 1);

		k_sleep(K_MSEC(PRODUCER_INTERVAL_MS));
	}
}

static void consumer_task(void *arg1, void *arg2, void *arg3)
{
	uint8_t sample = 0U;
	uint32_t ema_value = 0U;
	size_t consumed = 0U;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		k_sleep(K_MSEC(CONSUMER_INTERVAL_MS));
		k_mutex_lock(&buffer_mutex, K_FOREVER);
		while (circular_buffer_pop(&sample) == CBUFFER_OK) {
			ema_value = ema_update(&ema, sample);
			consumed++;
		}
		k_mutex_unlock(&buffer_mutex);
		atomic_set(&consumer_wdt_check, 1);
		if (consumed == 0U) {
			continue;
		}
		LOG_INF("latest EMA: %u", ema_value);
		consumed = 0U;
	}
}

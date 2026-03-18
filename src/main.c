#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util.h>

#include "circular_buffer.h"

LOG_MODULE_REGISTER(heart_monitor, LOG_LEVEL_INF);

#define THREAD_PRIORITY 4
#define PRODUCER_INTERVAL_MS 1000
#define CONSUMER_INTERVAL_MS 10000
#define EMA_ALPHA_PERCENT 25

K_THREAD_STACK_DEFINE(producer_stack, 1024);
K_THREAD_STACK_DEFINE(consumer_stack, 1024);

static struct k_thread producer_thread;
static struct k_thread consumer_thread;

static k_tid_t producer_tid;
static k_tid_t consumer_tid;

K_MUTEX_DEFINE(buffer_mutex);

struct ema_filter {
	uint8_t alpha_percent;
	uint32_t value;
	bool initialized;
}ema={EMA_ALPHA_PERCENT,0,false};

static void producer_task(void *arg1, void *arg2, void *arg3);
static void consumer_task(void *arg1, void *arg2, void *arg3);

static uint8_t generate_random_sample(void)
{
	uint32_t value = sys_rand32_get();

	return (uint8_t)(44U + (value % (185U - 44U + 1U)));
}

static uint32_t ema_update(struct ema_filter *filter, uint8_t sample)
{
	uint32_t alpha = filter->alpha_percent;

	if (!filter->initialized) {
		filter->value = sample;
		filter->initialized = true;
		return filter->value;
	}

	filter->value =
		((alpha * sample) + ((100U - alpha) * filter->value)) / 100U;

	return filter->value;
}

int main(void)
{       
        cb_err_t ret= circular_buffer_init_();
        /**
         cb_err_t ret= circular_buffer_init_(20); // for linked list approach
         */

	if (ret != CBUFFER_OK) {
		LOG_ERR("Failed to initialize circular buffer: %d", ret);
		return ret;
	}

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

	return 0;
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
			LOG_INF("Consumed sample: %u -> EMA: %u", sample, ema_value);
		}
		k_mutex_unlock(&buffer_mutex);

		if (consumed == 0U) {
			LOG_INF("Consumer found no buffered samples");
			continue;
		}

		LOG_INF("Consumer drained %u samples, latest EMA: %u",
			(unsigned int)consumed, ema_value);
		consumed = 0U;
	}
}

#ifndef ORANGE_BK_PICO_TIME_H
#define ORANGE_BK_PICO_TIME_H

/*
 * Linux host replacement for the Pico SDK <pico/time.h>: the microsecond
 * clock and sleeps (platform_linux.h) and repeating timers.
 *
 * On RP2350 a repeating timer is an alarm-pool interrupt on the core that
 * created it.  Here each timer runs its callback on its own thread, paced by
 * CLOCK_MONOTONIC.  As in the SDK, a positive delay is measured from the end
 * of one callback to the start of the next, a negative one from start to
 * start; the callback returns false to stop the timer.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "pico.h"
#include "platform_linux.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct repeating_timer repeating_timer_t;
typedef bool (*repeating_timer_callback_t)(repeating_timer_t *rt);

struct repeating_timer {
	int64_t delay_us;
	void *user_data;
	repeating_timer_callback_t callback;
	pthread_t thread;
	volatile bool running;
	bool started;
};

bool add_repeating_timer_us(int64_t delay_us, repeating_timer_callback_t callback,
			    void *user_data, repeating_timer_t *out);
bool cancel_repeating_timer(repeating_timer_t *timer);

static inline bool add_repeating_timer_ms(int32_t delay_ms, repeating_timer_callback_t callback,
					  void *user_data, repeating_timer_t *out)
{
	return add_repeating_timer_us((int64_t)delay_ms * 1000, callback, user_data, out);
}

static inline void busy_wait_us(uint64_t us)
{
	const uint64_t end = time_us_64() + us;
	while (time_us_64() < end)
		;
}

#ifdef __cplusplus
}
#endif

#endif /* ORANGE_BK_PICO_TIME_H */

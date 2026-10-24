#ifndef ORANGE_BK_PLATFORM_LINUX_H
#define ORANGE_BK_PLATFORM_LINUX_H

/*
 * Linux host services used in place of the Pico SDK: the microsecond clock
 * and sleeps (pico/time.h), and the requests that replace the watchdog
 * reset of the RP2350 build.
 */

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SDK time_us_64(): microseconds of CLOCK_MONOTONIC. */
static inline uint64_t time_us_64(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
}

static inline void sleep_us(uint64_t us)
{
	struct timespec ts;
	ts.tv_sec = (time_t)(us / 1000000u);
	ts.tv_nsec = (long)(us % 1000000u) * 1000L;
	while (nanosleep(&ts, &ts) != 0)
		;
}

static inline void sleep_ms(uint32_t ms)
{
	sleep_us((uint64_t)ms * 1000u);
}

/* Sleep until time_us_64() reaches deadline_us (no-op if it already has). */
static inline void sleep_until_us(uint64_t deadline_us)
{
	uint64_t now = time_us_64();
	if (deadline_us > now)
		sleep_us(deadline_us - now);
}

enum platform_request {
	PLATFORM_RUN = 0,
	PLATFORM_REBOOT,    /* Ctrl+Alt+Del: restart the emulator process */
	PLATFORM_QUIT,      /* Ctrl+Alt+End, SIGINT, SIGTERM */
};

/* Ask the render thread to reboot or quit; safe from any thread and from
 * signal handlers.  The render thread owns the display, so it performs the
 * request after its current frame (platform_service()). */
void platform_request(enum platform_request request);

/* Render thread: carry out a pending request.  Does not return if one was
 * pending: releases the display, audio and input devices, then re-executes
 * the process (reboot) or exits (quit). */
void platform_service(void);

/* Remember argv for the reboot re-execution. */
void platform_set_argv(char **argv);

#ifdef __cplusplus
}
#endif

#endif /* ORANGE_BK_PLATFORM_LINUX_H */

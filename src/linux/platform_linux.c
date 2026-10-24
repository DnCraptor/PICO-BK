#include "platform_linux.h"
#include "audio_linux.h"
#include "input_linux.h"
#include "video_drm.h"
#include "f_util.h"
#include "hardware/clocks.h"
#include "pico/time.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t pending = PLATFORM_RUN;
static char **saved_argv;

void platform_set_argv(char **argv)
{
	saved_argv = argv;
}

void platform_request(enum platform_request request)
{
	/* A quit outranks a reboot that is still pending. */
	if ((int)request > pending)
		pending = request;
}

/* Release everything the process holds outside itself: input grabs, the
 * CRTC (original console mode restored) and the PCM device. */
static void host_shutdown(void)
{
	input_shutdown();
	video_drm_close();
	audio_shutdown();
	fflush(NULL);
}

void platform_service(void)
{
	const int request = pending;
	if (request == PLATFORM_RUN)
		return;
	host_shutdown();
	if (request == PLATFORM_REBOOT && saved_argv) {
		/* RP2350: watchdog reset of the whole MCU.  Here: a fresh process,
		 * so every part of the emulator starts from its power-on state. */
		execv("/proc/self/exe", saved_argv);
		perror("reboot: execv");
		_exit(EXIT_FAILURE);
	}
	_exit(EXIT_SUCCESS);
}

/*
 * Pico SDK repeating timers (pico/time.h) on host threads.
 */
static void sleep_until_abs_us(uint64_t t)
{
	struct timespec ts = {
		.tv_sec = (time_t)(t / 1000000u),
		.tv_nsec = (long)(t % 1000000u) * 1000L,
	};
	while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) == EINTR)
		;
}

static void *repeating_timer_main(void *arg)
{
	repeating_timer_t *rt = arg;
	const uint64_t period = (uint64_t)(rt->delay_us < 0 ? -rt->delay_us : rt->delay_us);
	uint64_t next = time_us_64() + period;
	while (rt->running) {
		sleep_until_abs_us(next);
		if (!rt->running)
			break;
		const uint64_t start = time_us_64();
		if (!rt->callback(rt))
			break;
		/* delay_us > 0: end-to-start; delay_us < 0: start-to-start */
		next = rt->delay_us > 0 ? time_us_64() + period : start + period;
	}
	rt->running = false;
	return NULL;
}

bool add_repeating_timer_us(int64_t delay_us, repeating_timer_callback_t callback,
			    void *user_data, repeating_timer_t *out)
{
	if (!out || !callback || delay_us == 0)
		return false;
	/* A timer stopped by its own callback still owns a thread to reap. */
	if (out->started && !out->running) {
		pthread_join(out->thread, NULL);
		out->started = false;
	}
	if (out->started)
		return false;
	out->delay_us = delay_us;
	out->callback = callback;
	out->user_data = user_data;
	out->running = true;
	if (pthread_create(&out->thread, NULL, repeating_timer_main, out) != 0) {
		out->running = false;
		return false;
	}
	out->started = true;
	return true;
}

bool cancel_repeating_timer(repeating_timer_t *timer)
{
	if (!timer || !timer->started)
		return false;
	const bool was_running = timer->running;
	timer->running = false;
	/* Called from the timer's own callback (as the BK keyboard script
	 * does): the thread ends after the callback returns, so it cannot be
	 * joined here; add_repeating_timer_us() reaps it later. */
	if (pthread_equal(pthread_self(), timer->thread))
		return was_running;
	pthread_join(timer->thread, NULL);
	timer->started = false;
	return was_running;
}

uint32_t clock_get_hz(enum clock_index clk)
{
	(void)clk;
	FILE *f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
	if (!f)
		return 0;
	unsigned long khz = 0;
	if (fscanf(f, "%lu", &khz) != 1)
		khz = 0;
	fclose(f);
	return (uint32_t)(khz * 1000u);
}

const char *FRESULT_str(FRESULT i)
{
	switch (i) {
	case FR_OK: return "Succeeded";
	case FR_DISK_ERR: return "A hard error occurred in the low level disk I/O layer";
	case FR_INT_ERR: return "Assertion failed";
	case FR_NOT_READY: return "The physical drive cannot work";
	case FR_NO_FILE: return "Could not find the file";
	case FR_NO_PATH: return "Could not find the path";
	case FR_INVALID_NAME: return "The path name format is invalid";
	case FR_DENIED: return "Access denied due to prohibited access or directory full";
	case FR_EXIST: return "Access denied due to prohibited access (exists)";
	case FR_INVALID_OBJECT: return "The file/directory object is invalid";
	case FR_WRITE_PROTECTED: return "The physical drive is write protected";
	case FR_INVALID_DRIVE: return "The logical drive number is invalid";
	case FR_NOT_ENABLED: return "The volume has no work area (mount)";
	case FR_NO_FILESYSTEM: return "There is no valid FAT volume";
	case FR_MKFS_ABORTED: return "The f_mkfs() aborted due to any problem";
	case FR_TIMEOUT: return "Could not get a grant to access the volume within defined period";
	case FR_LOCKED: return "The operation is rejected according to the file sharing policy";
	case FR_NOT_ENOUGH_CORE: return "LFN working buffer could not be allocated";
	case FR_TOO_MANY_OPEN_FILES: return "Number of open files > FF_FS_LOCK";
	case FR_INVALID_PARAMETER: return "Given parameter is invalid";
	default: return "Unknown";
	}
}

/* BSD strnstr(): first occurrence of needle in the first len characters of
 * haystack (stops at a NUL in haystack). */
char *strnstr(const char *haystack, const char *needle, size_t len)
{
	const size_t n = strlen(needle);
	if (n == 0)
		return (char *)haystack;
	for (size_t i = 0; i + n <= len && haystack[i]; ++i)
		if (haystack[i] == needle[0] && strncmp(haystack + i, needle, n) == 0)
			return (char *)haystack + i;
	return NULL;
}

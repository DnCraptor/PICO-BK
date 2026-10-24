#ifndef ORANGE_BK_HARDWARE_WATCHDOG_H
#define ORANGE_BK_HARDWARE_WATCHDOG_H

/*
 * Linux host replacement for the Pico SDK <hardware/watchdog.h>.
 * reboot() resets the RP2350 through the watchdog; the host equivalent
 * restarts the emulator process (platform_service() in the render thread
 * re-executes it within one frame).
 */

#include <stdbool.h>
#include <stdint.h>

#include "pico.h"
#include "platform_linux.h"

static inline void watchdog_enable(uint32_t delay_ms, bool pause_on_debug)
{
	(void)delay_ms;
	(void)pause_on_debug;
	platform_request(PLATFORM_REBOOT);
}

#endif /* ORANGE_BK_HARDWARE_WATCHDOG_H */

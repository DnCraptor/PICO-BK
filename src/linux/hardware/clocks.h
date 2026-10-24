#ifndef ORANGE_BK_HARDWARE_CLOCKS_H
#define ORANGE_BK_HARDWARE_CLOCKS_H

/*
 * Linux host replacement for the Pico SDK <hardware/clocks.h>.
 * clock_get_hz(clk_sys) reports the current host CPU clock (cpufreq of
 * CPU 0), shown by the manager; 0 when unknown.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum clock_index { clk_sys = 0 };

uint32_t clock_get_hz(enum clock_index clk);

#ifdef __cplusplus
}
#endif

#endif /* ORANGE_BK_HARDWARE_CLOCKS_H */

#ifndef ORANGE_BK_HARDWARE_PWM_H
#define ORANGE_BK_HARDWARE_PWM_H

/*
 * Linux host replacement for the Pico SDK <hardware/pwm.h>.
 * The RP2350 PWM build drives the beeper pin directly; on Linux the beeper
 * state (beeper_on) is mixed into the ALSA stream instead, so pin levels are
 * dropped.
 */

#include <stdint.h>

typedef struct {
	uint32_t csr;
	uint32_t div;
	uint32_t top;
} pwm_config;

static inline void pwm_set_gpio_level(unsigned gpio, uint16_t level) { (void)gpio; (void)level; }

#endif /* ORANGE_BK_HARDWARE_PWM_H */

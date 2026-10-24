#ifndef ORANGE_BK_HARDWARE_GPIO_H
#define ORANGE_BK_HARDWARE_GPIO_H

/*
 * Linux host replacement for the Pico SDK <hardware/gpio.h>.
 * The emulator drives the board LED (disk activity) and reads the tape input
 * through GPIO; the host has neither, so outputs are dropped and inputs read
 * as low.
 */

#include <stdbool.h>
#include <stdint.h>

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

#define GPIO_OUT 1
#define GPIO_IN  0

static inline void gpio_put(unsigned gpio, bool value) { (void)gpio; (void)value; }
static inline bool gpio_get(unsigned gpio) { (void)gpio; return false; }
static inline void gpio_init(unsigned gpio) { (void)gpio; }
static inline void gpio_set_dir(unsigned gpio, bool out) { (void)gpio; (void)out; }

#endif /* ORANGE_BK_HARDWARE_GPIO_H */

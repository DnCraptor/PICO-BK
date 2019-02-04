#ifndef GPIO_LIB_H
#define GPIO_LIB_H


#include "ets.h"
#include "pin_mux_register.h"


#ifdef __cplusplus
extern "C" {
#endif

#define gpio_init_input_pu( gpio)   do {PIN_FUNC_SELECT (gpio##_REG, gpio##_FUNC); gpio_output_set(0, 0, 0, (1 << (gpio))); PIN_PULLUP_EN (gpio##_REG);} while (0)
#define gpio_init_input(    gpio)   do {PIN_FUNC_SELECT (gpio##_REG, gpio##_FUNC); gpio_output_set(0, 0, 0, (1 << (gpio)));} while (0)
#define gpio_init_output(   gpio)   do {PIN_FUNC_SELECT (gpio##_REG, gpio##_FUNC); gpio_output_set(0, 0, (1 << (gpio)), 0);} while (0)
#define gpio_on(            gpio)   do {gpio_output_set ((1 << (gpio)), 0, 0, 0);} while (0)
#define gpio_off(           gpio)   do {gpio_output_set (0, (1 << (gpio)), 0, 0);} while (0)
#define gpio_in(            gpio)   ((gpio_input_get () & (1 << (gpio))) != 0)

#ifdef __cplusplus
}
#endif


#endif

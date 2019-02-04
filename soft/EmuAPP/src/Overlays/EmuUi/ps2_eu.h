#ifndef PS2_EU_H
#define PS2_EU_H

#include <stdint.h>

void          ps2_periodic (void);
uint_fast16_t ps2_read     (void);
void          ps2_leds     (uint_fast8_t leds_status);

#endif

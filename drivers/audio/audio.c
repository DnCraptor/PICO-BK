/**
 * MIT License
 *
 * Copyright (c) 2022 Vincent Mistler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef PWM_PIN0
#define PWM_PIN0 (AUDIO_PWM_PIN&0xfe)
#endif
#ifndef PWM_PIN1
#define PWM_PIN1 (PWM_PIN0+1)
#endif

#include <hardware/pwm.h>
#include <hardware/clocks.h>

#include "audio.h"

/**
 * Initialize the I2S driver. Must be called before calling i2s_write or i2s_dma_write
 * i2s_config: I2S context obtained by i2s_get_default_config()
 */
void i2s_init(i2s_config_t *i2s_config) {
    if (is_i2s_enabled) {
        uint8_t func = GPIO_FUNC_PIO1;    // TODO: GPIO_FUNC_PIO0 for pio0 or GPIO_FUNC_PIO1 for pio1
        gpio_set_function(i2s_config->data_pin, func);
        gpio_set_function(i2s_config->bck_pin, func);
        gpio_set_function(i2s_config->lck_pin, func);
        
        i2s_config->sm = pio_claim_unused_sm(i2s_config->pio, true);

        /* Set PIO clock */
        uint32_t system_clock_frequency = clock_get_hz(clk_sys);
        uint32_t divider = system_clock_frequency * 4 / i2s_config->sample_freq; // avoid arithmetic overflow

        #ifdef I2S_CS4334
        i2s_config->program_offset = pio_add_program(i2s_config->pio, &audio_i2s_cs4334_program);
        audio_i2s_cs4334_program_init(i2s_config->pio, i2s_config->sm , offset, i2s_config->data_pin , i2s_config->bck_pin);
        divider >>= 3;
        #else
        i2s_config->program_offset = pio_add_program(i2s_config->pio, &audio_i2s_program);
        audio_i2s_program_init(i2s_config->pio, i2s_config->sm , i2s_config->program_offset, i2s_config->data_pin , i2s_config->bck_pin);
        #endif

        pio_sm_set_clkdiv_int_frac(i2s_config->pio, i2s_config->sm , divider >> 8u, divider & 0xffu);

        pio_sm_set_enabled(i2s_config->pio, i2s_config->sm, false);
    }

    pio_sm_set_enabled(i2s_config->pio, i2s_config->sm , true);
}

void i2s_deinit(i2s_config_t *i2s_config) {
    pio_sm_set_enabled(i2s_config->pio, i2s_config->sm, false);
    pio_remove_program(i2s_config->pio, &audio_i2s_program, i2s_config->program_offset);
    pio_sm_unclaim(i2s_config->pio, i2s_config->sm);
}

/**
 * Write samples to I2S directly and wait for completion (blocking)
 * i2s_config: I2S context obtained by i2s_get_default_config()
 *     sample: pointer to an array of len x 32 bits samples
 *             Each 32 bits sample contains 2x16 bits samples, 
 *             one for the left channel and one for the right channel
 *        len: length of sample in 32 bits words
 */
void i2s_write(const i2s_config_t *i2s_config, const int16_t *samples, const size_t len) {
    for (register size_t i = 0; i < len; ++i) {
        register uint32_t t = *samples++; // (uint32_t)(samples[i]);
        register uint32_t v = (t << 16) | *samples++; // v
        pio_sm_put_blocking(i2s_config->pio, i2s_config->sm, v);
    }
}

/**
 * Adjust the output volume
 * i2s_config: I2S context obtained by i2s_get_default_config()
 *     volume: desired volume between 0 (highest. volume) and 16 (lowest volume)
 */
void i2s_volume(i2s_config_t *i2s_config,uint8_t volume) {
    if(volume>16) volume=16;
    i2s_config->volume=volume;
}

/**
 * Increases the output volume
 */
void i2s_increase_volume(i2s_config_t *i2s_config) {
    if(i2s_config->volume>0) {
        i2s_config->volume--;
    }
}

/**
 * Decreases the output volume
 */
void i2s_decrease_volume(i2s_config_t *i2s_config) {
    if(i2s_config->volume<16) {
        i2s_config->volume++;
    }
}

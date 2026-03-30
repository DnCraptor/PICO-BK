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

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include "audio_i2s.pio.h"

typedef struct i2s_config 
{
    uint32_t sample_freq;        
    uint16_t channel_count; 
    uint8_t  data_pin;
    uint8_t  clock_pin_base;
    PIO	     pio;
    uint8_t  sm; 
    uint8_t  dma_channel;
    uint16_t dma_trans_count;
    uint32_t *dma_buf;      /* buffer holds dma_trans_count x uint32 stereo frames */
    uint8_t volume;
} i2s_config_t;


i2s_config_t i2s_get_default_config(void);
void i2s_init(i2s_config_t *i2s_config);
void i2s_write(const i2s_config_t *i2s_config,const int16_t *samples,const size_t len);
void i2s_dma_write(i2s_config_t *i2s_config,const int16_t *samples);
void i2s_volume(i2s_config_t *i2s_config,uint8_t volume);
void i2s_increase_volume(i2s_config_t *i2s_config);
void i2s_decrease_volume(i2s_config_t *i2s_config);

void audio_init(void);

/**
 * Submit one stereo frame to the I2S DAC via DMA.
 * Only available when I2S is defined. Safe to call from a repeating timer ISR
 * because i2s_dma_write() waits for the previous transfer before starting the next.
 */
#ifdef I2S
void audio_i2s_submit(int16_t l, int16_t r);
void i2s_recalc_clkdiv(void);
#endif

#ifdef __cplusplus
}
#endif

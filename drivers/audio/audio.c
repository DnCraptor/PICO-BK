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


#include "audio.h"
#include "debug.h"

#include <pico.h>
#include <hardware/pwm.h>
#include <hardware/clocks.h>

#ifdef HWAY
    #include "PinSerialData_595.h"
#endif

void PWM_init_pin(uint8_t pinN, uint16_t max_lvl) {
    static pwm_config config = { 0, 0, 0};
    if (config.csr == 0 && config.div == 0 && config.top == 0) {
        config = pwm_get_default_config();
    }
    gpio_set_function(pinN, GPIO_FUNC_PWM);
    pwm_config_set_clkdiv(&config, 1.0);
    pwm_config_set_wrap(&config, max_lvl); // MAX PWM value
    pwm_init(pwm_gpio_to_slice_num(pinN), &config, true);
}

/* Global I2S context — valid only when I2S is defined */
#ifdef I2S
static i2s_config_t g_i2s_config;
/* Double-buffer: one pair of int16 samples = one stereo frame = one uint32 word */
static int16_t g_i2s_samples[2] = { 0, 0 };
#endif

void audio_init(void) {
#ifdef HWAY
    Init_PWM_175(TSPIN_MODE_BOTH);
#elif defined(I2S)
    g_i2s_config = i2s_get_default_config();
    g_i2s_config.data_pin      = I2S_DATA_PIO;
    g_i2s_config.clock_pin_base = I2S_BCK_PIO;   /* BCLK=I2S_BCK_PIO, LRCLK=I2S_BCK_PIO+1 */
    g_i2s_config.sample_freq   = 44100;
    g_i2s_config.dma_trans_count = 1;             /* 1 x uint32 = one stereo frame */
    i2s_volume(&g_i2s_config, 0);
    i2s_init(&g_i2s_config);
#else
    PWM_init_pin(BEEPER_PIN, (1 << 12) - 1);
    #ifdef SOUND_SYSTEM
    PWM_init_pin(PWM_PIN0, (1 << 12) - 1);
    PWM_init_pin(PWM_PIN1, (1 << 12) - 1);
    #endif
#endif
}

/**
 * Submit one stereo audio frame to the I2S DAC via DMA (non-blocking).
 * Called from the 44100 Hz timer callback on Core 0.
 * Only compiled when I2S is defined.
 *
 * @param l  Left  channel sample, signed 16-bit
 * @param r  Right channel sample, signed 16-bit
 */
#ifdef I2S
void i2s_recalc_clkdiv(void) {
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    uint32_t divider = system_clock_frequency * 4 / g_i2s_config.sample_freq;
    pio_sm_set_clkdiv_int_frac(g_i2s_config.pio, g_i2s_config.sm,
                               divider >> 8u, divider & 0xffu);
}

void __not_in_flash_func(audio_i2s_submit)(int16_t l, int16_t r) {
    /* PIO word layout: bits[31:16] = WS=0 (right), bits[15:0] = WS=1 (left) */
    uint32_t frame = ((uint32_t)(uint16_t)r << 16) | (uint16_t)l;
    /* Non-blocking: drop the sample if FIFO is full rather than stalling the ISR */
    if (!pio_sm_is_tx_fifo_full(g_i2s_config.pio, g_i2s_config.sm)) {
        pio_sm_put(g_i2s_config.pio, g_i2s_config.sm, frame);
    }
}
#endif

/**
 * return the default i2s context used to store information about the setup
 */
i2s_config_t i2s_get_default_config(void) {
    i2s_config_t i2s_config = {
		.sample_freq = 44100, 
		.channel_count = 2,
		.data_pin = 26,
		.clock_pin_base = 27,
		.pio = pio1,
		.sm = 0,
        .dma_channel = 0,
        .dma_buf = NULL,
        .dma_trans_count = 0,
        .volume = 0,
	};

    return i2s_config;
}

/**
 * Initialize the I2S driver. Must be called before calling i2s_write or i2s_dma_write
 * i2s_config: I2S context obtained by i2s_get_default_config()
 */


void i2s_init(i2s_config_t *i2s_config) {

#ifndef AUDIO_PWM_PIN
    /* Determine GPIO_FUNC for the chosen PIO instance */
    uint8_t func;
    if      (i2s_config->pio == pio0) func = GPIO_FUNC_PIO0;
    else                              func = GPIO_FUNC_PIO1;

    gpio_set_function(i2s_config->data_pin,          func);
    gpio_set_function(i2s_config->clock_pin_base,     func);
    gpio_set_function(i2s_config->clock_pin_base + 1, func);

    i2s_config->sm = pio_claim_unused_sm(i2s_config->pio, true);

    uint offset = pio_add_program(i2s_config->pio, &audio_i2s_program);
    audio_i2s_program_init(i2s_config->pio, i2s_config->sm, offset,
                           i2s_config->data_pin, i2s_config->clock_pin_base);

    /* Set PIO clock divider for the target sample rate */
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    uint32_t divider = system_clock_frequency * 4 / i2s_config->sample_freq;
    pio_sm_set_clkdiv_int_frac(i2s_config->pio, i2s_config->sm,
                               divider >> 8u, divider & 0xffu);

    pio_sm_set_enabled(i2s_config->pio, i2s_config->sm, false);
#endif

    /* Allocate DMA buffer: dma_trans_count uint32 words */
    i2s_config->dma_buf = (uint32_t*)malloc(i2s_config->dma_trans_count * sizeof(uint32_t));

    /* DMA setup */
    i2s_config->dma_channel = dma_claim_unused_channel(true);

    dma_channel_config dma_config = dma_channel_get_default_config(i2s_config->dma_channel);
    channel_config_set_read_increment(&dma_config, true);
    channel_config_set_write_increment(&dma_config, false);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);

    uint32_t *addr_write_DMA = &(i2s_config->pio->txf[i2s_config->sm]);

#ifdef AUDIO_PWM_PIN
    gpio_set_function(PWM_PIN0, GPIO_FUNC_PWM);
    gpio_set_function(PWM_PIN1, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN0);

    pwm_config c_pwm = pwm_get_default_config();
    pwm_config_set_clkdiv(&c_pwm, 1.0);
    pwm_config_set_wrap(&c_pwm, clock_get_hz(clk_sys) / (i2s_config->sample_freq));
    pwm_init(slice_num, &c_pwm, true);

    channel_config_set_dreq(&dma_config, pwm_get_dreq(slice_num));
    addr_write_DMA = (uint32_t*)&pwm_hw->slice[slice_num].cc;
#else
    channel_config_set_dreq(&dma_config, pio_get_dreq(i2s_config->pio, i2s_config->sm, true));
#endif

    dma_channel_configure(i2s_config->dma_channel,
                          &dma_config,
                          addr_write_DMA,           /* Destination */
                          i2s_config->dma_buf,      /* Source      */
                          i2s_config->dma_trans_count,
                          false);

    pio_sm_set_enabled(i2s_config->pio, i2s_config->sm, true);
}

/**
 * Write samples to I2S directly and wait for completion (blocking)
 * i2s_config: I2S context obtained by i2s_get_default_config()
 *     sample: pointer to an array of len x 32 bits samples
 *             Each 32 bits sample contains 2x16 bits samples, 
 *             one for the left channel and one for the right channel
 *        len: length of sample in 32 bits words
 */
void __not_in_flash_func(i2s_write)(const i2s_config_t *i2s_config,const int16_t *samples,const size_t len) {
    for(size_t i=0;i<len;i++) {
        pio_sm_put_blocking(i2s_config->pio, i2s_config->sm, (uint32_t)samples[i]);
    }
}

/**
 * Write samples to DMA buffer and initiate DMA transfer (non blocking)
 * i2s_config: I2S context obtained by i2s_get_default_config()
 *     sample: pointer to an array of dma_trans_count x 32 bits samples
 */
void __not_in_flash_func(i2s_dma_write)(i2s_config_t *i2s_config, const int16_t *samples) {
    /* Wait for the previous DMA transfer to complete */
    dma_channel_wait_for_finish_blocking(i2s_config->dma_channel);

    /*
     * Pack samples into the uint32 DMA buffer.
     *
     * AUDIO_PWM_PIN path: DMA feeds PWM CC register directly.
     *   Buffer layout: each uint32 word holds two 16-bit PWM compare values
     *   (CH_A in low half, CH_B in high half).  Convert signed→unsigned and
     *   scale to the PWM wrap range.
     *
     * I2S PIO path: DMA feeds the PIO TX FIFO.
     *   Buffer layout: one uint32 per stereo frame.
     *   PIO program expects: bits[31:16] = WS=0 sample (right),
     *                        bits[15:0]  = WS=1 sample (left).
     *   samples[] arrives pre-packed as {right, left} from audio_i2s_submit().
     */
#ifdef AUDIO_PWM_PIN
    /* samples[] is pairs of int16: [r0, l0, r1, l1, ...], dma_trans_count words = 2 int16 each */
    uint16_t *buf16 = (uint16_t *)i2s_config->dma_buf;
    for (uint16_t i = 0; i < i2s_config->dma_trans_count * 2; i++) {
        buf16[i] = (uint16_t)((32768 + (int32_t)samples[i]) >> (5 + i2s_config->volume));
    }
#else
    /* Pure I2S PIO: treat the buffer as int16 pairs packed into uint32 words */
    int16_t *buf16 = (int16_t *)i2s_config->dma_buf;
    const uint16_t n = i2s_config->dma_trans_count * 2; /* 2 int16 per uint32 word */
    if (i2s_config->volume == 0) {
        memcpy(buf16, samples, n * sizeof(int16_t));
    } else {
        for (uint16_t i = 0; i < n; i++) {
            buf16[i] = samples[i] >> i2s_config->volume;
        }
    }
#endif

    /* Kick off the DMA transfer */
    dma_channel_transfer_from_buffer_now(i2s_config->dma_channel,
                                         i2s_config->dma_buf,
                                         i2s_config->dma_trans_count);
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

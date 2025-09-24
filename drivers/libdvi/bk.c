#include "dvi.h"
#include "dvi_timing.h"
#include "common_dvi_pin_configs.h"
#include "tmds_encode.h"

#include <pico/sem.h>
#include <hardware/clocks.h>
#if !PICO_RP2040
    #include <hardware/structs/qmi.h>
#endif
#include <hardware/structs/bus_ctrl.h>

#include "config_em.h"
#include "emulator.h"
#include "vga.h"

#define DVI_TIMING dvi_timing_800x600p_60hz
#define DVI_TIMING2 dvi_timing_1024x768p_60hz_custom

#define MAX_FRAME_WIDTH 1024

uint32_t FRAME_WIDTH = 800;
uint32_t FRAME_HEIGHT = 300;
extern uint8_t DVI_VERTICAL_REPEAT;

#define DWORDS_PER_PLANE (FRAME_WIDTH / DVI_SYMBOLS_PER_WORD)
#define BYTES_PER_PLANE (DWORDS_PER_PLANE * 4)
#define BLACK 0x7fd00
uint32_t __aligned(4) blank[MAX_FRAME_WIDTH / DVI_SYMBOLS_PER_WORD * 3];
semaphore_t vga_start_semaphore;

struct dvi_inst dvi0;

extern uint64_t tmds_2bpp_table_bk_b[16];   // только 01 - пиксель
extern uint64_t tmds_2bpp_table_bk_g[16];   // только 10 - пиксель
extern uint64_t tmds_2bpp_table_bk_r[16];   // только 11 - пиксель
extern uint64_t tmds_2bpp_table_bk_any[16]; // пиксель 01, 10 и 11, 00 - нет пикселя
extern uint64_t tmds_2bpp_table_bk_n11[16]; // пиксель только 01 и 10, 00 и 11 - нет пикселя
extern uint64_t tmds_2bpp_table_bk_n10[16]; // пиксель только 01 и 11, 00 и 10 - нет пикселя
extern uint64_t tmds_2bpp_table_bk_n01[16]; // пиксель только 10 и 11, 00 и 01 - нет пикселя
extern uint64_t tmds_2bpp_table_bk_noc[16]; // нет пикселя во всех кейсах
extern uint64_t tmds_2bpp_table_bk_2br[16]; // вариант с "честной" двухбиткой
extern uint64_t tmds_2bpp_table_bk_2bb[16]; // вариант с "честной" двухбиткой: 00 - чёрный, но 01 - светлее, чем 10, а 11 - самый светлый

typedef struct tmds_2bpp_tables_bk_s {
    uint64_t* b;
    uint64_t* g;
    uint64_t* r;
} tmds_2bpp_tables_bk_t;

const static tmds_2bpp_tables_bk_t tmds_2bpp_tables_bk[16] = {
    { tmds_2bpp_table_bk_b  , tmds_2bpp_table_bk_g  , tmds_2bpp_table_bk_r   }, //  0 чёрный-синий-зелёный-красный
    { tmds_2bpp_table_bk_g  , tmds_2bpp_table_bk_b  , tmds_2bpp_table_bk_any }, //  1 чёрный-жёлтый-пурпур-красный
    { tmds_2bpp_table_bk_b  , tmds_2bpp_table_bk_n11, tmds_2bpp_table_bk_r   }, //  2 чёрный-циан-синий-пурпур
    { tmds_2bpp_table_bk_b  , tmds_2bpp_table_bk_any, tmds_2bpp_table_bk_r   }, //  3 чёрный-зелёный-циан-жёлтый
    { tmds_2bpp_table_bk_any, tmds_2bpp_table_bk_n01, tmds_2bpp_table_bk_n10 }, //  4 чёрный-пурпур-циан-белый
    { tmds_2bpp_table_bk_any, tmds_2bpp_table_bk_any, tmds_2bpp_table_bk_any }, //  5 чёрный-белый-белый-белый
    { tmds_2bpp_table_bk_noc, tmds_2bpp_table_bk_noc, tmds_2bpp_table_bk_2bb }, //  6 чёрный-кирпич-тёмный_кирпич-красный
    { tmds_2bpp_table_bk_noc, tmds_2bpp_table_bk_2bb, tmds_2bpp_table_bk_2bb }, //  7
    { tmds_2bpp_table_bk_2bb, tmds_2bpp_table_bk_noc, tmds_2bpp_table_bk_2bb }, //  8
    { tmds_2bpp_table_bk_g  , tmds_2bpp_table_bk_b  , tmds_2bpp_table_bk_n01 }, //  9 - todo
    { tmds_2bpp_table_bk_g  , tmds_2bpp_table_bk_2br, tmds_2bpp_table_bk_n01 }, // 10 - todo
    { tmds_2bpp_table_bk_b  , tmds_2bpp_table_bk_g  , tmds_2bpp_table_bk_n01 }, // 11 чёрный-cyan-yellow-red
    { tmds_2bpp_table_bk_r  , tmds_2bpp_table_bk_n01, tmds_2bpp_table_bk_b   }, // 12 чёрный-red-green-cyan
    { tmds_2bpp_table_bk_n10, tmds_2bpp_table_bk_any, tmds_2bpp_table_bk_n01 }, // 13 чёрный-cyan-yellow-white
    { tmds_2bpp_table_bk_b  , tmds_2bpp_table_bk_any, tmds_2bpp_table_bk_n10 }, // 14 чёрный-yellow-green-white
    { tmds_2bpp_table_bk_n10, tmds_2bpp_table_bk_any, tmds_2bpp_table_bk_r   }, // 15 чёрный-cyan-green-white
};

extern uint32_t tmds_2bpp_table_bk_1024_b[4];   // только 01 - пиксель
extern uint32_t tmds_2bpp_table_bk_1024_g[4];   // только 10 - пиксель
extern uint32_t tmds_2bpp_table_bk_1024_r[4];   // только 11 - пиксель
extern uint32_t tmds_2bpp_table_bk_1024_any[4]; // пиксель 01, 10 и 11, 00 - нет пикселя
extern uint32_t tmds_2bpp_table_bk_1024_n11[4]; // пиксель только 01 и 10, 00 и 11 - нет пикселя
extern uint32_t tmds_2bpp_table_bk_1024_n10[4]; // пиксель только 01 и 11, 00 и 10 - нет пикселя
extern uint32_t tmds_2bpp_table_bk_1024_n01[4]; // пиксель только 10 и 11, 00 и 01 - нет пикселя
extern uint32_t tmds_2bpp_table_bk_1024_noc[4]; // нет пикселя во всех кейсах
extern uint32_t tmds_2bpp_table_bk_1024_2br[4]; // вариант с "честной" двухбиткой
extern uint32_t tmds_2bpp_table_bk_1024_2bb[4]; // вариант с "честной" двухбиткой: 00 - чёрный, но 01 - светлее, чем 10, а 11 - самый светлый

typedef struct tmds_2bpp_tables_bk_1024_s {
    uint32_t* b;
    uint32_t* g;
    uint32_t* r;
} tmds_2bpp_tables_bk_1024_t;

const static tmds_2bpp_tables_bk_1024_t tmds_2bpp_tables_1024_bk[16] = {
    { tmds_2bpp_table_bk_1024_b  , tmds_2bpp_table_bk_1024_g  , tmds_2bpp_table_bk_1024_r   }, //  0 чёрный-синий-зелёный-красный
    { tmds_2bpp_table_bk_1024_g  , tmds_2bpp_table_bk_1024_b  , tmds_2bpp_table_bk_1024_any }, //  1 чёрный-жёлтый-пурпур-красный
    { tmds_2bpp_table_bk_1024_b  , tmds_2bpp_table_bk_1024_n11, tmds_2bpp_table_bk_1024_r   }, //  2 чёрный-циан-синий-пурпур
    { tmds_2bpp_table_bk_1024_b  , tmds_2bpp_table_bk_1024_any, tmds_2bpp_table_bk_1024_r   }, //  3 чёрный-зелёный-циан-жёлтый
    { tmds_2bpp_table_bk_1024_any, tmds_2bpp_table_bk_1024_n01, tmds_2bpp_table_bk_1024_n10 }, //  4 чёрный-пурпур-циан-белый
    { tmds_2bpp_table_bk_1024_any, tmds_2bpp_table_bk_1024_any, tmds_2bpp_table_bk_1024_any }, //  5 чёрный-белый-белый-белый
    { tmds_2bpp_table_bk_1024_noc, tmds_2bpp_table_bk_1024_noc, tmds_2bpp_table_bk_1024_2bb }, //  6 чёрный-кирпич-тёмный_кирпич-красный
    { tmds_2bpp_table_bk_1024_noc, tmds_2bpp_table_bk_1024_2bb, tmds_2bpp_table_bk_1024_2bb }, //  7
    { tmds_2bpp_table_bk_1024_2bb, tmds_2bpp_table_bk_1024_noc, tmds_2bpp_table_bk_1024_2bb }, //  8
    { tmds_2bpp_table_bk_1024_g  , tmds_2bpp_table_bk_1024_b  , tmds_2bpp_table_bk_1024_n01 }, //  9 - todo
    { tmds_2bpp_table_bk_1024_g  , tmds_2bpp_table_bk_1024_2br, tmds_2bpp_table_bk_1024_n01 }, // 10 - todo
    { tmds_2bpp_table_bk_1024_b  , tmds_2bpp_table_bk_1024_g  , tmds_2bpp_table_bk_1024_n01 }, // 11 чёрный-cyan-yellow-red
    { tmds_2bpp_table_bk_1024_r  , tmds_2bpp_table_bk_1024_n01, tmds_2bpp_table_bk_1024_b   }, // 12 чёрный-red-green-cyan
    { tmds_2bpp_table_bk_1024_n10, tmds_2bpp_table_bk_1024_any, tmds_2bpp_table_bk_1024_n01 }, // 13 чёрный-cyan-yellow-white
    { tmds_2bpp_table_bk_1024_b  , tmds_2bpp_table_bk_1024_any, tmds_2bpp_table_bk_1024_n10 }, // 14 чёрный-yellow-green-white
    { tmds_2bpp_table_bk_1024_n10, tmds_2bpp_table_bk_1024_any, tmds_2bpp_table_bk_1024_r   }, // 15 чёрный-cyan-green-white
};

#define tmds_encode_2bpp_bk_800_b(t) tmds_encode_2bpp_bk_800(bk_page, tmdsbuf, t)
#define tmds_encode_2bpp_bk_800_g(t) tmds_encode_2bpp_bk_800(bk_page, tmdsbuf + DWORDS_PER_PLANE, t)
#define tmds_encode_2bpp_bk_800_r(t) tmds_encode_2bpp_bk_800(bk_page, tmdsbuf + 2 * DWORDS_PER_PLANE, t)
#define tmds_encode_2bpp_bk_1024_b(t) tmds_encode_2bpp_bk_1024(bk_page, tmdsbuf, t)
#define tmds_encode_2bpp_bk_1024_g(t) tmds_encode_2bpp_bk_1024(bk_page, tmdsbuf + DWORDS_PER_PLANE, t)
#define tmds_encode_2bpp_bk_1024_r(t) tmds_encode_2bpp_bk_1024(bk_page, tmdsbuf + 2 * DWORDS_PER_PLANE, t)

struct dvi_inst dvi0;

void __not_in_flash() flash_timings() {
#if !PICO_RP2040
	const int max_flash_freq = 88 * 1000000;
	const int clock_hz = DVI_TIMING.bit_clk_khz * 1000;
	int divisor = (clock_hz + max_flash_freq - 1) / max_flash_freq;
	if (divisor == 1 && clock_hz > 100000000) {
		divisor = 2;
	}
	int rxdelay = divisor;
	if (clock_hz / divisor > 100000000) {
		rxdelay += 1;
	}
	qmi_hw->m[0].timing = 0x60007000 |
						rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
						divisor << QMI_M0_TIMING_CLKDIV_LSB;
#endif
    sleep_ms(100);
	// Run system at TMDS bit clock
#if !PICO_RP2040
	set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);
	hw_set_bits(&bus_ctrl_hw->priority, BUSCTRL_BUS_PRIORITY_PROC1_BITS);
#else /// TODO: really?
	set_sys_clock_khz(366000, true);
#endif
}

static void __not_in_flash() flash_timings2() {
#if !PICO_RP2040
	const int max_flash_freq = 88 * 1000000;
	const int clock_hz = dvi0.timing->bit_clk_khz * 1000;
	int divisor = (clock_hz + max_flash_freq - 1) / max_flash_freq;
	if (divisor == 1 && clock_hz > 100000000) {
		divisor = 2;
	}
	int rxdelay = divisor;
	if (clock_hz / divisor > 100000000) {
		rxdelay += 1;
	}
	qmi_hw->m[0].timing = 0x60007000 |
						rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
						divisor << QMI_M0_TIMING_CLKDIV_LSB;
#endif
    sleep_ms(100);
	set_sys_clock_khz(dvi0.timing->bit_clk_khz, true);
}

static void __not_in_flash() dvi_init_bk() {
    if (g_conf.is_DVI_1024) {
        FRAME_WIDTH = 1024;
        FRAME_HEIGHT = (768 / 3);
        dvi0.timing = &DVI_TIMING2;
        DVI_VERTICAL_REPEAT = 3;
        flash_timings2();
    } else {
        FRAME_WIDTH = 800;
        FRAME_HEIGHT = 300;
        DVI_VERTICAL_REPEAT = 2;
        dvi0.timing = &DVI_TIMING;
    }
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
}

void __not_in_flash() dvi_on_core1() {
    dvi_init_bk();
	for (int i = 0; i < sizeof(blank) / sizeof(blank[0]); ++i) {
		blank[i] = BLACK;
	}
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_1);
    dvi_start(&dvi0);
    uint32_t *tmdsbuf = 0;
    sem_acquire_blocking(&vga_start_semaphore);
    while (true) {
        register enum graphics_mode_t gmode = get_graphics_mode();
        switch(gmode) {
            case TEXTMODE_: {
                register uint8_t* bk_text = (uint8_t*)TEXT_VIDEO_RAM;
                register uint32_t bytes_per_string = (dvi0.timing->h_active_pixels >> 3) << 1; // ширина символа - 8 пикселей, 2 байта на символ
                if (dvi0.timing->h_active_pixels >= 1024) {
                    for (uint y = 0; y < FRAME_HEIGHT; ++y) {
                        register uint32_t glyph_line = y & font_mask;
                        register uint8_t* bk_line = bk_text + (y >> font_shift) * bytes_per_string;
                        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
                        tmds_encode_64c_b_128(bk_line, tmdsbuf, glyph_line);
                        tmds_encode_64c_g_128(bk_line, tmdsbuf + DWORDS_PER_PLANE, glyph_line);
                        tmds_encode_64c_r_128(bk_line, tmdsbuf + DWORDS_PER_PLANE * 2, glyph_line);
                        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
                    }
                } else {
                    for (uint y = 0; y < FRAME_HEIGHT; ++y) {
                        register uint32_t glyph_line = y & font_mask;
                        register uint8_t* bk_line = bk_text + (y >> font_shift) * bytes_per_string;
                        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
                        tmds_encode_64c_b_100(bk_line, tmdsbuf, glyph_line);
                        tmds_encode_64c_g_100(bk_line, tmdsbuf + DWORDS_PER_PLANE, glyph_line);
                        tmds_encode_64c_r_100(bk_line, tmdsbuf + DWORDS_PER_PLANE * 2, glyph_line);
                        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
                    }
                }
                break;
            }
            case BK_256x256x2: {
                if (dvi0.timing->h_active_pixels >= 1024) {
                    const tmds_2bpp_tables_bk_1024_t* t = &tmds_2bpp_tables_1024_bk[g_conf.graphics_pallette_idx & 15];
                    register uint32_t* b = t->b;
                    register uint32_t* g = t->g;
                    register uint32_t* r = t->r;
                    for (uint y = 0; y < g_conf.graphics_buffer_height; ++y) {
                        register uint32_t* bk_page = (uint32_t*)get_graphics_buffer(y);
                        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
                        tmds_encode_2bpp_bk_1024_b(b);
                        tmds_encode_2bpp_bk_1024_g(g);
                        tmds_encode_2bpp_bk_1024_r(r);
                        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
                    }
                } else {
                    uint total_y = 0;
                    for (uint y = 0; y < (FRAME_HEIGHT - 256) / 2; ++y, ++total_y) {
                        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
                        memcpy(tmdsbuf, blank, sizeof(blank));
                        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
                    }
                    const tmds_2bpp_tables_bk_t* t = &tmds_2bpp_tables_bk[g_conf.graphics_pallette_idx & 15];
                    register uint64_t* b = t->b;
                    register uint64_t* g = t->g;
                    register uint64_t* r = t->r;
                    for (uint y = 0; y < g_conf.graphics_buffer_height; ++y, ++total_y) {
                        register uint32_t* bk_page = (uint32_t*)get_graphics_buffer(y);
                        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
                        tmds_encode_2bpp_bk_800_b(b);
                        tmds_encode_2bpp_bk_800_g(g);
                        tmds_encode_2bpp_bk_800_r(r);
                        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
                    }
                    for (; total_y < FRAME_HEIGHT; ++total_y) {
                        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
                        memcpy(tmdsbuf, blank, sizeof(blank));
                        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
                    }
                }
                *vsync_ptr = 1;
                break;
            }
            default: {
                if (FRAME_HEIGHT > 300) {
                    for (uint y = 0; y < 768; ++y) { // TODO:
                        register uint32_t* bk_page = (uint32_t*)get_graphics_buffer(y / 3);
                        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
                        tmds_encode_1bpp_bk_1024(bk_page, tmdsbuf, FRAME_WIDTH);
                        memcpy(tmdsbuf + DWORDS_PER_PLANE, tmdsbuf, BYTES_PER_PLANE);
                        memcpy(tmdsbuf + 2 * DWORDS_PER_PLANE, tmdsbuf, BYTES_PER_PLANE);
                        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
                    }
                } else {
                    uint total_y = 0;
                    for (uint y = 0; y < (FRAME_HEIGHT - 256) / 2; ++y, ++total_y) {
                        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
                        memcpy(tmdsbuf, blank, sizeof(blank));
                        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
                    }
                    for (uint y = 0; y < g_conf.graphics_buffer_height; ++y, ++total_y) {
                        register uint32_t* bk_page = (uint32_t*)get_graphics_buffer(y);
                        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
                        if (dvi0.timing->h_active_pixels < 1024) {
                            tmds_encode_1bpp_bk_800(bk_page, tmdsbuf, FRAME_WIDTH);
                        } else {
                            tmds_encode_1bpp_bk_1024(bk_page, tmdsbuf, FRAME_WIDTH);
                        }
                        memcpy(tmdsbuf + DWORDS_PER_PLANE, tmdsbuf, BYTES_PER_PLANE);
                        memcpy(tmdsbuf + 2 * DWORDS_PER_PLANE, tmdsbuf, BYTES_PER_PLANE);
                        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
                    }
                    for (; total_y < FRAME_HEIGHT; ++total_y) {
                        queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
                        memcpy(tmdsbuf, blank, sizeof(blank));
                        queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
                    }
                }
                *vsync_ptr = 1;
                break;
            }
        }
    }
}

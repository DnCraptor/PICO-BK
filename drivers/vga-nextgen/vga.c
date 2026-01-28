#include "vga.h"
#include "hardware/clocks.h"
#include "stdbool.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/systick.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include <string.h>
#include <stdio.h>
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include "fnt8x8.h"
#include "fnt8x16.h"
#include "pico-vision.h"
#include "config_em.h"
#include "ram_page.h"

int pallete_mask = 3; // 11 - 2 bits
uint8_t volatile vsync;
uint8_t* vsync_ptr = &vsync;
uint8_t* font = font_8x16;
uint8_t font_height = 16;
uint8_t font_shift = 4;
uint32_t font_mask = 15;
#if PICO_RP2040
extern uint8_t TEXT_VIDEO_RAM[128*48*2];
#else
extern uint8_t TEXT_VIDEO_RAM[128*96*2];
#endif

uint16_t pio_program_VGA_instructions[] = {
    //     .wrap_target
    0x6008, //  0: out    pins, 8
    //     .wrap
};

const struct pio_program pio_program_VGA = {
    .instructions = pio_program_VGA_instructions,
    .length = 1,
    .origin = -1,
};


static uint32_t* lines_pattern[4];
static uint32_t* lines_pattern_data = NULL;
static int _SM_VGA = -1;


static int N_lines_total = 525;
static int N_lines_visible = 480;
static int line_VS_begin = 490;
static int line_VS_end = 491;
static int shift_picture = 0;

static int begin_line_index = 0;
static int visible_line_size = 320;


static int dma_chan_ctrl;
static int dma_chan;

static volatile uint8_t* graphics_buffer;
uint8_t* __not_in_flash() get_graphics_buffer(int y) {
    int addr_in_buf = 64 * (y + g_conf.shift_y - 0330);
    while (addr_in_buf < 0) addr_in_buf += 16 << 10;
    while (addr_in_buf >= 16 << 10) addr_in_buf -= 16 << 10;
    return graphics_buffer + addr_in_buf;
}
static uint graphics_buffer_width = 0;
static int graphics_buffer_shift_x = 0;
static int graphics_buffer_shift_y = 0;

static bool is_flash_line = false;
static bool is_flash_frame = false;

//буфер 1к графической палитры
static uint16_t __scratch_y("vga_driver") palette[16*4];

static uint32_t bg_color[2];
static uint16_t palette16_mask = 0;

static uint8_t* text_buf_color;

uint text_buffer_width = 0;
uint text_buffer_height = 0;
void set_text_mode_size(uint w, uint h) {
    text_buffer_width = w;
    text_buffer_height = h;
}

static uint16_t __scratch_y("vga_driver") txt_palette[16];

//буфер 2К текстовой палитры для быстрой работы
static uint16_t* txt_palette_fast = NULL;
//static uint16_t txt_palette_fast[256*4];

volatile enum graphics_mode_t graphics_mode;
enum graphics_mode_t __not_in_flash() get_graphics_mode() { return graphics_mode; }

// TODO: separate header for sound mixer

// регистр "защёлка" для примитивного ковокса без буфера
volatile uint16_t true_covox = 0;
volatile uint16_t az_covox_L = 0;
volatile uint16_t az_covox_R = 0;
volatile uint16_t covox_mix = 0x0F;

void graphics_inc_palleter_offset() {
    g_conf.graphics_pallette_idx++;
    if (g_conf.graphics_pallette_idx > 0b1111) g_conf.graphics_pallette_idx = 0;
}


inline static void dma_handler_VGA_impl() {
    dma_hw->ints0 = 1u << dma_chan_ctrl;
    static uint32_t frame_number = 0;
    static uint32_t screen_line = 0;
    static uint8_t* input_buffer = NULL;
    static uint32_t* * prev_output_buffer = 0;
    static uint32_t screen_lines = 0;
    static const uint32_t d_lines = 806 * 60 / 50; // 967

    if (screen_lines == d_lines) {
        screen_lines = 0;
        *vsync_ptr = 1;
    }

    if (screen_line == N_lines_total) {
        screen_line = 0;
        frame_number++;
        input_buffer = graphics_buffer;
    }

    if (screen_line >= N_lines_visible) {
        //заполнение цветом фона
        if ((screen_line == N_lines_visible) | (screen_line == (N_lines_visible + 3))) {
            uint32_t* output_buffer_32bit = lines_pattern[2 + ((screen_line) & 1)];
            output_buffer_32bit += shift_picture / 4;
            uint32_t p_i = ((screen_line & is_flash_line) + (frame_number & is_flash_frame)) & 1;
            uint32_t color32 = 0xC0C0C0C0 ; // bg_color[p_i];
            for (int i = visible_line_size / 2; i--;) {
                *output_buffer_32bit++ = color32;
            }
        }

        //синхросигналы
        if ((screen_line >= line_VS_begin) && (screen_line <= line_VS_end)){
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[1], false); //VS SYNC
        }
        else
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false);
        screen_line++;
        screen_lines++;
        return;
    }

    if (!(input_buffer)) {
        dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false);
        screen_line++;
        screen_lines++;
        return;
    } //если нет видеобуфера - рисуем пустую строку

    int y, line_number;

    uint32_t* * output_buffer = &lines_pattern[2 + (screen_line & 1)];
    uint div_factor = 2;
    switch (graphics_mode) {
        case BK_256x256x2:
        case BK_512x256x1:
            if (screen_line % 3 != 0) { // три подряд строки рисуем одно и тоже
                if (prev_output_buffer) output_buffer = prev_output_buffer;
                dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
                screen_line++;
                screen_lines++;
                return;
            }
            prev_output_buffer = output_buffer;
            line_number = screen_line / 3;
            y = line_number - graphics_buffer_shift_y;
            break;
        case TEXTMODE_80x30: {
            register uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
            output_buffer_16bit += shift_picture / 2;
            // "слой" символа
            register uint32_t glyph_line = screen_line & font_mask;
            register uint8_t fs = font_shift;
            //указатель откуда начать считывать символы
            register uint8_t* text_buffer_line = &TEXT_VIDEO_RAM[(screen_line >> fs) * text_buffer_width * 2];
            for (int x = 0; x < text_buffer_width; x++) {
                //из таблицы символов получаем "срез" текущего символа
                register uint8_t glyph_pixels = font[(*text_buffer_line++ << fs) + glyph_line];
                //считываем из быстрой палитры начало таблицы быстрого преобразования 2-битных комбинаций цветов пикселей
                register uint16_t* color = &txt_palette_fast[4 * (*text_buffer_line++)];
                *output_buffer_16bit++ = color[glyph_pixels & 3];
                glyph_pixels >>= 2;
                *output_buffer_16bit++ = color[glyph_pixels & 3];
                glyph_pixels >>= 2;
                *output_buffer_16bit++ = color[glyph_pixels & 3];
                glyph_pixels >>= 2;
                *output_buffer_16bit++ = color[glyph_pixels & 3];
            }
            dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
            screen_line++;
            screen_lines++;
            return;
        }
        case TEXTMODE_:
        if (g_conf.is_128_48) {
            register uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
            output_buffer_16bit += shift_picture / 2;
            register uint8_t fs = font_shift;
            // "слой" символа
            register uint32_t glyph_line = screen_line & font_mask;
            //указатель откуда начать считывать символы
            register uint8_t* text_buffer_line = &TEXT_VIDEO_RAM[(screen_line >> fs) * text_buffer_width * 2];
            for (register int x = 0; x < text_buffer_width; x++) {
                //из таблицы символов получаем "срез" текущего символа
                register uint8_t glyph_pixels = font[(*text_buffer_line++ << fs) + glyph_line];
                //считываем из быстрой палитры начало таблицы быстрого преобразования 2-битных комбинаций цветов пикселей
                register uint16_t* color = &txt_palette_fast[4 * (*text_buffer_line++)];
                *output_buffer_16bit++ = color[glyph_pixels & 3];
                glyph_pixels >>= 2;
                *output_buffer_16bit++ = color[glyph_pixels & 3];
                glyph_pixels >>= 2;
                *output_buffer_16bit++ = color[glyph_pixels & 3];
                glyph_pixels >>= 2;
                *output_buffer_16bit++ = color[glyph_pixels & 3];
            }
            dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
            screen_line++;
            screen_lines++;
            return;
        } else {
            if (screen_line & 1) {
                dma_channel_set_read_addr(dma_chan_ctrl, prev_output_buffer, false);
                screen_line++;
                screen_lines++;
                return;
            }
            register uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
            output_buffer_16bit += shift_picture >> 1;
            register uint32_t y = screen_line >> 1;
            register uint8_t fs = font_shift;
            // "слой" символа
            register uint32_t glyph_line = y & font_mask;
            //указатель откуда начать считывать символы
            register uint8_t* text_buffer_line = &TEXT_VIDEO_RAM[(y >> fs) * text_buffer_width * 2];
            for (register int x = 0; x < text_buffer_width; x++) {
                //из таблицы символов получаем "срез" текущего символа
                register uint8_t glyph_pixels = font[(*text_buffer_line++ << fs) + glyph_line];
                //считываем из быстрой палитры начало таблицы быстрого преобразования 2-битных комбинаций цветов пикселей
                register uint16_t* color = &txt_palette_fast[4 * (*text_buffer_line++)];
                // преобразование полубайтов
                register uint32_t hi_pix = color[glyph_pixels & 3];
                register uint32_t lo_pix = hi_pix & 0xFF; hi_pix >>= 8;
                *output_buffer_16bit++ = (lo_pix << 8) | lo_pix;
                *output_buffer_16bit++ = (hi_pix << 8) | hi_pix;
                glyph_pixels >>= 2;
                hi_pix = color[glyph_pixels & 3];
                lo_pix = hi_pix & 0xFF; hi_pix >>= 8;
                *output_buffer_16bit++ = (lo_pix << 8) | lo_pix;
                *output_buffer_16bit++ = (hi_pix << 8) | hi_pix;
                glyph_pixels >>= 2;
                hi_pix = color[glyph_pixels & 3];
                lo_pix = hi_pix & 0xFF; hi_pix >>= 8;
                *output_buffer_16bit++ = (lo_pix << 8) | lo_pix;
                *output_buffer_16bit++ = (hi_pix << 8) | hi_pix;
                glyph_pixels >>= 2;
                hi_pix = color[glyph_pixels & 3];
                lo_pix = hi_pix & 0xFF; hi_pix >>= 8;
                *output_buffer_16bit++ = (lo_pix << 8) | lo_pix;
                *output_buffer_16bit++ = (hi_pix << 8) | hi_pix;
            }
            dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
            prev_output_buffer = output_buffer;
            screen_line++;
            screen_lines++;
            return;
        }
        default: {
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false); // TODO: ensue it is required
            screen_line++;
            screen_lines++;
            return;
        }
    }
    if (y < 0) {
        dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false); // TODO: ensue it is required
        screen_line++;
        screen_lines++;
        return;
    }
    uint graphics_buffer_height = g_conf.graphics_buffer_height;
    if (y >= graphics_buffer_height) {
        // заполнение линии цветом фона
        if ((y == graphics_buffer_height) | (y == (graphics_buffer_height + 1)) |
            (y == (graphics_buffer_height + 2))) {
            uint32_t* output_buffer_32bit = (uint32_t *)*output_buffer;
            uint32_t p_i = ((line_number & is_flash_line) + (frame_number & is_flash_frame)) & 1;
            uint32_t color32 = bg_color[p_i];
            output_buffer_32bit += shift_picture / 4;
            for (int i = visible_line_size / 2; i--;) {
                *output_buffer_32bit++ = color32;
            }
        }
        dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
        screen_line++;
        screen_lines++;
        return;
    };
    int addr_in_buf = 64 * (y + g_conf.shift_y - 0330);
    while (addr_in_buf < 0) addr_in_buf += 16 << 10;
    while (addr_in_buf >= 16 << 10) addr_in_buf -= 16 << 10;
    uint8_t* input_buffer_8bit = input_buffer + addr_in_buf;
    uint16_t* output_buffer_16bit = (uint16_t *)(*output_buffer);
    output_buffer_16bit += shift_picture / 2; //смещение началы вывода на размер синхросигнала
    if (graphics_mode == BK_512x256x1) {
        graphics_buffer_shift_x &= 0xfffffff1; //1bit buf
    }
    else {
        graphics_buffer_shift_x &= 0xfffffff2; //2bit buf
    }
    //для div_factor 2
    uint max_width = graphics_buffer_width;
    if (graphics_buffer_shift_x < 0) {
        if (BK_512x256x1 == graphics_mode) {
            input_buffer_8bit -= graphics_buffer_shift_x / 8; //1bit buf
        }
        else {
            input_buffer_8bit -= graphics_buffer_shift_x / 4; //2bit buf
        }
        max_width += graphics_buffer_shift_x;
    }
    else {
        output_buffer_16bit += graphics_buffer_shift_x * 2 / div_factor;
    }
    int width = MIN((visible_line_size - ((graphics_buffer_shift_x > 0) ? (graphics_buffer_shift_x) : 0)), max_width);
    if (width < 0) {
        screen_line++;
        screen_lines++;
        return; // TODO: detect a case
    }
    // TODO: Упростить
    uint16_t* current_palette = &palette[0];
    uint8_t* output_buffer_8bit = (uint8_t *)output_buffer_16bit;
    switch (graphics_mode) {
        case BK_512x256x1: {
            current_palette += 5*4;
            //1bit buf
            for (int x = 512/8; x--;) {
                register uint8_t i = *input_buffer_8bit++;
                for (register uint8_t shift = 0; shift < 8; ++shift) {
                    register uint8_t t = current_palette[(i >> shift) & 1];
                    *output_buffer_8bit++ = t;
                    *output_buffer_8bit++ = t;
                }
            }
            break;
        }
        case BK_256x256x2: {
            current_palette += g_conf.graphics_pallette_idx * 4;
            register uint16_t m = (3 << 6) | (pallete_mask << 4) | (pallete_mask << 2) | pallete_mask; // TODO: outside
            m |= m << 8;
            //2bit buf
            for (int x = 256 / 4; x--;) {
                register uint8_t i = *input_buffer_8bit++;
                for (register uint8_t shift = 0; shift < 8; shift += 2) {
                    register uint8_t t = current_palette[(i >> shift) & 3] & m;
                    *output_buffer_8bit++ = t;
                    *output_buffer_8bit++ = t;
                    *output_buffer_8bit++ = t;
                    *output_buffer_8bit++ = t;
                }
            }
            break;
        }
        default:
            break;
    }
    dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
    screen_line++;
    screen_lines++;
}

// to start sound later
void __not_in_flash_func(dma_handler_VGA)() {
    dma_handler_VGA_impl();
}

enum graphics_mode_t graphics_set_mode(enum graphics_mode_t mode) {
    switch (mode) {
        case BK_256x256x2:
        case BK_512x256x1:
          //  if (TEXT_VIDEO_RAM) free(TEXT_VIDEO_RAM);
          //  TEXT_VIDEO_RAM = 0;
            break;
        case TEXTMODE_80x30:
            text_buffer_width = 80;
            text_buffer_height = 30;
            font = font_8x16;
            font_height = 16;
            font_shift = 4;
            font_mask = 15;
         //   if (TEXT_VIDEO_RAM) free(TEXT_VIDEO_RAM);
         //   TEXT_VIDEO_RAM = calloc(text_buffer_width * text_buffer_height / 2, 4);
            break;
        default:
            if (!SELECT_VGA && g_conf.is_DVI_1024) { // TODO: real dvi0 clock
                text_buffer_width = 128;
                text_buffer_height = 16;
            } else if (!SELECT_VGA && !g_conf.is_DVI_1024) {
                text_buffer_width = 100;
                text_buffer_height = 19;
            } else if (!g_conf.is_128_48) {
                text_buffer_width = 64;
                text_buffer_height = 24;
            } else {
                text_buffer_width = 128;
                text_buffer_height = 48;
            }
            if (g_conf.is_8x8) {
                if (!SELECT_VGA && !g_conf.is_DVI_1024) {
                    text_buffer_height = 37;
                } else {
                    text_buffer_height <<= 1;
                }
                font = font_8x8;
                font_height = 8;
                font_shift = 3;
                font_mask = 7;
            } else {
                font = font_8x16;
                font_height = 16;
                font_shift = 4;
                font_mask = 15;
            }
          //  if (TEXT_VIDEO_RAM) free(TEXT_VIDEO_RAM);
          //  TEXT_VIDEO_RAM = calloc(text_buffer_width * text_buffer_height / 2, 4);
    }

    enum graphics_mode_t res = graphics_mode;
    graphics_mode = mode;

    // Если мы уже проиницилизированы - выходим
    if ((txt_palette_fast) && (lines_pattern_data)) {
        return res;
    };

    if (_SM_VGA < 0) return res; // если  VGA не инициализирована -

    uint8_t TMPL_VHS8 = 0;
    uint8_t TMPL_VS8 = 0;
    uint8_t TMPL_HS8 = 0;
    uint8_t TMPL_LINE8 = 0;

    int line_size;
    double fdiv = 100;
    int HS_SIZE = 4;
    int HS_SHIFT = 100;

    switch (graphics_mode) {
        case TEXTMODE_80x30:
        case TEXTMODE_:
            //текстовая палитра
            for (int i = 0; i < 16; i++) {
                txt_palette[i] = (txt_palette[i] & 0x3f) | (palette16_mask >> 8);
            }
            if (!(txt_palette_fast)) {
                txt_palette_fast = (uint16_t *)calloc(256 * 4, sizeof(uint16_t));
                for (int i = 0; i < 256; i++) {
                    uint8_t c1 = txt_palette[i & 0xf];
                    uint8_t c0 = txt_palette[i >> 4];
                    txt_palette_fast[i * 4 + 0] = (c0) | (c0 << 8);
                    txt_palette_fast[i * 4 + 1] = (c1) | (c0 << 8);
                    txt_palette_fast[i * 4 + 2] = (c0) | (c1 << 8);
                    txt_palette_fast[i * 4 + 3] = (c1) | (c1 << 8);
                }
            }
        case BK_256x256x2:
        case BK_512x256x1:
            TMPL_LINE8 = 0b11000000;
            // XGA Signal 1024 x 768 @ 60 Hz timing
            HS_SHIFT = 1024 + 24; // Front porch + Visible area
            HS_SIZE = 160; // Back porch
            line_size = 1344;
            shift_picture = line_size - HS_SHIFT;
            palette16_mask = 0xc0c0;
            visible_line_size = 1024 / 2;
            N_lines_visible = 768;
            line_VS_begin = 768 + 3; // + Front porch
            line_VS_end = 768 + 3 + 6; // ++ Sync pulse 2?
            N_lines_total = 806; // Whole frame
            fdiv = clock_get_hz(clk_sys) / (65000000.0); // 65.0 MHz
            break;
        default:
            return res;
    }

    //корректировка  палитры по маске бит синхры
    bg_color[0] = (bg_color[0] & 0x3f3f3f3f) | palette16_mask | (palette16_mask << 16);
    bg_color[1] = (bg_color[1] & 0x3f3f3f3f) | palette16_mask | (palette16_mask << 16);
    for (int i = 0; i < 16*4; i++) {
        palette[i] = (palette[i] & 0x3f3f) | palette16_mask;
    }

    //инициализация шаблонов строк и синхросигнала
    if (!(lines_pattern_data)) //выделение памяти, если не выделено
    {
        uint32_t div32 = (uint32_t)(fdiv * (1 << 16) + 0.0);
        PIO_VGA->sm[_SM_VGA].clkdiv = div32 & 0xfffff000; //делитель для конкретной sm
        dma_channel_set_trans_count(dma_chan, line_size / 4, false);

        lines_pattern_data = (uint32_t *)calloc(line_size * 4 / 4, sizeof(uint32_t));;

        for (int i = 0; i < 4; i++) {
            lines_pattern[i] = &lines_pattern_data[i * (line_size / 4)];
        }
        // memset(lines_pattern_data,N_TMPLS*1200,0);
        TMPL_VHS8 = TMPL_LINE8 ^ 0b11000000;
        TMPL_VS8 = TMPL_LINE8 ^ 0b10000000;
        TMPL_HS8 = TMPL_LINE8 ^ 0b01000000;

        uint8_t* base_ptr = (uint8_t *)lines_pattern[0];
        //пустая строка
        memset(base_ptr, TMPL_LINE8, line_size);
        //memset(base_ptr+HS_SHIFT,TMPL_HS8,HS_SIZE);
        //выровненная синхра вначале
        memset(base_ptr, TMPL_HS8, HS_SIZE);

        // кадровая синхра
        base_ptr = (uint8_t *)lines_pattern[1];
        memset(base_ptr, TMPL_VS8, line_size);
        //memset(base_ptr+HS_SHIFT,TMPL_VHS8,HS_SIZE);
        //выровненная синхра вначале
        memset(base_ptr, TMPL_VHS8, HS_SIZE);

        //заготовки для строк с изображением
        base_ptr = (uint8_t *)lines_pattern[2];
        memcpy(base_ptr, lines_pattern[0], line_size);
        base_ptr = (uint8_t *)lines_pattern[3];
        memcpy(base_ptr, lines_pattern[0], line_size);
    }
    return res;
};

void graphics_set_page(uint8_t* buffer, uint8_t pallette_idx) {
    g_conf.v_buff_offset = buffer - RAM;
    graphics_buffer = buffer;
    g_conf.graphics_pallette_idx = pallette_idx;
};

void graphics_set_pallette_idx(uint8_t pallette_idx) {
    g_conf.graphics_pallette_idx = pallette_idx;
};

void graphics_shift_screen(uint16_t Word) {
    g_conf.shift_y = Word & 0b11111111;
    // Разряд 9 - при записи “1” в этот разряд на экране отображается весь буфер экрана (256 телевизионных строк).
    // При нулевом значении в верхней части растра отображается 1/4 часть (старшие адреса) экранного ОЗУ,
    // нижняя часть экрана не отображается. Данный режим не используется базовой операционной системой.
    g_conf.graphics_buffer_height = (Word & 0b01000000000) ? 256 : 256 / 4;
}

void graphics_set_buffer(uint8_t* buffer, uint16_t width, uint16_t height) {
    g_conf.v_buff_offset = buffer - RAM;
    graphics_buffer = buffer;
    graphics_buffer_width = width;
    g_conf.graphics_buffer_height = height;
};

void graphics_set_offset(int x, int y) {
    graphics_buffer_shift_x = x;
    graphics_buffer_shift_y = y;
};

void graphics_set_flashmode(bool flash_line, bool flash_frame) {
    is_flash_frame = flash_frame;
    is_flash_line = flash_line;
};

static int current_line = 25;
static int start_debug_line = 25;

void clrScr(uint8_t color) {
    uint8_t* t_buf = TEXT_VIDEO_RAM + (size_t)start_debug_line * text_buffer_width * 2;
    const uint8_t attr = (color << 4) | (color & 0xF);
    for (int yi = start_debug_line; yi < text_buffer_height; yi++)
        for (int xi = 0; xi < (int)text_buffer_width; xi++) {
            *t_buf++ = ' ';
            *t_buf++ = attr;
        }
    current_line = start_debug_line;
};

void draw_text(char* string, int x, int y, uint8_t color, uint8_t bgcolor) {
    uint8_t* t_buf = TEXT_VIDEO_RAM + text_buffer_width * 2 * y + 2 * x;
    const uint8_t c = (bgcolor << 4) | (color & 0xF);
    for (int xi = x; xi < text_buffer_width; ++xi) {
        if (!(*string)) break;
        *t_buf++ = *string++;
        *t_buf++ = c;
    }
}

void draw_shadow(int x, int y, int w, int h, uint8_t color, uint8_t bgcolor) {
    uint8_t* max_t = TEXT_VIDEO_RAM + 2 * text_buffer_width * text_buffer_height;
    if (y + h < text_buffer_height) {
        for (int xi = x + 1; xi <= x + w && xi < text_buffer_width; ++xi) {
            uint8_t* t_buf = TEXT_VIDEO_RAM + text_buffer_width * 2 * (y + h) + 2 * xi + 1;
            if (t_buf >= max_t) continue;
            *t_buf = (bgcolor << 4) | (color & 0xF);
        }
    }
    if (x + w < text_buffer_width) {
        for (int yi = y + 1; yi < y + h && y < text_buffer_height; ++yi) {
            uint8_t* t_buf = TEXT_VIDEO_RAM + text_buffer_width * 2 * yi + 2 * (x + w) + 1;
            if (t_buf >= max_t) continue;
            *t_buf = (bgcolor << 4) | (color & 0xF);
        }
    }
}

void set_start_debug_line(int _start_debug_line) {
    start_debug_line = _start_debug_line;
}

#if BOOT_DEBUG || KBD_DEBUG || MNGR_DEBUG || DSK_DEBUG || INVALID_DEBUG
void logFile(char* msg);
#endif

void logMsg(char* msg) {
#if BOOT_DEBUG || KBD_DEBUG || MNGR_DEBUG || DSK_DEBUG || INVALID_DEBUG
    { char tmp[85]; sprintf(tmp, "%s\n", msg); logFile(tmp); }
#endif
    if (graphics_mode != TEXTMODE_ || manager_started) {
        // log in text mode only
        return;
    }
    if (current_line >= text_buffer_height - 1) {
        current_line = text_buffer_height - 1;
        size_t sz = text_buffer_width * 2;
        for (int i = start_debug_line; i < current_line; ++i) {
            uint8_t* t_buf1 = TEXT_VIDEO_RAM + sz * i;
            uint8_t* t_buf2 = TEXT_VIDEO_RAM + sz * (i + 1);
            memcpy(t_buf1, t_buf2, sz);
        }
        size_t w = text_buffer_width;
        uint8_t* t_buf = TEXT_VIDEO_RAM + (w * 2) * current_line;
        for (size_t x = 0; x < w; ++x) {
            *t_buf++ = ' ';
            *t_buf++ = 1 << 4;
        }
    }
    draw_text(msg, 0, current_line++, 7, 1);
}

void graphics_set_bgcolor(uint32_t color888) {
    uint8_t conv0[] = { 0b00, 0b00, 0b01, 0b10, 0b10, 0b10, 0b11, 0b11 };
    uint8_t conv1[] = { 0b00, 0b01, 0b01, 0b01, 0b10, 0b11, 0b11, 0b11 };

    uint8_t b = ((color888 & 0xff) / 42);

    uint8_t r = (((color888 >> 16) & 0xff) / 42);
    uint8_t g = (((color888 >> 8) & 0xff) / 42);

    uint8_t c_hi = (conv0[r] << 4) | (conv0[g] << 2) | conv0[b];
    uint8_t c_lo = (conv1[r] << 4) | (conv1[g] << 2) | conv1[b];
    bg_color[0] = (((((c_hi << 8) | c_lo) & 0x3f3f) | palette16_mask) << 16) |
                  ((((c_hi << 8) | c_lo) & 0x3f3f) | palette16_mask);
    bg_color[1] = (((((c_lo << 8) | c_hi) & 0x3f3f) | palette16_mask) << 16) |
                  ((((c_lo << 8) | c_hi) & 0x3f3f) | palette16_mask);
};

void graphics_init() {
    //инициализация палитры по умолчанию
    for (int i = 0; i < 16; ++i)
    { // black for 256*256*4
        palette[i*4] = 0xc0c0;
    }
    for (int i = 0; i < 16; ++i) {
        for (int j = 1; j < 4; ++j)
        { // white
            uint8_t rgb = 0b111111;
            uint8_t c = 0xc0 | rgb;
            palette[i*4+j] = (c << 8) | c;
        }
    }
    { // dark blue for 256*256*4
        uint8_t b = 0b11;
        uint8_t c = 0xc0 | b;
        palette[0*4+1] = (c << 8) | c;
        palette[2*4+2] = (c << 8) | c;
    }
    { // green for 256*256*4
        uint8_t g = 0b11 << 2;
        uint8_t c = 0xc0 | g;
        palette[0*4+2] = (c << 8) | c;
        palette[1*4+2] = (c << 8) | c;
        palette[3*4+1] = (c << 8) | c;
        palette[12*4+2] = (c << 8) | c;
        palette[14*4+2] = (c << 8) | c;
        palette[15*4+2] = (c << 8) | c;
    }
    { // red for 256*256*4
        uint8_t r = 0b11 << 4;
        uint8_t c = 0xc0 | r;
        palette[0*4+3] = (c << 8) | c;
        palette[1*4+3] = (c << 8) | c;
        palette[6*4+3] = (c << 8) | c;
        palette[11*4+3] = (c << 8) | c;
        palette[12*4+1] = (c << 8) | c;
    }
    { // yellow for 256*256*4
        uint8_t r = 0b11 << 4;
        uint8_t g = 0b11 << 2;
        uint8_t c = 0xc0 | r | g;
        palette[1*4+1] = (c << 8) | c;
        palette[3*4+3] = (c << 8) | c;
        palette[7*4+3] = (c << 8) | c;
        palette[11*4+2] = (c << 8) | c;
        palette[13*4+2] = (c << 8) | c;
        palette[14*4+1] = (c << 8) | c;
    }
    { // margenta for 256*256*4
        uint8_t r = 0b11 << 4;
        uint8_t b = 0b11;
        uint8_t c_hi = 0xc0 | r | b;
        palette[1*4+2] = (c_hi << 8) | c_hi;
        palette[2*4+3] = (c_hi << 8) | c_hi;
        palette[4*4+1] = (c_hi << 8) | c_hi;
        palette[8*4+3] = (c_hi << 8) | c_hi;
    }
    { // light blue for 256*256*4
        uint8_t r = 0b01 << 4;
        uint8_t g = 0b01 << 2;
        uint8_t b = 0b11;
        uint8_t c_hi = 0xc0 | r | g | b;
        palette[2*4+1] = (c_hi << 8) | c_hi;
        palette[3*4+2] = (c_hi << 8) | c_hi;
        palette[4*4+2] = (c_hi << 8) | c_hi;
        palette[11*4+1] = (c_hi << 8) | c_hi;
        palette[12*4+3] = (c_hi << 8) | c_hi;
        palette[13*4+1] = (c_hi << 8) | c_hi;
        palette[15*4+1] = (c_hi << 8) | c_hi;
    }
    { // deep red for 256*256*4
        uint8_t r = 0b10 << 4;
        uint8_t c_hi = 0xc0 | r;
        palette[6*4+1] = (c_hi << 8) | c_hi;
        palette[10*4+3] = (c_hi << 8) | c_hi;
    }
    { // dark red for 256*256*4
        uint8_t r = 0b01 << 4;
        uint8_t c_hi = 0xc0 | r;
        palette[6*4+2] = (c_hi << 8) | c_hi;
        palette[9*4+3] = (c_hi << 8) | c_hi;
    }
    { // yellow2 for 256*256*4
        uint8_t r = 0b10 << 4;
        uint8_t g = 0b10 << 2;
        uint8_t c_hi = 0xc0 | r | g;
        palette[7*4+1] = (c_hi << 8) | c_hi;
        palette[10*4+1] = (c_hi << 8) | c_hi;
    }
    { // yellow3 for 256*256*4
        uint8_t r = 0b10 << 4;
        uint8_t g = 0b11 << 2;
        uint8_t c_hi = 0xc0 | r | g;
        palette[7*4+2] = (c_hi << 8) | c_hi;
        palette[9*4+1] = (c_hi << 8) | c_hi;
    }
    { // margenta2 for 256*256*4
        uint8_t r = 0b10 << 4;
        uint8_t b = 0b10;
        uint8_t c_hi = 0xc0 | r | b;
        palette[8*4+1] = (c_hi << 8) | c_hi;
        palette[10*4+2] = (c_hi << 8) | c_hi;
    }
    { // margenta3 for 256*256*4
        uint8_t r = 0b10 << 4;
        uint8_t b = 0b10;
        uint8_t c_hi = 0xc0 | r | b;
        palette[8*4+2] = (c_hi << 8) | c_hi;
        palette[9*4+2] = (c_hi << 8) | c_hi;
    }
    //текстовая палитра
    for (int i = 0; i < 16; i++) {
        uint8_t b = (i & 1) ? ((i >> 3) ? 3 : 2) : 0;
        uint8_t r = (i & 4) ? ((i >> 3) ? 3 : 2) : 0;
        uint8_t g = (i & 2) ? ((i >> 3) ? 3 : 2) : 0;
        uint8_t c = (r << 4) | (g << 2) | b;
        txt_palette[i] = (c & 0x3f) | 0xc0;
    }
    //инициализация PIO
    //загрузка программы в один из PIO
    uint offset = pio_add_program(PIO_VGA, &pio_program_VGA);
    _SM_VGA = pio_claim_unused_sm(PIO_VGA, true);
    uint sm = _SM_VGA;

    for (int i = 0; i < 8; i++) {
        gpio_init(beginVGA_PIN + i);
        gpio_set_dir(beginVGA_PIN + i, GPIO_OUT);
        pio_gpio_init(PIO_VGA, beginVGA_PIN + i);
    }; //резервируем под выход PIO

    //pio_sm_config c = pio_vga_program_get_default_config(offset);

    pio_sm_set_consecutive_pindirs(PIO_VGA, sm, beginVGA_PIN, 8, true); //конфигурация пинов на выход

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + 0, offset + (pio_program_VGA.length - 1));

    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX); //увеличение буфера TX за счёт RX до 8-ми
    sm_config_set_out_shift(&c, true, true, 32);
    sm_config_set_out_pins(&c, beginVGA_PIN, 8);
    pio_sm_init(PIO_VGA, sm, offset, &c);

    pio_sm_set_enabled(PIO_VGA, sm, true);

    //инициализация DMA
    dma_chan_ctrl = dma_claim_unused_channel(true);
    dma_chan = dma_claim_unused_channel(true);
    //основной ДМА канал для данных
    dma_channel_config c0 = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);

    channel_config_set_read_increment(&c0, true);
    channel_config_set_write_increment(&c0, false);

    uint dreq = DREQ_PIO1_TX0 + sm;
    if (PIO_VGA == pio0) dreq = DREQ_PIO0_TX0 + sm;

    channel_config_set_dreq(&c0, dreq);
    channel_config_set_chain_to(&c0, dma_chan_ctrl); // chain to other channel

    dma_channel_configure(
        dma_chan,
        &c0,
        &PIO_VGA->txf[sm], // Write address
        lines_pattern[0], // read address
        600 / 4, //
        false // Don't start yet
    );
    //канал DMA для контроля основного канала
    dma_channel_config c1 = dma_channel_get_default_config(dma_chan_ctrl);
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);

    channel_config_set_read_increment(&c1, false);
    channel_config_set_write_increment(&c1, false);
    channel_config_set_chain_to(&c1, dma_chan); // chain to other channel
    //channel_config_set_dreq(&c1, DREQ_PIO0_TX0);

    dma_channel_configure(
        dma_chan_ctrl,
        &c1,
        &dma_hw->ch[dma_chan].read_addr, // Write address
        &lines_pattern[0], // read address
        1, //
        false // Don't start yet
    );
    //dma_channel_set_read_addr(dma_chan, &DMA_BUF_ADDR[0], false);

    graphics_set_mode(BK_256x256x2);

    irq_set_exclusive_handler(VGA_DMA_IRQ, dma_handler_VGA);

    dma_channel_set_irq0_enabled(dma_chan_ctrl, true);

    irq_set_enabled(VGA_DMA_IRQ, true);
    dma_start_channel_mask((1u << dma_chan));
};

#ifdef SAVE_VIDEO_RAM_ON_MANAGER
#include "emulator.h"
static FATFS fs;
static FIL file;
static int video_ram_level = 0;

bool save_video_ram() {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    char path[16];
    sprintf(path, "\\BK\\video%d.ram", video_ram_level);
    FRESULT result = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK) {
        return false;
    }
    UINT bw;
    result = f_write(&file, TEXT_VIDEO_RAM, sizeof(TEXT_VIDEO_RAM), &bw);
    if (result != FR_OK) {
        return false;
    }
    f_close(&file);
    video_ram_level++;
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    return true;
}
bool restore_video_ram() {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    video_ram_level--;
    char path[16];
    sprintf(path, "\\BK\\video%d.ram", video_ram_level);
    FRESULT result = f_open(&file, path, FA_READ);
    if (result == FR_OK) {
      UINT bw;
      result = f_read(&file, TEXT_VIDEO_RAM, sizeof(TEXT_VIDEO_RAM), &bw);
      if (result != FR_OK) {
        return false;
      }
    }
    f_close(&file);
    f_unlink(path);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    return true;
}
#else
bool save_video_ram() {}
bool restore_video_ram() {}
#endif
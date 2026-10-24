/*
 * Linux counterpart of drivers/vga-nextgen/vga.c.
 *
 * On the RP2350 the VGA DMA IRQ builds every scanline of a 1024x768@60
 * frame as 8-bit samples (bits 0..5: BBGGRR... 2 bits per colour, bits 6..7:
 * sync) from the BK video page or the text buffer.  vga_linux_compose()
 * builds the same 768 lines with the same rules (BK 256x256x2 and 512x256x1
 * pages shown three lines per BK line, the 128x48 / 64x24 / 80x30 text
 * modes, the dithered background below a 1/4-height BK screen) and turns the
 * 6-bit samples into a 1024x768 XRGB8888 frame for the DRM/KMS output.
 *
 * The API below (graphics_*, draw_text, logMsg, ...) is the one vga.c gives
 * to the emulator and the manager.
 */
#include "vga.h"
#include "vga_linux.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../drivers/vga-nextgen/fnt8x8.h"
#include "../../drivers/vga-nextgen/fnt8x16.h"
#include "config_em.h"
#include "ram_page.h"

int pallete_mask = 3; // 11 - 2 bits
extern volatile bool vsync;
/* vsync_ptr is part of the vga.h API; the Linux emulation loop raises the
 * 50 Hz interrupt itself (emu_e.c), so nothing writes through it here. */
uint8_t* vsync_ptr = (uint8_t*)&vsync;
static uint8_t* font = font_8x16;
uint8_t font_height = 16;
uint8_t font_shift = 4;
uint32_t font_mask = 15;
extern uint8_t TEXT_VIDEO_RAM[128*96*2];

#define N_LINES_VISIBLE 768

static volatile uint8_t* graphics_buffer;
uint8_t* get_graphics_buffer(int y) {
    int addr_in_buf = 64 * (y + g_conf.shift_y - 0330);
    while (addr_in_buf < 0) addr_in_buf += 16 << 10;
    while (addr_in_buf >= 16 << 10) addr_in_buf -= 16 << 10;
    return (uint8_t*)graphics_buffer + addr_in_buf;
}
static unsigned graphics_buffer_width = 0;
static int graphics_buffer_shift_x = 0;
static int graphics_buffer_shift_y = 0;

static bool is_flash_line = false;
static bool is_flash_frame = false;

// палитры BK: 16 палитр по 4 цвета, по два одинаковых 6-битных отсчёта
static uint16_t palette[16*4];
static uint16_t gray_palette[16*4];

// фон ниже экрана БК в режиме 1/4: два отсчёта (чередование по пикселям)
static uint8_t bg_lo, bg_hi;

static uint8_t txt_palette[16];

unsigned int text_buffer_width = 46;
unsigned int text_buffer_height = 30;
void set_text_mode_size(unsigned w, unsigned h) {
    text_buffer_width = w;
    text_buffer_height = h;
}

volatile enum graphics_mode_t graphics_mode;
enum graphics_mode_t get_graphics_mode() { return graphics_mode; }

volatile uint16_t covox_mix = 0x0F;

/* The HDMI 1024x768 renderer of the RP2350 build; nothing to select here
 * (the manager calls it only for HDMI output, SELECT_VGA is always true). */
void dvi_1024_select_renderer(enum graphics_mode_t mode) {
    (void)mode;
}

void graphics_inc_palleter_offset() {
    g_conf.graphics_pallette_idx++;
    if (g_conf.graphics_pallette_idx > 0b1111) g_conf.graphics_pallette_idx = 0;
}

/* 6-bit VGA sample (RRGGBB, 2 bits each) -> XRGB8888 */
static uint32_t rgb6[64];

static void build_rgb6(void) {
    for (int c = 0; c < 64; ++c) {
        const uint32_t r = ((c >> 4) & 3) * 0x55u;
        const uint32_t g = ((c >> 2) & 3) * 0x55u;
        const uint32_t b = (c & 3) * 0x55u;
        rgb6[c] = (r << 16) | (g << 8) | b;
    }
}

static inline void put2(uint32_t* dst, uint8_t lo, uint8_t hi) {
    dst[0] = rgb6[lo & 0x3f];
    dst[1] = rgb6[hi & 0x3f];
}

static void blank_line(uint32_t* dst) {
    memset(dst, 0, VGA_LINUX_WIDTH * sizeof(uint32_t));
}

/* Text line: font rows LSB first, fg/bg from the 16-colour text palette;
 * scale 1 (128x48) or 2 (64x24: pixels and lines doubled). */
static void text_line(uint32_t* dst, unsigned screen_line, unsigned scale) {
    const unsigned y = screen_line / scale;
    const unsigned row = y >> font_shift;
    if (row >= 96) {
        blank_line(dst);
        return;
    }
    const uint32_t glyph_line = y & font_mask;
    const uint8_t* text_buffer_line = &TEXT_VIDEO_RAM[row * text_buffer_width * 2];
    uint32_t* const end = dst + VGA_LINUX_WIDTH;
    for (unsigned x = 0; x < text_buffer_width && dst + 8 * scale <= end; x++) {
        uint8_t glyph_pixels = font[(*text_buffer_line++ << font_shift) + glyph_line];
        const uint8_t attr = *text_buffer_line++;
        const uint32_t fg = rgb6[txt_palette[attr & 0xf] & 0x3f];
        const uint32_t bg = rgb6[txt_palette[attr >> 4] & 0x3f];
        for (int bit = 0; bit < 8; ++bit) {
            const uint32_t c = (glyph_pixels & 1) ? fg : bg;
            for (unsigned k = 0; k < scale; ++k) *dst++ = c;
            glyph_pixels >>= 1;
        }
    }
    while (dst < end) *dst++ = 0;
}

static void bk_line(uint32_t* dst, int y, int line_number, uint32_t frame_number) {
    if (!graphics_buffer || y < 0) {
        blank_line(dst);
        return;
    }
    const unsigned graphics_buffer_height = g_conf.graphics_buffer_height;
    if ((unsigned)y >= graphics_buffer_height) {
        // заполнение линии цветом фона (чередование отсчётов, мигание)
        const uint32_t p_i = ((line_number & is_flash_line) + (frame_number & is_flash_frame)) & 1;
        const uint8_t first = p_i ? bg_hi : bg_lo;
        const uint8_t second = p_i ? bg_lo : bg_hi;
        for (int x = 0; x < VGA_LINUX_WIDTH; x += 2) put2(dst + x, first, second);
        return;
    }
    const uint8_t* input = get_graphics_buffer(y);
    int shift_x = graphics_buffer_shift_x;
    unsigned skip = 0;
    if (shift_x > 0) {
        skip = (unsigned)shift_x * 2 > VGA_LINUX_WIDTH ? VGA_LINUX_WIDTH : (unsigned)shift_x * 2;
        memset(dst, 0, skip * sizeof(uint32_t));
    }
    uint32_t* out = dst + skip;
    uint32_t* const end = dst + VGA_LINUX_WIDTH;
    const enum graphics_mode_t mode = graphics_mode;
    if (mode == BK_512x256x1) {
        if (shift_x < 0) input += (-shift_x) / 8;
        const uint16_t* current_palette = &palette[5*4];
        for (int x = 512/8; x-- && out < end;) {
            uint8_t i = *input++;
            for (uint8_t shift = 0; shift < 8 && out < end; ++shift) {
                const uint8_t t = (uint8_t)current_palette[(i >> shift) & 1];
                put2(out, t, t);
                out += 2;
            }
        }
    } else {
        if (shift_x < 0) input += (-shift_x) / 4;
        const uint16_t* current_palette = (g_conf.color_mode == BK_COLOR_GRAY) ? gray_palette : palette;
        current_palette += g_conf.graphics_pallette_idx * 4;
        uint16_t m = (3 << 6) | (pallete_mask << 4) | (pallete_mask << 2) | pallete_mask;
        m |= m << 8;
        for (int x = 256 / 4; x-- && out < end;) {
            uint8_t i = *input++;
            for (uint8_t shift = 0; shift < 8 && out + 4 <= end; shift += 2) {
                // Each 16-bit palette entry encodes two adjacent VGA samples.
                // Two writes expand one 2-bit BK pixel to four VGA samples.
                const uint16_t t = current_palette[(i >> shift) & 3] & m;
                put2(out, (uint8_t)t, (uint8_t)(t >> 8));
                put2(out + 2, (uint8_t)t, (uint8_t)(t >> 8));
                out += 4;
            }
        }
    }
    while (out < end) *out++ = 0;
}

void vga_linux_compose(uint32_t* frame) {
    static uint32_t frame_number = 0;
    frame_number++;
    for (unsigned screen_line = 0; screen_line < N_LINES_VISIBLE; ++screen_line) {
        uint32_t* dst = frame + (size_t)screen_line * VGA_LINUX_WIDTH;
        switch (graphics_mode) {
            case BK_256x256x2:
            case BK_512x256x1: {
                // три подряд строки рисуем одно и тоже
                if (screen_line % 3 != 0) {
                    memcpy(dst, dst - VGA_LINUX_WIDTH, VGA_LINUX_WIDTH * sizeof(uint32_t));
                    break;
                }
                const int line_number = (int)(screen_line / 3);
                bk_line(dst, line_number - graphics_buffer_shift_y, line_number, frame_number);
                break;
            }
            case TEXTMODE_80x30:
                text_line(dst, screen_line, 1);
                break;
            case TEXTMODE_:
                text_line(dst, screen_line, g_conf.is_128_48 ? 1 : 2);
                break;
            default:
                blank_line(dst);
                break;
        }
    }
}

enum graphics_mode_t graphics_set_mode(enum graphics_mode_t mode) {
    switch (mode) {
        case BK_256x256x2:
        case BK_512x256x1:
            break;
        case TEXTMODE_80x30:
            text_buffer_width = 80;
            text_buffer_height = 30;
            font = font_8x16;
            font_height = 16;
            font_shift = 4;
            font_mask = 15;
            break;
        default:
            if (!g_conf.is_128_48) {
                text_buffer_width = 64;
                text_buffer_height = 24;
            } else {
                text_buffer_width = 128;
                text_buffer_height = 48;
            }
            if (g_conf.is_8x8) {
                text_buffer_height <<= 1;
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
    }
    enum graphics_mode_t res = graphics_mode;
    graphics_mode = mode;
    return res;
}

void graphics_set_page(uint8_t* buffer, uint8_t pallette_idx) {
    g_conf.v_buff_offset = buffer - RAM;
    graphics_buffer = buffer;
    g_conf.graphics_pallette_idx = pallette_idx;
}

void graphics_set_pallette_idx(uint8_t pallette_idx) {
    g_conf.graphics_pallette_idx = pallette_idx;
}

void graphics_shift_screen(uint16_t Word) {
    g_conf.shift_y = Word & 0b11111111;
    // Разряд 9 - при записи “1” в этот разряд на экране отображается весь буфер экрана (256 телевизионных строк).
    // При нулевом значении в верхней части растра отображается 1/4 часть (старшие адреса) экранного ОЗУ,
    // нижняя часть экрана не отображается.
    g_conf.graphics_buffer_height = (Word & 0b01000000000) ? 256 : 256 / 4;
}

void graphics_set_buffer(uint8_t* buffer, uint16_t width, uint16_t height) {
    g_conf.v_buff_offset = buffer - RAM;
    graphics_buffer = buffer;
    graphics_buffer_width = width;
    g_conf.graphics_buffer_height = height;
}

void graphics_set_offset(int x, int y) {
    graphics_buffer_shift_x = x;
    graphics_buffer_shift_y = y;
}

void graphics_set_flashmode(bool flash_line, bool flash_frame) {
    is_flash_frame = flash_frame;
    is_flash_line = flash_line;
}

static int current_line = 25;
static int start_debug_line = 25;

void clrScr(uint8_t color) {
    uint8_t* t_buf = TEXT_VIDEO_RAM + (size_t)start_debug_line * text_buffer_width * 2;
    const uint8_t attr = (color << 4) | (color & 0xF);
    for (unsigned yi = start_debug_line; yi < text_buffer_height; yi++)
        for (unsigned xi = 0; xi < text_buffer_width; xi++) {
            *t_buf++ = ' ';
            *t_buf++ = attr;
        }
    current_line = start_debug_line;
}

void draw_text(const char* string, int x, int y, uint8_t color, uint8_t bgcolor) {
    uint8_t* t_buf = TEXT_VIDEO_RAM + text_buffer_width * 2 * y + 2 * x;
    const uint8_t c = (bgcolor << 4) | (color & 0xF);
    for (int xi = x; xi < (int)text_buffer_width; ++xi) {
        if (!(*string)) break;
        *t_buf++ = *string++;
        *t_buf++ = c;
    }
}

void draw_shadow(int x, int y, int w, int h, uint8_t color, uint8_t bgcolor) {
    uint8_t* max_t = TEXT_VIDEO_RAM + 2 * text_buffer_width * text_buffer_height;
    if (y + h < (int)text_buffer_height) {
        for (int xi = x + 1; xi <= x + w && xi < (int)text_buffer_width; ++xi) {
            uint8_t* t_buf = TEXT_VIDEO_RAM + text_buffer_width * 2 * (y + h) + 2 * xi + 1;
            if (t_buf >= max_t) continue;
            *t_buf = (bgcolor << 4) | (color & 0xF);
        }
    }
    if (x + w < (int)text_buffer_width) {
        for (int yi = y + 1; yi < y + h && y < (int)text_buffer_height; ++yi) {
            uint8_t* t_buf = TEXT_VIDEO_RAM + text_buffer_width * 2 * yi + 2 * (x + w) + 1;
            if (t_buf >= max_t) continue;
            *t_buf = (bgcolor << 4) | (color & 0xF);
        }
    }
}

void set_start_debug_line(int _start_debug_line) {
    start_debug_line = _start_debug_line;
}

void logMsg(char* msg) {
    if (graphics_mode != TEXTMODE_ || manager_started) {
        // log in text mode only
        return;
    }
    if (current_line >= (int)text_buffer_height - 1) {
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
    static const uint8_t conv0[] = { 0b00, 0b00, 0b01, 0b10, 0b10, 0b10, 0b11, 0b11 };
    static const uint8_t conv1[] = { 0b00, 0b01, 0b01, 0b01, 0b10, 0b11, 0b11, 0b11 };

    const uint8_t b = ((color888 & 0xff) / 42);
    const uint8_t r = (((color888 >> 16) & 0xff) / 42);
    const uint8_t g = (((color888 >> 8) & 0xff) / 42);

    bg_hi = (conv0[r] << 4) | (conv0[g] << 2) | conv0[b];
    bg_lo = (conv1[r] << 4) | (conv1[g] << 2) | conv1[b];
}

/* Without SAVE_VIDEO_RAM_ON_MANAGER (as in the RP2350 build) the text screen
 * is not saved around manager dialogs. */
bool save_video_ram() {
    return true;
}

bool restore_video_ram() {
    return true;
}

void graphics_init() {
    build_rgb6();
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
    // VGA GrS: BK brightness order is red > green > blue (not Rec. 709).
    for (int i = 0; i < 16*4; ++i) {
        uint8_t y = 0;
        uint8_t rgb = palette[i] & 0x3f;
        if ((rgb >> 4) & 3) y = 0b11;
        else if ((rgb >> 2) & 3) y = 0b10;
        else if (rgb & 3) y = 0b01;
        uint8_t c = 0xc0 | (y << 4) | (y << 2) | y;
        gray_palette[i] = (c << 8) | c;
    }
    //текстовая палитра
    for (int i = 0; i < 16; i++) {
        uint8_t b = (i & 1) ? ((i >> 3) ? 3 : 2) : 0;
        uint8_t r = (i & 4) ? ((i >> 3) ? 3 : 2) : 0;
        uint8_t g = (i & 2) ? ((i >> 3) ? 3 : 2) : 0;
        uint8_t c = (r << 4) | (g << 2) | b;
        txt_palette[i] = (c & 0x3f) | 0xc0;
    }
    graphics_set_mode(BK_256x256x2);
}

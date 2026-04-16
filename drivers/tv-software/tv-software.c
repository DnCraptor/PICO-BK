#
//программный композит
#include <stdio.h>
#include "graphics.h"
#include "tv-software.h"
#include "font6x8.h"
#include "hardware/clocks.h"
#include <stdalign.h>

#include "hardware/structs/pll.h"
#include "hardware/structs/systick.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include <string.h>
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "stdlib.h"

#include "emulator.h"
#include "config_em.h"

#pragma GCC optimize("Ofast")
uint8_t* text_buffer = &TEXT_VIDEO_RAM[0];

void graphics_set_palette(uint8_t i, uint32_t color888);

typedef enum {
    TV_SYS_PAL,
    TV_SYS_NTSC
} tv_system_t;

typedef struct tv_out_mode_t {
    tv_system_t tv_system;
    float color_freq;
    uint16_t N_lines;
    float color_index;
    enum graphics_mode_t mode_bpp;
    bool cb_sync_PI_shift_lines;
    bool cb_sync_PI_shift_half_frame;
} tv_out_mode_t;


static tv_out_mode_t tv_out_mode_NTSC = {
    .tv_system = TV_SYS_NTSC,
    .color_freq = 3579545.4545454545, // 315000000.0 / 88.0 ~ 3579545,
    .N_lines = 262,   // half-frame NTSC
    .mode_bpp = BK_256x256x2,
    .color_index = 1.0,
    .cb_sync_PI_shift_lines = false,
    .cb_sync_PI_shift_half_frame = true
};
static tv_out_mode_t tv_out_mode_PAL = {
    .tv_system = TV_SYS_PAL,
    .color_freq = 4433618.75,
    .N_lines = 312,
    .mode_bpp = BK_256x256x2,
    .color_index = 1.0, //0-1
    .cb_sync_PI_shift_lines = false,
    .cb_sync_PI_shift_half_frame = true
};
#ifndef TV_OUT_MODE
#define TV_OUT_MODE tv_out_mode_PAL
#endif
//параметры по умолчанию
static tv_out_mode_t tv_out_mode = {
    .tv_system = TV_SYS_PAL,
    .color_freq = 4433618.75,
    .N_lines = 312,
    .mode_bpp = BK_256x256x2,
    .color_index = 1.0, //0-1
    .cb_sync_PI_shift_lines = false,
    .cb_sync_PI_shift_half_frame = true
};


//программы PIO
//программа видеовывода
static uint16_t pio_program_TV_instructions[] = {
    //	 .wrap_target

    //	 .wrap_target
    0x6008, //  0: out	pins, 8
    //	 .wrap
    //	 .wrap
};

static const struct pio_program program_pio_TV = {
    .instructions = pio_program_TV_instructions,
    .length = 1,
    .origin = -1,
};

typedef struct TV_MODE {
    int H_len;
    int begin_img_shx;
    int img_W;
    int N_lines;
    int sync_size;
    uint8_t SYNC_TMPL;
    uint8_t NO_SYNC_TMPL;
    uint8_t LVL_C_MAX;
    uint8_t LVL_BLACK;
    uint8_t LVL_BLACK_TMPL;
    uint8_t LVL_Y_MAX;
} TV_MODE;

typedef struct G_BUFFER {
    uint width;
    uint height;
    int shift_x;
    int shift_y;
    uint8_t* data;
} G_BUFFER;


//режим видеовыхода
static TV_MODE video_mode = {
    .H_len = 512,
    .N_lines = 525,
    .SYNC_TMPL = 0,
    .NO_SYNC_TMPL = 0,
    .LVL_C_MAX = 0,
    .LVL_BLACK = 0,
    .LVL_Y_MAX = 0,
};


static G_BUFFER graphics_buffer = {
    .data = NULL,
    .shift_x = 0,
    .shift_y = 5,
    .height = 256,
    .width = 256
};

uint8_t* __not_in_flash() get_graphics_buffer(int y) {
    int addr_in_buf = 64 * (y + g_conf.shift_y - 0330);
    while (addr_in_buf < 0) addr_in_buf += 16 << 10;
    while (addr_in_buf >= 16 << 10) addr_in_buf -= 16 << 10;
    return graphics_buffer.data + addr_in_buf;
}
static inline uint8_t* bk_get_line(int y) { return get_graphics_buffer(y); }
static inline void     bk_vsync(void)     { *vsync_ptr = 1; }

//пины
//пин синхросигнала(для совместимости с RGB по ч.б.) 0-7
#define SYNC_PIN (6)
//максимальное значение DAC
#define MAX_DAC (63)
//перераспределение порядка пинов(для совместимости с RGB по ч.б.)
#define CONV_DAC(x) ((((x)<<2)&0x30)|(((x)>>2)&0x0c)|((x)&0x03))

//буферы строк
//количество буферов задавать кратно степени двойки
//например 2^2=4 буфера
#define N_LINE_BUF_log2 (2)


#define N_LINE_BUF_DMA (1<<N_LINE_BUF_log2)
#define N_LINE_BUF (N_LINE_BUF_DMA)

//максимальный размер строки(кратно 4)
#define LINE_SIZE_MAX (1152)
//указатели на буферы строк
//выравнивание нужно для кольцевого буфера
static uint32_t rd_addr_DMA_CTRL[N_LINE_BUF * 2]__attribute__ ((aligned (4*N_LINE_BUF_DMA)));
static uint32_t transfer_count_DMA_CTRL[N_LINE_BUF * 2]__attribute__ ((aligned (4*N_LINE_BUF_DMA)));
//непосредственно буферы строк

static uint32_t lines_buf[N_LINE_BUF][LINE_SIZE_MAX / 4];


static int SM_video = -1;


//DMA каналы
//каналы работы с первичным графическим буфером
static int dma_chan_ctrl = -1;
static int dma_chan_ctrl2 = -1;
static int dma_chan = -1;


//ДМА палитра для конвертации(256 знач)
static uint32_t conv_colorNORM[2][256]; //2к
static uint32_t conv_colorINV[2][256]; //2к
static uint32_t* conv_color[2];

//палитра сохранённая
static uint8_t __scratch_x("buff4") paletteRGB[3][256]; //768 байт

static repeating_timer_t video_timer;

void graphics_set_modeTV(tv_out_mode_t mode) {
    if (SM_video == -1) return;
    //можно добавить проверку на валидность данных, но пока так
    tv_out_mode = mode;

    video_mode.N_lines = tv_out_mode.N_lines;

    double color_freq = tv_out_mode.color_freq;
    video_mode.H_len = ((color_freq * 4) / 1e6) * 63.9;
    video_mode.H_len &= 0xfffffff8;

    video_mode.sync_size = 4.7 * video_mode.H_len / 64;
    video_mode.sync_size &= 0xfffffff8;

    if (tv_out_mode.tv_system == TV_SYS_PAL) {
        if (tv_out_mode.mode_bpp == TEXTMODE_) {
            video_mode.begin_img_shx = 10.5 * video_mode.H_len / 64 + 24;
        } else {
            video_mode.begin_img_shx = 10.5 * video_mode.H_len / 64 + 44;
        }
    } else {
        video_mode.begin_img_shx = 10.5 * video_mode.H_len / 64; // no offset in shorter line
    }
    video_mode.img_W = video_mode.H_len - ((12 * video_mode.H_len) / 64);
    video_mode.img_W &= 0xfffffffc;

    video_mode.LVL_C_MAX = 15;
    video_mode.SYNC_TMPL = 0;
    video_mode.NO_SYNC_TMPL = CONV_DAC(video_mode.LVL_C_MAX) | (1 << SYNC_PIN);
    video_mode.LVL_BLACK = 0 + video_mode.LVL_C_MAX;
    video_mode.LVL_Y_MAX = 40;
    if (tv_out_mode.tv_system == TV_SYS_NTSC) {
        video_mode.LVL_BLACK += 2;
        video_mode.LVL_Y_MAX += 3;
    }
    video_mode.LVL_BLACK_TMPL = CONV_DAC(video_mode.LVL_BLACK) | (1 << SYNC_PIN);
    for (int i = 0; i < 256; i++) {
        graphics_set_palette(i, (paletteRGB[2][i] << 16) | (paletteRGB[1][i] << 8) | (paletteRGB[0][i] << 0));
    }    
    sm_config_set_clkdiv((pio_sm_config*)PIO_VIDEO->sm, clock_get_hz(clk_sys) / (color_freq * 4));
};


static uint32_t cbNORM[2][10]; //цветовая вспышка 80байт
static uint32_t cbINV[2][10]; //цветовая вспышка	инвертированная 80 байт

static uint32_t* cb[2]; //цветовая вспышка
//определение палитры(переделать)
void graphics_set_palette(uint8_t i, uint32_t color888) {
    conv_color[0] = conv_colorNORM[0];
    conv_color[1] = conv_colorNORM[1];
    cb[0] = cbNORM[0];
    cb[1] = cbNORM[1];

    uint8_t R8 = (color888 >> 16) & 0xff;
    uint8_t G8 = (color888 >> 8) & 0xff;
    uint8_t B8 = (color888 >> 0) & 0xff;
    paletteRGB[2][i] = R8;
    paletteRGB[1][i] = G8;
    paletteRGB[0][i] = B8;
    float R = R8 / 255.0;
    float G = G8 / 255.0;
    float B = B8 / 255.0;

    float Y = 0.299 * R + 0.587 * G + 0.114 * B;
    // if (active_out==g_TV_OUT_NTSC) Y=0.299*R+0.587*G+0.114*B;
    uint8_t base8 = video_mode.LVL_BLACK;

    const int cycle_size = 4;
    int8_t Y8 = ((int)(Y * video_mode.LVL_Y_MAX)) + base8;

    uint32_t cd0_32, cd1_32;
    int8_t* cd0 = (int8_t*)&cd0_32;
    int8_t* cd1 = (int8_t*)&cd1_32;

    float sin[] = { 0, 1, 0, -1 };
    // float sin[]={-1,1,1,-1,-1};//test

    float cos[] = { 1, 0, -1, 0 };
    {
        float U = 0.493 * (B - Y);
        float V = 0.877 * (R - Y);

        int ph = 2;
        int dph = 0;
        if (tv_out_mode.cb_sync_PI_shift_lines) {
            dph = -1;
        }
        for (int i = 0; i < cycle_size; i++) {
            float k = 1.3 * tv_out_mode.color_index;
            //подобрать , чтобы не было перегруза 1.25 или увеличить для более ярких цветов
            int max_v = video_mode.LVL_C_MAX;
            int P = k * max_v * (U * sin[(i + ph + 1 + dph) % 4] + V * cos[(i + ph + 1 + dph) % 4]) + 0.0; //+1
            int M = k * max_v * (U * sin[(i + ph) % 4] - V * cos[(i + ph) % 4]) + 0.0;


            P = P < -max_v ? -max_v : P;
            P = P > max_v ? max_v : P;


            M = M < -max_v ? -max_v : M;
            M = M > max_v ? max_v : M;


            cd0[i] = (M);
            cd1[i] = (P);
        }
        // ph=0;

        //заполнение цветовой вспышки
        uint8_t* cb8_0 = (uint8_t *)cb[0];
        uint8_t* cb8_1 = (uint8_t *)cb[1];
        uint8_t* cb8_0_i = (uint8_t *)cbINV[0];
        uint8_t* cb8_1_i = (uint8_t *)cbINV[1];

        uint8_t ampl = 0;
        uint8_t max_ampl = video_mode.LVL_C_MAX;

        // изменение функций синуса и косинуса(доворот на пи/4)
        if (tv_out_mode.tv_system == TV_SYS_PAL) {
            float Q = 0.7;
            float I = 0.7;
            for (int i = 0; i < cycle_size; i++) {
                cos[i] = cos[i] * Q - sin[i] * I;
                sin[i] = cos[i] * I + sin[i] * Q;
            }

            ph = 3; //3
            Q = 1;
            I = 0;
            //  Q=0.8;
            //  I=-0.1;
            if (tv_out_mode.cb_sync_PI_shift_lines) dph = 3;

            for (int i = 0; i < 40; i++) {
                ampl = max_ampl * 1;
                if (i < cycle_size * 1) ampl = i * max_ampl / cycle_size;
                if (i > (cycle_size * 9)) ampl = (cycle_size * 10 - i) * (max_ampl) / cycle_size;

                if (tv_out_mode.color_index == 0) ampl = 0; //полное отклюение цвета

                int bb = ampl * (Q * sin[(i + ph) % 4] + I * cos[(i + ph) % 4]) + 0.0;

                bb = (bb > max_ampl) ? max_ampl : bb;
                bb = (bb < -max_ampl) ? -max_ampl : bb;
                cb8_0[i] = max_ampl + bb;

                bb = ampl * (Q * sin[(i + ph + dph) % 4] + I * cos[(i + ph + dph) % 4]) + 0.0;

                bb = (bb > max_ampl) ? max_ampl : bb;
                bb = (bb < -max_ampl) ? -max_ampl : bb;
                cb8_1[i] = max_ampl + bb;


                cb8_0[i] = CONV_DAC(cb8_0[i]) | (1 << SYNC_PIN);
                cb8_1[i] = CONV_DAC(cb8_1[i]) | (1 << SYNC_PIN);

                //инверсная вспышка
                bb = -ampl * (Q * sin[(i + ph) % 4] + I * cos[(i + ph) % 4]) + 0.0;

                bb = (bb > max_ampl) ? max_ampl : bb;
                bb = (bb < -max_ampl) ? -max_ampl : bb;
                cb8_0_i[i] = max_ampl + bb;

                bb = -ampl * (Q * sin[(i + ph + dph) % 4] + I * cos[(i + ph + dph) % 4]) + 0.0;

                bb = (bb > max_ampl) ? max_ampl : bb;
                bb = (bb < -max_ampl) ? -max_ampl : bb;
                cb8_1_i[i] = max_ampl + bb;


                cb8_0_i[i] = CONV_DAC(cb8_0_i[i]) | (1 << SYNC_PIN);
                cb8_1_i[i] = CONV_DAC(cb8_1_i[i]) | (1 << SYNC_PIN);
            }
        } else { // NTSC
            float Q = 0.4127 * (B - Y) + 0.4778 * (R - Y);
            float I = -0.268 * (B - Y) + 0.7358 * (R - Y);
            int ph = 3;
            for (int i = 0; i < 4; i++) {
                float k = 1.5 * tv_out_mode.color_index;
                int max_v = video_mode.LVL_C_MAX;
                int C = k * max_v * (Q * sin[(i + ph) % 4] + I * cos[(i + ph) % 4]);
                C = C < -max_v ? -max_v : C;
                C = C >  max_v ?  max_v : C;
                cd0[i] = C;
                cd1[i] = -C;
            }            
            // NTSC burst
            uint8_t* cb8_0 = (uint8_t *)cb[0];
            uint8_t* cb8_1 = (uint8_t *)cb[1];
            uint8_t* cb8_0_i = (uint8_t *)cbINV[0];
            uint8_t* cb8_1_i = (uint8_t *)cbINV[1];
            int max_ampl = video_mode.LVL_C_MAX;
            Q = 1;
            I = -1;
            for (int i = 0; i < 40; i++) {
                int ampl = max_ampl / 2;
                if (i < cycle_size * 1) ampl = i * max_ampl / cycle_size;
                if (i > (cycle_size * 9)) ampl = (cycle_size * 10 - i) * max_ampl / cycle_size;
                if (tv_out_mode.color_index == 0) ampl = 0;
                int dd = ampl * (Q * sin[i % 4] + I * cos[i % 4]);
                dd = dd >  max_ampl ?  max_ampl : dd;
                dd = dd < -max_ampl ? -max_ampl : dd;
                cb8_0[i] = max_ampl + dd;
                cb8_1[i] = max_ampl - dd;
                cb8_0[i] = CONV_DAC(cb8_0[i]) | (1 << SYNC_PIN);
                cb8_1[i] = CONV_DAC(cb8_1[i]) | (1 << SYNC_PIN);
                // для NTSC отдельная INV-логика не нужна, но таблицы должны быть валидны
                cb8_0_i[i] = cb8_0[i];
                cb8_1_i[i] = cb8_1[i];
            }
        }
    }

    uint32_t Y32 = (Y8 << 24) | (Y8 << 16) | (Y8 << 8) | (Y8 << 0);
    int8_t* yi = (int8_t*)&Y32;
    int8_t* ci = (int8_t*)&cd0_32;

    for (int i = 0; i < 4; i++) { yi[i] = CONV_DAC(yi[i]+ci[i]) | (1 << SYNC_PIN); };
    conv_color[0][i] = Y32;

    Y32 = (Y8 << 24) | (Y8 << 16) | (Y8 << 8) | (Y8 << 0);
    ci = (int8_t*)&cd1_32;
    for (int i = 0; i < 4; i++) { yi[i] = CONV_DAC(yi[i]+ci[i]) | (1 << SYNC_PIN); };
    conv_color[1][i] = Y32;

    //цвет со сдвигом фазы
    uint32_t c32 = conv_color[0][i];
    conv_colorINV[0][i] = (c32 >> 16) | ((c32 & 0xffff) << 16);
    c32 = conv_color[1][i];
    conv_colorINV[1][i] = (c32 >> 16) | ((c32 & 0xffff) << 16);
}

static inline void* __not_in_flash_func(nf_memset)(void* ptr, int value, size_t len)
{
    uint8_t* p = (uint8_t*)ptr;
    uint8_t v8 = (uint8_t)value;

    // --- выравниваем до 4 байт ---
    while (len && ((uintptr_t)p & 3)) {
        *p++ = v8;
        len--;
    }

    // --- основной 32-битный цикл ---
    if (len >= 4) {
        uint32_t v32 = v8;
        v32 |= v32 << 8;
        v32 |= v32 << 16;

        uint32_t* p32 = (uint32_t*)p;
        size_t n32 = len >> 2;

        while (n32--) {
            *p32++ = v32;
        }

        p = (uint8_t*)p32;
        len &= 3;
    }

    // --- хвост ---
    while (len--) {
        *p++ = v8;
    }

    return ptr;
}

//основная функция заполнения буферов видеоданных
static bool __time_critical_func(video_timer_callbackTV)(repeating_timer_t* rt) {
    static uint dma_inx_out = 0;
    static uint lines_buf_inx = 0;

    if (dma_chan_ctrl == -1) return 1; //не определен дма канал

    //получаем индекс выводимой строки
    uint dma_inx = (N_LINE_BUF_DMA - 2 + ((dma_channel_hw_addr(dma_chan_ctrl)->read_addr - (uint32_t)rd_addr_DMA_CTRL) /
                                          4)) % (N_LINE_BUF_DMA);

    //uint n_loop=(N_LINE_BUF_DMA+dma_inx-dma_inx_out)%N_LINE_BUF_DMA;

    static uint32_t line_active = 0;
    static uint32_t frame_i = 0;
    static uint32_t g_str_index = 1;
    //while(n_loop--)
    while (dma_inx_out != dma_inx) {
        //режим VGA
        line_active++;
        g_str_index++;

        int dec_str = 0;

        if (line_active >= video_mode.N_lines) {
            line_active = 0;
            frame_i++;
            bk_vsync();
        }

        lines_buf_inx = (lines_buf_inx + 1) % N_LINE_BUF;
        uint8_t* output_buffer8 = (uint8_t *)lines_buf[lines_buf_inx];

        bool is_line_visible = true;
        switch (line_active) {
            case 0:
            case 1:
                //|___|--|___|--| уравнивающие
                nf_memset(output_buffer8, video_mode.SYNC_TMPL, (video_mode.H_len / 2) - video_mode.sync_size);
                output_buffer8 += (video_mode.H_len / 2) - video_mode.sync_size;
                nf_memset(output_buffer8, video_mode.NO_SYNC_TMPL, video_mode.sync_size);
                output_buffer8 += video_mode.sync_size;
                nf_memset(output_buffer8, video_mode.SYNC_TMPL, (video_mode.H_len / 2) - video_mode.sync_size);
                output_buffer8 += (video_mode.H_len / 2) - video_mode.sync_size;
                nf_memset(output_buffer8, video_mode.NO_SYNC_TMPL, video_mode.sync_size);
                is_line_visible = false;
                break;

            case 2:
                // ____|--|_|---- переходная
                nf_memset(output_buffer8, video_mode.SYNC_TMPL, (video_mode.H_len / 2) - video_mode.sync_size);
                output_buffer8 += (video_mode.H_len / 2) - video_mode.sync_size;
                nf_memset(output_buffer8, video_mode.NO_SYNC_TMPL, video_mode.sync_size);
                output_buffer8 += video_mode.sync_size;
                nf_memset(output_buffer8, video_mode.SYNC_TMPL, video_mode.sync_size / 2);
                output_buffer8 += video_mode.sync_size / 2;
                nf_memset(output_buffer8, video_mode.NO_SYNC_TMPL,
                        (video_mode.H_len / 2) - (video_mode.sync_size / 2));
                is_line_visible = false;
                break;

            case 3:
            case 4:
                //|_|----|_|---- уравнивающие
                nf_memset(output_buffer8, video_mode.SYNC_TMPL, video_mode.sync_size / 2);
                output_buffer8 += video_mode.sync_size / 2;
                nf_memset(output_buffer8, video_mode.NO_SYNC_TMPL,
                        (video_mode.H_len / 2) - (video_mode.sync_size / 2));
                output_buffer8 += (video_mode.H_len / 2) - (video_mode.sync_size / 2);
                nf_memset(output_buffer8, video_mode.SYNC_TMPL, video_mode.sync_size / 2);
                output_buffer8 += video_mode.sync_size / 2;
                nf_memset(output_buffer8, video_mode.NO_SYNC_TMPL,
                        (video_mode.H_len / 2) - (video_mode.sync_size / 2));
                is_line_visible = false;
                break;

            case 5: break; // первая видимая строка: шаблон как у видимой, без картинки

            // строки 6–311: видимые — обрабатываются ниже (is_line_visible = true)
        }

        int li = 0;
        if (tv_out_mode.tv_system == TV_SYS_PAL) {
            li = g_str_index & 1;

            if (tv_out_mode.cb_sync_PI_shift_lines) {
                if (tv_out_mode.cb_sync_PI_shift_half_frame) {
                    if (line_active == 0) {
                        dec_str += 2;
                        static bool is_inv;
                        if (is_inv) {
                            cb[0] = cbINV[0];
                            conv_color[0] = conv_colorINV[0];

                            cb[1] = cbINV[1];
                            conv_color[1] = conv_colorINV[1];
                        }
                        else {
                            cb[0] = cbNORM[0];
                            conv_color[0] = conv_colorNORM[0];
                            cb[1] = cbNORM[1];
                            conv_color[1] = conv_colorNORM[1];
                        }
                        is_inv = !is_inv;
                    } //нейтрализация сдвига фазы "лишней строки"(не кратной 4)
                }
            }
            else {
                if (tv_out_mode.cb_sync_PI_shift_half_frame) {
                    if ((line_active == 0)) {
                        g_str_index += 2;
                        dec_str += 2;
                    }
                }
                dec_str += 1;
                switch (g_str_index & 3) {
                    case 0:
                        cb[0] = cbNORM[0];
                        conv_color[0] = conv_colorNORM[0];
                        break;
                    case 3:
                        cb[1] = cbNORM[1];
                        conv_color[1] = conv_colorNORM[1];
                        break;
                    case 2:
                        cb[0] = cbINV[0];
                        conv_color[0] = conv_colorINV[0];
                        break;
                    case 1:
                        cb[1] = cbINV[1];
                        conv_color[1] = conv_colorINV[1];
                    default:
                        break;
                }
            }
        } else { // NTSC
            if (tv_out_mode.cb_sync_PI_shift_lines) {
                dec_str = 2;
            } else {
                g_str_index++;
            }
            if (tv_out_mode.cb_sync_PI_shift_half_frame && line_active == 0) {
                dec_str ^= 2;
                g_str_index--;
            }
            li = g_str_index & 1;            
        }


        //ТВ строка с изображением
        if (is_line_visible) {
            nf_memset(output_buffer8, video_mode.SYNC_TMPL, video_mode.sync_size);
            nf_memset(output_buffer8 + video_mode.sync_size, video_mode.NO_SYNC_TMPL,
                   video_mode.begin_img_shx - video_mode.sync_size);
            int post_img_clear = 60;
            nf_memset(output_buffer8 + (video_mode.H_len - post_img_clear), video_mode.NO_SYNC_TMPL, post_img_clear);

            // цветовая вспышка
            int mul_sh = (tv_out_mode.tv_system == TV_SYS_NTSC) ? 19 : 23;; // сдвиг вспышки для более высокой частоты
            if (li) memcpy(output_buffer8 + 0 + mul_sh * 4, cb[1], 40);
            else memcpy(output_buffer8 + 0 + mul_sh * 4, cb[0], 40);

            output_buffer8 += video_mode.begin_img_shx;
            //di коэффициент сжатия с учётом количества строк и частоты поднесущей
         //   uint16_t di = 0x100;
            int d_end = 4;
            int y = -1;
         //   di = (graphics_buffer.width << 8) / (video_mode.img_W - d_end); // 0xD7 / 2;
            if (tv_out_mode.tv_system == TV_SYS_PAL) {
                if ((line_active > 4) && (line_active < 310)) {
                    y = line_active - 23;
                }
                y -= graphics_buffer.shift_y;
            } else { // NTSC
                if ((line_active > 5) && (line_active < 250)) {
                    y = line_active - 10; // 18;
                }
                 // W/A: ignore shift_y for NTSC
            }
            if ((y >= graphics_buffer.height) || (y < 0) || (graphics_buffer.data == NULL)) {
                //вне изображения
                nf_memset(output_buffer8, video_mode.LVL_BLACK_TMPL, video_mode.img_W);
            }
            else {
                //зона изображения
                uint ibuf = 0;
                // int next_ibuf=0;
                int next_ibuf = 0x100;

                uint32_t* out_buf32 = (uint32_t *)lines_buf[lines_buf_inx];
                out_buf32 += video_mode.begin_img_shx / 4;

                if (graphics_buffer.data != NULL || text_buffer != NULL )
                    switch (tv_out_mode.mode_bpp) {
                        default: {
                            output_buffer8 += 8;
                            for (int x = 0; x < TEXTMODE_COLS; x++) {
                                const uint16_t offset = y / 8 * (TEXTMODE_COLS * 2) + x * 2;
                                const uint8_t c = text_buffer[offset];
                                const uint8_t colorIndex = text_buffer[offset + 1];
                                uint8_t glyph_row = font_6x8[c * 8 + y % 8];
                                for (int bit = 6; bit--;) {
                                    uint32_t cout32 = conv_color[li][glyph_row & 1
                                                                         ? textmode_palette[colorIndex & 0xf] //цвет шрифта
                                                                         : textmode_palette[colorIndex >> 4] //цвет фона
                                    ];
                                    uint8_t* c_4 = (uint8_t*)&cout32;
                                    uint8_t c = c_4[bit % 4];
                                    *output_buffer8++ = c;
                                    *output_buffer8++ = c;
                                    *output_buffer8++ = c;
                                    glyph_row >>= 1;
                                }
                            }
                        }
                        break;
                        case BK_512x256x1: {
                            uint8_t* input_buffer8 = bk_get_line(y);
                            uint8_t y_black = video_mode.LVL_BLACK_TMPL;
                            uint8_t y_gray  = CONV_DAC(video_mode.LVL_BLACK + 20 / (4 - pallete_mask)) | (1 << SYNC_PIN);
                            uint8_t y_white = CONV_DAC(video_mode.LVL_BLACK + 40 / (4 - pallete_mask)) | (1 << SYNC_PIN);
                            int clk_pixel = 0;
                            const int clk_total_pixels = video_mode.img_W - d_end;
                            uint8_t packed = *input_buffer8++;
                            int subpixel = 0;
                            const int logical_total_pixels = 512;
                            int logical_pixel = 0;
                            while (clk_pixel < clk_total_pixels && logical_pixel < logical_total_pixels) {
                                // --- первый пиксель ---
                                uint8_t a_bit = (packed >> (subpixel++)) & 1;
                                uint8_t a = a_bit ? y_white : y_black;
                                if (subpixel == 8) {
                                    subpixel = 0;
                                    packed = *input_buffer8++;
                                }
                                logical_pixel++;
                                // если это последний пиксель — просто вывести
                                if (logical_pixel >= logical_total_pixels) {
                                    *output_buffer8++ = a;
                                    clk_pixel++;
                                    break;
                                }
                                // --- второй пиксель ---
                                uint8_t b_bit = (packed >> (subpixel++)) & 1;
                                uint8_t b = b_bit ? y_white : y_black;
                                if (subpixel == 8) {
                                    subpixel = 0;
                                    packed = *input_buffer8++;
                                }
                                logical_pixel++;
                                // --- вывод (A, blend, B) ---
                                if (clk_pixel < clk_total_pixels) {
                                    *output_buffer8++ = a;
                                    clk_pixel++;
                                }
                                if (clk_pixel < clk_total_pixels) {
                                    uint8_t mid = (a == b) ? a : y_gray;
                                    *output_buffer8++ = mid;
                                    clk_pixel++;
                                }
                                if (clk_pixel < clk_total_pixels) {
                                    *output_buffer8++ = b;
                                    clk_pixel++;
                                }
                            }
                            // добивка
                            while (clk_pixel < clk_total_pixels) {
                                *output_buffer8++ = y_black;
                                ++clk_pixel;
                            }
                        }
                        break;
                        case BK_256x256x2: {
                            uint8_t* input_buffer8 = bk_get_line(y);
                            uint32_t lut32;
                            switch(g_conf.graphics_pallette_idx) {
                                case 15: // WGC
                                    if (pallete_mask == 3)
                                        lut32 = ((215 << 24) | (220 << 16) | (224 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((207 << 24) | (202 << 16) | (203 << 8) | 200);
                                    else 
                                        lut32 = ((196 << 24) | (217 << 16) | (197 << 8) | 200);
                                    break;
                                case 14: // WGY
                                    if (pallete_mask == 3)
                                        lut32 = ((215 << 24) | (220 << 16) | (222 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((207 << 24) | (202 << 16) | (214 << 8) | 200);
                                    else 
                                        lut32 = ((196 << 24) | (217 << 16) | (199 << 8) | 200);
                                    break;
                                case 13: // WYC
                                    if (pallete_mask == 3)
                                        lut32 = ((215 << 24) | (222 << 16) | (224 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((207 << 24) | (214 << 16) | (203 << 8) | 200);
                                    else 
                                        lut32 = ((196 << 24) | (199 << 16) | (197 << 8) | 200);
                                    break;
                                case 12: // CGR
                                    if (pallete_mask == 3)
                                        lut32 = ((224 << 24) | (220 << 16) | (221 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((203 << 24) | (202 << 16) | (204 << 8) | 200);
                                    else 
                                        lut32 = ((197 << 24) | (217 << 16) | (218 << 8) | 200);
                                    break;
                                case 11: // RYC
                                    if (pallete_mask == 3)
                                        lut32 = ((221 << 24) | (222 << 16) | (224 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((204 << 24) | (214 << 16) | (203 << 8) | 200);
                                    else 
                                        lut32 = ((218 << 24) | (199 << 16) | (197 << 8) | 200);
                                    break;
                                case 10: // R-BrGr
                                    if (pallete_mask == 3)
                                        lut32 = ((204 << 24) | (186 << 16) | (192 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((218 << 24) | (184 << 16) | (190 << 8) | 200);
                                    else 
                                        lut32 = ((194 << 24) | (182 << 16) | (188 << 8) | 200);
                                    break;
                                case 9: // R-Br-Gr-
                                    if (pallete_mask == 3)
                                        lut32 = ((218 << 24) | (185 << 16) | (191 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((194 << 24) | (183 << 16) | (189 << 8) | 200);
                                    else 
                                        lut32 = ((193 << 24) | (181 << 16) | (187 << 8) | 200);
                                    break;
                                case 8: // MBr-Br
                                    if (pallete_mask == 3)
                                        lut32 = ((223 << 24) | (185 << 16) | (186 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((205 << 24) | (183 << 16) | (184 << 8) | 200);
                                    else 
                                        lut32 = ((198 << 24) | (181 << 16) | (182 << 8) | 200);
                                    break;
                                case 7: // YGr-Gr
                                    if (pallete_mask == 3)
                                        lut32 = ((222 << 24) | (191 << 16) | (192 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((214 << 24) | (189 << 16) | (190 << 8) | 200);
                                    else 
                                        lut32 = ((199 << 24) | (187 << 16) | (188 << 8) | 200);
                                    break;
                                case 6: // RR--R-
                                    if (pallete_mask == 3)
                                        lut32 = ((221 << 24) | (218 << 16) | (204 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((204 << 24) | (194 << 16) | (218 << 8) | 200);
                                    else 
                                        lut32 = ((218 << 24) | (193 << 16) | (194 << 8) | 200);
                                    break;
                                case 5: // WWW
                                    if (pallete_mask == 3)
                                        lut32 = ((215 << 24) | (215 << 16) | (215 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((207 << 24) | (207 << 16) | (207 << 8) | 200);
                                    else 
                                        lut32 = ((196 << 24) | (196 << 16) | (196 << 8) | 200);
                                    break;
                                case 4: // WCM
                                    if (pallete_mask == 3)
                                        lut32 = ((215 << 24) | (224 << 16) | (223 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((207 << 24) | (203 << 16) | (205 << 8) | 200);
                                    else 
                                        lut32 = ((196 << 24) | (197 << 16) | (198 << 8) | 200);
                                    break;
                                case 3: // YCG
                                    if (pallete_mask == 3)
                                        lut32 = ((222 << 24) | (224 << 16) | (220 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((214 << 24) | (203 << 16) | (202 << 8) | 200);
                                    else 
                                        lut32 = ((199 << 24) | (197 << 16) | (217 << 8) | 200);
                                    break;
                                case 2: // MBC
                                    if (pallete_mask == 3)
                                        lut32 = ((223 << 24) | (219 << 16) | (224 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((205 << 24) | (201 << 16) | (203 << 8) | 200);
                                    else 
                                        lut32 = ((198 << 24) | (216 << 16) | (197 << 8) | 200);
                                    break;
                                case 1: // RMY
                                    if (pallete_mask == 3)
                                        lut32 = ((221 << 24) | (223 << 16) | (222 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((204 << 24) | (205 << 16) | (214 << 8) | 200);
                                    else 
                                        lut32 = ((218 << 24) | (198 << 16) | (199 << 8) | 200);
                                    break;
                                case 0: // RGB
                                default:
                                    if (pallete_mask == 3)
                                        lut32 = ((221 << 24) | (220 << 16) | (219 << 8) | 200);
                                    else if(pallete_mask == 2)
                                        lut32 = ((204 << 24) | (202 << 16) | (201 << 8) | 200);
                                    else 
                                        lut32 = ((218 << 24) | (217 << 16) | (216 << 8) | 200);
                            }
                            uint8_t* lut = (uint8_t*)&lut32;
                            uint8_t packed = *input_buffer8++;
                            int subpixel = 0;
                            int clk_pixel = 0;
                            const int logical_total_pixels = 256;
                            const int clk_total_pixels = video_mode.img_W - d_end;
                            uint32_t cout32 = conv_color[li][200]; // initial black
                            uint8_t* c_4 = (uint8_t*)&cout32; // bytes alias for cout32
                            for (int logical_pixel = 0; clk_pixel < clk_total_pixels && logical_pixel < logical_total_pixels; ++logical_pixel) {
                                uint8_t color2bpp = (packed >> (subpixel++ * 2)) & 0x3;
                                uint8_t color = lut[color2bpp];
                                cout32 = conv_color[li][color]; // палитру можно менять только на границе 4-ёх фаз ?
                                *output_buffer8++ = c_4[clk_pixel++ & 3];
                                *output_buffer8++ = c_4[clk_pixel++ & 3];
                                *output_buffer8++ = c_4[clk_pixel++ & 3];
//                                *output_buffer8++ = c_4[clk_pixel++ % 4];
                                if (subpixel == 4) {
                                    subpixel = 0;
                                    packed = *input_buffer8++;
                                }
                            }
                            cout32 = conv_color[li][200]; // палитру можно менять только на границе 4-ёх фаз
                            while (clk_pixel < clk_total_pixels) {
                                *output_buffer8++ = c_4[clk_pixel++ & 3];
                            }
                        }
                        break;
                    }
            }
        }

        //управление длиной строки
        transfer_count_DMA_CTRL[dma_inx_out] = video_mode.H_len - dec_str;
        rd_addr_DMA_CTRL[dma_inx_out] = (uint32_t)&lines_buf[lines_buf_inx];
        //включаем заполненный буфер в данные для вывода
        dma_inx_out = (dma_inx_out + 1) % (N_LINE_BUF_DMA);
        dma_inx = (N_LINE_BUF_DMA - 2 + ((dma_channel_hw_addr(dma_chan_ctrl)->read_addr - (uint32_t)rd_addr_DMA_CTRL) /
                                         4)) % (N_LINE_BUF_DMA);
    }
    return true;
}

void graphics_set_buffer(uint8_t* buffer, const uint16_t width, const uint16_t height) {
    graphics_buffer.data = buffer;
    graphics_buffer.width = width;
    graphics_buffer.height = height;
}

//выделение и настройка общих ресурсов - 4 DMA канала, PIO программ и 2 SM
void graphics_init() {
    //настройка PIO
    SM_video = pio_claim_unused_sm(PIO_VIDEO, true);
    //выделение  DMA каналов
    dma_chan_ctrl = dma_claim_unused_channel(true);
    dma_chan_ctrl2 = dma_claim_unused_channel(true);
    dma_chan = dma_claim_unused_channel(true);

    //---------------

    //заполнение палитры по умолчанию(ч.б.)
    for (int ci = 0; ci < 256; ci++) graphics_set_palette(ci, (ci << 16) | (ci << 8) | ci); //


    //настройка рабочей SM TV

    uint offs_prg0 = 0;
    offs_prg0 = pio_add_program(PIO_VIDEO, &program_pio_TV);
    uint16_t* conv_color16 = (uint16_t *)conv_color;

    pio_sm_config c_c = pio_get_default_sm_config();

    sm_config_set_wrap(&c_c, offs_prg0, offs_prg0 + (program_pio_TV.length - 1));
    for (int i = 0; i < 8; i++) {
        gpio_set_slew_rate(TV_BASE_PIN + i, GPIO_SLEW_RATE_FAST);
        pio_gpio_init(PIO_VIDEO, TV_BASE_PIN + i);
        gpio_set_drive_strength(TV_BASE_PIN + i, GPIO_DRIVE_STRENGTH_12MA);
        gpio_set_slew_rate(TV_BASE_PIN + i, GPIO_SLEW_RATE_FAST);
    }
    pio_sm_set_consecutive_pindirs(PIO_VIDEO, SM_video, TV_BASE_PIN, 8, true); //конфигурация пинов на выход
    sm_config_set_out_pins(&c_c, TV_BASE_PIN, 8);

    sm_config_set_out_shift(&c_c, true, true, 8); //16,32
    sm_config_set_fifo_join(&c_c, PIO_FIFO_JOIN_TX);

    pio_sm_init(PIO_VIDEO, SM_video, offs_prg0, &c_c);
    pio_sm_set_enabled(PIO_VIDEO, SM_video, true);

    //установка параметров по умолчанию
    graphics_set_modeTV(TV_OUT_MODE);

    //настройки DMA

    //основной рабочий канал
    dma_channel_config cfg_dma = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_8);
    channel_config_set_chain_to(&cfg_dma, dma_chan_ctrl2); // chain to other channel

    channel_config_set_read_increment(&cfg_dma, true);
    channel_config_set_write_increment(&cfg_dma, false);


    uint dreq = DREQ_PIO1_TX0 + SM_video;
    if (PIO_VIDEO == pio0) dreq = DREQ_PIO0_TX0 + SM_video;
    channel_config_set_dreq(&cfg_dma, dreq);

    dma_channel_configure(
        dma_chan,
        &cfg_dma,
        &PIO_VIDEO->txf[SM_video], // Write address
        lines_buf[0], // read address
        video_mode.H_len / 1, //
        false // Don't start yet
    );

    //контрольный канал для основного(адрес чтения)
    cfg_dma = dma_channel_get_default_config(dma_chan_ctrl);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
    channel_config_set_chain_to(&cfg_dma, dma_chan); // chain to other channel

    channel_config_set_read_increment(&cfg_dma, true);
    channel_config_set_write_increment(&cfg_dma, false);
    channel_config_set_ring(&cfg_dma,false, 2 + N_LINE_BUF_log2);


    dma_channel_configure(
        dma_chan_ctrl,
        &cfg_dma,
        &dma_hw->ch[dma_chan].read_addr, // Write address
        // &dma_hw->ch[dma_chan].al2_transfer_count,
        rd_addr_DMA_CTRL, // read address
        1, //
        false // Don't start yet
    );


    //контрольный канал для основного(количество транзакций)
    cfg_dma = dma_channel_get_default_config(dma_chan_ctrl2);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
    channel_config_set_chain_to(&cfg_dma, dma_chan_ctrl); // chain to other channel

    channel_config_set_read_increment(&cfg_dma, true);
    channel_config_set_write_increment(&cfg_dma, false);
    channel_config_set_ring(&cfg_dma,false, 2 + N_LINE_BUF_log2);

    for (int i = 0; i < N_LINE_BUF * 2; i++) {
        transfer_count_DMA_CTRL[i] = video_mode.H_len / 1;
    }

    dma_channel_configure(
        dma_chan_ctrl2,
        &cfg_dma,
        &dma_hw->ch[dma_chan].transfer_count, // Write address
        // &dma_hw->ch[dma_chan].al2_transfer_count,
        transfer_count_DMA_CTRL, // read address
        1, //
        false // Don't start yet
    );


    dma_start_channel_mask((1u << dma_chan_ctrl2));

    int hz = 30000;
    if (!alarm_pool_add_repeating_timer_us(alarm_pool_create(2, 16), 1000000 / hz, video_timer_callbackTV, NULL,
                                           &video_timer)) {
        return;
    }

    graphics_set_modeTV(TV_OUT_MODE);
    // FIXME сделать конфигурацию пользователем
    graphics_set_palette(200, RGB888(0x00, 0x00, 0x00)); //black
    graphics_set_palette(201, RGB888(0x00, 0x00, 0xC4)); //blue
    graphics_set_palette(202, RGB888(0x00, 0xC4, 0x00)); //green
    graphics_set_palette(203, RGB888(0x00, 0xC4, 0xC4)); //cyan
    graphics_set_palette(204, RGB888(0xC4, 0x00, 0x00)); //red
    graphics_set_palette(205, RGB888(0xC4, 0x00, 0xC4)); //magenta
    graphics_set_palette(206, RGB888(0xC4, 0x7E, 0x00)); //brown
    graphics_set_palette(207, RGB888(0xC4, 0xC4, 0xC4)); //light gray
    graphics_set_palette(208, RGB888(0x4E, 0x4E, 0x4E)); //dark gray
    graphics_set_palette(209, RGB888(0x4E, 0x4E, 0xDC)); //light blue
    graphics_set_palette(210, RGB888(0x4E, 0xDC, 0x4E)); //light green
    graphics_set_palette(211, RGB888(0x4E, 0xF3, 0xF3)); //light cyan
    graphics_set_palette(212, RGB888(0xDC, 0x4E, 0x4E)); //light red
    graphics_set_palette(213, RGB888(0xF3, 0x4E, 0xF3)); //light magenta
    graphics_set_palette(214, RGB888(0xc4, 0xc4, 0x00)); //yellow

    graphics_set_palette(215, RGB888(0xFF, 0xFF, 0xFF)); //white
    graphics_set_palette(192, RGB888(0xC0, 0xFF, 0x00)); //Gr-
    graphics_set_palette(191, RGB888(0x90, 0xFF, 0x00)); //Gr--
    graphics_set_palette(186, RGB888(0xC0, 0x00, 0xFF)); //Br
    graphics_set_palette(185, RGB888(0x90, 0x00, 0xFF)); //Br-
// тёмные
    graphics_set_palette(216, RGB888(0x00, 0x00, 0x74)); //blue
    graphics_set_palette(217, RGB888(0x00, 0x74, 0x00)); //green
    graphics_set_palette(218, RGB888(0x74, 0x00, 0x00)); //red
    graphics_set_palette(199, RGB888(0x74, 0x74, 0x00)); //yellow
    graphics_set_palette(198, RGB888(0x74, 0x00, 0x74)); //magenta
    graphics_set_palette(197, RGB888(0x00, 0x74, 0x74)); //cyan
    graphics_set_palette(196, RGB888(0x74, 0x74, 0x74)); //white
    graphics_set_palette(194, RGB888(0x44, 0x00, 0x00)); //red---
    graphics_set_palette(193, RGB888(0x24, 0x00, 0x00)); //red----
    graphics_set_palette(190, RGB888(0x74, 0xC4, 0x00)); //Gr-
    graphics_set_palette(189, RGB888(0x44, 0xC4, 0x00)); //Gr--
    graphics_set_palette(188, RGB888(0x44, 0x74, 0x00)); //Gr-
    graphics_set_palette(187, RGB888(0x24, 0x74, 0x00)); //Gr--
    graphics_set_palette(184, RGB888(0x74, 0x00, 0xC4)); //Br-
    graphics_set_palette(183, RGB888(0x44, 0x00, 0xC4)); //Br--
    graphics_set_palette(182, RGB888(0x44, 0x00, 0x74)); //Br-
    graphics_set_palette(181, RGB888(0x24, 0x00, 0x74)); //Br--
// светлые
    graphics_set_palette(219, RGB888(0x00, 0x00, 0xFF)); //blue
    graphics_set_palette(220, RGB888(0x00, 0xFF, 0x00)); //green
    graphics_set_palette(221, RGB888(0xFF, 0x00, 0x00)); //red
    graphics_set_palette(222, RGB888(0xFF, 0xFF, 0x00)); //yellow
    graphics_set_palette(223, RGB888(0xFF, 0x00, 0xFF)); //magenta
    graphics_set_palette(224, RGB888(0x00, 0xFF, 0xFF)); //cyan
};

void graphics_set_textbuffer(uint8_t* buffer) {
    text_buffer = buffer;
};

void graphics_set_offset(const int x, const int y) {
    graphics_buffer.shift_x = x;
    graphics_buffer.shift_y = y;
};

void graphics_set_mode(const enum graphics_mode_t mode) {
    TV_OUT_MODE.mode_bpp = mode;
    TV_OUT_MODE.color_index = BK_256x256x2 != mode ? 0.0 : 1.0;
    TV_OUT_MODE.cb_sync_PI_shift_lines = BK_256x256x2 != mode ? true : false;
    graphics_set_modeTV(TV_OUT_MODE);
    clrScr(0);
}

void graphics_set_page(uint8_t* buffer, uint8_t pallette_idx) {
    g_conf.v_buff_offset = buffer - RAM;
    graphics_buffer.data = buffer;
    g_conf.graphics_pallette_idx = pallette_idx;
};

#pragma once
#include <stdint.h>
#include <stddef.h>

typedef enum BK_MODE {
    BK_FDD,
    BK_0010,
    BK_0010_01,
    BK_0011M_FDD,
    BK_0011M
} bk_mode_t;

typedef struct config_em {
    bool is_covox_on;
    bool is_AY_on;
    bool color_mode;
    bk_mode_t bk0010mode;
    int8_t snd_volume;
    uint8_t graphics_pallette_idx;
    uint8_t shift_y;
    uint16_t graphics_buffer_height;
    size_t v_buff_offset;
    uint64_t      cycles_cnt1;
    int_fast32_t  Time;
    uint_fast32_t T;
    uint_fast16_t CodeAndFlags;
    uint_fast16_t Key;
    uint_fast32_t LastKey;
    uint_fast8_t  RunState;
    uint_fast16_t PrevCAF;
} config_em_t;

extern volatile config_em_t g_conf;
extern volatile bool is_dendy_joystick;
extern volatile bool is_kbd_joystick;

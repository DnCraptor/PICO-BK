#pragma once
#include <stdint.h>

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
} config_em_t;

extern config_em_t g_conf;

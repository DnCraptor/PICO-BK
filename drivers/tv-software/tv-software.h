#pragma once

#include "inttypes.h"
#include "stdbool.h"

#define PIO_VIDEO pio0

#define TV_BASE_PIN (beginVGA_PIN)

#define TEXTMODE_COLS 46
#define TEXTMODE_ROWS 30
#define RGB888(r, g, b) ((r<<16) | (g << 8 ) | b )

// TODO: Сделать настраиваемо
static const uint8_t textmode_palette[16] = {
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215
};
static void graphics_set_flashmode(bool flash_line, bool flash_frame) {
    // dummy
}

static void graphics_set_bgcolor(uint32_t color888) {
    // dummy
}

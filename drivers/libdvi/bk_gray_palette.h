// Rec.709 luma levels for each source 2bpp BK palette. Read only on core 0
// before the HDMI core is launched; never used in the scanline hot path.
static const uint8_t bk_gray_levels[16][4] = {
    {  0,  5, 45, 13 },
    {  0, 58, 18, 13 },
    {  0, 50, 45, 13 },
    {  0, 50, 45, 58 },
    {  0, 18, 50, 63 },
    {  0, 63, 63, 63 },
    {  0, 13,  4, 13 },
    {  0, 57, 18, 57 },
    {  0, 18,  6, 18 },
    {  0, 45, 18, 13 },
    {  0, 14, 49, 58 },
    {  0,  5, 58, 13 },
    {  0, 13, 45, 50 },
    {  0, 50, 58, 63 },
    {  0, 63, 45, 58 },
    {  0, 50, 45, 63 }
};

static const uint32_t bk_gray_tmds[64] = {
#include "tmds_table.h"
};

// Prepared on core 0 at startup. These arrays are in .bss (SRAM), not
// .time_critical: that section is reserved for the scanline/IRQ code.
static uint64_t bk_gray_pairs[16][16];
static uint32_t bk_gray_pixels[16][4];

void bk_init_gray_palettes(void) {
    for (unsigned pal = 0; pal < 16; ++pal) {
        for (unsigned pixel = 0; pixel < 4; ++pixel) {
            unsigned y = bk_gray_levels[pal][pixel];
            bk_gray_pixels[pal][pixel] = y == 63 ? 0xbfe00u : bk_gray_tmds[y];
        }
        for (unsigned pair = 0; pair < 16; ++pair) {
            bk_gray_pairs[pal][pair] = (uint64_t)bk_gray_pixels[pal][pair & 3u] |
                ((uint64_t)bk_gray_pixels[pal][pair >> 2u] << 32);
        }
    }
}

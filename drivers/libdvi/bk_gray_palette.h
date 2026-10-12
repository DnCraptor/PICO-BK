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

// Map linear-light Rec.709 luma to display signal levels (gamma ~2.2).
// Prepared once on core 0; the hot TMDS scanline encoder sees only SRAM LUTs.
// 0 and 63 are invariant, and saturated blue/red/green map to 20/31/54.
static const uint8_t bk_gray_display_levels[64] = {
     0, 10, 13, 16, 18, 20, 22, 23,
    25, 26, 27, 28, 30, 31, 32, 33,
    34, 35, 36, 37, 37, 38, 39, 40,
    41, 41, 42, 43, 44, 44, 45, 46,
    46, 47, 48, 48, 49, 49, 50, 51,
    51, 52, 52, 53, 54, 54, 55, 55,
    56, 56, 57, 57, 58, 58, 59, 59,
    60, 60, 61, 61, 62, 62, 63, 63
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
            unsigned y = bk_gray_display_levels[bk_gray_levels[pal][pixel]];
            bk_gray_pixels[pal][pixel] = y == 63 ? 0xbfe00u : bk_gray_tmds[y];
        }
        for (unsigned pair = 0; pair < 16; ++pair) {
            bk_gray_pairs[pal][pair] = (uint64_t)bk_gray_pixels[pal][pair & 3u] |
                ((uint64_t)bk_gray_pixels[pal][pair >> 2u] << 32);
        }
    }
}

#pragma once

/* vga_linux.c: the VGA output of the RP2350 build as a 1024x768 frame. */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VGA_LINUX_WIDTH  1024
#define VGA_LINUX_HEIGHT 768

/* Build one complete 1024x768 XRGB8888 frame (tightly packed) from the
 * current BK video page or text buffer, as the VGA scanline IRQ does. */
void vga_linux_compose(uint32_t *frame);

#ifdef __cplusplus
}
#endif

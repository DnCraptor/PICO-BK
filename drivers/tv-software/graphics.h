#pragma once
/*
 * graphics.h — compatibility shim between tv-software and PICO-BK.
 *
 * Included by tv-software.c in place of its original "graphics.h".
 * Provides everything the translation unit needs:
 *   - TV-specific types and pin macros  (tv-software.h)
 *   - BK graphics buffer / vsync API    (vga.h)
 *   - enum graphics_mode_t + GRAPHICSMODE_DEFAULT
 *
 * Conflict resolutions
 * --------------------
 * tv-software.h defines graphics_set_flashmode() and graphics_set_bgcolor()
 * as static no-ops.  vga.h declares the real implementations.  Since
 * tv-software.c uses PIO0 directly and does not need VGA's flash/bgcolor
 * machinery we keep the static stubs from tv-software.h and suppress vga.h's
 * declarations with a guard macro.
 *
 * tv-software.h references VGA_BASE_PIN; map it to beginVGA_PIN (defined
 * per-board in CMakeLists.txt compile definitions).
 *
 * graphics_set_mode() has a different return type in vga.h (enum) vs
 * tv-software.c (void).  tv-software.c provides its own definition, so we
 * hide vga.h's declaration and cast the return value away via a macro.
 */

/* 1. TV-specific types (g_out_TV_t, NUM_TV_LINES_t, COLOR_FREQ_t, PIO_VIDEO,
 *    TV_BASE_PIN, static stubs for flashmode/bgcolor).
 *    VGA_BASE_PIN is not defined project-wide; alias it to beginVGA_PIN. */
#ifndef VGA_BASE_PIN
#  define VGA_BASE_PIN beginVGA_PIN
#endif

/* Suppress vga.h's conflicting declarations before including it. */
#define graphics_set_flashmode  _vga_graphics_set_flashmode
#define graphics_set_bgcolor    _vga_graphics_set_bgcolor
#define graphics_set_mode       _vga_graphics_set_mode

#include "vga.h"   /* enum graphics_mode_t, get_graphics_buffer(), vsync_ptr */

#undef graphics_set_flashmode
#undef graphics_set_bgcolor
#undef graphics_set_mode

/* Now include tv-software.h which re-defines the stubs as static functions
 * and provides all TV-specific enum/macro definitions. */
#include "tv-software.h"

/* GRAPHICSMODE_DEFAULT is the token tv-software.c uses in tv_out_mode_t.
 * Map to BK_256x256x2; the actual render dispatch uses get_graphics_mode(). */
#define GRAPHICSMODE_DEFAULT  BK_256x256x2

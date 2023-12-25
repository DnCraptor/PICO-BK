#pragma once

#include "inttypes.h"
#include "stdbool.h"

#define PIO_VGA (pio0)
#define beginVGA_PIN (6)
#define VGA_DMA_IRQ (DMA_IRQ_0)

enum graphics_mode_t {
    TEXTMODE_80x30,
    TEXTMODE_,
    BK_256x256x2,
    BK_512x256x1,
};

extern volatile uint16_t true_covox;
extern bool color_mode;
extern int pallete_mask;

void graphics_inc_palleter_offset();

void graphics_init();

void graphics_set_buffer(uint8_t *buffer, uint16_t width, uint16_t height);

void graphics_set_textbuffer(uint8_t *buffer);

void graphics_set_offset(int x, int y);

void graphics_shift_screen(uint16_t Word);

enum graphics_mode_t graphics_set_mode(enum graphics_mode_t mode);

void graphics_set_flashmode(bool flash_line, bool flash_frame);

void graphics_set_bgcolor(uint32_t color888);

void clrScr(uint8_t color);

void draw_text(char *string, int x, int y, uint8_t color, uint8_t bgcolor);

void logMsg(char * msg);

void set_start_debug_line(int _start_debug_line);

char* get_free_vram_ptr();

bool save_video_ram();

bool restore_video_ram();

void graphics_set_page(uint8_t* buffer, uint8_t pallette_idx);

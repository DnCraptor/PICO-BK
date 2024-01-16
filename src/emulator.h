#pragma once
#define INLINE static

// Settings for max 4MB 0f PSRAM
#define TOTAL_VIRTUAL_MEMORY_KBS (4ul << 10)
#define ON_BOARD_RAM_KB 1024ul
#define BASE_X86_KB 1024ul

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include "ram_page.h"
#include <hardware/pwm.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <pico/stdlib.h>

#include "f_util.h"
#include "ff.h"

#define BEEPER_PIN 28

extern uint8_t RAM[RAM_SIZE];
extern uint8_t TEXT_VIDEO_RAM[VIDEORAM_SIZE];
extern bool PSRAM_AVAILABLE;
extern bool SD_CARD_AVAILABLE;
extern uint32_t DIRECT_RAM_BORDER;

extern int timer_period;
extern pwm_config config;

// TODO: remove this trash
void logMsg(char*);

uint8_t insertdisk(uint8_t drivenum, size_t size, char* ROM, char* pathname);

uint8_t insermouse(uint16_t portnum);

void outsermouse(uint16_t portnum, uint8_t value);

//void sermouseevent(uint8_t buttons, int8_t xrel, int8_t yrel);

void initsermouse(uint16_t baseport, uint8_t irq);

void dss_out(uint16_t portnum, uint8_t value);

uint8_t dss_in(uint16_t portnum);

extern uint8_t inadlib(uint16_t portnum);

extern void initadlib(uint16_t baseport);

extern void outadlib(uint16_t portnum, uint8_t value);

extern void tickadlib(void);
extern int16_t adlibgensample(void);


extern int16_t getBlasterSample(void);

extern void tickBlaster(void);

extern void initBlaster(uint16_t baseport, uint8_t irq);

extern void outBlaster(uint16_t portnum, uint8_t value);

extern uint8_t inBlaster(uint16_t portnum);

void sn76489_out( uint16_t value);
void sn76489_reset();

void sn76489_sample_stereo(int32_t out[2]);

void cms_out(uint16_t addr, uint16_t value);
uint8_t cms_in(uint16_t addr);

extern struct i8259_s {
    uint8_t imr; //mask register
    uint8_t irr; //request register
    uint8_t isr; //service register
    uint8_t icwstep; //used during initialization to keep track of which ICW we're at
    uint8_t icw[5];
    uint8_t intoffset; //interrupt vector offset
    uint8_t priority; //which IRQ has highest priority
    uint8_t autoeoi; //automatic EOI mode
    uint8_t readmode; //remember what to return on read register from OCW3
    uint8_t enabled;
} i8259;

extern struct i8253_s {
    uint16_t chandata[3];
    uint8_t accessmode[3];
    uint8_t bytetoggle[3];
    uint32_t effectivedata[3];
    float chanfreq[3];
    uint8_t active[3];
    uint16_t counter[3];
} i8253;

#define rgb(r, g, b) ((r<<16) | (g << 8 ) | b )
#define rgb1(b, g, r) r | (g<<8) | (b<<16);

void detect_os_type(const char* path, char* os_type, size_t sz);

#if EXT_DRIVES_MOUNT
bool mount_img(const char* path);
#endif
void m_cleanup_ext();
int m_add_file_ext(size_t i, const char* fname);
void m_set_file_attr(size_t i, int c, const char* str);
const char* m_get_file_data(size_t i);
bool is_browse_os_supported();


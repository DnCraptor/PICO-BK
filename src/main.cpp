extern "C" {
#include "config_em.h"
#include "emulator.h"
#include "manager.h"
#include "debug.h"
}

#include "disk_img.h"
#include <pico.h>
#include <pico/stdlib.h>

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "audio_linux.h"
#include "input_linux.h"
#include "platform_linux.h"
#include "video_drm.h"
#include "vga_linux.h"
#include "nespad.h"
#include "vga.h"
#include "ps2.h"
#include "CPU.h"
#include "main_i.h"
#include "emu_e.h"
#include "aySoundSoft.h"
#include <stdlib.h>
#include "fdd.h"
#include "ram_config.h"
}

volatile config_em_t g_conf {
   false, // is_covox_on
   true, // is_AY_on
   BK_COLOR_RGB, // color_mode
   BK_FDD, // bk0010mode
   0, // snd_volume
   0, // graphics_pallette_idx
   0330, // shift_y
   256, // graphics_buffer_height
   0, // v_buff_offset
   0, // cpu_freq
   0, // T
   0, // CodeAndFlags
   0, // Key
   0, // LastKey
   0, // RunState
   0, // PrevCAF
   0, // manager_pallette_idx
   false, // is_128_48
   0, // dvi_mode
   false, // is_8x8
};

/* The Linux host shows the VGA picture (1024x768) and keeps the whole BK
 * RAM in process memory, as the RP2350 does with PSRAM. */
bool SELECT_VGA = true; // C linkage: declared in vga.h
bool PSRAM_AVAILABLE = true;
bool SD_CARD_AVAILABLE = false;
uint32_t DIRECT_RAM_BORDER = PSRAM_AVAILABLE ? RAM_SIZE : (SD_CARD_AVAILABLE ? RAM_PAGE_SIZE : RAM_SIZE);

uint8_t __aligned(4096) RAM[RAM_SIZE] = { 0 };
uint8_t __aligned(4) TEXT_VIDEO_RAM[128*96*2] = { 0 };

/*
 * Sound: one stereo sample per 1/44100 s of emulated time (emu_e.c), mixed
 * as the RP2350 audio timer does for its HDMI/I2S outputs: AY-3-8910, Covox
 * and the beeper, scaled by the volume setting.
 */
extern "C" void bk_audio_sample(void) {
    uint16_t outL = 0;
    uint16_t outR = 0;
    #ifdef AYSOUND
    if (g_conf.is_AY_on) {
        uint8_t* AY_data = get_AY_Out(5);
        outL = ((uint16_t)AY_data[0] + AY_data[1]) << 1;
        outR = ((uint16_t)AY_data[2] + AY_data[1]) << 1;
    }
    #endif
#ifdef COVOX
    if (!outL && !outR && g_conf.is_covox_on && (true_covox || az_covox_L || az_covox_R)) {
        outL = (az_covox_L + (uint16_t)true_covox);
        outR = (az_covox_R + (uint16_t)true_covox);
    }
#endif
    const int8_t volume = g_conf.snd_volume;
    if (outR || outL) {
        if (volume >= 0) {
            const uint8_t mult = volume > 5 ? 5 : (uint8_t)volume;
            outL <<= mult;
            outR <<= mult;
        } else {
            const uint8_t div = -volume > 16 ? 16 : (uint8_t)-volume;
            outL >>= div;
            outR >>= div;
        }
    }
    // Beeper/AY ratio as on PWM: beep contributes 1 << (volume + 7).
    const int beep_shift = volume + 7;
    const uint16_t beep_add = (beeper_on && beep_shift >= 0) ? (uint16_t)(1u << beep_shift) : 0u;
    const int16_t sampleL = (int16_t)((int32_t)(outL + beep_add) * 6);
    const int16_t sampleR = (int16_t)((int32_t)(outR + beep_add) * 6);
    audio_push(sampleL, sampleR);
}

extern "C" FATFS fatfs;

static void init_fs() {
    FRESULT result = f_mount(&fatfs, "", 1);
    if (FR_OK != result) {
        char tmp[80];
        snprintf(tmp, sizeof(tmp), "Unable to mount SD-card: %s (%d)", FRESULT_str(result), result);
        logMsg(tmp);
    } else {
        SD_CARD_AVAILABLE = true;
    }

    DIRECT_RAM_BORDER = PSRAM_AVAILABLE ? RAM_SIZE : (SD_CARD_AVAILABLE ? RAM_PAGE_SIZE : RAM_SIZE);

    if (SD_CARD_AVAILABLE) {
        DIR dir;
        if (f_opendir(&dir, "\\BK") != FR_OK) {
            f_mkdir("\\BK");
        } else {
            f_closedir(&dir);
        }
        insertdisk(0, fdd0_sz(), fdd0_rom(), (char*)"\\BK\\fdd0.img");
        insertdisk(1, fdd1_sz(), fdd1_rom(), (char*)"\\BK\\fdd1.img");
        insertdisk(2, 819200, 0, (char*)"\\BK\\fdd2.img");
        insertdisk(3, 819200, 0, (char*)"\\BK\\fdd3.img");
        if (!spacePressed)
            read_config("\\BK\\bk.conf");
        ram_config_save();
    } else {
        if (!spacePressed)
            ram_config_load();
    }
}

/* Renderer loop, the role of the RP2350 VGA scanline interrupt: the picture
 * at 60 Hz. */
static void* render_core(void*) {
    static uint32_t frame[VGA_LINUX_WIDTH * VGA_LINUX_HEIGHT];
    const uint64_t frame_us = 16667;
    uint64_t next_frame = time_us_64();
    while (true) {
        platform_service();
        vga_linux_compose(frame);
        video_drm_present(frame, VGA_LINUX_WIDTH, VGA_LINUX_HEIGHT);
        next_frame += frame_us;
        const uint64_t now = time_us_64();
        if (next_frame + frame_us < now)
            next_frame = now;   // fell behind by more than a frame: resync
        sleep_until_us(next_frame);
    }
    return nullptr;
}

static void on_signal(int) {
    platform_request(PLATFORM_QUIT);
}

int main(int argc, char** argv) {
    /* bk [data directory [DRM device]]: the data directory plays the role
     * of the SD card (BK\ disk images, bk.conf, snapshots). */
    const char* root = argc > 1 ? argv[1] : ".";
    const char* drm_device = argc > 2 ? argv[2] : "/dev/dri/card0";
    platform_set_argv(argv);
    ff_set_root(root);

    keyboard_init();

    graphics_init();
    graphics_set_buffer(CPU_PAGE51_MEM_ADR, 512, 256);
    graphics_set_bgcolor(0x80808080);
    graphics_set_offset(0, 0);
    graphics_set_flashmode(true, true);

    if (!video_drm_open(drm_device, VGA_LINUX_WIDTH, VGA_LINUX_HEIGHT)) {
        input_shutdown();
        return EXIT_FAILURE;
    }

    struct sigaction sa = {};
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);

    pthread_t render_thread;
    if (pthread_create(&render_thread, nullptr, render_core, nullptr) != 0) {
        perror("render thread");
        input_shutdown();
        video_drm_close();
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 50; ++i) {
        keyboard_tick();
        sleep_ms(1);
    }

    init_fs();
    audio_init(44100);

    reset(255);
    graphics_set_mode(g_conf.color_mode != BK_COLOR_BW ? BK_256x256x2 : BK_512x256x1);

    emu_start();
    __unreachable();
}

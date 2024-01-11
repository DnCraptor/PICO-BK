extern "C" {
#include "config_em.h"
#include "emulator.h"
#include "manager.h"
}

#include <pico/time.h>
#include <pico/multicore.h>
#include <hardware/pwm.h>
#include "hardware/clocks.h"
#include <pico/stdlib.h>
#include <hardware/vreg.h>
#include <pico/stdio.h>

#include "psram_spi.h"
#include "nespad.h"

extern "C" {
#include "vga.h"
#include "ps2.h"
#include "usb.h"
#include "CPU.h"
#include "main_i.h"
#include "emu_e.h"
}

config_em_t g_conf {
   true, // is_covox_on
   true, // color_mode
   BK_0010_01, // bk0010mode
   7, // snd_volume
};

bool PSRAM_AVAILABLE = false;
bool SD_CARD_AVAILABLE = false;
uint32_t DIRECT_RAM_BORDER = PSRAM_AVAILABLE ? RAM_SIZE : (SD_CARD_AVAILABLE ? RAM_PAGE_SIZE : RAM_SIZE);

uint8_t __aligned(4096) TEXT_VIDEO_RAM[VIDEORAM_SIZE] = { 0 };
uint8_t __aligned(4096) RAM[RAM_SIZE] = { 0 };

pwm_config config = pwm_get_default_config();
#define PWM_PIN0 (26)
#define PWM_PIN1 (27)

void PWM_init_pin(uint8_t pinN) {
    gpio_set_function(pinN, GPIO_FUNC_PWM);
    pwm_config_set_clkdiv(&config, 1.0);
    pwm_config_set_wrap(&config, (1 << 12) - 1); // MAX PWM value
    pwm_init(pwm_gpio_to_slice_num(pinN), &config, true);
}
#if NESPAD_ENABLED
int timer_period = 54925;
#endif
struct semaphore vga_start_semaphore;
/* Renderer loop on Pico's second core */
void __time_critical_func(render_core)() {
    graphics_init();
    graphics_set_buffer(CPU_PAGE51_MEM_ADR, 512, 256);
    graphics_set_textbuffer(TEXT_VIDEO_RAM);
    graphics_set_bgcolor(0x80808080);
    graphics_set_offset(0, 0);
    graphics_set_flashmode(true, true);
    sem_acquire_blocking(&vga_start_semaphore);
#if NESPAD_ENABLED
    uint8_t tick50ms_counter = 0;
    while (true) {
        busy_wait_us(timer_period);
        if (tick50ms_counter % 2 == 0 && nespad_available) {
            nespad_read();
            if (nespad_state) {
                sermouseevent(nespad_state & DPAD_B ? 1 : nespad_state & DPAD_A ? 2 : 0,
                              nespad_state & DPAD_LEFT ? -3 : nespad_state & DPAD_RIGHT ? 3 : 0,
                              nespad_state & DPAD_UP ? -3 : nespad_state & DPAD_DOWN ? 3 : 0);
            }
        }
        if (tick50ms_counter < 20) {
            tick50ms_counter++;
        }
        else {
            tick50ms_counter = 0;
        }
    }
#endif
}

void inInit(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

extern "C" {
#include "aySoundSoft.h"
}

#ifdef SOUND_SYSTEM
bool __not_in_flash_func(AY_timer_callback)(repeating_timer_t *rt) {
    static int16_t outL = 0;  
    static int16_t outR = 0;
    pwm_set_gpio_level(PWM_PIN0, outR); // Право
    pwm_set_gpio_level(PWM_PIN1, outL); // Лево
#ifdef AYSOUND
    if (!g_conf.is_covox_on) {
        uint8_t* AY_data = get_AY_Out(5);
        if (g_conf.snd_volume > 6) {
            register uint8_t mult = g_conf.snd_volume - 6;
            if (mult > 16) mult = 16;
            outL = (int16_t)((AY_data[0] + AY_data[1]) << mult);
            outR = (int16_t)((AY_data[2] + AY_data[1]) << mult);
        } else {
            register int8_t div = 6 - g_conf.snd_volume;
            if (div < 0) div = 0;
            outL = (int16_t)((AY_data[0] + AY_data[1]) >> div);
            outR = (int16_t)((AY_data[2] + AY_data[1]) >> div);
        }
    }
#endif
#ifdef COVOX
    if (g_conf.is_covox_on && true_covox) {
        if (g_conf.snd_volume > 7) {
            register uint8_t mult = g_conf.snd_volume - 7;
            if (mult > 16) mult = 16;
            outL = true_covox << mult;
            outR = true_covox << mult;
        } else {
            register int8_t div = 6 - g_conf.snd_volume;
            if (div < 0) div = 0;
            outL = true_covox >> div;
            outR = true_covox >> div;
        }
    }
#endif
    if (outR || outL) {
        register int32_t v_l = ((int32_t)outL - (int32_t)0x8000);
        register int32_t v_r = ((int32_t)outR - (int32_t)0x8000);
        outL = (int16_t)v_l;
        outR = (int16_t)v_r;
    }
    return true;
}
#endif

int main() {
#if (OVERCLOCKING > 270)
    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(33);
    set_sys_clock_khz(OVERCLOCKING * 1000, true);
#else
    vreg_set_voltage(VREG_VOLTAGE_1_15);
    sleep_ms(33);
    set_sys_clock_khz(270000, true);
#endif
    gpio_set_function(BEEPER_PIN, GPIO_FUNC_PWM);
    pwm_init(pwm_gpio_to_slice_num(BEEPER_PIN), &config, true);

#ifdef SOUND_SYSTEM
    PWM_init_pin(PWM_PIN0);
    PWM_init_pin(PWM_PIN1);
#endif

#if LOAD_WAV_PIO
    //пин ввода звука
    inInit(LOAD_WAV_PIO);
#endif

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    for (int i = 0; i < 6; i++) {
        sleep_ms(23);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(23);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
    }

    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);
    keyboard_init();

    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);

    sleep_ms(50);

    memset(RAM, 0, sizeof RAM);
    memset(TEXT_VIDEO_RAM, 0, sizeof TEXT_VIDEO_RAM);
    graphics_set_mode(g_conf.color_mode ? BK_256x256x2 : BK_512x256x1);

    init_psram();

    FRESULT result = f_mount(&fs, "", 1);
    if (FR_OK != result) {
        char tmp[80];
        sprintf(tmp, "Unable to mount SD-card: %s (%d)", FRESULT_str(result), result);
        logMsg(tmp);
    } else {
        SD_CARD_AVAILABLE = true;
    }

    DIRECT_RAM_BORDER = PSRAM_AVAILABLE ? RAM_SIZE : (SD_CARD_AVAILABLE ? RAM_PAGE_SIZE : RAM_SIZE);

    if (SD_CARD_AVAILABLE) {
        insertdisk(0, fdd0_sz(), fdd0_rom(), "\\BK\\fdd0.img");
        insertdisk(1, fdd1_sz(), fdd1_rom(), "\\BK\\fdd1.img");
        insertdisk(2, 0, 0, "\\BK\\hdd0.img");
        insertdisk(3, 0, 0, "\\BK\\hdd1.img");
    }

#ifdef SOUND_SYSTEM
    repeating_timer_t timer;
	int hz = 44100;	//44000 //44100 //96000 //22050
	// negative timeout means exact delay (rather than delay between callbacks)
	if (!add_repeating_timer_us(-1000000 / hz, AY_timer_callback, NULL, &timer)) {
		logMsg("Failed to add timer");
		return 1;
	}
#endif

    main_init();
    emu_start();
    return 0;
}

extern "C" {
#include "config_em.h"
#include "emulator.h"
#include "manager.h"
#include "debug.h"
}

#include "disk_img.h"
#include <pico/time.h>
#include <pico/sem.h>
#include <pico/multicore.h>
#include <pico.h>
#include <hardware/clocks.h>
#include <hardware/pwm.h>
#include <pico/stdlib.h>
#include <hardware/vreg.h>
#include <pico/stdio.h>

#include "psram_spi.h"

extern "C" {
#include "nespad.h"
#include "util_Wii_Joy.h"
#include "vga.h"
#include "ps2.h"
#include "CPU.h"
#include "main_i.h"
#include "emu_e.h"
#include "aySoundSoft.h"
#include <stdlib.h>
#include "fdd.h"
}

volatile config_em_t g_conf {
   false, // is_covox_on
   true, // is_AY_on
   true, // color_mode
   BK_0011M_FDD, // bk0010mode
   0, // snd_volume
   0, // graphics_pallette_idx
   0330, // shift_y
   256, // graphics_buffer_height
   0, // v_buff_offset
};

extern "C" bool SELECT_VGA = true;
bool PSRAM_AVAILABLE = false;
bool SD_CARD_AVAILABLE = false;
uint32_t DIRECT_RAM_BORDER = PSRAM_AVAILABLE ? RAM_SIZE : (SD_CARD_AVAILABLE ? RAM_PAGE_SIZE : RAM_SIZE);

uint8_t __aligned(4096) RAM[RAM_SIZE] = { 0 };
#if PICO_RP2040
uint8_t __aligned(4) TEXT_VIDEO_RAM[128*48*2] = { 0 };
#else
uint8_t __aligned(4) TEXT_VIDEO_RAM[128*96*2] = { 0 };
#endif

pwm_config config = pwm_get_default_config();

void PWM_init_pin(uint8_t pinN, uint16_t max_lvl) {
    gpio_set_function(pinN, GPIO_FUNC_PWM);
    pwm_config_set_clkdiv(&config, 1.0);
    pwm_config_set_wrap(&config, max_lvl); // MAX PWM value
    pwm_init(pwm_gpio_to_slice_num(pinN), &config, true);
}

#define VREG_VSEL VREG_VOLTAGE_1_60
extern "C" semaphore_t vga_start_semaphore;
extern "C" void dvi_on_core1();
extern "C" void flash_timings();

/* Renderer loop on Pico's second core */
void __time_critical_func(render_core)() {
    graphics_set_buffer(CPU_PAGE51_MEM_ADR, 512, 256);
    if (SELECT_VGA) {
        graphics_init();
        graphics_set_bgcolor(0x80808080);
        graphics_set_offset(0, 0);
        graphics_set_flashmode(true, true);
        sem_acquire_blocking(&vga_start_semaphore);
        return;
    }
    dvi_on_core1();
	__builtin_unreachable();
}

void inInit(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

#ifdef USE_WII
static repeating_timer_t Wii_timer;
static bool __not_in_flash_func(Wii_Joystick_Timer_CB)(repeating_timer_t *rt) {
    if (is_WII_Init()) {
        Wii_decode_joy();
    }
    return true;
}
#endif

#ifdef HWAY
extern "C" {
    #include "PinSerialData_595.h"
}
#endif

#ifdef SOUND_SYSTEM
static repeating_timer_t timer;
static bool __not_in_flash_func(AY_timer_callback)(repeating_timer_t *rt) {
    static uint16_t outL = 0;  
    static uint16_t outR = 0;
#ifdef HWAY
#ifdef COVOX
    static uint8_t latch_val = 0;
    uint8_t val = (outL + outR) >> 6;
    if (g_conf.is_covox_on && latch_val != val) {
        latch_val = val;
        // выбор второго чипа
		HIGH(CS_AY1);
        LOW(CS_AY0);
		// выбор регистра разрешений
    	send_to_595(HIGH (BDIR | BC1) | 7); 
		send_to_595( LOW (BDIR | BC1) | 7);
		// разрешение записи в регистр portB
		send_to_595(LOW (BDIR) | 0x80);
		send_to_595(HIGH(BDIR) | 0x80);
		send_to_595(LOW (BDIR) | 0x80);
        // выбор IO регистра port B 
    	send_to_595(HIGH (BDIR | BC1) | 0x0F); 
		send_to_595( LOW (BDIR | BC1) | 0x0F);
        // запись значения в регистр port B второго чипа
		send_to_595(LOW (BDIR) | val);
		send_to_595(HIGH(BDIR) | val);
		send_to_595(LOW (BDIR) | val);
    }
#endif
#else
    pwm_set_gpio_level(PWM_PIN0, outR); // Право
    pwm_set_gpio_level(PWM_PIN1, outL); // Лево
#endif
    outL = outR = 0;
    if (manager_started) {
        return true;
    }
#ifndef HWAY
    #ifdef AYSOUND
    if (g_conf.is_AY_on) {
        uint8_t* AY_data = get_AY_Out(5);
        outL = ((uint16_t)AY_data[0] + AY_data[1]) << 1;
        outR = ((uint16_t)AY_data[2] + AY_data[1]) << 1;
    }
    #endif
#endif
#ifdef COVOX
    if (!outL && !outR && g_conf.is_covox_on && (true_covox || az_covox_L || az_covox_R)) {
        outL = (az_covox_L + (uint16_t)true_covox);
        outR = (az_covox_R + (uint16_t)true_covox);
    }
#endif
    if (outR || outL) {
        register uint8_t mult = g_conf.snd_volume;
        if (mult >= 0) {
            if (mult > 5) mult = 5;
            outL <<= mult;
            outR <<= mult;
        } else {
            register int8_t div = -mult;
            if (div > 16) div = 16;
            outL >>= div;
            outR >>= div;
        }
      //  #ifndef HWAY
      //  beep(0);
      //  #endif
    }
    return true;
}
#endif

extern "C" FATFS fatfs;

static void init_fs() {
    FRESULT result = f_mount(&fatfs, "", 1);
    if (FR_OK != result) {
        char tmp[80];
        sprintf(tmp, "Unable to mount SD-card: %s (%d)", FRESULT_str(result), result);
        logMsg(tmp);
    } else {
        SD_CARD_AVAILABLE = true;
        #if BOOT_DEBUG || KBD_DEBUG || MNGR_DEBUG || DSK_DEBUG || INVALID_DEBUG
        f_unlink("\\bk.log");
        #endif
    }

    DIRECT_RAM_BORDER = PSRAM_AVAILABLE ? RAM_SIZE : (SD_CARD_AVAILABLE ? RAM_PAGE_SIZE : RAM_SIZE);

    if (SD_CARD_AVAILABLE) {
        DIR dir;
        if (f_opendir(&dir, "\\BK") != FR_OK) {
            f_mkdir("\\BK");
        } else {
            f_closedir(&dir);
        }
        insertdisk(0, fdd0_sz(), fdd0_rom(), "\\BK\\fdd0.img");
        insertdisk(1, fdd1_sz(), fdd1_rom(), "\\BK\\fdd1.img");
        insertdisk(2, 819200, 0, "\\BK\\fdd2.img");
        insertdisk(3, 819200, 0, "\\BK\\fdd3.img");
        if (!spacePressed)
            read_config("\\BK\\bk.conf");
    }
}

// connection is possible 00->00 (external pull down)
static int test_0000_case(uint32_t pin0, uint32_t pin1, int res) {
    gpio_init(pin0);
    gpio_set_dir(pin0, GPIO_OUT);
    sleep_ms(33);
    gpio_put(pin0, 1);

    gpio_init(pin1);
    gpio_set_dir(pin1, GPIO_IN);
    gpio_pull_down(pin1); /// external pulled down (so, just to ensure)
    sleep_ms(33);
    if ( gpio_get(pin1) ) { // 1 -> 1, looks really connected
        res |= (1 << 5) | 1;
    }
    gpio_deinit(pin0);
    gpio_deinit(pin1);
    return res;
}

// connection is possible 01->01 (no external pull up/down)
static int test_0101_case(uint32_t pin0, uint32_t pin1, int res) {
    gpio_init(pin0);
    gpio_set_dir(pin0, GPIO_OUT);
    sleep_ms(33);
    gpio_put(pin0, 1);

    gpio_init(pin1);
    gpio_set_dir(pin1, GPIO_IN);
    gpio_pull_down(pin1);
    sleep_ms(33);
    if ( gpio_get(pin1) ) { // 1 -> 1, looks really connected
        res |= (1 << 5) | 1;
    }
    gpio_deinit(pin0);
    gpio_deinit(pin1);
    return res;
}

// connection is possible 11->11 (externally pulled up)
static int test_1111_case(uint32_t pin0, uint32_t pin1, int res) {
    gpio_init(pin0);
    gpio_set_dir(pin0, GPIO_OUT);
    sleep_ms(33);
    gpio_put(pin0, 0);

    gpio_init(pin1);
    gpio_set_dir(pin1, GPIO_IN);
    gpio_pull_up(pin1); /// external pulled up (so, just to ensure)
    sleep_ms(33);
    if ( !gpio_get(pin1) ) { // 0 -> 0, looks really connected
        res |= 1;
    }
    gpio_deinit(pin0);
    gpio_deinit(pin1);
    return res;
}

static int testPins(uint32_t pin0, uint32_t pin1) {
    int res = 0b000000;
    /// do not try to test butter psram this way
#ifdef BUTTER_PSRAM_GPIO
    if (pin0 == BUTTER_PSRAM_GPIO || pin1 == BUTTER_PSRAM_GPIO) return res;
#endif
    if (pin0 == PICO_DEFAULT_LED_PIN || pin1 == PICO_DEFAULT_LED_PIN) return res; // LED
    if (pin0 == 23 || pin1 == 23) return res; // SMPS Power
    if (pin0 == 24 || pin1 == 24) return res; // VBus sense
    // try pull down case (passive)
    gpio_init(pin0);
    gpio_set_dir(pin0, GPIO_IN);
    gpio_pull_down(pin0);
    gpio_init(pin1);
    gpio_set_dir(pin1, GPIO_IN);
    gpio_pull_down(pin1);
    sleep_ms(33);
    int pin0vPD = gpio_get(pin0);
    int pin1vPD = gpio_get(pin1);
    gpio_deinit(pin0);
    gpio_deinit(pin1);
    /// try pull up case (passive)
    gpio_init(pin0);
    gpio_set_dir(pin0, GPIO_IN);
    gpio_pull_up(pin0);
    gpio_init(pin1);
    gpio_set_dir(pin1, GPIO_IN);
    gpio_pull_up(pin1);
    sleep_ms(33);
    int pin0vPU = gpio_get(pin0);
    int pin1vPU = gpio_get(pin1);
    gpio_deinit(pin0);
    gpio_deinit(pin1);

    res = (pin0vPD << 4) | (pin0vPU << 3) | (pin1vPD << 2) | (pin1vPU << 1);

    if (pin0vPD == 1) {
        if (pin0vPU == 1) { // pin0vPD == 1 && pin0vPU == 1
            if (pin1vPD == 1) { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 1
                if (pin1vPU == 1) { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 1 && pin1vPU == 1
                    // connection is possible 11->11 (externally pulled up)
                    return test_1111_case(pin0, pin1, res);
                } else { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 1 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            } else { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 0
                if (pin1vPU == 1) { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 0 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 0 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            }
        } else {  // pin0vPD == 1 && pin0vPU == 0
            if (pin1vPD == 1) { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 1
                if (pin1vPU == 1) { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 1 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 1 && pin1vPU == 0
                    // connection is possible 10->10 (pulled up on down, and pulled down on up?)
                    return res |= (1 << 5) | 1; /// NOT SURE IT IS POSSIBLE TO TEST SUCH CASE (TODO: think about real cases)
                }
            } else { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 0
                if (pin1vPU == 1) { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 0 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 0 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            }
        }
    } else { // pin0vPD == 0
        if (pin0vPU == 1) { // pin0vPD == 0 && pin0vPU == 1
            if (pin1vPD == 1) { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 1
                if (pin1vPU == 1) { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 1 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 1 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            } else { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 0
                if (pin1vPU == 1) { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 0 && pin1vPU == 1
                    // connection is possible 01->01 (no external pull up/down)
                    return test_0101_case(pin0, pin1, res);
                } else { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 0 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            }
        } else {  // pin0vPD == 0 && pin0vPU == 0
            if (pin1vPD == 1) { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 1
                if (pin1vPU == 1) { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 1 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 1 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            } else { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 0
                if (pin1vPU == 1) { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 0 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 0 && pin1vPU == 0
                    // connection is possible 00->00 (externally pulled down)
                    return test_0000_case(pin0, pin1, res);
                }
            }
        }
    }
    return res;
}

int main() {
#if !PICO_RP2040
	vreg_disable_voltage_limit();
	vreg_set_voltage(VREG_VSEL);
#else
    vreg_set_voltage(VREG_VOLTAGE_1_30);
#endif
    flash_timings();

    DBGM_PRINT(("Before keyboard_init"));
    keyboard_init();
#ifdef HWAY
    Init_PWM_175(TSPIN_MODE_BOTH);
#else
    PWM_init_pin(BEEPER_PIN, (1 << 12) - 1);
    #ifdef SOUND_SYSTEM
    PWM_init_pin(PWM_PIN0, (1 << 12) - 1);
    PWM_init_pin(PWM_PIN1, (1 << 12) - 1);
    #endif
#endif

#if LOAD_WAV_PIO
    //пин ввода звука
    inInit(LOAD_WAV_PIO);
#endif

    DBGM_PRINT(("gpio_init(PICO_DEFAULT_LED_PIN)"));
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    for (int i = 0; i < 6; i++) {
        keyboard_tick();
        sleep_ms(23);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(23);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
    }
    DBGM_PRINT(("Before Init_Wii_Joystick"));
    #ifdef USE_WII
    if (!Init_Wii_Joystick()) {
    #endif
        #ifdef USE_NESPAD
        DBGM_PRINT(("Before nespad_begin"));
        nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);
        #endif
    #ifdef USE_WII
    } else {
        add_repeating_timer_ms(60, Wii_Joystick_Timer_CB, NULL, &Wii_timer);
    }
    #endif

    for (int i = 0; i < 50; ++i) {
        keyboard_tick();
        sleep_ms(1);
    }

    init_fs();

    uint8_t link = testPins(beginVGA_PIN, beginVGA_PIN + 1);
    SELECT_VGA = (link == 0) || (link == 0x1F);

    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);

    reset(255);
    graphics_set_mode(g_conf.color_mode ? BK_256x256x2 : BK_512x256x1);

#ifdef SOUND_SYSTEM
	int hz = 44100;	//44000 //44100 //96000 //22050
	// negative timeout means exact delay (rather than delay between callbacks)
	if (!add_repeating_timer_us(-1000000 / hz, AY_timer_callback, NULL, &timer)) {
		logMsg("Failed to add timer");
		return 1;
	}
#endif

    emu_start();
    while(1);
    __unreachable();
}

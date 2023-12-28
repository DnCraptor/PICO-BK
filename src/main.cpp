extern "C" {
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
    graphics_set_buffer(CPU_PAGE5_MEM_ADR, 512, 256);
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

#include "fdd_controller.h"
CMotherBoard mb;
CFDDController m_fdd;

static void cb() {
	/*	switch (m_fdd.GetFDDType())
		{
			case BK_DEV_MPI::A16M:
			{
				const uint16_t m = GetAltProMode();

				if (m == 0 || m == 040)
				{
					return false;
				}

				break;
			}

			case BK_DEV_MPI::SMK512:
			{
				const uint16_t m = GetAltProMode();

				if (m == 0 || m == 040 || m == 20 || m == 120 || m == 0100)
				{
					return false;
				}

				break;
			}
		}
*/
		/*
		эмуляция работы с дисководом. Если мы работаем стандартными методами,
		а если нестандартными - то у нас есть полная эмуляция работы с дисководом через порты.
		*/
	//	if (g_Config.m_bEmulateFDDIO)
		{
			m_fdd.EmulateFDD(&mb);
		}
}

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
    graphics_set_mode(color_mode ? BK_256x256x2 : BK_512x256x1);

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
    main_init();
    emu_start(cb);
    return 0;
}

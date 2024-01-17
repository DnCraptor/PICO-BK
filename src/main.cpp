extern "C" {
#include "config_em.h"
#include "emulator.h"
#include "manager.h"
#include "debug.h"
}

#include "BKImage.h"
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

volatile config_em_t g_conf {
   true, // is_covox_on
   true, // is_AY_on
   true, // color_mode
   BK_0011M_FDD, // bk0010mode
   0, // snd_volume
   0, // graphics_pallette_idx
   0330, // shift_y
   256, // graphics_buffer_height
   0, // v_buff_offset
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
    static uint16_t outL = 0;  
    static uint16_t outR = 0;
    pwm_set_gpio_level(PWM_PIN0, outR); // Право
    pwm_set_gpio_level(PWM_PIN1, outL); // Лево
    outL = outR = 0;
    if (manager_started) {
        return true;
    }
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
    }
    return true;
}
#endif

static FATFS fatfs;
static PARSE_RESULT parse_result;

bool is_browse_os_supported() {
   return parse_result.imageOSType == IMAGE_TYPE::MKDOS;
}

void detect_os_type(const char* path, char* os_type, size_t sz) {
    try {
        CBKParseImage* parserImage = new CBKParseImage();
        parse_result = parserImage->ParseImage(path, 0);
        delete parserImage;
        auto s = std::to_string(parse_result.nImageSize >> 10) + " KB " + CBKParseImage::GetOSName(parse_result.imageOSType);
        if (parse_result.bImageBootable) {
            s += " [bootable]";
        }
        DBGM_PRINT(("detect_os_type: %s %s", path, s.c_str()));
        strncpy(os_type, s.c_str(), sz);
    } catch(...) {
        DBGM_PRINT(("detect_os_type: %s FAILED", path));
        strncpy(os_type, "DETECT OS TYPE FAILED", sz);
    }
}

#if EXT_DRIVES_MOUNT
bool mount_img(const char* path) {
    DBGM_PRINT(("mount_img: %s", path));
    if ( !is_browse_os_supported() ) {
        DBGM_PRINT(("mount_img: %s unsupported file type (resources)", path));
        return false;
    }
    m_cleanup_ext();
    try {
        CBKImage* BKImage = new CBKImage();
        BKImage->Open(parse_result);
        BKImage->ReadCurrentDir(BKImage->GetTopItemIndex());
        BKImage->Close();
        DBGM_PRINT(("mount_img: %s done", path));
    } catch(...) {
        DBGM_PRINT(("mount_img: %s FAILED", path));
        return false;
    }
    return true;
}
#endif

static void init_fs() {
    FRESULT result = f_mount(&fatfs, "", 1);
    if (FR_OK != result) {
        char tmp[80];
        sprintf(tmp, "Unable to mount SD-card: %s (%d)", FRESULT_str(result), result);
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
        insertdisk(0, fdd0_sz(), fdd0_rom(), "\\BK\\fdd0.img");
        insertdisk(1, fdd1_sz(), fdd1_rom(), "\\BK\\fdd1.img"); // TODO: why not attached?
        insertdisk(2, 819200, 0, "\\BK\\hdd0.img");
        insertdisk(3, 819200, 0, "\\BK\\hdd1.img"); // TODO: why not attached?
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

//    init_psram();
    init_fs();

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

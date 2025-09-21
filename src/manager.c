#if PICO_ON_DEVICE
#include <hardware/clocks.h>
#include "manager.h"
#include "emulator.h"
#include "vga.h"
#include "pico-vision.h"
#include "CPU.h"
#include "CPU_i.h"
#include "stdlib.h"
#include "debug.h"
#include "aySoundSoft.h"
#include "ps2.h"
#include "nespad.h"
#include "reboot.h"
#include "main_i.h"

void detect_os_type(const char* path, char* os_type, size_t sz);
bool mount_img(const char* path, int curr_dir_num);

//#include "EmuUi/Key_eu.h"
extern bool is_swap_wins_enabled;
extern bool swap_wins;
volatile bool is_dendy_joystick = true;

static void bottom_line();
static void redraw_window();

static uint8_t kbdpad_state  = 0;  // Joystick 1
static uint8_t kbdpad_state2 = 0;  // Joystick 2
static volatile bool backspacePressed = false;
volatile bool enterPressed = false;
static volatile bool plusPressed = false;
static volatile bool minusPressed = false;
static volatile bool ctrlPressed = false;
static volatile bool altPressed = false;
volatile bool shiftPressed = false;
static volatile bool delPressed = false;
static volatile bool f1Pressed = false;
static volatile bool f2Pressed = false;
static volatile bool f3Pressed = false;
static volatile bool f4Pressed = false;
static volatile bool f5Pressed = false;
static volatile bool f6Pressed = false;
static volatile bool f7Pressed = false;
static volatile bool f8Pressed = false;
static volatile bool f9Pressed = false;
static volatile bool f10Pressed = false;
static volatile bool f11Pressed = false;
static volatile bool f12Pressed = false;
static volatile bool tabPressed = false;
volatile bool spacePressed = false;
static volatile bool confEditMode = false;
volatile bool escWasPressed = false;
static volatile bool leftPressed = false;
static volatile bool rightPressed = false;
volatile bool upPressed = false;
volatile bool downPressed = false;
static volatile bool aPressed = false;
static volatile bool cPressed = false;
static volatile bool gPressed = false;
static volatile bool tPressed = false;
static volatile bool dPressed = false;
static volatile bool sPressed = false;

static volatile bool xPressed = false;
static volatile bool ePressed = false;
static volatile bool uPressed = false;
static volatile bool hPressed = false;

bool already_swapped_fdds = false;
volatile bool manager_started = false;
static char line[MAX_WIDTH + 2];

static volatile uint32_t lastCleanableScanCode = 0;
static uint32_t lastSavedScanCode = 0;
int nespad_state_delay = DPAD_STATE_DELAY;

uint8_t kbdpad1_A = 0x1E; // A
uint8_t kbdpad2_A = 0x11; // W
uint8_t kbdpad1_B = 0x10; // Q
uint8_t kbdpad2_B = 0x2C; // Z
uint8_t kbdpad1_START = 0x20; // D
uint8_t kbdpad2_START = 0x18; // O
uint8_t kbdpad1_SELECT = 0x1F; // S
uint8_t kbdpad2_SELECT = 0x2D; // X
uint8_t kbdpad1_UP = 0x19; // P
uint8_t kbdpad2_UP = 0x25; // K
uint8_t kbdpad1_DOWN = 0x27; // ;
uint8_t kbdpad2_DOWN = 0x34; // .
uint8_t kbdpad1_LEFT = 0x26; // L
uint8_t kbdpad2_LEFT = 0x12; // E
uint8_t kbdpad1_RIGHT = 0x28; // ,(")
uint8_t kbdpad2_RIGHT = 0x17; // I

inline static void scan_code_processed() {
  if (lastCleanableScanCode) {
      lastSavedScanCode = lastCleanableScanCode;
  }
  lastCleanableScanCode = 0;
}

void scan_code_cleanup() {
  lastSavedScanCode = 0;
  lastCleanableScanCode = 0;
}
static bool m_prompt(const char* txt);
static bool m_prompt_ex(const char* txt, const char* bottom);

const static const uint8_t PANEL_TOP_Y = 0;
#define TOTAL_SCREEN_LINES text_buffer_height
#define F_BTN_Y_POS (TOTAL_SCREEN_LINES - 1)
#define CMD_Y_POS (F_BTN_Y_POS - 1)
#define PANEL_LAST_Y (CMD_Y_POS - 1)

const static uint8_t FIRST_FILE_LINE_ON_PANEL_Y = PANEL_TOP_Y + 1;
#define LAST_FILE_LINE_ON_PANEL_Y (PANEL_LAST_Y - 1)

typedef struct {
    FSIZE_t fsize;   /* File size */
    WORD    fdate;   /* Modified date */
    WORD    ftime;   /* Modified time */
    BYTE    fattrib; /* File attribute */
    char    name[MAX_WIDTH >> 1];
    int     dir_num;
} file_info_t;

#define MAX_FILES 500

static file_info_t files_info[MAX_FILES] = { 0 };
static size_t files_count = 0;

typedef struct {
    int selected_file_idx;
    int start_file_offset;
    int dir_num;
} indexes_t;

typedef struct file_panel_desc {
    int left;
    int width;
    int files_number;
    char path[256];
    indexes_t indexes[16]; // TODO: some ext. limit logic
    int level;
#if EXT_DRIVES_MOUNT
    bool in_dos;
#endif
} file_panel_desc_t;

static file_panel_desc_t left_panel = {
    0, MAX_WIDTH / 2,
    0,
    { "\\" },
    { FIRST_FILE_LINE_ON_PANEL_Y, 0 },
    0,
    false
};

static file_panel_desc_t right_panel = {
    MAX_WIDTH / 2, MAX_WIDTH / 2,
     0,
    { "\\BK" },
    { FIRST_FILE_LINE_ON_PANEL_Y, 0 },
    0,
    false
};

static volatile bool left_panel_make_active = true;
static file_panel_desc_t* psp = &left_panel;

static bool mark_to_exit_flag = false;
static void mark_to_exit(uint8_t cmd) {
    f10Pressed = false;
    escWasPressed = false;
    mark_to_exit_flag = true;
}
static void return_to_mos(uint8_t cmd) {
    f_unlink(MOS_FILE);
    reboot();
    while (true);
}

static inline void fill_panel(file_panel_desc_t* p);

static uint16_t kbd_script[] = { // S160000 Enter
    0x001B,
    0xF01B,
    0x0016,
    0xF016,
    0x0036,
    0xF036,
    0x0045,
    0xF045,
    0x0045,
    0xF045,
    0x0045,
    0xF045,
    0x0045,
    0xF045,
    0x005A,
    0xF05A
};
static volatile int kbd_script_idx = 0;
static repeating_timer_t timer;
static bool __not_in_flash_func(timer_callback)(repeating_timer_t *rt) {
    push_script_scan_code(kbd_script[kbd_script_idx++]);
    bool res = kbd_script_idx < sizeof(kbd_script) / sizeof(uint16_t);
    if (!res) {
        kbd_script_idx = 0;
        cancel_repeating_timer(&timer);
    }
    return res;
}

static void InitMemoryValues(uint16_t* pPtr, size_t nMemSize)
{
	uint16_t val = 0;
	uint8_t flag = 0;
	for (int i = 0; i < (nMemSize >> 1); i++, flag--) {
		pPtr[i] = val;
		val = ~val;
		if (flag == 192) {
			val = ~val;
			flag = 0;
		}
	}
}

void reset(uint8_t cmd) {
    f12Pressed = false;
    true_covox = 0;
    az_covox_R = 0;
    az_covox_L = 0;
#ifdef AYSOUND
    AY_reset();
#endif
    InitMemoryValues((uint16_t*)RAM, 32 << 10); // TODO: other chess for 11M
    memset(CPU_PAGE11_MEM_ADR, 0, 0x04000); // W/A for 16K RAM in ROM
    graphics_set_page(CPU_PAGE51_MEM_ADR, is_bk0011mode() ? 15 : 0);
    graphics_shift_screen((uint16_t)0330 | 0b01000000000);
    main_init();
    mark_to_exit_flag = true;
    if (g_conf.bk0010mode == BK_FDD) {
        if (cmd == 255) { // ensure kbd load in time
            sleep_ms(1500);
        }
        kbd_script_idx = 0;
        add_repeating_timer_ms(200, timer_callback, NULL, &timer);
    }
}

inline static void swap_drive_message() {
    save_video_ram();
    enum graphics_mode_t ret = graphics_set_mode(TEXTMODE_);
    if (ret != TEXTMODE_) clrScr(1);
    if (already_swapped_fdds) {
        const line_t lns[3] = {
            { -1, "Swap FDD0 and FDD1 drive images" },
            {  0, "" },
            { -1, "To return images back, press Ctrl + Tab + Backspace"}
        };
        const lines_t lines = { 3, 2, lns };
        draw_box(10, 7, 60, 10, "Info", &lines);
    } else {
        const line_t lns[1] = {
            { -1, "Swap FDD0 and FDD1 drive images back" }
        };
        const lines_t lines = { 1, 3, lns };
        draw_box(10, 7, 60, 10, "Info", &lines);
    }
    sleep_ms(2500);
    graphics_set_mode(ret);
    restore_video_ram();
}

inline static void level_state_message(uint8_t divider, char* sys_name) {
    save_video_ram();
    enum graphics_mode_t ret = graphics_set_mode(TEXTMODE_);
    if (ret != TEXTMODE_) clrScr(1);
    char ln[MAX_WIDTH];
    snprintf(ln, text_buffer_width, "%s volume: %d (div: 1 << %d = %d)", sys_name, 16 - divider, divider, (1 << divider));
    const line_t lns[1] = {
        { -1, ln }
    };
    const lines_t lines = { 1, 3, lns };
    draw_box(5, 7, 70, 10, "Info", &lines);
    sleep_ms(2500);
    graphics_set_mode(ret);
    restore_video_ram();
}

inline static void swap_sound_state_message(volatile bool* p_state, char* sys_name, char switch_char) {
    save_video_ram();
    enum graphics_mode_t ret = graphics_set_mode(TEXTMODE_);
    if (ret != TEXTMODE_) clrScr(1);
    char ln[MAX_WIDTH];
    snprintf(ln, text_buffer_width, "Turn %s %s", sys_name, *p_state ? "OFF" : "ON");
    if (*p_state) {
        char ln3[42];
        snprintf(ln3, 42, "To turn it ON back, press Ctrl + Tab + %c", switch_char);
        const line_t lns[3] = {
            { -1, ln },
            {  0, "" },
            { -1, ln3}
        };
        const lines_t lines = { 3, 2, lns };
        draw_box(10, 7, 60, 10, "Info", &lines);
    } else {
        const line_t lns[1] = {
            { -1, ln }
        };
        const lines_t lines = { 1, 3, lns };
        draw_box(10, 7, 60, 10, "Info", &lines);
    }
    *p_state = !*p_state;
    sleep_ms(2500);
    graphics_set_mode(ret);
    restore_video_ram();
}

static void do_nothing(uint8_t cmd) {
    snprintf(line, 130, "CMD: F%d", cmd + 1);
    const line_t lns[2] = {
        { -1, "Not yet implemented function" },
        { -1, line }
    };
    const lines_t lines = { 2, 3, lns };
    draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Info", &lines);
    sleep_ms(1500);
    redraw_window();
}


static char scan_code_2_cp866[0x80] = {
     0 ,  0 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  0 , ' ', // 0D - TAB
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',  0 ,  0 , 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ';',  0 , '~',  0 ,  0 , 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', ',', '.',  0 ,  0 , '*',  0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
     0 ,  0,   0 ,  0 ,  0 ,  0,   0 , '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',  0 , 0 
};
/*
static char scan_code_2_cp866[0x80] = {
     0 ,  0 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  0 , ' ', // 0D - TAB
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',  0 ,  0 , 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ';','\'', '~',  0 ,'\\', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', ',', '.', '/',  0 , '*',  0 , ' ',  0 , 0 ,  0 , 0,  0 , 0,  0 , 0
};
*/
static const line_t bk_mode_lns[] = {
    { 1, " \x81\x8A-0010 + \x8A\x8D\x83\x8C\x84 16K " },
    { 1, " \x81\x8A-0010 Focal + \x8C\x91\x92\x84" },
    { 1, " \x81\x8A-0010-01 Basic 86" },
    { 1, " \x81\x8A-0011M + \x8A\x8D\x83\x8C\x84    " },
    { 1, " \x81\x8A-0011M + \x8C\x91\x92\x84     " }
};

#define kbd_j_label_val(kbdpad, idx) \
        snprintf(b, 8, "%d", kbdpad); \
        draw_label(xl, yl, 3, b, !blink && z_idx == idx, confEditMode && z_idx == idx); \
        if (scan_code_2_cp866[kbdpad & 0x7F]) { \
            b[0] = scan_code_2_cp866[kbdpad & 0x7F]; b[1] = 0; \
            draw_label(xl+3, yl, 1, b, false, false); \
        } ++yl \


static int MAX_Z = 27;
static int z_idx = 0;
static bool blink = false;
static void in_conf(int x, int y) {
    draw_label(x+1, y, 36, "Use SPACE to edit, ESC to exit", false, true);

    draw_label(x, y+1, 18, "          machine:", false, z_idx == 0);
    draw_label(x, y+2, 18, "            covox:", false, z_idx == 1);
    draw_label(x, y+3, 18, "        AY-3-8910:", false, z_idx == 2);
    draw_label(x, y+4, 18, "    \x81\x8A color mode:", false, z_idx == 3);
    draw_label(x, y+5, 18, "     sound volume:", false, z_idx == 4);
    draw_label(x, y+6, 18, "\x81\x8A pallette index:", false, z_idx == 5);
    draw_label(x, y+7, 18, "        swap wins:", false, z_idx == 6);
    draw_label(x, y+8, 18, "   dendy joystick:", false, z_idx == 7);
    draw_label(x, y+9, 18, "keyboard joystick:", false, z_idx == 8);
    draw_label(x, y+10,18, " manager pallette:", false, z_idx == 9);
    draw_label(x, y+11,31, "   manager 128x48:   (VGA only)", false, z_idx == 10);
    draw_label(x, y+12,30, "    HDMI 1024x768:   (512 MHz)", false, z_idx == 11);
    if(already_swapped_fdds) { // TODO: save it?
        draw_label(x, y+13, 23, "  swap FDD drives: true", false, false);
    } else {
        draw_label(x, y+13, 24, "  swap FDD drives: false", false, false);
    }
    // TODO: swap fdd

    char b[64] = { 0 };
    uint8_t nk = nespad_state | kbdpad_state;
    snprintf(b, 64, "Joy bits #1: %d%d%d%d%d%d%d%d #2: %d%d%d%d%d%d%d%d",
        (nk >> 7) & 1,
        (nk >> 6) & 1,
        (nk >> 5) & 1,
        (nk >> 4) & 1,
        (nk >> 3) & 1,
        (nk >> 2) & 1,
        (nk >> 1) & 1,
         nk & 1,
        (kbdpad_state2 >> 7) & 1,
        (kbdpad_state2 >> 6) & 1,
        (kbdpad_state2 >> 5) & 1,
        (kbdpad_state2 >> 4) & 1,
        (kbdpad_state2 >> 3) & 1,
        (kbdpad_state2 >> 2) & 1,
        (kbdpad_state2 >> 1) & 1,
         kbdpad_state2 & 1
    );
    draw_label(x, y+16, 37, b, false, false);


    if (is_kbd_joystick) {
        draw_label(x+40, y+0, 15, "     kbdpad1_A:", false, z_idx == 12);
        draw_label(x+40, y+1, 15, "     kbdpad1_B:", false, z_idx == 13);
        draw_label(x+40, y+2, 15, " kbdpad1_START:", false, z_idx == 14);
        draw_label(x+40, y+3, 15, "kbdpad1_SELECT:", false, z_idx == 15);
        draw_label(x+40, y+4, 15, "    kbdpad1_UP:", false, z_idx == 16);
        draw_label(x+40, y+5, 15, "  kbdpad1_DOWN:", false, z_idx == 17);
        draw_label(x+40, y+6, 15, "  kbdpad1_LEFT:", false, z_idx == 18);
        draw_label(x+40, y+7, 15, " kbdpad1_RIGHT:", false, z_idx == 19);

        draw_label(x+40, y+9 , 15, "     kbdpad2_A:", false, z_idx == 20);
        draw_label(x+40, y+10, 15, "     kbdpad2_B:", false, z_idx == 21);
        draw_label(x+40, y+11, 15, " kbdpad2_START:", false, z_idx == 22);
        draw_label(x+40, y+12, 15, "kbdpad2_SELECT:", false, z_idx == 23);
        draw_label(x+40, y+13, 15, "    kbdpad2_UP:", false, z_idx == 24);
        draw_label(x+40, y+14, 15, "  kbdpad2_DOWN:", false, z_idx == 25);
        draw_label(x+40, y+15, 15, "  kbdpad2_LEFT:", false, z_idx == 26);
        draw_label(x+40, y+16, 15, " kbdpad2_RIGHT:", false, z_idx == 27);
    }

    draw_label(x+18, y+1, 22, bk_mode_lns[g_conf.bk0010mode].txt, z_idx == 0, z_idx == 0);
    const static char b_on [2] = { 0xFB, 0 };
    const static char b_off[2] = { 0xB0, 0 };
    draw_label(x+19, y+2, 1, g_conf.is_covox_on ? b_on : b_off, z_idx == 1, z_idx == 1);
    draw_label(x+19, y+3, 1, g_conf.is_AY_on ? b_on : b_off, z_idx == 2, z_idx == 2);
    draw_label(x+19, y+4, 1, g_conf.color_mode ? b_on : b_off, z_idx == 3, z_idx == 3);
    snprintf(b, 8, "%d", g_conf.snd_volume);
    draw_label(x+19, y+5, 3, b, z_idx == 4, z_idx == 4);
    snprintf(b, 4, "%d", g_conf.graphics_pallette_idx);
    draw_label(x+19, y+6, 3, b, z_idx == 5, z_idx == 5);
    draw_label(x+19, y+7, 1, is_swap_wins_enabled ? b_on : b_off, z_idx == 6, z_idx == 6);
    draw_label(x+19, y+8, 1, is_dendy_joystick ? b_on : b_off, z_idx == 7, z_idx == 7);
    draw_label(x+19, y+9, 1, is_kbd_joystick ? b_on : b_off, z_idx == 8, z_idx == 8);
    snprintf(b, 4, "%d", g_conf.manager_pallette_idx);
    draw_label(x+19, y+10, 3, b, z_idx == 9, z_idx == 9);
    draw_label(x+19, y+11, 1, g_conf.is_128_48 ? b_on : b_off, z_idx == 10, z_idx == 10);
    draw_label(x+19, y+12, 1, g_conf.is_DVI_1024 ? b_on : b_off, z_idx == 11, z_idx == 11);

    if (is_kbd_joystick) {
        MAX_Z = 27;
        int xl = x+56;
        int yl = y;
        kbd_j_label_val(kbdpad1_A, 12);
        kbd_j_label_val(kbdpad1_B, 13);
        kbd_j_label_val(kbdpad1_START, 14);
        kbd_j_label_val(kbdpad1_SELECT, 15);
        kbd_j_label_val(kbdpad1_UP, 16);
        kbd_j_label_val(kbdpad1_DOWN, 17);
        kbd_j_label_val(kbdpad1_LEFT, 18);
        kbd_j_label_val(kbdpad1_RIGHT, 19);
        ++yl;
        kbd_j_label_val(kbdpad2_A, 20);
        kbd_j_label_val(kbdpad2_B, 21);
        kbd_j_label_val(kbdpad2_START, 22);
        kbd_j_label_val(kbdpad2_SELECT, 23);
        kbd_j_label_val(kbdpad2_UP, 24);
        kbd_j_label_val(kbdpad2_DOWN, 25);
        kbd_j_label_val(kbdpad2_LEFT, 26);
        kbd_j_label_val(kbdpad2_RIGHT, 27);
    } else {
        MAX_Z = 11;
    }
    snprintf(b, 64, "[%d MHz]", (clock_get_hz(clk_sys) / 1000000)); draw_label(x+2, y+17, 9, b, false, true);
    snprintf(b, 64, "[%d MHz]", (g_conf.cpu_freq / 1000000)); draw_label(x+63-10, y+17, 7, b, false, true);
}

static void saveConf() {
    FIL fil;
    f_open(&fil, "\\BK\\bk.conf", FA_CREATE_ALWAYS | FA_WRITE);
    char buf[256];
    snprintf(buf, 256,
     "mode:%d\r\nis_covox_on:%d\r\nis_AY_on:%d\r\ncolor_mode:%d\r\nsnd_volume:%d\r\n"
     "graphics_pallette_idx:%d\r\nis_swap_wins_enabled:%d\r\nis_dendy_joystick:%d\r\nis_kbd_joystick:%d\r\n",
        g_conf.bk0010mode,
        g_conf.is_covox_on,
        g_conf.is_AY_on,
        g_conf.color_mode,
        g_conf.snd_volume,
        g_conf.graphics_pallette_idx,
        is_swap_wins_enabled,
        is_dendy_joystick,
        is_kbd_joystick
    );
    UINT bw;
    f_write(&fil, buf, strlen(buf), &bw);
    snprintf(buf, 256,
     "kbdpad1_A:%d\r\n"
     "kbdpad2_A:%d\r\n"
     "kbdpad1_B:%d\r\n"
     "kbdpad2_B:%d\r\n"
     "kbdpad1_START:%d\r\n"
     "kbdpad2_START:%d\r\n"
     "kbdpad1_SELECT:%d\r\n"
     "kbdpad2_SELECT:%d\r\n"
     "kbdpad1_UP:%d\r\n"
     "kbdpad2_UP:%d\r\n"
     ,
        kbdpad1_A,
        kbdpad2_A,
        kbdpad1_B,
        kbdpad2_B,
        kbdpad1_START,
        kbdpad2_START,
        kbdpad1_SELECT,
        kbdpad2_SELECT,
        kbdpad1_UP,
        kbdpad2_UP
    );
    f_write(&fil, buf, strlen(buf), &bw);
    snprintf(buf, 256,
     "kbdpad1_DOWN:%d\r\n"
     "kbdpad2_DOWN:%d\r\n"
     "kbdpad1_LEFT:%d\r\n"
     "kbdpad2_LEFT:%d\r\n"
     "kbdpad1_RIGHT:%d\r\n"
     "kbdpad2_RIGHT:%d\r\n"
     ,
        kbdpad1_DOWN,
        kbdpad2_DOWN,
        kbdpad1_LEFT,
        kbdpad2_LEFT,
        kbdpad1_RIGHT,
        kbdpad2_RIGHT
    );
    f_write(&fil, buf, strlen(buf), &bw);
    snprintf(buf, 256, "manager_pallette_idx:%d\r\nis_128_48:%dr\nis_DVI_1024:%dr\n",
        g_conf.manager_pallette_idx,
        g_conf.is_128_48,
        g_conf.is_DVI_1024
    );
    f_write(&fil, buf, strlen(buf), &bw);
    f_close(&fil);
}

#include <string.h>
extern char *strnstr(const char *haystack, const char *needle, size_t len);

static int parse_conf_word(const char* buf, const char* param, size_t plen, size_t lim) {
    char b[16];
    const char* pc = strnstr(buf, param, lim);
    DBGM_PRINT(("param %s pc: %08Xh", param, pc));
    if (pc) {
        pc += plen - 1;
        const char* pc2 = strnstr(pc, "\r\n", lim - (pc - buf));
        DBGM_PRINT(("param %s \\r\\n pc: %08Xh pc2: %08Xh", param, pc, pc2));
        if (pc2) {
            memcpy(b, pc, pc2 - pc);
            b[pc2 - pc] = 0;
            DBGM_PRINT(("param %s b: %s atoi(b): %d", param, b, atoi(b)));
            return atoi(b);
        }
        pc2 = strnstr(pc, ";", lim - (pc - buf));
        DBGM_PRINT(("param %s ; pc2: %08Xh", param, pc2));
        if (pc2) {
            memcpy(b, pc, pc2 - pc);
            b[pc2 - pc] = 0;
            DBGM_PRINT(("param %s b: %s atoi(b): %d", param, b, atoi(b)));
            return atoi(b);
        }
        pc2 = strnstr(pc, "\n", lim - (pc - buf));
        DBGM_PRINT(("param %s \\n pc2: %08Xh", param, pc2));
        if (pc2) {
            memcpy(b, pc, pc2 - pc);
            b[pc2 - pc] = 0;
            DBGM_PRINT(("param %s b: %s atoi(b): %d", param, b, atoi(b)));
            return atoi(b);
        }
        DBGM_PRINT(("param %s pc: %d atoi(pc): %d", param, pc, atoi(pc)));
        return atoi(pc);
    }
    return -100;
}


const color_schema_t color_schema0 = {
   /*BACKGROUND_FIELD_COLOR =*/0b0001, // Blue
   /*FOREGROUND_FIELD_COLOR =*/0b0011, // Cyan
   /*HIGHLIGHTED_FIELD_COLOR=*/0b0111, // White

   /*BACKGROUND_F1_10_COLOR =*/0b0000, // Black
   /*FOREGROUND_F1_10_COLOR =*/0b0111, // White

   /*BACKGROUND_F_BTN_COLOR =*/0b0011, // Cyan
   /*FOREGROUND_F_BTN_COLOR =*/0b0000, // Black
 
   /*BACKGROUND_CMD_COLOR   =*/0b0000, // Black
   /*FOREGROUND_CMD_COLOR   =*/0b0011, // Cyan
   /*BACKGROUND_SEL_BTN_COLOR*/0b0111, // White
  
   /*FOREGROUND_SELECTED_COLOR =*/0b0000, // Black
   /*BACKGROUND_SELECTED_COLOR =*/0b0111, // White
};

const color_schema_t color_schema1 = {
   /*BACKGROUND_FIELD_COLOR =*/0b0000, // Black
   /*FOREGROUND_FIELD_COLOR =*/0b0011, // Cyan
   /*HIGHLIGHTED_FIELD_COLOR=*/0b0111, // White

   /*BACKGROUND_F1_10_COLOR =*/0b0000, // Black
   /*FOREGROUND_F1_10_COLOR =*/0b0111, // White

   /*BACKGROUND_F_BTN_COLOR =*/0b0011, // Cyan
   /*FOREGROUND_F_BTN_COLOR =*/0b0000, // Black
 
   /*BACKGROUND_CMD_COLOR   =*/0b0000, // Black
   /*FOREGROUND_CMD_COLOR   =*/0b0011, // Cyan
   /*BACKGROUND_SEL_BTN_COLOR*/0b0111, // White
  
   /*FOREGROUND_SELECTED_COLOR =*/0b0000, // Black
   /*BACKGROUND_SELECTED_COLOR =*/0b0111, // White
};

#define MAX_CONF 512
void read_config(const char* path) {
    FIL fil;
    if (f_open(&fil, path, FA_READ) != FR_OK) {
        DBGM_PRINT(("f_open %s failed", path));
        return;
    }
    char buf[MAX_CONF] = { 0 };
    UINT br;
    if (f_read(&fil, buf, MAX_CONF, &br) != FR_OK) {
        DBGM_PRINT(("f_read %s failed. br: %d", br));
        f_close(&fil);
        return;
    }
    DBGM_PRINT(("f_read %s passed. br: %d", br));
    const char p0[] = "mode:";
    int mode = parse_conf_word(buf, p0, sizeof(p0), MAX_CONF);
    if (mode >= 0 && mode <= BK_0011M) {
        g_conf.bk0010mode = (bk_mode_t)mode;
    }
    const char p1[] = "is_covox_on:";
    mode = parse_conf_word(buf, p1, sizeof(p1), MAX_CONF);
    if (mode >= 0 && mode <= 1) {
        g_conf.is_covox_on = (bool)mode;
    }
    const char p2[] = "is_AY_on:";
    mode = parse_conf_word(buf, p2, sizeof(p2), MAX_CONF);
    if (mode >= 0 && mode <= 1) {
        g_conf.is_AY_on = (bool)mode;
    }
    const char p3[] = "color_mode:";
    mode = parse_conf_word(buf, p3, sizeof(p3), MAX_CONF);
    if (mode >= 0 && mode <= 1) {
        g_conf.color_mode = (bool)mode;
    }
    const char p4[] = "snd_volume:";
    mode = parse_conf_word(buf, p4, sizeof(p4), MAX_CONF);
    if (mode >= -16 && mode <= 5) {
        g_conf.snd_volume = mode;
    }
    const char p5[] = "graphics_pallette_idx:";
    mode = parse_conf_word(buf, p5, sizeof(p5), MAX_CONF);
    if (mode >= 0 && mode <= 15) {
        g_conf.graphics_pallette_idx = mode;
    }
    const char p6[] = "is_swap_wins_enabled:";
    mode = parse_conf_word(buf, p6, sizeof(p6), MAX_CONF);
    if (mode >= 0 && mode <= 1) {
        is_swap_wins_enabled = (bool)mode;
    }
    const char p7[] = "is_dendy_joystick:";
    mode = parse_conf_word(buf, p7, sizeof(p7), MAX_CONF);
    if (mode >= 0 && mode <= 1) {
        is_dendy_joystick = (bool)mode;
    }
    const char p8[] = "is_kbd_joystick:";
    mode = parse_conf_word(buf, p8, sizeof(p8), MAX_CONF);
    if (mode >= 0 && mode <= 1) {
        is_kbd_joystick = (bool)mode;
    }
    const char p9[] = "kbdpad1_A:";
    mode = parse_conf_word(buf, p9, sizeof(p9), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad1_A = mode;
    const char p10[] = "kbdpad2_A:";
    mode = parse_conf_word(buf, p10, sizeof(p10), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad2_A = mode;
    const char p11[] = "kbdpad1_B:";
    mode = parse_conf_word(buf, p11, sizeof(p11), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad1_B = mode;
    const char p12[] = "kbdpad2_B:";
    mode = parse_conf_word(buf, p12, sizeof(p12), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad2_B = mode;
    const char p13[] = "kbdpad1_START:";
    mode = parse_conf_word(buf, p13, sizeof(p13), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad1_START = mode;
    const char p14[] = "kbdpad2_START:";
    mode = parse_conf_word(buf, p14, sizeof(p14), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad2_START = mode;
    const char p15[] = "kbdpad1_SELECT:";
    mode = parse_conf_word(buf, p15, sizeof(p15), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad1_SELECT = mode;
    const char p16[] = "kbdpad2_SELECT:";
    mode = parse_conf_word(buf, p16, sizeof(p16), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad2_SELECT = mode;
    const char p17[] = "kbdpad1_UP:";
    mode = parse_conf_word(buf, p17, sizeof(p17), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad1_UP = mode;
    const char p18[] = "kbdpad2_UP:";
    mode = parse_conf_word(buf, p18, sizeof(p18), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad2_UP = mode;
    const char p19[] = "kbdpad1_DOWN:";
    mode = parse_conf_word(buf, p19, sizeof(p19), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad1_DOWN = mode;
    const char p20[] = "kbdpad2_DOWN:";
    mode = parse_conf_word(buf, p20, sizeof(p20), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad2_DOWN = mode;
    const char p21[] = "kbdpad1_LEFT:";
    mode = parse_conf_word(buf, p21, sizeof(p21), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad1_LEFT = mode;
    const char p22[] = "kbdpad2_LEFT:";
    mode = parse_conf_word(buf, p22, sizeof(p22), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad2_LEFT = mode;
    const char p23[] = "kbdpad1_RIGHT:";
    mode = parse_conf_word(buf, p23, sizeof(p23), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad1_RIGHT = mode;
    const char p24[] = "kbdpad2_RIGHT:";
    mode = parse_conf_word(buf, p24, sizeof(p24), MAX_CONF);
    if (mode >= 0 && mode < 0x80)  kbdpad2_RIGHT = mode;
    const char p25[] = "manager_pallette_idx:";
    mode = parse_conf_word(buf, p25, sizeof(p25), MAX_CONF);
    g_conf.manager_pallette_idx = !!mode;
    set_color_schema(mode ? &color_schema1 : &color_schema0);
    const char p26[] = "is_128_48:";
    mode = parse_conf_word(buf, p26, sizeof(p26), MAX_CONF);
    if (mode >= 0 && mode <= 1) {
        g_conf.is_128_48 = (bool)mode;
    }
    const char p27[] = "is_DVI_1024:";
    mode = parse_conf_word(buf, p27, sizeof(p27), MAX_CONF);
    if (mode >= 0 && mode <= 1) {
        g_conf.is_DVI_1024 = (bool)mode;
    }

    f_close(&fil);
}

color_schema_t* pcs = &color_schema0;

static void conf_it(uint8_t cmd) {
    int x = (text_buffer_width - 64) / 2;
    int y = (text_buffer_height - 18) / 2;
    int x0 = x; int y0 = y;
    draw_panel(x0, y0, 64, 19, "Startup configuration", 0);
    x += 1; y += 1;
    in_conf(x, y);
    uint16_t prev_nespad_state = 0;
    while(1) {
        if (confEditMode && lastCleanableScanCode && lastCleanableScanCode != 0xB9 /* SPACE down */) {
            uint8_t kk = lastCleanableScanCode & 0x7F;
            switch (z_idx)
            {
                case 12: kbdpad1_A = kk; break;
                case 13: kbdpad1_B = kk; break;
                case 14: kbdpad1_START = kk; break;
                case 15: kbdpad1_SELECT = kk; break;
                case 16: kbdpad1_UP = kk; break;
                case 17: kbdpad1_DOWN = kk; break;
                case 18: kbdpad1_LEFT = kk; break;
                case 19: kbdpad1_RIGHT = kk; break;
                case 20: kbdpad2_A = kk; break;
                case 21: kbdpad2_B = kk; break;
                case 22: kbdpad2_START = kk; break;
                case 23: kbdpad2_SELECT = kk; break;
                case 24: kbdpad2_UP = kk; break;
                case 25: kbdpad2_DOWN = kk; break;
                case 26: kbdpad2_LEFT = kk; break;
                case 27: kbdpad2_RIGHT = kk; break;
            }
            confEditMode = false;
            lastCleanableScanCode = false;
            upPressed = false;
            downPressed = false;
            enterPressed = false;
            escWasPressed = false;
            tabPressed = false;
            spacePressed = false;
            blink = false;
            scan_code_cleanup();
            in_conf(x, y);
        }
        if (confEditMode) {
            blink = !blink;
            sleep_ms(250);
            in_conf(x, y);
        }
     //   if (is_dendy_joystick || is_kbd_joystick) {
            if (is_dendy_joystick) nespad_read();
            if (nespad_state_delay > 0) {
                nespad_state_delay--;
                sleep_ms(1);
            }
            else {
                nespad_state_delay = DPAD_STATE_DELAY;
                if(nespad_state & DPAD_UP) {
                    upPressed = true;
                } else if(nespad_state & DPAD_DOWN) {
                    downPressed = true;
                } else if (nespad_state & DPAD_A) {
                    enterPressed = true;
                } else if (nespad_state & DPAD_B) {
                    escWasPressed = true;
                } else if (nespad_state & DPAD_SELECT) {
                    downPressed = true;
                } else if ((nespad_state & DPAD_LEFT) || (nespad_state & DPAD_RIGHT)) {
                    spacePressed = true;
                }
            }
       // }
        if (escWasPressed) {
            escWasPressed = false;
            scan_code_cleanup();
            // exit from the window
            break;
        }
        if (enterPressed) {
            enterPressed = false;
            if (!m_prompt_ex("Save and reboot?", "Hold ALT - noreboot")) {
                draw_panel(x0, y0, 64, 19, "Startup configuration", 0);
                in_conf(x, y);
                continue;
            }
            saveConf();
            if (!altPressed) {
                reset(255);
            }
            break;
        }
        if (MAX_Z > 11 && (tabPressed || rightPressed || leftPressed)) {
            tabPressed = rightPressed = leftPressed = false;
            if (z_idx <= 11) z_idx = 12;
            else z_idx = 0;
            in_conf(x, y);
        }
        if (upPressed) {
            if (--z_idx < 0) z_idx = MAX_Z;
            upPressed = false;
            in_conf(x, y);
        }
        if (downPressed) {
            downPressed = false;
            if (++z_idx > MAX_Z) z_idx = 0;
            in_conf(x, y);
        }
        if (spacePressed) {
            spacePressed = false;
            if (z_idx == 0) {
                if (g_conf.bk0010mode < 4) {
                    g_conf.bk0010mode++;
                } else {
                    g_conf.bk0010mode = 0;
                }
                g_conf.cpu_freq = g_conf.bk0010mode >= BK_0011M_FDD ? 4000000 : 3000000;
            } else if (z_idx > 11) {
                confEditMode = true;
                lastCleanableScanCode = 0;
            }
            else switch (z_idx)
            {
            case 1:
              g_conf.is_covox_on = !g_conf.is_covox_on;
              break;
            case 2:
              g_conf.is_AY_on = !g_conf.is_AY_on;
              break;
            case 3:
              g_conf.color_mode = !g_conf.color_mode;
              break;
            case 4:
              g_conf.snd_volume++;
              if (g_conf.snd_volume == 6) g_conf.snd_volume = -16;
              break;
            case 5:
              g_conf.graphics_pallette_idx++;
              if (g_conf.graphics_pallette_idx == 16) g_conf.graphics_pallette_idx = 0;
              break;
            case 6:
              is_swap_wins_enabled = !is_swap_wins_enabled;
              break;
            case 7:
              is_dendy_joystick = !is_dendy_joystick;
              break;
            case 8:
              is_kbd_joystick = !is_kbd_joystick;
              draw_panel(x0, y0, 64, 19, "Startup configuration", 0);
              break;
            case 9:
              g_conf.manager_pallette_idx = !g_conf.manager_pallette_idx;
              set_color_schema(g_conf.manager_pallette_idx ? &color_schema1 : &color_schema0);
              draw_panel(x0, y0, 64, 19, "Startup configuration", 0);
              break;
            case 10:
              g_conf.is_128_48 = !g_conf.is_128_48;
              break;
            case 11:
              g_conf.is_DVI_1024 = !g_conf.is_DVI_1024;
              break;
            }
            in_conf(x, y);
        }
        if (nespad_state || kbdpad_state || kbdpad_state2 || prev_nespad_state) {
            in_conf(x, y);
            prev_nespad_state = ((uint16_t)(nespad_state | kbdpad_state) << 8) | kbdpad_state2;
        }
    }
    redraw_window();
}

typedef struct drive_state {
    char path[256];
    const char* lbl;
} drive_state_t;

static drive_state_t drives_states[4] = {
    { { 0 }, "FDD0" },
    { { 0 }, "FDD1" },
    { { 0 }, "FDD2" },
    { { 0 }, "FDD3" }
};

void notify_image_insert_action(uint8_t drivenum, char *pathname) {
    if (pathname) {
        strncpy(drives_states[drivenum].path, pathname, 256);
    }
}

static void swap_drives(uint8_t cmd) {
    sprintf(line, "F%d pressed - swap FDD images", cmd + 1);
    draw_cmd_line(0, CMD_Y_POS, line);
    char path1[256];
    strncpy(path1, drives_states[1].path, 256);
    insertdisk(1, 819200, 0, drives_states[0].path);
    insertdisk(0, 819200, 0, path1);
    already_swapped_fdds = !already_swapped_fdds;
    redraw_window();
}

inline static void if_swap_drives() {
    if (backspacePressed && tabPressed && ctrlPressed) {
        backspacePressed = false; // TODO: was!
        swap_drives(8);
    }
}

static void save_snap(uint8_t cmd) {
    f2Pressed = false;
    FIL file;
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    char buf[64];
    snprintf(buf, 64, "\\BK\\SNAP%d.BKE", cmd + 1);
    FRESULT result = f_open(&file, buf, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK) {
        return;
    }
    UINT bw;
    result = f_write(&file, &Device_Data, sizeof(Device_Data), &bw); // TODO: error handling
    result = f_write(&file, &g_conf, sizeof(g_conf), &bw);
    result = f_write(&file, RAM, sizeof(RAM), &bw);
    for (int i = 0; i < 4; ++i) {
        result = f_write(&file, drives_states[i].path, sizeof(drives_states[i].path), &bw);
    }
    // TODO: graphics_set_page
    if (result != FR_OK) {
  //      return;
    }
    f_close(&file);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    redraw_window();
}

inline static void restore_snap_by_filename(const char* path) {
    FIL file;
    FRESULT result = f_open(&file, path, FA_READ);
    UINT bw;
    if (result == FR_OK) {
      result = f_read(&file, &Device_Data, sizeof(Device_Data), &bw);  // TODO: error handling
      result = f_read(&file, &g_conf, sizeof(g_conf), &bw);
      result = f_read(&file, RAM, sizeof(RAM), &bw);
      for (int i = 0; i < 4; ++i) {
          result = f_read(&file, drives_states[i].path, sizeof(drives_states[i].path), &bw);
          if (drives_states[i].path[0]) {
              stpncpy(path, drives_states[i].path, 256);
              insertdisk(i, 819200, 0, path); // TODO: size? removed?
          }
      }
      if (result != FR_OK) {
    //    return;
      }
    }
    f_close(&file);
    set_bk0010mode(g_conf.bk0010mode);
    init_rom();
    graphics_set_page(CPU_PAGE51_MEM_ADR, g_conf.graphics_pallette_idx);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    mark_to_exit(0);
}

static void restore_snap(uint8_t cmd) {
    f2Pressed = false;
    main_init();
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    char path[256] = { 0 };
    snprintf(path, 256, "\\BK\\SNAP%d.BKE", cmd + 1);
    restore_snap_by_filename(path);
}

inline static void if_video_mode() {
  if (!ctrlPressed && !altPressed) {
    if (f12Pressed) {
      f12Pressed = false;
      g_conf.color_mode = !g_conf.color_mode;
      graphics_set_mode(g_conf.color_mode ? BK_256x256x2 : BK_512x256x1);
    }
    if (f11Pressed) {
      f11Pressed = false;
      pallete_mask--;
      if (pallete_mask <= 0) pallete_mask = 3;
    }
    if (f10Pressed) {
      f10Pressed = false;
      graphics_inc_palleter_offset();
    }
  }
  if (ctrlPressed && altPressed && delPressed) {
      reset(255);
      ctrlPressed = altPressed = delPressed = false;
  }
  if (ctrlPressed || altPressed)
    if(f1Pressed) {
        if (ctrlPressed) save_snap(0);
        else restore_snap(0);
        f1Pressed = false;
    } else if (f2Pressed) {
        if (ctrlPressed) save_snap(1);
        else restore_snap(1);
        f2Pressed = false;
    } else if (f3Pressed) {
        if (ctrlPressed) save_snap(2);
        else restore_snap(2);
        f3Pressed = false;
    } else if (f4Pressed) {
        if (ctrlPressed) save_snap(3);
        else restore_snap(3);
        f4Pressed = false;
    } else if (f5Pressed) {
        f5Pressed = false;
        if (ctrlPressed) save_snap(4);
        else restore_snap(4);
    } else if (f6Pressed) {
        if (ctrlPressed) save_snap(5);
        else restore_snap(5);
        f6Pressed = false;
    } else if (f7Pressed) {
        if (ctrlPressed) save_snap(6);
        else restore_snap(6);
        f7Pressed = false;
    } else if (f8Pressed) {
        if (ctrlPressed) save_snap(7);
        else restore_snap(7);
        f8Pressed = false;
    } else if (f10Pressed) {
        f10Pressed = false;
        if (altPressed) g_conf.graphics_pallette_idx = 0;
        else g_conf.graphics_pallette_idx = 15;
    }
}

static void draw_window() {
    int pw = text_buffer_width >> 1;
    left_panel.width = pw;
    right_panel.left = pw;
    right_panel.width = pw;
    sprintf(line, "SD:%s", left_panel.path);
    draw_panel( 0, PANEL_TOP_Y, pw, PANEL_LAST_Y + 1, line, 0);
    sprintf(line, "SD:%s", right_panel.path);
    draw_panel(pw, PANEL_TOP_Y, pw, PANEL_LAST_Y + 1, line, 0);
}

static char os_type[160] = { 0 }; 
inline static void update_menu_color();

static void redraw_window() {
    draw_window();
    if (SD_CARD_AVAILABLE) {
        fill_panel(&left_panel);
        fill_panel(&right_panel);
    }
    draw_cmd_line(0, CMD_Y_POS, os_type);
    update_menu_color();
}

static void switch_mode(uint8_t cmd);

static void switch_color(uint8_t cmd);

static file_info_t* selected_file();

inline static void construct_full_name(char* dst, const char* folder, const char* file) {
    if (strlen(folder) > 1) {
        snprintf(dst, 256, "%s\\%s", folder, file);
    } else {
        snprintf(dst, 256, "\\%s", file);
    }
}

inline static void no_selected_file() {
    const line_t lns[1] = {
        { -1, "Pls. select some file for this action" },
    };
    const lines_t lines = { 1, 3, lns };
    draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Info", &lines);
    sleep_ms(1500);
    redraw_window();
}

static bool m_prompt(const char* txt) {
    return m_prompt_ex(txt, 0);
}

static bool m_prompt_ex(const char* txt, const char* bottom) {
    const line_t lns[1] = {
        { -1, txt },
    };
    const lines_t lines = { 1, 2, lns };
    int y = (text_buffer_height - 10) / 2;
    draw_box_ex((text_buffer_width - 60) / 2, y, 60, 10, "Are you sure?", bottom, &lines);
    bool yes = true;
    draw_button((text_buffer_width - 60) / 2 + 16, 5 + y, 11, "Yes", yes);
    draw_button((text_buffer_width - 60) / 2 + 35, 5 + y, 10, "No", !yes);
    while(1) {
        if (is_dendy_joystick || is_kbd_joystick) {
            if (is_dendy_joystick) nespad_read();
            if (nespad_state_delay > 0) {
                nespad_state_delay--;
                sleep_ms(1);
            }
            else {
                nespad_state_delay = DPAD_STATE_DELAY;
                if(nespad_state & DPAD_UP) {
                    upPressed = true;
                } else if(nespad_state & DPAD_DOWN) {
                    downPressed = true;
                } else if (nespad_state & DPAD_A) {
                    enterPressed = true;
                } else if (nespad_state & DPAD_B) {
                    escWasPressed = true;
                } else if (nespad_state & DPAD_LEFT) {
                    leftPressed = true;
                } else if (nespad_state & DPAD_RIGHT) {
                    rightPressed = true;
                } else if (nespad_state & DPAD_SELECT) {
                    tabPressed = true;
                }
            }
        }
        if (enterPressed) {
            enterPressed = false;
            scan_code_cleanup();
            return yes;
        }
        if (tabPressed || leftPressed || rightPressed) { // TODO: own msgs cycle
            yes = !yes;
            draw_button((text_buffer_width - 60) / 2 + 16, 5 + y, 11, "Yes", yes);
            draw_button((text_buffer_width - 60) / 2 + 35, 5 + y, 10, "No", !yes);
            tabPressed = leftPressed = rightPressed = false;
            scan_code_cleanup();
        }
        if (escWasPressed) {
            escWasPressed = false;
            scan_code_cleanup();
            return false;
        }
    }
}

static FRESULT m_unlink_recursive(char * path) {
    DIR dir;
    FRESULT res = f_opendir(&dir, path);
    if (res != FR_OK) return res;
    FILINFO fileInfo;
    while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        char path2[256];
        construct_full_name(path2, path, fileInfo.fname);
        draw_cmd_line(0, CMD_Y_POS, path2);
        if (fileInfo.fattrib & AM_DIR) {
            res = m_unlink_recursive(path2);
        } else {
            res = f_unlink(path2);
        }
        if (res != FR_OK) break;
    }
    f_closedir(&dir);
    if (res == FR_OK) {
        draw_cmd_line(0, CMD_Y_POS, 0);
        res = f_unlink(path);
    }
    return res;
}

static void m_delete_file(uint8_t cmd) {
#if EXT_DRIVES_MOUNT
    if (psp->in_dos) {
        // TODO:
        do_nothing(cmd);
        return;
    }
#endif
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    file_info_t* fp = selected_file();
    if (!fp) {
       no_selected_file();
       return;
    }
    char path[256];
    snprintf(path, 256, "Remove %s %s?", fp->name, fp->fattrib & AM_DIR ? "folder" : "file");
    if (m_prompt(path)) {
        construct_full_name(path, psp->path, fp->name);
        FRESULT result = fp->fattrib & AM_DIR ? m_unlink_recursive(path) : f_unlink(path);
        if (result != FR_OK) {
            snprintf(line, text_buffer_width, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to delete selected item!" },
                { -1, path },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Error", &lines);
            sleep_ms(2500);
        } else {
            psp->indexes[psp->level].selected_file_idx--;
        }
    }
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    redraw_window();    
}

inline static FRESULT m_copy(char* path, char* dest) {
    FIL file1;
    FRESULT result = f_open(&file1, path, FA_READ);
    if (result != FR_OK) return result;
    FIL file2;
    result = f_open(&file2, dest, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK) {
        f_close(&file1);
        return result;
    }
    UINT br;
    do {
        result = f_read(&file1, files_info, sizeof(files_info), &br);
        if (result != FR_OK || br == 0) break;
        UINT bw;
        f_write(&file2, files_info, br, &bw);
        if (result != FR_OK) break;
    } while (br);
    f_close(&file1);
    f_close(&file2);
    return result;
}

inline static FRESULT m_copy_recursive(char* path, char* dest) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    DIR dir;
    FRESULT res = f_opendir(&dir, path);
    if (res != FR_OK) return res;
    res = f_mkdir(dest);
    draw_cmd_line(0, CMD_Y_POS, dest);
    if (res != FR_OK) return res;
    FILINFO fileInfo;
    while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        char path2[256];
        construct_full_name(path2, path, fileInfo.fname);
        char dest2[256];
        construct_full_name(dest2, dest, fileInfo.fname);
        draw_cmd_line(0, CMD_Y_POS, dest2);
        if (fileInfo.fattrib & AM_DIR) {
            res = m_copy_recursive(path2, dest2);
        } else {
            res = m_copy(path2, dest2);
        }
        if (res != FR_OK) break;
    }
    f_closedir(&dir);
    if (res == FR_OK) {
        draw_cmd_line(0, CMD_Y_POS, 0);
    }
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    return res;
}

static void m_copy_file(uint8_t cmd) {
#if EXT_DRIVES_MOUNT
    if (psp->in_dos) {
        // TODO:
        do_nothing(cmd);
        return;
    }
#endif
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    file_info_t* fp = selected_file();
    if (!fp) {
       no_selected_file();
       return;
    }
    char path[256];
    file_panel_desc_t* dsp = psp == &left_panel ? &right_panel : &left_panel;
    snprintf(path, 256, "Copy %s %s to %s?", fp->name, fp->fattrib & AM_DIR ? "folder" : "file", dsp->path);
    if (m_prompt(path)) { // TODO: ask name
        construct_full_name(path, psp->path, fp->name);
        char dest[256];
        construct_full_name(dest, dsp->path, fp->name);
        FRESULT result = fp->fattrib & AM_DIR ? m_copy_recursive(path, dest) : m_copy(path, dest);
        if (result != FR_OK) {
            snprintf(line, text_buffer_width, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to copy selected item!" },
                { -1, path },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Error", &lines);
            sleep_ms(2500);
        }
    }
    redraw_window();
    gpio_put(PICO_DEFAULT_LED_PIN, false);
}

static void m_mk_dir(uint8_t cmd) {
#if EXT_DRIVES_MOUNT
    if (psp->in_dos) {
        // TODO:
        do_nothing(cmd);
        return;
    }
#endif
    char dir[256];
    construct_full_name(dir, psp->path, "_");
    size_t len = strnlen(dir, 256) - 1;
    draw_panel(20, text_buffer_height / 2 - 20, text_buffer_width - 40, 5, "DIR NAME", 0);
    draw_label(22, text_buffer_height / 2 - 18, text_buffer_width - 44, dir, true, true);
    while(1) {
        if (escWasPressed) {
            escWasPressed = false;
            scan_code_cleanup();
            redraw_window();
            return;
        }
        if (backspacePressed) {
            backspacePressed = false;
            scan_code_cleanup();
            if (len == 0) continue;
            dir[len--] = 0;
            dir[len] = '_';
            draw_label(22, text_buffer_height / 2 - 18, text_buffer_width - 44, dir, true, true);
        }
        if (enterPressed) {
            enterPressed = false;
            break;
        }
        uint8_t sc = (uint8_t)lastCleanableScanCode;
        scan_code_cleanup();
        if (!sc || sc >= 0x80) continue;
        char c = scan_code_2_cp866[sc]; // TODO: shift, caps lock, alt, rus...
        if (!c) continue;
        if (len + 2 == text_buffer_width - 44) continue;
        dir[len++] = c;
        dir[len] = '_';
        dir[len + 1] = 0;
        draw_label(22, text_buffer_height / 2 - 18, text_buffer_width - 44, dir, true, true);
    }
    if (len) {
        dir[len] = 0;
        f_mkdir(dir);
    }
    scan_code_cleanup();
    redraw_window();
}

static void m_move_file(uint8_t cmd) {
#if EXT_DRIVES_MOUNT
    if (psp->in_dos) {
        // TODO:
        do_nothing(cmd);
        return;
    }
#endif
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    file_info_t* fp = selected_file();
    if (!fp) {
       no_selected_file();
       return;
    }
    char path[256];
    file_panel_desc_t* dsp = psp == &left_panel ? &right_panel : &left_panel;
    snprintf(path, 256, "Move %s %s to %s?", fp->name, fp->fattrib & AM_DIR ? "folder" : "file", dsp->path);
    if (m_prompt(path)) { // TODO: ask name
        construct_full_name(path, psp->path, fp->name);
        char dest[256];
        construct_full_name(dest, dsp->path, fp->name);
        FRESULT result = f_rename(path, dest);
        if (result != FR_OK) {
            snprintf(line, text_buffer_width, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to move selected item!" },
                { -1, path },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Error", &lines);
            sleep_ms(2500);
        }
    }
    redraw_window();
    gpio_put(PICO_DEFAULT_LED_PIN, false);
}

static void m_info(uint8_t cmd) {
    line_t plns[26] = {
        { 1, "Key mapping in the emulation mode:" },
        { 1, " - Alt   + key  - AP2                       - F7             - BLOCK REDACT" },
        { 1, " - Shift + key  - register up/down          - F8             - STEP" },
        { 1, " - Ctrl  + key  - CU                        - F9             - SBROS" },  // TODO: rus
        { 1, " - Caps Lock      - lock up/down register   - PgUp | PgDown  - BC" },
        { 1, " - left  Win      - RUS                    Emulation time hot keys:" },
        { 1, " - right Win      - LAT                     - Print Screen     - Reset RP2040/RP2350 CPU" },
        { 1, " - Pause          - STOP                    - F10              - cyclic change pallete" },
        { 1, " - F1             - POVTOR                  - Alt  + F10       - default BK-0010 pallete" },
        { 1, " - F2             - KT                      - Ctrl + F10       - default BK-0011 pallete" },
        { 1, " - F3             - =|=>|                   - F12              - Switch B/W 512x256 & Color 256x256 and back" },
        { 1, " - F4             - |<==                    - Ctrl + F1..F8    - fast save a snapshot (BK\\SNAP[1..8].BKE)" },
        { 1, " - F5             - |==>                    - Alt  + F1..F8    - fast restore from related snapshot" },
        { 1, " - F6             - IND SU                  - Ctrl + F12       - faster emulation" },
        { 1, " - Ctrl + Alt + Del - Reset BM1 CPU, RAM clenup, set default pages, deafult speed, init system registers" },
        { 1, " - Ctrl + F11       - slower emulation (default emulation is about to BK on 3 MHz)" },
        { 1, " - F11              - adjust brightness" },
        { 1, " - Ctrl + \"+\"       - increase volume" },
        { 1, " - Ctrl + \"-\"       - decrease volume" },
        { 1, " " },
        { 1, " " },
        { 1, "File Manager hot keys:" },
        { 1, " - F10           - exit from the File Manager" },
        { 1, " - F11           - change emulation mode BK-0010-01, BK-0011M..." },
        { 1, " - F12           - Switch B/W 512x256 to Color 256x256 and back" },
        { 1, " - Ctrl + F10    - exit to Murmulator OS 2.0" },
    };
    lines_t lines = { 26, 0, plns };
    draw_box(1, 1, text_buffer_width - 2, text_buffer_height - 2, "Help", &lines);
    enterPressed = escWasPressed = false;
    nespad_state_delay = DPAD_STATE_DELAY;
    f1Pressed = true;
    while(!escWasPressed) {
        if (is_dendy_joystick || is_kbd_joystick) {
            if (is_dendy_joystick) nespad_read();
            if (nespad_state && !(nespad_state & DPAD_START) && !(nespad_state & DPAD_SELECT)) {
                nespad_state_delay = DPAD_STATE_DELAY;
                break;
            }
        }
    }
    escWasPressed = false;
    f1Pressed = false;
    scan_code_cleanup();
    redraw_window();
}

static void fast_0010(uint8_t cmd) {
    g_conf.bk0010mode = BK_FDD;
    set_bk0010mode(g_conf.bk0010mode);
    saveConf();
    reset(3);
}

static void fast_0011M(uint8_t cmd) {
    g_conf.bk0010mode = BK_0011M_FDD;
    set_bk0010mode(g_conf.bk0010mode);
    saveConf();
    reset(4);
}

static fn_1_12_tbl_t fn_1_12_tbl = {
    ' ', '1', " Help ", m_info,
    ' ', '2', " Conf ", conf_it,
    ' ', '3', " 0010 ", fast_0010,
    ' ', '4', " 0011M", fast_0011M,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', "MkDir ", m_mk_dir,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', " Swap ", swap_drives,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "EmMODE", switch_mode,
    '1', '2', " B/W  ", switch_color
};

static fn_1_12_tbl_t fn_1_12_tbl_alt = {
    ' ', '1', "Right ", do_nothing,
    ' ', '2', "ReSnap", restore_snap,
    ' ', '3', " 0010 ", fast_0010,
    ' ', '4', " 0011M", fast_0011M,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', " Find ", do_nothing,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', " UpMn ", do_nothing,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "EmMODE", switch_mode,
    '1', '2', " B/W  ", switch_color
};

static fn_1_12_tbl_t fn_1_12_tbl_ctrl = {
    ' ', '1', "Eject ", do_nothing,
    ' ', '2', " Snap ", save_snap,
    ' ', '3', " 0010 ", fast_0010,
    ' ', '4', " 0011M", fast_0011M,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', " Find ", do_nothing,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', " Swap ", swap_drives,
    '1', '0', " M-OS ", return_to_mos,
    '1', '1', "EmMODE", switch_mode,
    '1', '2', " B/W  ", switch_color
};

static inline fn_1_12_tbl_t* actual_fn_1_12_tbl() {
    const fn_1_12_tbl_t * ptbl = &fn_1_12_tbl;
    if (altPressed) {
        ptbl = &fn_1_12_tbl_alt;
    } else if (ctrlPressed) {
        ptbl = &fn_1_12_tbl_ctrl;
    }
    return ptbl;
}

static void bottom_line() {
    int bw = BTN_WIDTH;
    if (SELECT_VGA && !g_conf.is_128_48) { // W/A for 64 characters in a line
        bw -= 3;
    }
    for (int i = 0; i < BTNS_COUNT; ++i) {
        const fn_1_12_tbl_rec_t* rec = &(*actual_fn_1_12_tbl())[i];
        if (SELECT_VGA && !g_conf.is_128_48) { // W/A for 64 characters in a line
            fn_1_12_tbl_rec_t tmp;
            memcpy(&tmp, rec, sizeof(fn_1_12_tbl_rec_t));
            strncpy(tmp.name, rec->name + 1, BTN_WIDTH  - 2);
            draw_fn_btn(&tmp, i * bw, F_BTN_Y_POS, true);
        } else {
            draw_fn_btn(rec, i * bw, F_BTN_Y_POS, false);
        }
    }
    draw_text(
        bk_mode_lns[g_conf.bk0010mode].txt,
        text_buffer_width - strlen(bk_mode_lns[g_conf.bk0010mode].txt) - 2,
        PANEL_LAST_Y,
        get_color_schema()->HIGHLIGHTED_FIELD_COLOR,
        get_color_schema()->BACKGROUND_FIELD_COLOR
    );
    char buf[4];
    snprintf(buf, 4, " %s%s", g_conf.is_AY_on ? "A" : " ", g_conf.is_covox_on ? "C" : " ");
    draw_text( // TODO: move to pico-vision
        buf,
        bw * BTNS_COUNT,
        F_BTN_Y_POS,
        5, // red
        0  // black
    );
    draw_cmd_line(0, CMD_Y_POS, os_type);
}

inline static bool switch_mode_dialog(bk_mode_t* pbk0010mode) {
    bk_mode_t bk0010mode = *pbk0010mode;
    const lines_t lines = { 5, 1, bk_mode_lns };
    bk0010mode = draw_selector(
        (text_buffer_width - 30) / 2,
        (text_buffer_height - 9) / 2,
         30,
         9,
         "BK Emulation Mode",
         &lines,
         bk0010mode,
         0
    );
    if (escWasPressed) {
        escWasPressed = false;
        scan_code_cleanup();
        return false;
    }
    *pbk0010mode = bk0010mode;
    return true;
}

static void switch_mode(uint8_t cmd) {
    if (switch_mode_dialog(&g_conf.bk0010mode)) {
        set_bk0010mode(g_conf.bk0010mode);
        saveConf();
        reset(11);
    }
    redraw_window();
}

inline static void update_menu_color() {
    const char * m = g_conf.color_mode ? " B/W  " : " Color";
    snprintf(fn_1_12_tbl[11].name, BTN_WIDTH, m);
    snprintf(fn_1_12_tbl_alt[11].name, BTN_WIDTH, m);
    snprintf(fn_1_12_tbl_ctrl[11].name, BTN_WIDTH, m);
    bottom_line();
}

static void switch_color(uint8_t cmd) {
    g_conf.color_mode = !g_conf.color_mode;
    update_menu_color();
}

void m_cleanup_ext() {
    DBGM_PRINT(("m_cleanup_ext"));
    files_count = 0;
}

inline static void m_cleanup() {
    files_count = 0;
}

static const char koi8_2_cp866_arr[] = {
    0xC4, 0xB3, 0xDA, 0xBF, 0xC0, 0xD9, 0xC3, 0xB4, 0xC2, 0xC1, 0xC5, 0xDF, 0xDC, 0xDB, 0xDD, 0xDE,
    0xB0, 0xB1, 0xB2, 0xF4, 0xFE, 0xF9, 0xFB, 0xF7, 0xF3, 0xF2, 0xFF, 0xF5, 0xF8, 0xFD, 0xFA, 0xF6,
    0xCD, 0xBA, 0xD5, 0xF1, 0xD6, 0xC9, 0xB8, 0xB7, 0xBB, 0xD4, 0xD3, 0xC8, 0xBE, 0xBD, 0xBC, 0xC6,
    0xC7, 0xCC, 0xB5, 0xF0, 0xB6, 0xB9, 0xD1, 0xD2, 0xCB, 0xCF, 0xD0, 0xCA, 0xD8, 0xD7, 0xCE, 0x63,
    0xEE, 0xA0, 0xA1, 0xE6, 0xA4, 0xA5, 0xE4, 0xA3, 0xE5, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
    0xAF, 0xEF, 0xE0, 0xE1, 0xE2, 0xE3, 0xA6, 0xA2, 0xEC, 0xEB, 0xA7, 0xE8, 0xED, 0xE9, 0xE7, 0xEA,
    0x9E, 0x80, 0x81, 0x96, 0x84, 0x85, 0x94, 0x83, 0x95, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E,
    0x8F, 0x9F, 0x90, 0x91, 0x92, 0x93, 0x86, 0x82, 0x9C, 0x9B, 0x87, 0x98, 0x9D, 0x99, 0x97, 0x9A 
};

inline static char koi8_2_cp866(char c) {
   if (c < 0x80) return c;
   return koi8_2_cp866_arr[c - 0x80];
}

int m_add_file_ext(const char* fname, bool dir, int dir_num) {
    file_info_t* fp = &files_info[files_count++];
    fp->fattrib = dir ? AM_DIR : 0;
    fp->fsize = 0;
    fp->dir_num = dir_num;
    int i = 0;
    for (; i < (text_buffer_width >> 1) && fname[i] != 0; ++i) {
        fp->name[i] = koi8_2_cp866(fname[i]);
    }
    do { // trim trailing spaces
        fp->name[i] = 0;
    } while (--i && (fp->name[i] == ' ' || fp->name[i] == '\t'));
    return files_count;
}

inline static void m_add_file(FILINFO* fi) {
    if (files_count >= MAX_FILES) {
        // WARN?
        return;
    }
    file_info_t* fp = &files_info[files_count++];
    fp->fattrib = fi->fattrib;
    fp->fdate   = fi->fdate;
    fp->ftime   = fi->ftime;
    fp->fsize   = fi->fsize;
    strncpy(fp->name, fi->fname, text_buffer_width >> 1);
}

inline static bool m_opendir(
	DIR* dp,			/* Pointer to directory object to create */
	const TCHAR* path	/* Pointer to the directory path */
) {
    if (f_opendir(dp, path) != FR_OK) {
        const line_t lns[1] = {
            { -1, "It is not a folder!" }
        };
        const lines_t lines = { 1, 4, lns };
        draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Warning", &lines);
        sleep_ms(1500);
        redraw_window();
        return false;
    }
    return true;
}

static int m_comp(const file_info_t * e1, const file_info_t * e2) {
    if ((e1->fattrib & AM_DIR) && !(e2->fattrib & AM_DIR)) return -1;
    if (!(e1->fattrib & AM_DIR) && (e2->fattrib & AM_DIR)) return 1;
    return strncmp(e1->name, e2->name, text_buffer_width >> 1);
}

inline static void collect_files(file_panel_desc_t* p) {
    if (!SD_CARD_AVAILABLE) return;
    m_cleanup();
#if EXT_DRIVES_MOUNT
    if (p->in_dos) {
        if(!mount_img(p->path, p->indexes[p->level].dir_num)) {
           return;
        }
        qsort (files_info, files_count, sizeof(file_info_t), (__compar_fn_t)m_comp);
        return;
    }
#endif
    DIR dir;
    if (!m_opendir(&dir, p->path)) return;
    FILINFO fileInfo;
    while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        m_add_file(&fileInfo);
    }
    f_closedir(&dir);
    qsort (files_info, files_count, sizeof(file_info_t), (__compar_fn_t)m_comp);
}

static char selected_file_path[260];

static inline void fill_panel(file_panel_desc_t* p) {
    if (!SD_CARD_AVAILABLE) return;
    collect_files(p);
    indexes_t* pp = &p->indexes[p->level];
    if (pp->selected_file_idx < FIRST_FILE_LINE_ON_PANEL_Y)
        pp->selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
    if (pp->start_file_offset < 0)
        pp->start_file_offset = 0;
    int y = 1;
    p->files_number = 0;
    int start_file_offset = pp->start_file_offset;
    int selected_file_idx = pp->selected_file_idx;
    if (start_file_offset == 0 && strlen(p->path) > 1) {
        draw_label(p->left + 1, y, p->width - 2, "..", p == psp && selected_file_idx == y, true);
        y++;
        p->files_number++;
    }
    for(int fn = 0; fn < files_count; ++ fn) {
        file_info_t* fp = &files_info[fn];
        if (start_file_offset <= p->files_number && y <= LAST_FILE_LINE_ON_PANEL_Y) {
            char* filename = fp->name;
            snprintf(line, text_buffer_width, "%s\\%s", p->path, fp->name);
#if EXT_DRIVES_MOUNT
            if (!p->in_dos)
#endif
            for (int i = 0; i < 4; ++i) { // mark mounted drived by labels FDD0,FDD1,FDD2,FDD3...
                if (drives_states[i].path && strncmp(drives_states[i].path, line, text_buffer_width) == 0) {
                    snprintf(line, p->width, "%s", fp->name);
                    for (int j = strlen(fp->name); j < p->width - 6; ++j) {
                        line[j] = ' ';
                    }
                    snprintf(line + p->width - 6, 6, "%s", drives_states[i].lbl);
                    filename = line;
                    break;
                }
            }
            bool selected = p == psp && selected_file_idx == y;
            draw_label(p->left + 1, y, p->width - 2, filename, selected, fp->fattrib & AM_DIR);
            if (
#if EXT_DRIVES_MOUNT
                !p->in_dos &&
#endif
                selected && (fp->fattrib & AM_DIR) == 0
            ) {
                char path[260];
                construct_full_name(path, p->path, fp->name);
                if (strncmp(selected_file_path, path, 260) != 0) {
                    detect_os_type(path, os_type, 160);
                    strncpy(selected_file_path, path, 260);
                }
                draw_cmd_line(0, CMD_Y_POS, os_type);
            } else if (
#if EXT_DRIVES_MOUNT
              p->in_dos ||
#endif
              selected
            ) {
                os_type[0] = 0;
                draw_cmd_line(0, CMD_Y_POS, 0);
            }
            y++;
        }
        p->files_number++;
    }
    for (; y <= LAST_FILE_LINE_ON_PANEL_Y; ++y) {
        draw_label(p->left + 1, y, p->width - 2, "", false, false);
    }
}

inline static void select_right_panel() {
    psp = &right_panel;
    fill_panel(&left_panel);
    fill_panel(&right_panel);
}

inline static void select_left_panel() {
    psp = &left_panel;
    fill_panel(&left_panel);
    fill_panel(&right_panel);
}

inline static void fn_1_12_btn_pressed(uint8_t fn_idx) {
    if (fn_idx > 11) fn_idx -= 18; // F11-12
    (*actual_fn_1_12_tbl())[fn_idx].action(fn_idx);
}

inline static void handle_pagedown_pressed() {
    indexes_t* p = &psp->indexes[psp->level];
    for (int i = 0; i < text_buffer_height / 2; ++i) {
        if (p->selected_file_idx < LAST_FILE_LINE_ON_PANEL_Y &&
            p->start_file_offset + p->selected_file_idx < psp->files_number
        ) {
            p->selected_file_idx++;
        } else if (
            p->selected_file_idx == LAST_FILE_LINE_ON_PANEL_Y &&
            p->start_file_offset + p->selected_file_idx < psp->files_number
        ) {
            p->selected_file_idx--;
            p->start_file_offset++;
        }
    }
    fill_panel(psp);
    scan_code_processed();
}

inline static void handle_down_pressed() {
    indexes_t* p = &psp->indexes[psp->level];
    if (p->selected_file_idx < LAST_FILE_LINE_ON_PANEL_Y &&
        p->start_file_offset + p->selected_file_idx < psp->files_number
    ) {
        p->selected_file_idx++;
        fill_panel(psp);
    } else if (
        p->selected_file_idx == LAST_FILE_LINE_ON_PANEL_Y &&
        p->start_file_offset + p->selected_file_idx < psp->files_number
    ) {
        p->selected_file_idx -= 5;
        p->start_file_offset += 5;
        fill_panel(psp);    
    }
    scan_code_processed();
}

inline static void handle_pageup_pressed() {
    indexes_t* p = &psp->indexes[psp->level];
    for (int i = 0; i < text_buffer_height / 2; ++i) {
        if (p->selected_file_idx > FIRST_FILE_LINE_ON_PANEL_Y) {
            p->selected_file_idx--;
        } else if (p->selected_file_idx == FIRST_FILE_LINE_ON_PANEL_Y && p->start_file_offset > 0) {
            p->selected_file_idx++;
            p->start_file_offset--;
        }
    }
    fill_panel(psp);
    scan_code_processed();
}

inline static void handle_up_pressed() {
    indexes_t* p = &psp->indexes[psp->level];
    if (p->selected_file_idx > FIRST_FILE_LINE_ON_PANEL_Y) {
        p->selected_file_idx--;
        fill_panel(psp);
    } else if (p->selected_file_idx == FIRST_FILE_LINE_ON_PANEL_Y && p->start_file_offset > 0) {
        p->selected_file_idx += 5;
        p->start_file_offset -= 5;
        fill_panel(psp);       
    }
    scan_code_processed();
}

static inline void redraw_current_panel() {
    sprintf(line, "SD:%s", psp->path);
    draw_panel(psp->left, PANEL_TOP_Y, psp->width, PANEL_LAST_Y + 1, line, 0);
    fill_panel(psp);
    draw_cmd_line(0, CMD_Y_POS, os_type);
}

static inline bool run_bin(char* path) {
    enterPressed = false;
#if EXT_DRIVES_MOUNT
    if (psp->in_dos) {
        // TODO:
        do_nothing(0);
        return false;
    }
#endif
///    bk_mode_t m = BK_0010_01;
///    if (!switch_mode_dialog(&m)) {
///        redraw_window();
///        return false;
///    }
    gpio_put(PICO_DEFAULT_LED_PIN, true);
///    set_bk0010mode(m);
    main_init();
    FIL file;
    FRESULT result = f_open(&file, path, FA_READ);
    if (result != FR_OK) {
        const line_t lns[2] = {
            { -1, "Selected file was not found!" },
            { -1, path }
        };
        const lines_t lines = { 2, 3, lns };
        draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Error", &lines);
        sleep_ms(1500);
        redraw_window();
        return false;
    }
    UINT bw;
    result = f_read(&file, line, 4, &bw);
    uint16_t offset = ((uint16_t)line[1] << 8) | line[0];
    uint16_t len = ((uint16_t)line[3] << 8) | line[2];
    if (result != FR_OK) {
        f_close(&file);
        snprintf(line, text_buffer_width, "FRESULT: %d (bw: %d)", result, bw);
        const line_t lns[3] = {
            { -1, "Unable to read selected file!" },
            { -1, path },
            { -1, line }
        };
        const lines_t lines = { 3, 2, lns };
        draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Error", &lines);
        sleep_ms(1500);
        redraw_window();
        return false;
    }
    uint16_t len2 = (len > (16 << 10) - offset) ? (16 << 10) - offset : len;
    result = f_read(&file, RAM + offset, len2, &bw);
    if (result != FR_OK) {
        f_close(&file);
        snprintf(line, text_buffer_width, "FRESULT: %d (bw: %d)", result, bw);
        const line_t lns[3] = {
            { -1, "Unable to read selected file!" },
            { -1, path },
            { -1, line }
        };
        const lines_t lines = { 3, 2, lns };
        draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Error", &lines);
        sleep_ms(1500);
        redraw_window();
        return false;
    }
    if (len2 != len) {
        result = f_read(&file, CPU_PAGE51_MEM_ADR, len - len2, &bw);
        // TODO: more than 1 page, error handling
    }
    f_close(&file);
    if (offset < 01000) { // assimed autostat
        PC = ((uint16_t)RAM[offset + 1] << 8) | RAM[offset];
    } else {
        PC = offset;
    }
    SP = 01000;
    mark_to_exit_flag = true;
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    return true;
}

static const line_t drive_num_lns[] = {
    { -1, " FDD0 (A:) " },
    { -1, " FDD1 (B:) " },
    { -1, " FDD2 (C:) " },
    { -1, " FDD3 (D:) " }
};

static inline bool run_img(char* path) {
    DBGM_PRINT(("run_img: %s, ctrlPressed: %d", path, ctrlPressed));
    enterPressed = false;
#if EXT_DRIVES_MOUNT
    if (ctrlPressed) {
        if (psp->in_dos || !is_browse_os_supported()) {
            // TODO:
            do_nothing(0);
            return false;
        }
        strncpy(psp->path, path, 256);
        psp->in_dos = true;
        psp->level++;
        if (psp->level >= 16) {
            psp->level = 15;
        }
        psp->indexes[psp->level].selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
        psp->indexes[psp->level].start_file_offset = 0;
        redraw_window();
        DBGM_PRINT(("run_img: %s done", path));
        return true;
    }
#endif
    const lines_t lines = { 4, 1, drive_num_lns };
    int mount_as = draw_selector(
        (text_buffer_width - 30) / 2,
        (text_buffer_height - 8) / 2,
        30,
        8,
        "Device to mount", &lines, 2, "Hold ALT - noreboot"
    );
    if (escWasPressed) {
        escWasPressed = false;
        scan_code_cleanup();
        redraw_window();
        return false;
    }
    insertdisk(mount_as, 819200, 0, path);
    if ( !is_fdd_suppored() ) {
        set_bk0010mode(BK_0011M_FDD);
    }
    if (altPressed) {
        redraw_window();
        return false;
    }
    reset(0);
    mark_to_exit_flag = true;
    return true;
}

static file_info_t* selected_file() {
    int start_file_offset = psp->indexes[psp->level].start_file_offset;
    int selected_file_idx = psp->indexes[psp->level].selected_file_idx;
    if (selected_file_idx == FIRST_FILE_LINE_ON_PANEL_Y && start_file_offset == 0 && strlen(psp->path) > 1) {
        return 0;
    }
    collect_files(psp);
    int y = 1;
    int files_number = 0;
    if (start_file_offset == 0 && strlen(psp->path) > 1) {
        y++;
        files_number++;
    }
    for(int fn = 0; fn < files_count; ++ fn) {
        file_info_t* fp = &files_info[fn];
        if (start_file_offset <= files_number && y <= LAST_FILE_LINE_ON_PANEL_Y) {
            if (selected_file_idx == y) {
                return fp;
            }
            y++;
        }
        files_number++;
    }
    return 0; // ?? what a case?
}

static inline void enter_pressed() {
    file_info_t* fp = selected_file();
    if (!fp) { // up to parent dir
        int i = strlen(psp->path);
        while(--i > 0) {
            if (psp->path[i] == '\\') {
                psp->path[i] = 0;
                psp->level--;
                redraw_current_panel();
                return;
            }
        }
        psp->path[0] = '\\';
        psp->path[1] = 0;
#if EXT_DRIVES_MOUNT
        psp->in_dos = false;
#endif
        psp->level--;
        redraw_current_panel();
        return;
    }
    if (fp->fattrib & AM_DIR) {
        char path[256];
        construct_full_name(path, psp->path, fp->name);
        strncpy(psp->path, path, 256);
        psp->level++;
        if (psp->level >= 16) {
            psp->level = 15;
        }
        psp->indexes[psp->level].selected_file_idx = FIRST_FILE_LINE_ON_PANEL_Y;
        psp->indexes[psp->level].start_file_offset = 0;
        psp->indexes[psp->level].dir_num = fp->dir_num;
        redraw_current_panel();
        return;
    } else {
        size_t slen = strlen(fp->name);
        if (slen > 4 && fp->name[slen - 4] == '.')
        if(
            (fp->name[slen - 1] == 'N' || fp->name[slen - 1] == 'n') &&
            (fp->name[slen - 2] == 'I' || fp->name[slen - 2] == 'i') &&
            (fp->name[slen - 3] == 'B' || fp->name[slen - 3] == 'b')
        ) {
            char path[256];
            construct_full_name(path, psp->path, fp->name);
            run_bin(path);
            return;
        } else if (
            (fp->name[slen - 1] == 'G' || fp->name[slen - 1] == 'g') &&
            (fp->name[slen - 2] == 'M' || fp->name[slen - 2] == 'm') &&
            (fp->name[slen - 3] == 'I' || fp->name[slen - 3] == 'i')
        ) {
            char path[256];
            construct_full_name(path, psp->path, fp->name);
            run_img(path);
            return;
        } else if (
            (fp->name[slen - 1] == 'D' || fp->name[slen - 1] == 'd') &&
            (fp->name[slen - 2] == 'K' || fp->name[slen - 2] == 'k') &&
            (fp->name[slen - 3] == 'B' || fp->name[slen - 3] == 'b')
        ) {
            char path[256];
            construct_full_name(path, psp->path, fp->name);
            run_img(path); // TODO: separate support for .BKD
            return;
        } else if (
            (fp->name[slen - 1] == 'E' || fp->name[slen - 1] == 'e') &&
            (fp->name[slen - 2] == 'K' || fp->name[slen - 2] == 'k') &&
            (fp->name[slen - 3] == 'B' || fp->name[slen - 3] == 'b')
        ) {
            char path[256];
            construct_full_name(path, psp->path, fp->name);
            restore_snap_by_filename(path);
            return;
        }
    }
}

static inline void if_sound_control() { // core #0
    if (ctrlPressed && plusPressed && !altPressed) {
        g_conf.snd_volume++;
        if(g_conf.snd_volume > 5) g_conf.snd_volume = 5;
        plusPressed = false;
        saveConf();
    } else if (ctrlPressed && minusPressed && !altPressed) {
        g_conf.snd_volume--;
        if(g_conf.snd_volume < -16) g_conf.snd_volume = -16;
        minusPressed = false;
        saveConf();
    } else if (ctrlPressed && plusPressed && altPressed) {
        covox_mix = covox_mix << 1 | 01;
        if (covox_mix > 0x3F) covox_mix = 0x3F;
        plusPressed = false;
    } else if (ctrlPressed && minusPressed && altPressed) {
        covox_mix = covox_mix >> 1;
        minusPressed = false;
    } else if (ctrlPressed && tabPressed && cPressed) {
        g_conf.is_covox_on = !g_conf.is_covox_on;
        cPressed = false;
        saveConf();
    } else if (ctrlPressed && tabPressed && aPressed) {
        g_conf.is_AY_on = !g_conf.is_AY_on;
        cPressed = false;
        saveConf();
    }
}

static uint8_t repeat_cnt = 0;

static inline void work_cycle() {
    while(1) {
        if (is_dendy_joystick || is_kbd_joystick) {
            if (is_dendy_joystick) nespad_read();
            if (nespad_state_delay > 0) {
                nespad_state_delay--;
                sleep_ms(1);
            }
            else {
                nespad_state_delay = DPAD_STATE_DELAY;
                if(nespad_state & DPAD_UP) {
                    handle_up_pressed();
                } else if(nespad_state & DPAD_DOWN) {
                    handle_down_pressed();
                } else if (nespad_state & DPAD_START) {
                    enter_pressed();
                } else if ((nespad_state & DPAD_A) && (nespad_state & DPAD_START)) {
                    return_to_mos(0);
                } else if ((nespad_state & DPAD_B) && (nespad_state & DPAD_SELECT)) {
                    mark_to_exit(0);
                } else if ((nespad_state & DPAD_LEFT) || (nespad_state & DPAD_RIGHT)) {
                    left_panel_make_active = !left_panel_make_active;
                } else if ((nespad_state & DPAD_A) && (nespad_state & DPAD_SELECT)) {
                    conf_it(0);
                } else if ((nespad_state & DPAD_B) && (nespad_state & DPAD_START)) {
                    reset(0);
                    return;
                }
            }
        }
        if (ctrlPressed && altPressed && delPressed) {
            ctrlPressed = altPressed = delPressed = false;
            reset(0);
            return;
        }
        if_swap_drives();
      //  if_overclock();
        if_sound_control();
        if (psp == &left_panel && !left_panel_make_active) {
            select_right_panel();
        }
        if (psp != &left_panel && left_panel_make_active) {
            select_left_panel();
        }
        if (lastSavedScanCode != lastCleanableScanCode && lastSavedScanCode > 0x80) {
            repeat_cnt = 0;
        } else {
            repeat_cnt++;
            if (repeat_cnt > 0xFE && lastSavedScanCode < 0x80) {
               lastCleanableScanCode = lastSavedScanCode + 0x80;
               repeat_cnt = 0;
            }
        }
        //if (lastCleanableScanCode) DBGM_PRINT(("lastCleanableScanCode: %02Xh", lastCleanableScanCode));
        switch(lastCleanableScanCode) {
          case 0x01: // Esc down
            scan_code_cleanup();
            break;
          case 0x81: // Esc up
         //   mark_to_exit(9);
            scan_code_cleanup();
            break;
          case 0x3B: // F1..10 down
          case 0x3C: // F2
          case 0x3D: // F3
          case 0x3E: // F4
          case 0x3F: // F5
          case 0x40: // F6
          case 0x41: // F7
          case 0x42: // F8
          case 0x43: // F9
          case 0x44: // F10
          case 0x57: // F11
          case 0x58: // F12
            scan_code_processed();
            break;
          case 0xBB: // F1..10 up
          case 0xBC: // F2
          case 0xBD: // F3
          case 0xBE: // F4
          case 0xBF: // F5
          case 0xC0: // F6
          case 0xC1: // F7
          case 0xC2: // F8
          case 0xC3: // F9
          case 0xC4: // F10
          case 0xD7: // F11
          case 0xD8: // F12
            if (lastSavedScanCode != lastCleanableScanCode - 0x80) {
                break;
            }
            fn_1_12_btn_pressed(lastCleanableScanCode - 0xBB);
            scan_code_processed();
            break;
          case 0x1D: // Ctrl down
          case 0x9D: // Ctrl up
          case 0x38: // ALT down
          case 0xB8: // ALT up
            bottom_line();
            scan_code_processed();
            break;
          case 0x50: // down arr down
            scan_code_processed();
            break;
          case 0xD0: // down arr up
            if (lastSavedScanCode != 0x50) {
                break;
            }
            handle_down_pressed();
            break;
          case 0x49: // pageup arr down
            scan_code_processed();
            break;
          case 0xC9: // pageup arr up
            if (lastSavedScanCode != 0x49) {
                break;
            }
            handle_pageup_pressed();
            break;
          case 0x51: // pagedown arr down
            scan_code_processed();
            break;
          case 0xD1: // pagedown arr up
            if (lastSavedScanCode != 0x51) {
                break;
            }
            handle_pagedown_pressed();
            break;
          case 0x48: // up arr down
            scan_code_processed();
            break;
          case 0xC8: // up arr up
            if (lastSavedScanCode != 0x48) {
                break;
            }
            handle_up_pressed();
            break;
          case 0xCB: // left
            break;
          case 0xCD: // right
            break;
          case 0x1C: // Enter down
            scan_code_processed();
            break;
          case 0x9C: // Enter up
            if (lastSavedScanCode != 0x1C) {
                break;
            }
            enter_pressed();
            scan_code_processed();
            break;
        }
        if(mark_to_exit_flag) {
            return;
        }
        //sprintf(line, "scan-code: %02Xh / saved scan-code: %02Xh", lastCleanableScanCode, lastSavedScanCode);
        //draw_cmd_line(0, CMD_Y_POS, line);
    }
}

inline static void start_manager() {
    mark_to_exit_flag = false;
    save_video_ram();
    graphics_set_mode(TEXTMODE_);
    set_start_debug_line(text_buffer_height);
    if (SD_CARD_AVAILABLE) {
        draw_window();
        select_left_panel();
        update_menu_color();
      ///  m_info(0); // F1 TODO: ensure it is not too aggressive
        work_cycle();
    } else {
        if (switch_mode_dialog(&g_conf.bk0010mode) ) {
            reset(0);
        }
    }
    set_start_debug_line(25); // ?? to be removed
    graphics_set_mode(g_conf.color_mode ? BK_256x256x2 : BK_512x256x1);
    restore_video_ram();
}

inline static void handleJoystickEmulation(uint8_t sc) { // core 1
    if (!is_kbd_joystick) {
        kbdpad_state = 0;
        kbdpad_state2 = 0;
        return;
    }
    DBGM_PRINT(("handleJoystickEmulation: %02Xh", sc));
    if (sc == kbdpad1_A)               kbdpad_state  |=  DPAD_A;
    if (sc == kbdpad1_A + 0x80)        kbdpad_state  &= ~DPAD_A;
    if (sc == kbdpad2_A)               kbdpad_state2 |=  DPAD_A;
    if (sc == kbdpad2_A + 0x80)        kbdpad_state2 &= ~DPAD_A;
    if (sc == kbdpad1_B)               kbdpad_state  |=  DPAD_B;
    if (sc == kbdpad1_B + 0x80)        kbdpad_state  &= ~DPAD_B;
    if (sc == kbdpad2_B)               kbdpad_state2 |=  DPAD_B;
    if (sc == kbdpad2_B + 0x80)        kbdpad_state2 &= ~DPAD_B;
    if (sc == kbdpad1_START)           kbdpad_state  |=  DPAD_START;
    if (sc == kbdpad1_START + 0x80)    kbdpad_state  &= ~DPAD_START;
    if (sc == kbdpad2_START)           kbdpad_state2 |=  DPAD_START;
    if (sc == kbdpad2_START + 0x80)    kbdpad_state2 &= ~DPAD_START;
    if (sc == kbdpad1_SELECT)          kbdpad_state  |=  DPAD_SELECT;
    if (sc == kbdpad1_SELECT + 0x80)   kbdpad_state  &= ~DPAD_SELECT;
    if (sc == kbdpad2_SELECT)          kbdpad_state2 |=  DPAD_SELECT;
    if (sc == kbdpad2_SELECT + 0x80)   kbdpad_state2 &= ~DPAD_SELECT;
    if (sc == kbdpad1_UP)              kbdpad_state  |=  DPAD_UP;
    if (sc == kbdpad1_UP + 0x80)       kbdpad_state  &= ~DPAD_UP;
    if (sc == kbdpad2_UP)              kbdpad_state2 |=  DPAD_UP;
    if (sc == kbdpad2_UP + 0x80)       kbdpad_state2 &= ~DPAD_UP;
    if (sc == kbdpad1_DOWN)            kbdpad_state  |=  DPAD_DOWN;
    if (sc == kbdpad1_DOWN + 0x80)     kbdpad_state  &= ~DPAD_DOWN;
    if (sc == kbdpad2_DOWN)            kbdpad_state2 |=  DPAD_DOWN;
    if (sc == kbdpad2_DOWN + 0x80)     kbdpad_state2 &= ~DPAD_DOWN;
    if (sc == kbdpad1_LEFT)            kbdpad_state  |=  DPAD_LEFT;
    if (sc == kbdpad1_LEFT + 0x80)     kbdpad_state  &= ~DPAD_LEFT;
    if (sc == kbdpad2_LEFT)            kbdpad_state2 |=  DPAD_LEFT;
    if (sc == kbdpad2_LEFT + 0x80)     kbdpad_state2 &= ~DPAD_LEFT;
    if (sc == kbdpad1_RIGHT)           kbdpad_state  |=  DPAD_RIGHT;
    if (sc == kbdpad1_RIGHT + 0x80)    kbdpad_state  &= ~DPAD_RIGHT;
    if (sc == kbdpad2_RIGHT)           kbdpad_state2 |=  DPAD_RIGHT;
    if (sc == kbdpad2_RIGHT + 0x80)    kbdpad_state2 &= ~DPAD_RIGHT;
}

bool handleScancode(uint32_t ps2scancode) { // core 1
    DBGM_PRINT(("handleScancode: %08Xh", ps2scancode));
    handleJoystickEmulation((uint8_t)ps2scancode);
    lastCleanableScanCode = ps2scancode;
    switch (ps2scancode) {
      case 0x01: // Esc down
        scan_code_cleanup(); break;
      case 0x81: // Esc up
        escWasPressed = true; break;
      case 0x4B: // left
        leftPressed = true; break;
      case 0xCB: // left
        leftPressed = false; break;
      case 0x4D: // right
        rightPressed = true;  break;
      case 0xCD: // right
        rightPressed = false;  break;
      case 0x48:
        upPressed = true;
        break;
      case 0xC8:
        upPressed = false;
        break;
      case 0x50:
        downPressed = true;
        break;
      case 0xD0:
        downPressed = false;
        break;
        case 0x38:
        altPressed = true;
        break;
      case 0xB8:
        altPressed = false;
        break;
      case 0x2A:
      case 0x36:
        shiftPressed = true;
        break;
      case 0xAA:
      case 0xB6:
        shiftPressed = false;
        break;
      case 0x0E:
        backspacePressed = true;
        break;
      case 0x8E:
        backspacePressed = false;
        break;
      case 0x1C:
        enterPressed = true;
        break;
      case 0x9C:
        enterPressed = false;
        break;
      case 0x0C: // -
      case 0x4A: // numpad -
        minusPressed = true;
        break;
      case 0x8C: // -
      case 0xCA: // numpad 
        minusPressed = false;
        break;
      case 0x0D: // +=
      case 0x4E: // numpad +
        plusPressed = true;
        break;
      case 0x8D: // += 82?
      case 0xCE: // numpad +
        plusPressed = false;
        break;
      case 0x1D:
        ctrlPressed = true;
        break;
      case 0x9D:
        ctrlPressed = false;
        break;
      case 0x20:
        dPressed = true;
        break;
      case 0xA0:
        dPressed = false;
        break;
      case 0x2E:
        cPressed = true;
        break;
      case 0xAE:
        cPressed = false;
        break;
      case 0x14:
        tPressed = true;
        break;
      case 0x94:
        tPressed = false;
        break;
      case 0x22:
        gPressed = true;
        break;
      case 0xA2:
        gPressed = false;
        break;
      case 0x1E:
        aPressed = true;
        break;
      case 0x9E:
        aPressed = false;
        break;
      case 0x1F:
        sPressed = true;
        break;
      case 0x9F:
        sPressed = false;
        break;
      case 0x2D:
        xPressed = true;
        break;
      case 0xAD:
        xPressed = false;
        break;
      case 0x12:
        ePressed = true;
        break;
      case 0x92:
        ePressed = false;
        break;
      case 0x16:
        uPressed = true;
        break;
      case 0x96:
        uPressed = false;
        break;
      case 0x23:
        hPressed = true;
        break;
      case 0xA3:
        hPressed = false;
        break;
      case 0x3B: // F1..10 down
        f1Pressed = true; break;
      case 0x3C: // F2
        f2Pressed = true; break;
      case 0x3D: // F3
        f3Pressed = true; break;
      case 0x3E: // F4
        f4Pressed = true; break;
      case 0x3F: // F5
        f5Pressed = true; break;
      case 0x40: // F6
        f6Pressed = true; break;
      case 0x41: // F7
        f7Pressed = true; break;
      case 0x42: // F8
        f8Pressed = true; break;
      case 0x43: // F9
        f9Pressed = true; break;
      case 0x44: // F10
        f10Pressed = true; break;
      case 0x57: // F11
        f11Pressed = true; break;
      case 0x58: // F12
        f12Pressed = true; break;
      case 0xBB: // F1..10 up
        f1Pressed = false; break;
      case 0xBC: // F2
        f2Pressed = false; break;
      case 0xBD: // F3
        f3Pressed = false; break;
      case 0xBE: // F4
        f4Pressed = false; break;
      case 0xBF: // F5
        f5Pressed = false; break;
      case 0xC0: // F6
        f6Pressed = false; break;
      case 0xC1: // F7
        f7Pressed = false; break;
      case 0xC2: // F8
        f8Pressed = false; break;
      case 0xC3: // F9
        f9Pressed = false; break;
      case 0xC4: // F10
        f10Pressed = false; break;
      case 0xD7: // F11
        f11Pressed = false; break;
      case 0xD8: // F12
        f12Pressed = false; break;
      case 0x53: // Del down
        delPressed = true; break;
      case 0xD3: // Del up
        delPressed = false; break;
      case 0x39:
        spacePressed = true; break;
      case 0xB9:
        spacePressed = false; break;
      case 0x0F:
        tabPressed = true;
        break;
      case 0x8F:
        if (manager_started)
            left_panel_make_active = !left_panel_make_active; // TODO: combinations?
        tabPressed = false;
        break;
      default:
        //DBGM_PRINT(("handleScancode default: %02Xh", ps2scancode));
        break;
    }
    //if (ps2scancode) DBGM_PRINT(("handleScancode processed: %02Xh", ps2scancode));
    return manager_started;
}

inline static int overclock() {
  if (tabPressed && ctrlPressed) {
    if (plusPressed) return 1;
    if (minusPressed) return -1;
  }
  return 0;
}

inline void if_overclock() {
    /**
    int oc = overclock();
    if (oc > 0) overcloking_khz += 1000;
    if (oc < 0) overcloking_khz -= 1000;
    if (oc != 0) {
    save_video_ram();
        enum graphics_mode_t ret = graphics_set_mode(TEXTMODE_);
        if (ret != TEXTMODE_) clrScr(1);
        uint vco, postdiv1, postdiv2;
        if (check_sys_clock_khz(overcloking_khz, &vco, &postdiv1, &postdiv2)) {
            set_sys_clock_pll(vco, postdiv1, postdiv2);
            sprintf(line, "overcloking_khz: %u kHz", overcloking_khz);
            line_t lns[1] = {
                { -1, line }
            };
            lines_t lines = { 1, 3, lns };
            draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Info", &lines);
        }
        else {
            sprintf(line, "System clock of %u kHz cannot be achieved", overcloking_khz);
            line_t lns[1] = {
                { -1, line }
            };
            lines_t lines = { 1, 3, lns };
            draw_box((text_buffer_width - 60) / 2, 7, 60, 10, "Warning", &lines);
        }
        sleep_ms(2500);
        graphics_set_mode(ret);
        restore_video_ram();
    }
    */
}

void manager(bool force) {
    if (ctrlPressed) {
        if (f11Pressed) {
            f11Pressed = false;
            if (g_conf.cpu_freq > 500000) g_conf.cpu_freq -= 500000; // 500 MHz
        }
        if (f12Pressed) {
            f12Pressed = false;
            g_conf.cpu_freq += 500000; // 500 MHz
        }
    }
    if_video_mode();
    if_swap_drives();
    if_overclock();
    if_sound_control();
    if (manager_started) {
        return;
    }
    if (is_dendy_joystick || is_kbd_joystick) {
        if (is_dendy_joystick) nespad_read();
        if((nespad_state & DPAD_START) && (nespad_state & DPAD_SELECT)) {
            nespad_state_delay = DPAD_STATE_DELAY;
            force = true;
        } else {
            Device_Data.SysRegs.RdReg177714 = ((uint16_t)kbdpad_state2 << 8) | nespad_state | kbdpad_state;
        }
    }
    if (force) {
        true_covox = 0;
        az_covox_R = 0;
        az_covox_L = 0;
        escWasPressed = false;
        scan_code_cleanup();
        beep(0);
        ps2cleanup();
        manager_started = true;
        start_manager();
        manager_started = false;
    }
}

#endif

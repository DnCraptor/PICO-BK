#if PICO_ON_DEVICE
#include "manager.h"
#include "emulator.h"
#include "vga.h"
#include "usb.h"
#include "pico-vision.h"
#include "CPU.h"
#include "CPU_i.h"
#include "stdlib.h"

#ifdef MNGR_DEBUG
extern void logMsg(char* msg);
#define printf(...) { char tmp[80]; snprintf(tmp, 80, __VA_ARGS__); logMsg(tmp); }
#define DBGM_PRINT( X) printf X
#else
#define DBGM_PRINT( X)
#endif

static void bottom_line();
static void redraw_window();

static volatile int tormoz = 6;
static volatile bool backspacePressed = false;
static volatile bool enterPressed = false;
static volatile bool plusPressed = false;
static volatile bool minusPressed = false;
static volatile bool ctrlPressed = false;
static volatile bool altPressed = false;
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
static volatile bool escPressed = false;
static volatile bool leftPressed = false;
static volatile bool rightPressed = false;
static volatile bool upPressed = false;
static volatile bool downPressed = false;
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

volatile bool is_covox_on = true;
volatile bool is_sound_on = true;
volatile uint8_t snd_divider = 0;
volatile int8_t covox_multiplier = 7;

bool already_swapped_fdds = false;
volatile bool manager_started = false;
volatile bool usb_started = false;
static char line[MAX_WIDTH + 2];

static volatile uint32_t lastCleanableScanCode = 0;
static uint32_t lastSavedScanCode = 0;

inline static void scan_code_processed() {
  if (lastCleanableScanCode) {
      lastSavedScanCode = lastCleanableScanCode;
  }
  lastCleanableScanCode = 0;
}

inline static void scan_code_cleanup() {
  lastSavedScanCode = 0;
  lastCleanableScanCode = 0;
}

static const uint8_t PANEL_TOP_Y = 0;
static const uint8_t TOTAL_SCREEN_LINES = MAX_HEIGHT;
static const uint8_t F_BTN_Y_POS = TOTAL_SCREEN_LINES - 1;
static const uint8_t CMD_Y_POS = F_BTN_Y_POS - 1;
static const uint8_t PANEL_LAST_Y = CMD_Y_POS - 1;

static uint8_t FIRST_FILE_LINE_ON_PANEL_Y = PANEL_TOP_Y + 1;
static uint8_t LAST_FILE_LINE_ON_PANEL_Y = PANEL_LAST_Y - 1;

typedef struct {
	  FSIZE_t fsize;   /* File size */
	  WORD    fdate;   /* Modified date */
	  WORD    ftime;   /* Modified time */
	  BYTE    fattrib; /* File attribute */
    char    name[MAX_WIDTH >> 1];
} file_info_t;

#define MAX_FILES 500

static file_info_t files_info[MAX_FILES] = { 0 };
static size_t files_count = 0;

typedef struct file_panel_desc {
    int left;
    int width;
    int selected_file_idx;
    int start_file_offset;
    int files_number;
    char path[256];
} file_panel_desc_t;

static file_panel_desc_t left_panel = {
    0, MAX_WIDTH / 2, 1, 0, 0,
    { "\\" },
};

static file_panel_desc_t right_panel = {
    MAX_WIDTH / 2, MAX_WIDTH / 2, 1, 0, 0,
    { "\\BK" },
};

static volatile bool left_panel_make_active = true;
static file_panel_desc_t* psp = &left_panel;

static bool mark_to_exit_flag = false;
static void mark_to_exit(uint8_t cmd) {
    f10Pressed = false;
    escPressed = false;
    if (!usb_started) // TODO: USB in emulation mode
        mark_to_exit_flag = true;
}

static inline void fill_panel(file_panel_desc_t* p);

static void reset(uint8_t cmd) {
    f12Pressed = false;
    tormoz = 6;
    memset(RAM, 0, sizeof RAM);
    graphics_set_page(CPU_PAGE5_MEM_ADR, get_bk0010mode() == BK_0011M ? 15 : 0);
    graphics_shift_screen((uint16_t)0330 | 0b01000000000);
    main_init();
    mark_to_exit_flag = true;
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
    snprintf(ln, MAX_WIDTH, "%s volume: %d (div: 1 << %d = %d)", sys_name, 16 - divider, divider, (1 << divider));
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
    snprintf(ln, MAX_WIDTH, "Turn %s %s", sys_name, *p_state ? "OFF" : "ON");
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
    draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Info", &lines);
    sleep_ms(1500);
    redraw_window();
}

typedef struct drive_state {
    char* path;
    char* lbl;
} drive_state_t;

static drive_state_t drives_states[4] = { // TODO:
    0, "FDD0",
    0, "FDD1",
    0, "HDD0",
    0, "HDD1"
};

void notify_image_insert_action(uint8_t drivenum, char *pathname) {
    drives_states[drivenum].path = pathname;
}

static void swap_drives(uint8_t cmd) {
    //sprintf(line, "F%d pressed - swap FDD images", cmd + 1);
    //draw_cmd_line(0, CMD_Y_POS, line);
    if (already_swapped_fdds) {
        insertdisk(0, 819200, 0, drives_states[0].path);
        insertdisk(2, 819200, 0, drives_states[1].path);
    } else {
        insertdisk(2, 819200, 0, drives_states[0].path);
        insertdisk(0, 819200, 0, drives_states[1].path);
    }
    already_swapped_fdds = !already_swapped_fdds;
    //swap_drive_message();
}

inline static void if_swap_drives() {
    if (backspacePressed && tabPressed && ctrlPressed) {
        swap_drives(8);
    }
}

inline static void if_video_mode() {
  if (!ctrlPressed && !altPressed) {
    if (f12Pressed) {
      f12Pressed = false;
      color_mode = !color_mode;
      graphics_set_mode(color_mode ? BK_256x256x2 : BK_512x256x1);
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
      reset(11);
      ctrlPressed = altPressed = delPressed = false;
  }
  if (ctrlPressed || altPressed)
    if(f1Pressed) {
        graphics_set_buffer(CPU_PAGE0_MEM_ADR, 512, 256);
        if (altPressed) graphics_set_mode(BK_256x256x2);
        else graphics_set_mode(BK_512x256x1);
    } else if (f2Pressed) {
        graphics_set_buffer(CPU_PAGE1_MEM_ADR, 512, 256);
        if (altPressed) graphics_set_mode(BK_256x256x2);
        else graphics_set_mode(BK_512x256x1);
    } else if (f3Pressed) {
        graphics_set_buffer(CPU_PAGE2_MEM_ADR, 512, 256);
        if (altPressed) graphics_set_mode(BK_256x256x2);
        else graphics_set_mode(BK_512x256x1);
    } else if (f4Pressed) {
        graphics_set_buffer(CPU_PAGE3_MEM_ADR, 512, 256);
        if (altPressed) graphics_set_mode(BK_256x256x2);
        else graphics_set_mode(BK_512x256x1);
    } else if (f5Pressed) {
        graphics_set_buffer(CPU_PAGE4_MEM_ADR, 512, 256);
        if (altPressed) graphics_set_mode(BK_256x256x2);
        else graphics_set_mode(BK_512x256x1);
    } else if (f6Pressed) {
        graphics_set_buffer(CPU_PAGE5_MEM_ADR, 512, 256);
        if (altPressed) graphics_set_mode(BK_256x256x2);
        else graphics_set_mode(BK_512x256x1);
    } else if (f7Pressed) {
        graphics_set_buffer(CPU_PAGE6_MEM_ADR, 512, 256);
        if (altPressed) graphics_set_mode(BK_256x256x2);
        else graphics_set_mode(BK_512x256x1);
    } else if (f8Pressed) {
        graphics_set_buffer(CPU_PAGE7_MEM_ADR, 512, 256);
        if (altPressed) graphics_set_mode(BK_256x256x2);
        else graphics_set_mode(BK_512x256x1);
    } else if (f10Pressed) {
        if (altPressed) graphics_set_pallette_idx(0);
        else graphics_set_pallette_idx(15);
    }
}

static void draw_window() {
    sprintf(line, "SD:%s", left_panel.path);
    draw_panel( 0, PANEL_TOP_Y, MAX_WIDTH / 2, PANEL_LAST_Y + 1, line, 0);
    sprintf(line, "SD:%s", right_panel.path);
    draw_panel(MAX_WIDTH / 2, PANEL_TOP_Y, MAX_WIDTH / 2, PANEL_LAST_Y + 1, line, 0);
}

static void redraw_window() {
    draw_window();
    fill_panel(&left_panel);
    fill_panel(&right_panel);
    draw_cmd_line(0, CMD_Y_POS, 0);
    bottom_line();
}

static void turn_usb_on(uint8_t cmd);

static void switch_rom(uint8_t cmd);

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
    draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Info", &lines);
    sleep_ms(1500);
    redraw_window();
}

static bool m_prompt(const char* txt) {
    const line_t lns[1] = {
        { -1, txt },
    };
    const lines_t lines = { 1, 2, lns };
    draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Are you sure?", &lines);
    bool yes = true;
    draw_button((MAX_WIDTH - 60) / 2 + 16, 12, 11, "Yes", yes);
    draw_button((MAX_WIDTH - 60) / 2 + 35, 12, 10, "No", !yes);
    while(1) {
        if (enterPressed) {
            enterPressed = false;
            scan_code_cleanup();
            return yes;
        }
        if (tabPressed || leftPressed || rightPressed) { // TODO: own msgs cycle
            yes = !yes;
            draw_button((MAX_WIDTH - 60) / 2 + 16, 12, 11, "Yes", yes);
            draw_button((MAX_WIDTH - 60) / 2 + 35, 12, 10, "No", !yes);
            tabPressed = leftPressed = rightPressed = false;
            scan_code_cleanup();
        }
        if (escPressed) {
            escPressed = false;
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
            snprintf(line, MAX_WIDTH, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to delete selected item!" },
                { -1, path },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
            sleep_ms(2500);
        }
    }
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
    return res;
}

static void m_copy_file(uint8_t cmd) {
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
            snprintf(line, MAX_WIDTH, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to copy selected item!" },
                { -1, path },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
            sleep_ms(2500);
        }
    }
    redraw_window();
}

static void m_move_file(uint8_t cmd) {
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
            snprintf(line, MAX_WIDTH, "FRESULT: %d", result);
            const line_t lns[3] = {
                { -1, "Unable to move selected item!" },
                { -1, path },
                { -1, line }
            };
            const lines_t lines = { 3, 2, lns };
            draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
            sleep_ms(2500);
        }
    }
    redraw_window();
}

static void m_info(uint8_t cmd) {
    line_t plns[33] = {
        { 1, "Key mapping BK-0011M:" },
        { 1, " - Alt   + \"key\"    - AP2" },
        { 1, " - Shift + \"key\"  - register up/down" },
        { 1, " - Ctrl  + \"key\"   - CU" },
        { 1, " - Caps Lock      - lock up/down register" },
        { 1, " - left  Win      - RUS" },
        { 1, " - right Win      - LAT" },
        { 1, " - Pause          - STOP" },
        { 1, " - F1             - POVTOR" },
        { 1, " - F2             - KT" },
        { 1, " - F3             - =|=>|" },
        { 1, " - F4             - |<==" },
        { 1, " - F5             - |==>" },
        { 1, " - F6             - IND SU" },
        { 1, " - F7             - BLOCK REDACT" },
        { 1, " - F8             - STEP" },
        { 1, " - F9             - SBROS" },
        { 1, " " },
        { 1, "Emulation hot keys:" },
        { 1, " - Ctrl+Alt+Del - Reset CPU, RAM clenup, set default pages, deafult speed, init system registers" },
        { 1, " - F10          - cyclic change pallete" },
        { 1, " - Alt + F10    - default BK-0010 pallete" },
        { 1, " - Ctrl + F10   - default BK-0011 pallete" },
        { 1, " - F11          - adjust brightness" },
        { 1, " - F12          - Switch B/W 512x256 to Color 256x256 and back" },
        { 1, " - Alt + F1..F8 - fast review RAM pages in color representation" },
        { 1, " - Ctrl+ F1..F8 - fast review RAM pages in B/W representation" },
        { 1, " - Ctrl + F11   - slower emulation (default emulation is about to BK on 3 MHz)" },
        { 1, " - Ctrl + F12   - faster emulation" },
        { 1, " - Ctrl + \"+\"   - increase volume" },
        { 1, " - Ctrl + \"-\"   - decrease volume" },
        { 1, " - Esc          - go to File Manager" },
    };
    lines_t lines = { 33, 1, plns };
    draw_box(3, 2, MAX_WIDTH - 10, MAX_HEIGHT - 10, "Help", &lines);
    int t = lastCleanableScanCode;
    while(t == lastCleanableScanCode) { }
    redraw_window();
}

static fn_1_12_tbl_t fn_1_12_tbl = {
    ' ', '1', " Help ", m_info,
    ' ', '2', " Menu ", do_nothing,
    ' ', '3', " View ", do_nothing,
    ' ', '4', " Edit ", do_nothing,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', "MkDir ", do_nothing,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', " Swap ", swap_drives,
    '1', '0', " USB  ", turn_usb_on,
    '1', '1', " 0010B", switch_rom,
    '1', '2', "Reset ", reset
};

static fn_1_12_tbl_t fn_1_12_tbl_alt = {
    ' ', '1', "Right ", do_nothing,
    ' ', '2', " Left ", do_nothing,
    ' ', '3', " View ", do_nothing,
    ' ', '4', " Edit ", do_nothing,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', " Find ", do_nothing,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', " UpMn ", do_nothing,
    '1', '0', " USB  ", turn_usb_on,
    '1', '1', " 0010B", switch_rom,
    '1', '2', "Reset ", reset
};

static fn_1_12_tbl_t fn_1_12_tbl_ctrl = {
    ' ', '1', " EjtL ", do_nothing,
    ' ', '2', " EjtR ", do_nothing,
    ' ', '3', "Debug ", do_nothing,
    ' ', '4', " Edit ", do_nothing,
    ' ', '5', " Copy ", m_copy_file,
    ' ', '6', " Move ", m_move_file,
    ' ', '7', " Find ", do_nothing,
    ' ', '8', " Del  ", m_delete_file,
    ' ', '9', " Swap ", swap_drives,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', " B/W  ", switch_color,
    '1', '2', "Reset ", reset
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
    for (int i = 0; i < BTNS_COUNT; ++i) {
        const fn_1_12_tbl_rec_t* rec = &(*actual_fn_1_12_tbl())[i];
        draw_fn_btn(rec, i * BTN_WIDTH, F_BTN_Y_POS);
    }
    draw_cmd_line(0, CMD_Y_POS, 0);
}

static inline void turn_usb_off(uint8_t cmd) { // TODO: support multiple enter for USB mount
    set_tud_msc_ejected(true);
    usb_started = false;
    // Alt + F10 no more actions
    memset(fn_1_12_tbl_alt[9].name, ' ', BTN_WIDTH);
    fn_1_12_tbl_alt[9].action = do_nothing;
    // Ctrl + F10 - Exit
    sprintf(fn_1_12_tbl_ctrl[9].name, " Exit ");
    fn_1_12_tbl_ctrl[9].action = mark_to_exit;
    redraw_window();
}

static void turn_usb_on(uint8_t cmd) {
    init_pico_usb_drive();
    usb_started = true;
    // do not USB after it was turned on
    memset(fn_1_12_tbl[9].name, ' ', BTN_WIDTH);
    fn_1_12_tbl[9].action = do_nothing;
    // do not Exit in usb mode
    memset(fn_1_12_tbl_ctrl[9].name, ' ', BTN_WIDTH);
    fn_1_12_tbl_ctrl[9].action = do_nothing;
    // Alt + F10 - force unmount usb
    snprintf(fn_1_12_tbl_alt[9].name, BTN_WIDTH, " UnUSB ");
    fn_1_12_tbl_alt[9].action = turn_usb_off;
    bottom_line();
}

inline static const char* curr_mode() {
    switch (get_bk0010mode()) {
        case BK_FDD:
            return "10+FDD";
        case BK_0010:
            return " 0010F";
        case BK_0010_01:
            return " 0010B";
        case BK_0011M:
            return " 0011M";
    }
    return "";
}

static void switch_rom(uint8_t cmd) {
    bk_mode_t bk0010mode = get_bk0010mode();
    bk0010mode++;
    if (bk0010mode > BK_0011M) bk0010mode = BK_FDD;
    set_bk0010mode(bk0010mode);
  //  if ( is_fdd_suppored() && color_mode ) {
  //      color_mode = false;
  //      snprintf(fn_1_12_tbl_ctrl[10].name, BTN_WIDTH, color_mode ? " B/W  " : " Color");
  //  }
    const char* cm = curr_mode();
    snprintf(fn_1_12_tbl     [10].name, BTN_WIDTH, cm);
    snprintf(fn_1_12_tbl_alt [10].name, BTN_WIDTH, cm);
    init_rom();
    bottom_line();
}

static void switch_color(uint8_t cmd) {
    color_mode = !color_mode;
    snprintf(fn_1_12_tbl_ctrl[10].name, BTN_WIDTH, color_mode ? " B/W  " : " Color");
    bottom_line();
}

inline static void m_cleanup() {
    files_count = 0;
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
    strncpy(fp->name, fi->fname, MAX_WIDTH >> 1);
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
        draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Warning", &lines);
        sleep_ms(1500);
        redraw_window();
        return false;
    }
    return true;
}

static int m_comp(const file_info_t * e1, const file_info_t * e2) {
    if ((e1->fattrib & AM_DIR) && !(e2->fattrib & AM_DIR)) return -1;
    if (!(e1->fattrib & AM_DIR) && (e2->fattrib & AM_DIR)) return 1;
    return strncmp(e1->name, e2->name, MAX_WIDTH >> 1);
}

inline static void collect_files(file_panel_desc_t* p) {
    m_cleanup();
    DIR dir;
    if (!m_opendir(&dir, p->path)) return;
    FILINFO fileInfo;
    while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != '\0') {
        m_add_file(&fileInfo);
    }
    f_closedir(&dir);
    qsort (files_info, files_count, sizeof(file_info_t), m_comp);
}

static inline void fill_panel(file_panel_desc_t* p) {
    collect_files(p);
    int y = 1;
    p->files_number = 0;
    if (p->start_file_offset == 0 && strlen(p->path) > 1) {
        draw_label(p->left + 1, y, p->width - 2, "..", p == psp && p->selected_file_idx == y, true);
        y++;
        p->files_number++;
    }
    for(int fn = 0; fn < files_count; ++ fn) {
        file_info_t* fp = &files_info[fn];
        if (p->start_file_offset <= p->files_number && y <= LAST_FILE_LINE_ON_PANEL_Y) {
            snprintf(line, MAX_WIDTH, "%s\\%s", p->path, fp->name);
            for (int i = 0; i < 3; ++i) { // mark mounted drived by labels FDD0,FDD1,HDD0...
                if (drives_states[i].path && strncmp(drives_states[i].path, line, MAX_WIDTH) == 0) {
                    snprintf(line, p->width, "%s", fp->name);
                    for (int j = strlen(fp->name); j < p->width - 6; ++j) {
                        line[j] = ' ';
                    }
                    snprintf(line + p->width - 6, 6, "%s", drives_states[i].lbl);
                    break;
                }
            }
            draw_label(p->left + 1, y, p->width - 2, fp->name, p == psp && p->selected_file_idx == y, fp->fattrib & AM_DIR);
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

inline static fn_1_12_btn_pressed(uint8_t fn_idx) {
    if (fn_idx > 11) fn_idx -= 18; // F11-12
    (*actual_fn_1_12_tbl())[fn_idx].action(fn_idx);
}

inline static void handle_pagedown_pressed() {
  for (int i = 0; i < MAX_HEIGHT / 2; ++i)
    if (psp->selected_file_idx < LAST_FILE_LINE_ON_PANEL_Y &&
        psp->start_file_offset + psp->selected_file_idx < psp->files_number
    ) {
        psp->selected_file_idx++;
    } else if (
        psp->selected_file_idx == LAST_FILE_LINE_ON_PANEL_Y &&
        psp->start_file_offset + psp->selected_file_idx < psp->files_number
    ) {
        psp->selected_file_idx--;
        psp->start_file_offset++;
    }
  fill_panel(psp);
  scan_code_processed();
}

inline static void handle_down_pressed() {
    if (psp->selected_file_idx < LAST_FILE_LINE_ON_PANEL_Y &&
        psp->start_file_offset + psp->selected_file_idx < psp->files_number
    ) {
        psp->selected_file_idx++;
        fill_panel(psp);
    } else if (
        psp->selected_file_idx == LAST_FILE_LINE_ON_PANEL_Y &&
        psp->start_file_offset + psp->selected_file_idx < psp->files_number
    ) {
        psp->selected_file_idx -= 5;
        psp->start_file_offset += 5;
        fill_panel(psp);    
    }
    scan_code_processed();
}

inline static void handle_pageup_pressed() {
  for (int i = 0; i < MAX_HEIGHT / 2; ++i)
    if (psp->selected_file_idx > FIRST_FILE_LINE_ON_PANEL_Y) {
        psp->selected_file_idx--;
    } else if (psp->selected_file_idx == FIRST_FILE_LINE_ON_PANEL_Y && psp->start_file_offset > 0) {
        psp->selected_file_idx++;
        psp->start_file_offset--;
    }
  fill_panel(psp);
  scan_code_processed();
}

inline static void handle_up_pressed() {
    if (psp->selected_file_idx > FIRST_FILE_LINE_ON_PANEL_Y) {
        psp->selected_file_idx--;
        fill_panel(psp);
    } else if (psp->selected_file_idx == FIRST_FILE_LINE_ON_PANEL_Y && psp->start_file_offset > 0) {
        psp->selected_file_idx += 5;
        psp->start_file_offset -= 5;
        fill_panel(psp);       
    }
    scan_code_processed();
}

static inline void redraw_current_panel() {
    psp->selected_file_idx = 1;
    psp->start_file_offset = 0;
    sprintf(line, "SD:%s", psp->path);
    draw_panel(psp->left, PANEL_TOP_Y, psp->width, PANEL_LAST_Y + 1, line, 0);
    fill_panel(psp);
    draw_cmd_line(0, CMD_Y_POS, 0);
}

static inline bool run_bin(char* path) {
    FIL file;
    FRESULT result = f_open(&file, path, FA_READ);
    if (result != FR_OK) {
        const line_t lns[2] = {
            { -1, "Selected file was not found!" },
            { -1, path }
        };
        const lines_t lines = { 2, 3, lns };
        draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
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
        snprintf(line, MAX_WIDTH, "FRESULT: %d (bw: %d)", result, bw);
        const line_t lns[3] = {
            { -1, "Unable to read selected file!" },
            { -1, path },
            { -1, line }
        };
        const lines_t lines = { 3, 2, lns };
        draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
        sleep_ms(1500);
        redraw_window();
        return false;
    }
    // TODO: ensue it is ok for ever game
    Device_Data.MemPages [0] = CPU_PAGE0_MEM_ADR; /* RAM Page 0 */
    Device_Data.MemPages [1] = CPU_PAGE5_MEM_ADR; /* RAM Page 4 video 0 */
    graphics_set_page(CPU_PAGE5_MEM_ADR, get_bk0010mode() == BK_0011M ? 15 : 0);
    graphics_shift_screen((uint16_t)0330 | 0b01000000000);
    snprintf(line, MAX_WIDTH, "offset = 0%o; len = %d", offset, len);
    const line_t lns[3] = {
        { -1, "Selected file header info:" },
        { -1, path },
        { -1, line }
    };
    const lines_t lines = { 3, 2, lns };
    draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Info", &lines);
    sleep_ms(2500);
    uint16_t len2 = (len > (16 << 10) - offset) ? (16 << 10) - offset : len;
    result = f_read(&file, RAM + offset, len2, &bw);
    if (result != FR_OK) {
        f_close(&file);
        snprintf(line, MAX_WIDTH, "FRESULT: %d (bw: %d)", result, bw);
        const line_t lns[3] = {
            { -1, "Unable to read selected file!" },
            { -1, path },
            { -1, line }
        };
        const lines_t lines = { 3, 2, lns };
        draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
        sleep_ms(1500);
        redraw_window();
        return false;
    }
    if (len2 != len) {
        result = f_read(&file, CPU_PAGE5_MEM_ADR, len - len2, &bw);
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
    return true;
}

static inline bool run_img(char* path) {
    insertdisk(2, 819200, 0, path); // TODO: select drive #
    if ( !is_fdd_suppored() ) {
//        color_mode = false;
        set_bk0010mode(BK_0011M);
    }
    reset(0);
    mark_to_exit_flag = true;
    return true;
}

static file_info_t* selected_file() {
    if (psp->selected_file_idx == 1 && psp->start_file_offset == 0 && strlen(psp->path) > 1) {
        return 0;
    }
    collect_files(psp);
    int y = 1;
    int files_number = 0;
    if (psp->start_file_offset == 0 && strlen(psp->path) > 1) {
        y++;
        files_number++;
    }
    for(int fn = 0; fn < files_count; ++ fn) {
        file_info_t* fp = &files_info[fn];
        if (psp->start_file_offset <= files_number && y <= LAST_FILE_LINE_ON_PANEL_Y) {
            if (psp->selected_file_idx == y) {
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
                redraw_current_panel();
                return;
            }
        }
        psp->path[0] = '\\';
        psp->path[1] = 0;
        redraw_current_panel();
        return;
    }
    if (fp->fattrib & AM_DIR) {
        char path[256];
        construct_full_name(path, psp->path, fp->name);
        strncpy(psp->path, path, 256);
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
        }
    }
}

static inline void if_sound_control() { // core #0
    if (ctrlPressed && plusPressed) {
        covox_multiplier++;
        if(covox_multiplier > 8) covox_multiplier = 8;
        plusPressed = false;
    } else if (ctrlPressed && minusPressed) {
        covox_multiplier--;
        if(covox_multiplier < 0) covox_multiplier = 0;
        minusPressed = false;
    }
}

static uint8_t repeat_cnt = 0;

static inline void work_cycle() {
    while(1) {
        if_swap_drives();
        if_overclock();
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
          //  mark_to_exit(9);
          case 0x81: // Esc up
          //  scan_code_processed();
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
        if (usb_started && tud_msc_ejected()) {
            turn_usb_off(0);
        }
        if (usb_started) {
            pico_usb_drive_heartbeat();
        } else if(mark_to_exit_flag) {
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
    set_start_debug_line(MAX_HEIGHT);
    draw_window();
    select_left_panel();

    const char* cm = curr_mode();
    snprintf(fn_1_12_tbl     [10].name, BTN_WIDTH, cm);
    snprintf(fn_1_12_tbl_alt [10].name, BTN_WIDTH, cm);
    bottom_line();

    work_cycle();
    
    set_start_debug_line(25); // ?? to be removed
    graphics_set_mode(color_mode ? BK_256x256x2 : BK_512x256x1);
    restore_video_ram();
}

bool handleScancode(uint32_t ps2scancode) { // core 1
    lastCleanableScanCode = ps2scancode;
    switch (ps2scancode) {
      case 0x01: // Esc down
        escPressed = true;
        break;
      case 0x81: // Esc up
        escPressed = false; break;
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

uint32_t overcloking_khz = OVERCLOCKING * 1000;

inline void if_overclock() {
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
            draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Info", &lines);
        }
        else {
            sprintf(line, "System clock of %u kHz cannot be achieved", overcloking_khz);
            line_t lns[1] = {
                { -1, line }
            };
            lines_t lines = { 1, 3, lns };
            draw_box((MAX_WIDTH - 60) / 2, 7, 60, 10, "Warning", &lines);
        }
        sleep_ms(2500);
        graphics_set_mode(ret);
        restore_video_ram();
    }
}

void ps2cleanup(); // TODO:

int if_manager(bool force) {
    if (ctrlPressed) {
        if (f11Pressed) {
            f11Pressed = false;
            tormoz++;
        }
        if (f12Pressed) {
            f12Pressed = false;
            tormoz--;
            if (tormoz < 0) tormoz = 0;
        }
    }
    if_video_mode();
    if_swap_drives();
    if_overclock();
    if_sound_control();
    if (manager_started) {
        return tormoz;
    }
    if (force) {
        escPressed = false;
        ps2cleanup();
        manager_started = true;
        start_manager();
        manager_started = false;
    }
    return tormoz;
}

#endif

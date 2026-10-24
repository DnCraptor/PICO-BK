/*
 * Keyboard for the Linux host: evdev key events -> HID boot keyboard reports
 * (as the PS/2 driver builds them) -> XT scancodes for the manager
 * (handleScancode) and AT scancodes for the emulated BK keyboard
 * (ps2get_raw_code), exactly as drivers/ps2kbd/ps2kbd_mrmltr.cpp converts
 * PS/2 and USB keyboard reports.  The conversion tables and
 * process_kbd_report() are copied from that file unchanged.
 *
 * keyboard_tick() reads the devices; the emulation thread calls it (the
 * manager does, every ~256 emulated instructions and in its own loops).
 * The AT scancode queue is also fed by the manager's keyboard-script timer
 * thread, so it is guarded by a mutex.
 */
#include "hid_keys.h"
#include "input_linux.h"
#include "platform_linux.h"

#include <linux/input.h>
#include <pthread.h>
#include <string.h>

extern "C" {
#include "ps2.h"
}

#define HID_KEY_EUROPE_2 0x64

typedef struct mod2xt_s {
  uint8_t mod;
  uint8_t kd;
  uint8_t ku;
  uint16_t kd_at;
  uint16_t ku_at;
} mod2xt_t;

static mod2xt_t mod2xt[] = {
  { KEYBOARD_MODIFIER_LEFTCTRL , 0x1D, 0x9D, 0x14, 0xF014 },
  { KEYBOARD_MODIFIER_LEFTSHIFT, 0x2A, 0xAA, 0x12, 0xF012 },
  { KEYBOARD_MODIFIER_LEFTALT  , 0x38, 0xB8, 0x11, 0xF011 },
  { KEYBOARD_MODIFIER_LEFTGUI  , 0x5B, 0xDB, 0x011F, 0xF11F },
  { KEYBOARD_MODIFIER_RIGHTCTRL, 0x1D, 0x9D, 0x0114, 0xF114 },
  { KEYBOARD_MODIFIER_RIGHTSHIFT,0x36, 0xB6, 0x0059, 0xF059 },
  { KEYBOARD_MODIFIER_RIGHTALT  ,0x38, 0xB8, 0x0111, 0xF111 },
  { KEYBOARD_MODIFIER_RIGHTGUI  ,0x5C, 0xDC, 0x0127, 0xF127 }
};

typedef struct hid2xt_s {
  uint8_t kd;
  uint8_t ku;
} hid2xt_t;
typedef struct hid2at_s {
  uint16_t kd;
  uint16_t ku;
} hid2at_t;

static hid2xt_t hid2xt[256] = {
  { 0, 0 }, // 0x00 HID_KEY_NONE 
  { 0, 0 }, // 1
  { 0, 0 }, // 2
  { 0, 0 }, // 3
  { 0x1E, 0x9E }, // A 04 HID_KEY_A                         
	{ 0x30, 0xB0 }, // B 05
  {	0x2E,	0xAE },	// C 06
	{ 0x20, 0xA0 }, // D 07
  { 0x12, 0x92 }, // E 08
  { 0x21, 0xA1 }, // F 09
  { 0x22, 0xA2 }, // G 0A
  { 0x23, 0xA3 }, // H 0B
  { 0x17, 0x97 }, // I 0C
  { 0x24, 0xA4 }, // J 0D
  { 0x25, 0xA5 }, // K 0E
  { 0x26, 0xA6 }, // L 0F
  { 0x32, 0xB2 }, // M 10
  { 0x31, 0xB1 }, // N 11
  { 0x18, 0x98 }, // O 12
  { 0x19, 0x99 }, // P 13
  { 0x10, 0x90 }, // Q 14
  { 0x13, 0x93 }, // R 15
  { 0x1F, 0x9F }, // S 16
  { 0x14, 0x94 }, // T 17
  { 0x16, 0x96 }, // U 18
  { 0x2F, 0xAF }, // V 19
  { 0x11, 0x91 }, // W 1A
  { 0x2D, 0xAD }, // X 1B
  { 0x15, 0x95 }, // Y 1C
  { 0x2C, 0xAC }, // Z 1D
  { 0x02, 0x82 }, // 1 1E
  { 0x03, 0x83 }, // 2
  { 0x04, 0x84 }, // 3
  { 0x05, 0x85 }, // 4
  { 0x06, 0x86 }, // 5
  { 0x07, 0x87 }, // 6
  { 0x08, 0x88 }, // 7
  { 0x09, 0x89 }, // 8
  { 0x0A, 0x8A }, // 9
  { 0x0B, 0x8B }, // 0 27
  { 0x1C,	0x9C }, // 28 Enter
  { 0x01,	0x81 }, // 29 Esc
  { 0x0E,	0x8E }, // 2A BS
  { 0x0F,	0x8F }, // 2B Tab
  { 0x39,	0xB9 }, // 2C Space
  { 0x0C,	0x8C }, // 2D -
  { 0x0D,	0x8D }, // 2E =
  { 0x1A,	0x9A }, // 2F [
  { 0x1B,	0x9B }, // 30 ]
  { 0x2B,	0xAB }, // 31 '\'
  { 0x56,	0xD6 }, // 32 EU
  { 0x28,	0xA8 }, // 34 '
  { 0x27,	0xA7 }, // 33 ;
  { 0x29,	0xA9 }, // 35 ~
	{ 0x33,	0xB3 }, // 36 ,
  { 0x34,	0xB4 }, // 37 .
	{ 0x35,	0xB5 }, // 38 /
  { 0x3A,	0xBA }, // 39 CapsLock
  { 0x3B,	0xBB }, // 3A F1
  { 0x3C,	0xBC }, // 
  { 0x3D,	0xBD }, // 
  { 0x3E,	0xBE }, // 
  { 0x3F,	0xBF }, // 
  { 0x40,	0xC0 }, // 
  { 0x41,	0xC1 }, // 
  { 0x42,	0xC2 }, // 
  { 0x43,	0xC3 }, // 
  { 0x44,	0xC4 }, // 
  { 0x57,	0xD7 }, // 
  { 0x58,	0xD8 }, // 45 F12
  { 0x00,	0x80 }, // 46 PrtScr ?
  { 0x46,	0xC6 },	// 47 Scroll Lock
  { 0x00, 0x00 }, // 48 Pause (todo)
	{ 0x52, 0xD2 }, // 49 Insert
	{ 0x47, 0xC7 }, // 4A Home
 	{ 0x49, 0xC9 }, // 4B Page Up
	{ 0x53, 0xD3 }, // 4C Delete
	{ 0x4F, 0xCF }, // 4D End
	{ 0x51, 0xD1 }, // 4E Page Down
	{ 0x4D, 0xCD }, // 4F →
	{ 0x4B, 0xCB }, // 50 ←
	{ 0x50, 0xD0 }, // 51 ↓
	{ 0x48, 0xC8 }, // 52 ↑
  // keypad
	{ 0x45, 0xC5 }, // 53 Num Lock
	{ 0x35,	0xB5 },	// 54 /
  { 0x37,	0xB7 },	// 55 *
	{ 0x4A,	0xCA },	// 56 -
	{ 0x4E,	0xCE },	// 57 +
	{ 0x1C,	0x9C },	// 58 ↵ Enter
  { 0x4F,	0xCF },	// 59 1
	{ 0x50,	0xD0 },	// 5A 2
	{ 0x51,	0xD1 },	// 5B 3
	{ 0x4B,	0xCB },	// 5C 4
	{ 0x4C,	0xCC },	// 5D 5
	{ 0x4D,	0xCD },	// 5E 6
	{ 0x47,	0xC7 },	// 5F 7
	{ 0x48,	0xC8 },	// 60 8
	{ 0x49,	0xC9 },	// 61 9
	{ 0x52,	0xD2 },	// 62 0
	{ 0x53,	0xD3 },	// 63 .
  { 0x2B,	0xAB }, // 64 '\'
};
static hid2at_t hid2at[256] = {
  { 0, 0 }, // 0 HID_KEY_NONE
  { 0, 0 }, // 1
  { 0, 0 }, // 2
  { 0, 0 }, // 3
  { 0x1C, 0xF01C }, // A 04 HID_KEY_A
  { 0x32, 0xF032 }, // B 05
  {	0x21, 0xF021 }, // C 06
	{ 0x23, 0xF023 }, // D 07
  { 0x24, 0xF024 }, // E 08
  { 0x2B, 0xF02B }, // F 09
  { 0x34, 0xF034 }, // G 0A
  { 0x33, 0xF033 }, // H 0B
  { 0x43, 0xF043 }, // I 0C
  { 0x3B, 0xF03B }, // J 0D
  { 0x42, 0xF042 }, // K 0E
  { 0x4B, 0xF04B }, // L 0F
  { 0x3A, 0xF03A }, // M 10
  { 0x31, 0xF031 }, // N 11
  { 0x44, 0xF044 }, // O 12
  { 0x4D, 0xF04D }, // P 13
  { 0x15, 0xF015 }, // Q 14
  { 0x2D, 0xF02D }, // R 15
  { 0x1B, 0xF01B }, // S 16
  { 0x2C, 0xF02C }, // T 17
  { 0x3C, 0xF03C }, // U 18
  { 0x2A, 0xF02A }, // V 19
  { 0x1D, 0xF01D }, // W 1A
  { 0x22, 0xF022 }, // X 1B
  { 0x35, 0xF035 }, // Y 1C
  { 0x1A, 0xF01A }, // Z 1D
  { 0x16, 0xF016 }, // 1
  { 0x1E, 0xF01E }, // 2
  { 0x26, 0xF026 }, // 3
  { 0x25, 0xF025 }, // 4
  { 0x2E, 0xF02E }, // 5
  { 0x36, 0xF036 }, // 6
  { 0x3D, 0xF03D }, // 7
  { 0x3E, 0xF03E }, // 8
  { 0x46, 0xF046 }, // 9
  { 0x45, 0xF045 }, // 0 27
  { 0x5A, 0xF05A }, // Enter
  { 0x76, 0xF076 }, // Esc
  { 0x66, 0xF066 }, // Backspace
  { 0x0D, 0xF00D }, // Tab
  { 0x29, 0xF029 }, // Space
  { 0x4E, 0xF04E }, // -
  { 0x55, 0xF055 }, // =
  { 0x54, 0xF054 }, // [
  { 0x5B, 0xF05B }, // ]
  { 0x5D, 0xF05D }, // '\'
  { 0x56, 0xF056 }, // Europe 1 (ISO key)
  { 0x4C, 0xF04C }, // ;
  { 0x52, 0xF052 }, // '
  { 0x0E, 0xF00E }, // ~ (Grave)
  { 0x41, 0xF041 }, // ,
  { 0x49, 0xF049 }, // .
  { 0x4A, 0xF04A }, // 38 /
  { 0x58,	0xF058 }, // 39 CapsLock
  { 0x05,	0xF005 }, // 3A F1
  { 0x06,	0xF006 }, // 
  { 0x04,	0xf004 }, // 
  { 0x0C,	0xf00c }, // 
  { 0x03,	0xf003 }, // 
  { 0x0B,	0xf00b }, // 
  { 0x83,	0xf083 }, // 
  { 0x0A,	0xf00a }, // 
  { 0x01,	0xf001 }, // 
  { 0x09,	0xf009 }, // 
  { 0x78,	0xf078 }, // 
  { 0x07,	0xf007 }, // 45 F12
  { 0x017C,	0xF17C }, // 46 PrtScr
  { 0x007E,	0xF07E }, // 47 Scroll Lock
  { 0x0214, 0xF214 }, // 48 Pause
	{ 0x170,	0xF170 },	// 49 Insert
	{ 0x16C,	0xF16C },	// 4A Home
 	{ 0x17D,	0xF17D },	// 4B Page Up
	{ 0x171,	0xF171 },	// 4C Delete
	{ 0x169,	0xF169 },	// 4D End
	{ 0x17A,	0xF17A },	// 4E Page Down
	{ 0x174,	0xF174 },	// 4F →
	{ 0x16B,	0xF16B },	// 50 ←
	{ 0x172,	0xF172 },	// 51 ↓
	{ 0x175,	0xF175 },	// 52 ↑
  // keypad
	{ 0x77,	0xF077 },	// 53 Num Lock
	{ 0x4A, 0xF04A },	// 54 / (map)
  { 0x7C,	0xF07C },	// 55 *
	{ 0x7B,	0xF07B },	// 56 -
	{ 0x79,	0xF079 },	// 57 +
	{ 0x5A, 0xF05A },	// 58 ↵ Enter (map)
  { 0x69,	0xF069 },	// 59 1
	{ 0x72,	0xF072 },	// 5A 2
	{ 0x7A,	0xF07A },	// 5B 3
	{ 0x6B,	0xF06B },	// 5C 4
	{ 0x73,	0xF073 },	// 5D 5
	{ 0x74,	0xF074 },	// 5E 6
	{ 0x6C,	0xF06C },	// 5F 7
	{ 0x75,	0xF075 },	// 60 8
	{ 0x7D,	0xF07D },	// 61 9
	{ 0x70,	0xF070 },	// 62 0
	{ 0x71,	0xF071 },	// 63 .
  { 0x5D, 0xF05D }, // 64 '\'
};

static pthread_mutex_t at_queue_lock = PTHREAD_MUTEX_INITIALIZER;
static uint8_t last_at_scancode_idx = 0;
static uint32_t last_at_scancodes[16] = { 0 };

static inline void push_at_scancode(uint32_t at) {
  if (!at) return;
  pthread_mutex_lock(&at_queue_lock);
  if (last_at_scancode_idx < 15)
    last_at_scancodes[last_at_scancode_idx++] = at;
  pthread_mutex_unlock(&at_queue_lock);
}

extern "C" bool handleScancode(uint32_t ps2xt_scancode);

static void process_kbd_report(
    hid_keyboard_report_t const *report,
    hid_keyboard_report_t const *prev_report
) {
  for (size_t i = 0; i < sizeof(mod2xt) / sizeof(mod2xt_t); ++i) {
    const mod2xt_t& mod = mod2xt[i];
    bool pressed = report->modifier & mod.mod;
    bool was_pressed = prev_report->modifier & mod.mod;
    if (pressed && !was_pressed) {
      handleScancode(mod.kd);
      push_at_scancode(mod.kd_at);
    }
    else if (!pressed && was_pressed) {
      handleScancode(mod.ku);
      push_at_scancode(mod.ku_at);
    }
  }
  for (int i = 0; i < 6; ++i) {
    uint8_t kc_pressed = report->keycode[i];
    if (!kc_pressed) continue;
    bool found = false;
    for (int j = 0; j < 6; ++j) {
      if (kc_pressed == prev_report->keycode[j]) {
        found = true; break;
      }
    }
    if (!found) {
      push_at_scancode( hid2at[kc_pressed].kd );
      kc_pressed = hid2xt[kc_pressed].kd;
      if (kc_pressed) handleScancode(kc_pressed);
    }
  }
  for (int i = 0; i < 6; ++i) {
    uint8_t kc_was_pressed = prev_report->keycode[i];
    if (!kc_was_pressed) continue;
    bool found = false;
    for (int j = 0; j < 6; ++j) {
      if (kc_was_pressed == report->keycode[j]) {
        found = true; break;
      }
    }
    if (!found) {
      push_at_scancode( hid2at[kc_was_pressed].ku );
      kc_was_pressed = hid2xt[kc_was_pressed].ku;
      if (kc_was_pressed) handleScancode(kc_was_pressed);
    }
  }
}

/* --- evdev -> HID report (as Ps2Kbd_Mrmltr::handleHidKeyPress/Release) --- */

static hid_keyboard_report_t report = {};

static uint8_t linux_to_hid(int code) {
    switch (code) {
        case KEY_A: return HID_KEY_A;
        case KEY_B: return HID_KEY_B;
        case KEY_C: return HID_KEY_C;
        case KEY_D: return HID_KEY_D;
        case KEY_E: return HID_KEY_E;
        case KEY_F: return HID_KEY_F;
        case KEY_G: return HID_KEY_G;
        case KEY_H: return HID_KEY_H;
        case KEY_I: return HID_KEY_I;
        case KEY_J: return HID_KEY_J;
        case KEY_K: return HID_KEY_K;
        case KEY_L: return HID_KEY_L;
        case KEY_M: return HID_KEY_M;
        case KEY_N: return HID_KEY_N;
        case KEY_O: return HID_KEY_O;
        case KEY_P: return HID_KEY_P;
        case KEY_Q: return HID_KEY_Q;
        case KEY_R: return HID_KEY_R;
        case KEY_S: return HID_KEY_S;
        case KEY_T: return HID_KEY_T;
        case KEY_U: return HID_KEY_U;
        case KEY_V: return HID_KEY_V;
        case KEY_W: return HID_KEY_W;
        case KEY_X: return HID_KEY_X;
        case KEY_Y: return HID_KEY_Y;
        case KEY_Z: return HID_KEY_Z;
        case KEY_1: return HID_KEY_1;
        case KEY_2: return HID_KEY_2;
        case KEY_3: return HID_KEY_3;
        case KEY_4: return HID_KEY_4;
        case KEY_5: return HID_KEY_5;
        case KEY_6: return HID_KEY_6;
        case KEY_7: return HID_KEY_7;
        case KEY_8: return HID_KEY_8;
        case KEY_9: return HID_KEY_9;
        case KEY_0: return HID_KEY_0;
        case KEY_ENTER: return HID_KEY_ENTER;
        case KEY_ESC: return HID_KEY_ESCAPE;
        case KEY_BACKSPACE: return HID_KEY_BACKSPACE;
        case KEY_TAB: return HID_KEY_TAB;
        case KEY_SPACE: return HID_KEY_SPACE;
        case KEY_MINUS: return HID_KEY_MINUS;
        case KEY_EQUAL: return HID_KEY_EQUAL;
        case KEY_LEFTBRACE: return HID_KEY_BRACKET_LEFT;
        case KEY_RIGHTBRACE: return HID_KEY_BRACKET_RIGHT;
        case KEY_BACKSLASH: return HID_KEY_BACKSLASH;
        case KEY_SEMICOLON: return HID_KEY_SEMICOLON;
        case KEY_APOSTROPHE: return HID_KEY_APOSTROPHE;
        case KEY_GRAVE: return HID_KEY_GRAVE;
        case KEY_COMMA: return HID_KEY_COMMA;
        case KEY_DOT: return HID_KEY_PERIOD;
        case KEY_SLASH: return HID_KEY_SLASH;
        case KEY_CAPSLOCK: return HID_KEY_CAPS_LOCK;
        case KEY_F1: return HID_KEY_F1;
        case KEY_F2: return HID_KEY_F2;
        case KEY_F3: return HID_KEY_F3;
        case KEY_F4: return HID_KEY_F4;
        case KEY_F5: return HID_KEY_F5;
        case KEY_F6: return HID_KEY_F6;
        case KEY_F7: return HID_KEY_F7;
        case KEY_F8: return HID_KEY_F8;
        case KEY_F9: return HID_KEY_F9;
        case KEY_F10: return HID_KEY_F10;
        case KEY_F11: return HID_KEY_F11;
        case KEY_F12: return HID_KEY_F12;
        case KEY_SYSRQ: return HID_KEY_PRINT_SCREEN;
        case KEY_SCROLLLOCK: return HID_KEY_SCROLL_LOCK;
        case KEY_PAUSE: return HID_KEY_PAUSE;
        case KEY_INSERT: return HID_KEY_INSERT;
        case KEY_HOME: return HID_KEY_HOME;
        case KEY_PAGEUP: return HID_KEY_PAGE_UP;
        case KEY_DELETE: return HID_KEY_DELETE;
        case KEY_END: return HID_KEY_END;
        case KEY_PAGEDOWN: return HID_KEY_PAGE_DOWN;
        case KEY_RIGHT: return HID_KEY_ARROW_RIGHT;
        case KEY_LEFT: return HID_KEY_ARROW_LEFT;
        case KEY_DOWN: return HID_KEY_ARROW_DOWN;
        case KEY_UP: return HID_KEY_ARROW_UP;
        case KEY_NUMLOCK: return HID_KEY_NUM_LOCK;
        case KEY_KPSLASH: return HID_KEY_KEYPAD_DIVIDE;
        case KEY_KPASTERISK: return HID_KEY_KEYPAD_MULTIPLY;
        case KEY_KPMINUS: return HID_KEY_KEYPAD_SUBTRACT;
        case KEY_KPPLUS: return HID_KEY_KEYPAD_ADD;
        case KEY_KPENTER: return HID_KEY_KEYPAD_ENTER;
        case KEY_KP1: return HID_KEY_KEYPAD_1;
        case KEY_KP2: return HID_KEY_KEYPAD_2;
        case KEY_KP3: return HID_KEY_KEYPAD_3;
        case KEY_KP4: return HID_KEY_KEYPAD_4;
        case KEY_KP5: return HID_KEY_KEYPAD_5;
        case KEY_KP6: return HID_KEY_KEYPAD_6;
        case KEY_KP7: return HID_KEY_KEYPAD_7;
        case KEY_KP8: return HID_KEY_KEYPAD_8;
        case KEY_KP9: return HID_KEY_KEYPAD_9;
        case KEY_KP0: return HID_KEY_KEYPAD_0;
        case KEY_KPDOT: return HID_KEY_KEYPAD_DECIMAL;
        case KEY_102ND: return HID_KEY_EUROPE_2;
        case KEY_LEFTCTRL: return HID_KEY_CONTROL_LEFT;
        case KEY_LEFTSHIFT: return HID_KEY_SHIFT_LEFT;
        case KEY_LEFTALT: return HID_KEY_ALT_LEFT;
        case KEY_LEFTMETA: return HID_KEY_GUI_LEFT;
        case KEY_RIGHTCTRL: return HID_KEY_CONTROL_RIGHT;
        case KEY_RIGHTSHIFT: return HID_KEY_SHIFT_RIGHT;
        case KEY_RIGHTALT: return HID_KEY_ALT_RIGHT;
        case KEY_RIGHTMETA: return HID_KEY_GUI_RIGHT;
        default: return HID_KEY_NONE;
    }
}

static uint8_t hid_key_to_modifier(uint8_t hid) {
    switch (hid) {
        case HID_KEY_CONTROL_LEFT: return KEYBOARD_MODIFIER_LEFTCTRL;
        case HID_KEY_SHIFT_LEFT: return KEYBOARD_MODIFIER_LEFTSHIFT;
        case HID_KEY_ALT_LEFT: return KEYBOARD_MODIFIER_LEFTALT;
        case HID_KEY_GUI_LEFT: return KEYBOARD_MODIFIER_LEFTGUI;
        case HID_KEY_CONTROL_RIGHT: return KEYBOARD_MODIFIER_RIGHTCTRL;
        case HID_KEY_SHIFT_RIGHT: return KEYBOARD_MODIFIER_RIGHTSHIFT;
        case HID_KEY_ALT_RIGHT: return KEYBOARD_MODIFIER_RIGHTALT;
        case HID_KEY_GUI_RIGHT: return KEYBOARD_MODIFIER_RIGHTGUI;
        default: return 0;
    }
}

static void key_press(uint8_t hid) {
    const hid_keyboard_report_t prev = report;
    for (uint8_t k : report.keycode)
        if (k == hid) return;   // already held (autorepeat)
    report.modifier |= hid_key_to_modifier(hid);
    for (uint8_t& k : report.keycode) {
        if (k == HID_KEY_NONE) {
            k = hid;
            process_kbd_report(&report, &prev);
            return;
        }
    }
    // More than 6 keys held: dropped, as by the PS/2 driver.
}

static void key_release(uint8_t hid) {
    const hid_keyboard_report_t prev = report;
    report.modifier &= (uint8_t)~hid_key_to_modifier(hid);
    for (uint8_t& k : report.keycode) {
        if (k == hid) {
            k = HID_KEY_NONE;
            process_kbd_report(&report, &prev);
            return;
        }
    }
}

static void kbd_linux_event(int code, int value) {
    const uint8_t hid = linux_to_hid(code);
    if (hid == HID_KEY_NONE) return;
    if (value == 1) key_press(hid);
    else if (value == 0) key_release(hid);
    // value 2 (autorepeat): the key is held already, nothing changes.

    // Ctrl+Alt+End: leave the emulator (Linux host only).
    if (value == 1 && hid == HID_KEY_END &&
        (report.modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL)) &&
        (report.modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT)))
        platform_request(PLATFORM_QUIT);
}

extern "C" void keyboard_init(void) {
    input_init();
}

extern "C" void keyboard_tick(void) {
    input_poll(kbd_linux_event);
}

extern "C" void ps2cleanup() {
  pthread_mutex_lock(&at_queue_lock);
  memset(last_at_scancodes, 0, sizeof(last_at_scancodes));
  last_at_scancode_idx = 0;
  pthread_mutex_unlock(&at_queue_lock);
}

extern "C" void push_script_scan_code(uint16_t sc) {
  push_at_scancode(sc);
}

extern "C" uint32_t ps2get_raw_code() {
  pthread_mutex_lock(&at_queue_lock);
  uint32_t res = last_at_scancodes[0];
  if (last_at_scancode_idx > 0) {
    for (int i = 0; i < 15; ++i) {
      uint32_t nv = last_at_scancodes[i + 1];
      if (!nv) break;
      last_at_scancodes[i] = nv;
    }
    last_at_scancodes[last_at_scancode_idx--] = 0;
  }
  pthread_mutex_unlock(&at_queue_lock);
  return res;
}

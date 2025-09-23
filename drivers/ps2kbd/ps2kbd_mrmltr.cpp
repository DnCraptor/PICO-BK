// You may use, distribute and modify this code under the
// terms of the GPLv2 license, which unfortunately won't be
// written for another century.
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// See:
// http://www.vetra.com/scancodes.html
// https://wiki.osdev.org/PS/2_Keyboard
//
#include "ps2kbd_mrmltr.h"
#if KBD_CLOCK_PIN == 2
#include "ps2kbd_mrmltr2.pio.h"
#else
#include "ps2kbd_mrmltr.pio.h"
#endif
#include "hardware/clocks.h"

#ifdef DEBUG_PS2
#define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DBG_PRINTF(...)
#endif

#define HID_KEYBOARD_REPORT_MAX_KEYS 6

// PS/2 set 2 to HID key conversion
static uint8_t ps2kbd_page_0[] {
  /* 00 (  0) */ HID_KEY_NONE,
  /* 01 (  1) */ HID_KEY_F9,
  /* 02 (  2) */ 0x00,
  /* 03 (  3) */ HID_KEY_F5,
  /* 04 (  4) */ HID_KEY_F3,
  /* 05 (  5) */ HID_KEY_F1,
  /* 06 (  6) */ HID_KEY_F2,
  /* 07 (  7) */ HID_KEY_F12,
  /* 08 (  8) */ HID_KEY_F13,
  /* 09 (  9) */ HID_KEY_F10,
  /* 0A ( 10) */ HID_KEY_F8,
  /* 0B ( 11) */ HID_KEY_F6,
  /* 0C ( 12) */ HID_KEY_F4,
  /* 0D ( 13) */ HID_KEY_TAB,
  /* 0E ( 14) */ HID_KEY_GRAVE,
  /* 0F ( 15) */ HID_KEY_KEYPAD_EQUAL,
  /* 10 ( 16) */ HID_KEY_F14,
  /* 11 ( 17) */ HID_KEY_ALT_LEFT,
  /* 12 ( 18) */ HID_KEY_SHIFT_LEFT,
  /* 13 ( 19) */ 0x00,
  /* 14 ( 20) */ HID_KEY_CONTROL_LEFT,
  /* 15 ( 21) */ HID_KEY_Q,
  /* 16 ( 22) */ HID_KEY_1,
  /* 17 ( 23) */ 0x00,
  /* 18 ( 24) */ HID_KEY_F15,
  /* 19 ( 25) */ 0x00,
  /* 1A ( 26) */ HID_KEY_Z,
  /* 1B ( 27) */ HID_KEY_S,
  /* 1C ( 28) */ HID_KEY_A,
  /* 1D ( 29) */ HID_KEY_W,
  /* 1E ( 30) */ HID_KEY_2,
  /* 1F ( 31) */ 0x00,
  /* 20 ( 32) */ HID_KEY_F16,
  /* 21 ( 33) */ HID_KEY_C,
  /* 22 ( 34) */ HID_KEY_X,
  /* 23 ( 35) */ HID_KEY_D,
  /* 24 ( 36) */ HID_KEY_E,
  /* 25 ( 37) */ HID_KEY_4,
  /* 26 ( 38) */ HID_KEY_3,
  /* 27 ( 39) */ 0x00,
  /* 28 ( 40) */ HID_KEY_F17,
  /* 29 ( 41) */ HID_KEY_SPACE,
  /* 2A ( 42) */ HID_KEY_V,
  /* 2B ( 43) */ HID_KEY_F,
  /* 2C ( 44) */ HID_KEY_T,
  /* 2D ( 45) */ HID_KEY_R,
  /* 2E ( 46) */ HID_KEY_5,
  /* 2F ( 47) */ 0x00,
  /* 30 ( 48) */ HID_KEY_F18,
  /* 31 ( 49) */ HID_KEY_N,
  /* 32 ( 50) */ HID_KEY_B,
  /* 33 ( 51) */ HID_KEY_H,
  /* 34 ( 52) */ HID_KEY_G,
  /* 35 ( 53) */ HID_KEY_Y,
  /* 36 ( 54) */ HID_KEY_6,
  /* 37 ( 55) */ 0x00,
  /* 38 ( 56) */ HID_KEY_F19,
  /* 39 ( 57) */ 0x00,
  /* 3A ( 58) */ HID_KEY_M,
  /* 3B ( 59) */ HID_KEY_J,
  /* 3C ( 60) */ HID_KEY_U,
  /* 3D ( 61) */ HID_KEY_7,
  /* 3E ( 62) */ HID_KEY_8,
  /* 3F ( 63) */ 0x00,
  /* 40 ( 64) */ HID_KEY_F20,
  /* 41 ( 65) */ HID_KEY_COMMA,
  /* 42 ( 66) */ HID_KEY_K,
  /* 43 ( 67) */ HID_KEY_I,
  /* 44 ( 68) */ HID_KEY_O,
  /* 45 ( 69) */ HID_KEY_0,
  /* 46 ( 70) */ HID_KEY_9,
  /* 47 ( 71) */ 0x00,
  /* 48 ( 72) */ HID_KEY_F21,
  /* 49 ( 73) */ HID_KEY_PERIOD,
  /* 4A ( 74) */ HID_KEY_SLASH,
  /* 4B ( 75) */ HID_KEY_L,
  /* 4C ( 76) */ HID_KEY_SEMICOLON,
  /* 4D ( 77) */ HID_KEY_P,
  /* 4E ( 78) */ HID_KEY_MINUS,
  /* 4F ( 79) */ 0x00,
  /* 50 ( 80) */ HID_KEY_F22,
  /* 51 ( 81) */ 0x00,
  /* 52 ( 82) */ HID_KEY_APOSTROPHE,
  /* 53 ( 83) */ 0x00,
  /* 54 ( 84) */ HID_KEY_BRACKET_LEFT,
  /* 55 ( 85) */ HID_KEY_EQUAL,
  /* 56 ( 86) */ 0x00,
  /* 57 ( 87) */ HID_KEY_F23,
  /* 58 ( 88) */ HID_KEY_CAPS_LOCK,
  /* 59 ( 89) */ HID_KEY_SHIFT_RIGHT,
  /* 5A ( 90) */ HID_KEY_ENTER, // RETURN ??
  /* 5B ( 91) */ HID_KEY_BRACKET_RIGHT,
  /* 5C ( 92) */ 0x00,
  /* 5D ( 93) */ HID_KEY_EUROPE_1,
  /* 5E ( 94) */ 0x00,
  /* 5F ( 95) */ HID_KEY_F24,
  /* 60 ( 96) */ 0x00,
  /* 61 ( 97) */ HID_KEY_EUROPE_2,
  /* 62 ( 98) */ 0x00,
  /* 63 ( 99) */ 0x00,
  /* 64 (100) */ 0x00,
  /* 65 (101) */ 0x00,
  /* 66 (102) */ HID_KEY_BACKSPACE,
  /* 67 (103) */ 0x00,
  /* 68 (104) */ 0x00,
  /* 69 (105) */ HID_KEY_KEYPAD_1,
  /* 6A (106) */ 0x00,
  /* 6B (107) */ HID_KEY_KEYPAD_4,
  /* 6C (108) */ HID_KEY_KEYPAD_7,
  /* 6D (109) */ 0x00,
  /* 6E (110) */ 0x00,
  /* 6F (111) */ 0x00,
  /* 70 (112) */ HID_KEY_KEYPAD_0,
  /* 71 (113) */ HID_KEY_KEYPAD_DECIMAL,
  /* 72 (114) */ HID_KEY_KEYPAD_2,
  /* 73 (115) */ HID_KEY_KEYPAD_5,
  /* 74 (116) */ HID_KEY_KEYPAD_6,
  /* 75 (117) */ HID_KEY_KEYPAD_8,
  /* 76 (118) */ HID_KEY_ESCAPE,
  /* 77 (119) */ HID_KEY_NUM_LOCK,
  /* 78 (120) */ HID_KEY_F11,
  /* 79 (121) */ HID_KEY_KEYPAD_ADD,
  /* 7A (122) */ HID_KEY_KEYPAD_3,
  /* 7B (123) */ HID_KEY_KEYPAD_SUBTRACT,
  /* 7C (124) */ HID_KEY_KEYPAD_MULTIPLY,
  /* 7D (125) */ HID_KEY_KEYPAD_9,
  /* 7E (126) */ HID_KEY_SCROLL_LOCK,
  /* 7F (127) */ 0x00,
  /* 80 (128) */ 0x00,
  /* 81 (129) */ 0x00,
  /* 82 (130) */ 0x00,
  /* 83 (131) */ HID_KEY_F7
};


Ps2Kbd_Mrmltr::Ps2Kbd_Mrmltr(PIO pio, uint base_gpio, std::function<void(hid_keyboard_report_t *curr, hid_keyboard_report_t *prev)> keyHandler) :
  _pio(pio),
  _base_gpio(base_gpio),
  _double(false),
  _overflow(false),
  _keyHandler(keyHandler)
{
  clearHidKeys();
  clearActions();
}

void Ps2Kbd_Mrmltr::clearHidKeys() {
  _report.modifier = 0;
  for (int i = 0; i < HID_KEYBOARD_REPORT_MAX_KEYS; ++i) _report.keycode[i] = HID_KEY_NONE;
}

inline static uint8_t hidKeyToMod(uint8_t hidKeyCode) {
  uint8_t m = 0;
  switch(hidKeyCode) {
  case HID_KEY_CONTROL_LEFT: m = KEYBOARD_MODIFIER_LEFTCTRL; break;
  case HID_KEY_SHIFT_LEFT: m = KEYBOARD_MODIFIER_LEFTSHIFT; break;
  case HID_KEY_ALT_LEFT: m = KEYBOARD_MODIFIER_LEFTALT; break;
  case HID_KEY_GUI_LEFT: m = KEYBOARD_MODIFIER_LEFTGUI; break;
  case HID_KEY_CONTROL_RIGHT: m = KEYBOARD_MODIFIER_RIGHTCTRL; break;
  case HID_KEY_SHIFT_RIGHT: m = KEYBOARD_MODIFIER_RIGHTSHIFT; break;
  case HID_KEY_ALT_RIGHT: m = KEYBOARD_MODIFIER_RIGHTALT; break;
  case HID_KEY_GUI_RIGHT: m = KEYBOARD_MODIFIER_RIGHTGUI; break;
  default: break;
  } 
  return m;
}

void Ps2Kbd_Mrmltr::handleHidKeyPress(uint8_t hidKeyCode) {
  hid_keyboard_report_t prev = _report;
  
  // Check the key is not alreay pressed
  for (int i = 0; i < HID_KEYBOARD_REPORT_MAX_KEYS; ++i) {
    if (_report.keycode[i] == hidKeyCode) {
      return;
    }
  }
  
  _report.modifier |= hidKeyToMod(hidKeyCode);
  
  for (int i = 0; i < HID_KEYBOARD_REPORT_MAX_KEYS; ++i) {
    if (_report.keycode[i] == HID_KEY_NONE) {
      _report.keycode[i] = hidKeyCode;      
      _keyHandler(&_report, &prev);
      return;
    }
  }
  
  // TODO Overflow
  DBG_PRINTF("PS/2 keyboard HID overflow\n");
}

void Ps2Kbd_Mrmltr::handleHidKeyRelease(uint8_t hidKeyCode) {
  hid_keyboard_report_t prev = _report;
  
  _report.modifier &= ~hidKeyToMod(hidKeyCode);
  
  for (int i = 0; i < HID_KEYBOARD_REPORT_MAX_KEYS; ++i) {
    if (_report.keycode[i] == hidKeyCode) {
      _report.keycode[i] = HID_KEY_NONE;
      _keyHandler(&_report, &prev);
      return;
    }
  }
}

uint8_t Ps2Kbd_Mrmltr::hidCodePage0(uint8_t ps2code) {
  return ps2code < sizeof(ps2kbd_page_0) ? ps2kbd_page_0[ps2code] : HID_KEY_NONE;
}

// PS/2 set 2 after 0xe0 to HID key conversion
uint8_t Ps2Kbd_Mrmltr::hidCodePage1(uint8_t ps2code) {
  switch(ps2code) {
// TODO these belong to a different HID usage page
//  case 0x37: return HID_KEY_POWER;
//  case 0x3f: return HID_KEY_SLEEP;
//  case 0x5e: return HID_KEY_WAKE;
  case 0x11: return HID_KEY_ALT_RIGHT;
  case 0x1f: return HID_KEY_GUI_LEFT;
  case 0x14: return HID_KEY_CONTROL_RIGHT;
  case 0x27: return HID_KEY_GUI_RIGHT;
  case 0x4a: return HID_KEY_KEYPAD_DIVIDE;
  case 0x5a: return HID_KEY_KEYPAD_ENTER;
  case 0x69: return HID_KEY_END;
  case 0x6b: return HID_KEY_ARROW_LEFT;
  case 0x6c: return HID_KEY_HOME;
  case 0x7c: return HID_KEY_PRINT_SCREEN;
  case 0x70: return HID_KEY_INSERT;
  case 0x71: return HID_KEY_DELETE;
  case 0x72: return HID_KEY_ARROW_DOWN;
  case 0x74: return HID_KEY_ARROW_RIGHT;
  case 0x75: return HID_KEY_ARROW_UP;
  case 0x7a: return HID_KEY_PAGE_DOWN;
  case 0x7d: return HID_KEY_PAGE_UP;

  default: 
    return HID_KEY_NONE;
  }
}

#include "ff.h"

void Ps2Kbd_Mrmltr::handleActions() {
  /*
  FIL f;
  f_open(&f, "1.log", FA_OPEN_APPEND | FA_WRITE);
  char tmp[64];
  for (uint i = 0; i <= _action; ++i) {
    snprintf(tmp, 64, "PS/2 key %s (i: %d) page %2.2X (%3.3d) code %2.2X (%3.3d)\n",
      _actions[i].release ? "release" : "press", i,
      _actions[i].page,
      _actions[i].page,
      _actions[i].code,
      _actions[i].code);
    UINT bw;
    f_write(&f, tmp, strlen(tmp), &bw); 
  }
  f_close(&f);
  */
  uint8_t hidCode;
  bool release;
  if (_action == 0) {
    switch (_actions[0].page) {
      case 1: {
        hidCode = hidCodePage1(_actions[0].code);
        break;
      }
      default: {
        hidCode = hidCodePage0(_actions[0].code);
        break;
      }
    }
    release = _actions[0].release;
  }
  else {
    if (_action == 1 && _actions[0].code == 0x14 && _actions[1].code == 0x77) {
       hidCode = HID_KEY_PAUSE;
       release = _actions[0].release;
    } else {
    // TODO get the HID code for extended PS/2 codes
      hidCode = HID_KEY_NONE;
      release = false;
    }
  }
  
  if (hidCode != HID_KEY_NONE) {
    
    DBG_PRINTF("HID key %s code %2.2X (%3.3d)\n",
      release ? "release" : "press",
      hidCode,
      hidCode);
      
    if (release) {
      handleHidKeyRelease(hidCode);
    }
    else {
      handleHidKeyPress(hidCode);
    }
  }
  
  DBG_PRINTF("PS/2 HID m=%2X ", _report.modifier);
  #ifdef DEBUG_PS2
  for (int i = 0; i < HID_KEYBOARD_REPORT_MAX_KEYS; ++i) printf("%2X ", _report.keycode[i]);
  printf("\n");
  #endif
}

void Ps2Kbd_Mrmltr::tick() {
  if (pio_sm_is_rx_fifo_full(_pio, _sm)) {
    DBG_PRINTF("PS/2 keyboard PIO overflow\n");
    _overflow = true;
    while (!pio_sm_is_rx_fifo_empty(_pio, _sm)) {
      // pull a scan code from the PIO SM fifo
      uint32_t rc = _pio->rxf[_sm];    
      printf("PS/2 drain rc %4.4lX (%ld)\n", rc, rc);
    }
    clearHidKeys();
    clearActions();
  }
  
  while (!pio_sm_is_rx_fifo_empty(_pio, _sm)) {
    // pull a scan code from the PIO SM fifo
    uint32_t rc = _pio->rxf[_sm];    
    DBG_PRINTF("PS/2 rc %4.4lX (%ld)\n", rc, rc);
    
    uint32_t code = (rc << 2) >> 24;
    DBG_PRINTF("PS/2 keycode %2.2lX (%ld)\n", code, code);

    // TODO Handle PS/2 overflow/error messages
    switch (code) {
      case 0xaa: {
         DBG_PRINTF("PS/2 keyboard Self test passed\n");
         break;       
      }
      case 0xe1: {
        _double = true;
        break;
      }
      case 0xe0: {
        _actions[_action].page = 1;
        break;
      }
      case 0xf0: {
        _actions[_action].release = true;
        break;
      }
      default: {
        _actions[_action].code = code;
        if (_double) {
          _action = 1;
          _double = false;
        }
        else {
          handleActions();
          clearActions();
        }
        break;
      }
    }
  }
}

// TODO Error checking and reporting
void Ps2Kbd_Mrmltr::init_gpio() {
    // init KBD pins to input
    gpio_init(_base_gpio);     // Data
    gpio_init(_base_gpio + 1); // Clock
    // with pull up
    gpio_pull_up(_base_gpio);
    gpio_pull_up(_base_gpio + 1);
    // get a state machine
    _sm = pio_claim_unused_sm(_pio, true);
    // reserve program space in SM memory
#if KBD_CLOCK_PIN == 2
    uint offset = pio_add_program(_pio, &m2ps2kbd_program);
#else
    uint offset = pio_add_program(_pio, &ps2kbd_program);
#endif
    // Set pin directions base
    pio_sm_set_consecutive_pindirs(_pio, _sm, _base_gpio, 2, false);
    // program the start and wrap SM registers
#if KBD_CLOCK_PIN == 2
    pio_sm_config c = m2ps2kbd_program_get_default_config(offset);
#else
    pio_sm_config c = ps2kbd_program_get_default_config(offset);
#endif
    // Set the base input pin. pin index 0 is DAT, index 1 is CLK  // Murmulator: 0->CLK 1->DAT ( _base_gpio + 1)
    //  sm_config_set_in_pins(&c, _base_gpio);
    sm_config_set_in_pins(&c, _base_gpio + 1);
    // Shift 8 bits to the right, autopush enabled
    sm_config_set_in_shift(&c, true, true, 10);
    // Deeper FIFO as we're not doing any TX
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    // We don't expect clock faster than 16.7KHz and want no less
    // than 8 SM cycles per keyboard clock.
    float div = (float)clock_get_hz(clk_sys) / (8 * 16700);
    sm_config_set_clkdiv(&c, div);
    // Ready to go
    pio_sm_init(_pio, _sm, offset, &c);
    pio_sm_set_enabled(_pio, _sm, true);
}

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

static uint8_t last_at_scancode_idx = 0;
static uint32_t last_at_scancodes[16] = { 0 };

static inline void push_at_scancode(uint32_t at) {
  if (!at) return;
  if (last_at_scancode_idx >= 15) return;
  last_at_scancodes[last_at_scancode_idx++] = at;
}

extern "C" bool handleScancode(uint32_t ps2xt_scancode);

void __not_in_flash_func(process_kbd_report)(
    hid_keyboard_report_t const *report,
    hid_keyboard_report_t const *prev_report
) {
  for (int i = 0; i < sizeof(mod2xt) / sizeof(mod2xt_t); ++i) {
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

Ps2Kbd_Mrmltr ps2kbd(
    pio1,
    KBD_CLOCK_PIN,
    process_kbd_report
);

extern "C" void keyboard_init(void) {
    tuh_init(BOARD_TUH_RHPORT);
    ps2kbd.init_gpio();
}

#include <host/usbh.h>

extern "C" void keyboard_tick(void) {
    ps2kbd.tick();
    tuh_task();
}

extern "C" void ps2cleanup() {
  memset(last_at_scancodes, 0, sizeof(last_at_scancodes));
  last_at_scancode_idx = 0;
}

extern "C" void push_script_scan_code(uint16_t sc) {
  push_at_scancode(sc);
}

extern "C" uint32_t ps2get_raw_code() {
  uint32_t res = last_at_scancodes[0];
  if (last_at_scancode_idx > 0) {
    for (int i = 0; i < 16; ++i) {
      uint32_t nv = last_at_scancodes[i + 1];
      if (!nv) break;
      last_at_scancodes[i] = nv;
    }
    last_at_scancodes[last_at_scancode_idx--] = 0;
  }
  return res;
}

#include <stdint.h>
#include "ps2_codes.h"
#include "reboot.h"
#include "Key.h"

#define AT_OVL __attribute__((section(".ovl1_eu.text")))

#include "Key_eu.h"
//#include "CPU_eu.h"

static const uint8_t Key_ShiftTab [32][2] =
{
    { '`',  '~'},   // 0x80    `~
    {'\'', '\''},   // 0x81    Ё            - В русской раскладке символа ~ нет, т.к. это Ч. Буквы Ё/ё нет на клавиатуре БК
    { '1',  '!'},   // 0x82    1!
    { '2',  '@'},   // 0x83    2@
    { '2',  '"'},   // 0x84    2"
    { '4',  '$'},   // 0x85    4$
    { '4',  ';'},   // 0x86    4;
    { '3',  '#'},   // 0x87    3#
    { '5',  '%'},   // 0x88    5%
    { '6',  '^'},   // 0x89    6^
    { '6',  ':'},   // 0x8A    6:
    { '7',  '&'},   // 0x8B    7&
    { '7',  '?'},   // 0x8C    7?
    { '8',  '*'},   // 0x8D    8*
    { ',',  '<'},   // 0x8E    ,<
    { '0',  ')'},   // 0x8F    0)
    { '9',  '('},   // 0x90    9(
    { '.',  '>'},   // 0x91    .>
    { '/',  '?'},   // 0x92    /?
    { '.',  ','},   // 0x93    .,
    { ';',  ':'},   // 0x94    ;:
    { '-',  '_'},   // 0x95    -_
    { '-',  '-'},   // 0x96    -_           - В русской раскладке нет символа _
    {'\'',  '"'},   // 0x97    '"
    { '[',  '{'},   // 0x98    [{
    { '=',  '+'},   // 0x99    =+
    { ']',  '}'},   // 0x9A    ]}
    {'\\',  '|'},   // 0x9B    \|
    { '/',  '/'},   // 0x9C    \/           - В русской раскладке нет символа '\'

    { ' ',  ' '},   // 0x9D
    { ' ',  ' '},   // 0x9E
    { ' ',  ' '},   // 0x9F
};

static const uint8_t Key_ASDF_RusLatTab [4][2] =
{
    {                 'A',                  'F'},       // PS2_A           0x1C    A       Ф    F
    {                 'S',                  'Y'},       // PS2_S           0x1B    S       Ы    Y
    {                 'D',                  'W'},       // PS2_D           0x23    D       В    W
    {                 'F',                  'A'},       // PS2_F           0x2B    F       А    A
};

static const uint8_t Key_RusLatTab [0x84] [2] =
{
                                                        // Key Name      Key Code  Lat     Rus
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x00
    {                  12,                   12},       // PS2_F9          0x01    СБР      (12)
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x02
    {                  23,                   23},       // PS2_F5          0x03    |==>     (23)
    {           0xA0 + 11,            0xA0 + 11},       // PS2_F3          0x04    =|=>|    (11)
    {           0xA0 +  1,            0xA0 +  1},       // PS2_F1          0x05    ПОВТ     (1)
    {                   3,                    3},       // PS2_F2          0x06    КТ       (3)
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_F12         0x07
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x08
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_F10         0x09
    {           0xA0 +  0,            0xA0 +  0},       // PS2_F8          0x0A    ШАГ      (0)
    {           0xA0 +  2,            0xA0 +  2},       // PS2_F6          0x0B    ИНД СУ   (2)
    {                  22,                   22},       // PS2_F4          0x0C    |<==     (22)
    {           0xA0 +  9,            0xA0 +  9},       // PS2_TAB         0x0D
    {                0x80,                 0x81},       // PS2_ACCENT      0x0E    `~      Ё            - В русской раскладке символа ~ нет, т.к. это Ч. Буквы Ё/ё нет на клавиатуре БК
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x0F
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x10
    {            KEY_LALT,             KEY_LALT},       // PS2_L_ALT       0x11
    {          KEY_LSHIFT,           KEY_LSHIFT},       // PS2_L_SHIFT     0x12
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x13
    {           KEY_LCTRL,            KEY_LCTRL},       // PS2_L_CTRL      0x14
    {                 'Q',                  'J'},       // PS2_Q           0x15    Q       Й    J
    {                0x82,                 0x82},       // PS2_1           0x16    1!      1!
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x17
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x18
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x19
    {                 'Z',                  'Q'},       // PS2_Z           0x1A    Z       Я    Q
    {               KEY_S,                KEY_S},       // PS2_S           0x1B    S       Ы    Y
    {               KEY_A,                KEY_A},       // PS2_A           0x1C    A       Ф    F
    {                 'W',                  'C'},       // PS2_W           0x1D    W       Ц    C
    {                0x83,                 0x84},       // PS2_2           0x1E    2@      2"
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x1F
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x20
    {                 'C',                  'S'},       // PS2_C           0x21    C       С    S
    {                 'X',                  '^'},       // PS2_X           0x22    X       Ч    ^
    {               KEY_D,                KEY_D},       // PS2_D           0x23    D       В    W
    {                 'E',                  'U'},       // PS2_E           0x24    E       У    U
    {                0x85,                 0x86},       // PS2_4           0x25    4$      4;
    {                0x87,                 0x87},       // PS2_3           0x26    3#      3#
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x27
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x28
    {           KEY_SPACE,            KEY_SPACE},       // PS2_SPACE       0x29
    {                 'V',                  'M'},       // PS2_V           0x2A    V       М    M
    {               KEY_F,                KEY_F},       // PS2_F           0x2B    F       А    A
    {                 'T',                  'E'},       // PS2_T           0x2C    T       Е    E
    {                 'R',                  'K'},       // PS2_R           0x2D    R       К    K
    {                0x88,                 0x88},       // PS2_5           0x2E    5%      5%
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x2F
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x30
    {                 'N',                  'T'},       // PS2_N           0x31    N       Т    T
    {                 'B',                  'I'},       // PS2_B           0x32    B       И    I
    {                 'H',                  'R'},       // PS2_H           0x33    H       Р    R
    {                 'G',                  'P'},       // PS2_G           0x34    G       П    P
    {                 'Y',                  'N'},       // PS2_Y           0x35    Y       Н    N
    {                0x89,                 0x8A},       // PS2_6           0x36    6^      6:
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x37
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x38
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x39
    {                 'M',                  'X'},       // PS2_M           0x3A    M       Ь    X
    {                 'J',                  'O'},       // PS2_J           0x3B    J       О    O
    {                 'U',                  'G'},       // PS2_U           0x3C    U       Г    G
    {                0x8B,                 0x8C},       // PS2_7           0x3D    7&      7?
    {                0x8D,                 0x8D},       // PS2_8           0x3E    8*      8*
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x3F
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x40
    {                0x8E,                  'B'},       // PS2_COMMA       0x41    ,<      Б    B
    {                 'K',                  'L'},       // PS2_K           0x42    K       Л    L
    {                 'I',                  '['},       // PS2_I           0x43    I       Ш    [
    {                 'O',                  ']'},       // PS2_O           0x44    O       Щ    ]
    {                0x8F,                 0x8F},       // PS2_0           0x45    0)      0)
    {                0x90,                 0x90},       // PS2_9           0x46    9(      9(
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x47
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x48
    {                0x91,                  '@'},       // PS2_PERIOD      0x49    .>      Ю    @
    {                0x92,                 0x93},       // PS2_SLASH       0x4A    /?      .,
    {                 'L',                  'D'},       // PS2_L           0x4B    L       Д    D
    {                0x94,                  'V'},       // PS2_SEMICOLON   0x4C    ;:      Ж    V
    {                 'P',                  'Z'},       // PS2_P           0x4D    P       З    Z
    {                0x95,                 0x96},       // PS2_MINUS       0x4E    -_      -_           - В русской раскладке нет символа _
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x4F
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x50
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x51
    {                0x97,                 '\\'},       // PS2_QUOTE       0x52    '"      Э    '\'
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x53
    {                0x98,                  'H'},       // PS2_L_BRACKET   0x54    [{      Х    H
    {                0x99,                 0x99},       // PS2_EQUALS      0x55    =+      =+
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x56
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x57
    {        KEY_CAPSLOCK,         KEY_CAPSLOCK},       // PS2_CAPS        0x58
    {          KEY_RSHIFT,           KEY_RSHIFT},       // PS2_R_SHIFT     0x59
    {                  10,                   10},       // PS2_ENTER       0x5A
    {                0x9A,                  '_'},       // PS2_R_BRACKET   0x5B    ]}      Ъ    _
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x5C
    {                0x9B,                 0x9C},       // PS2_BACK_SLASH  0x5D    \|      \/
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x5E
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x5F
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x60
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x61
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x62
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x63
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x64
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x65
    {                  24,                   24},       // PS2_BACKSPACE   0x66
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x67
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x68
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_1        0x69
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x6A
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_4        0x6B
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_7        0x6C
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x6D
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x6E
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x6F
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_0        0x70
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_PERIOD   0x71
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_2        0x72
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_5        0x73
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_6        0x74
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_8        0x75
    {             KEY_ESC,              KEY_ESC},       // PS2_ESC         0x76
    {         KEY_NUMLOCK,          KEY_NUMLOCK},       // PS2_NUMLOCK     0x77
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_F11         0x78
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_PLUS     0x79
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_3        0x7A
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_MINUS    0x7B
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_STAR     0x7C
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       // PS2_KP_9        0x7D
    {          KEY_SCROLL,           KEY_SCROLL},       // PS2_SCROLL      0x7E
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x7F
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x80
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x81
    {         KEY_UNKNOWN,          KEY_UNKNOWN},       //                 0x82
    {           0xA0 +  4,            0xA0 +  4},       // PS2_F7          0x83    БЛОК РЕД (4)
};

static uint_fast16_t AT_OVL ReturnKeyCode (uint_fast8_t Key, uint_fast32_t KeyFlags)
{
    if (Key >= 0100 && Key <= 0137 && (KeyFlags & (KEY_FLAGS_LCTRL | KEY_FLAGS_RCTRL))) Key &= ~0140;
    if (KeyFlags & (KEY_FLAGS_LALT | KEY_FLAGS_RALT)) Key |= KEY_AR2_PRESSED;

    return Key;
}

static uint_fast16_t AT_OVL ReturnShiftedKeyCode (uint_fast8_t Key, uint_fast32_t KeyFlags)
{
    if ((((KeyFlags >> KEY_FLAGS_LSHIFT_POS) | (KeyFlags >> KEY_FLAGS_RSHIFT_POS)) ^ (KeyFlags >> KEY_FLAGS_CAPSLOCK_POS) ^ (KeyFlags >> KEY_FLAGS_RUSLAT_POS)) & 1) Key += 32;

    return ReturnKeyCode (Key, KeyFlags);
}

uint_fast16_t AT_OVL Key_Translate (uint_fast16_t CodeAndFlags)
{
    uint_fast16_t Key;
    uint_fast8_t  Code;
    uint_fast32_t KeyFlags = Key_Flags;

    Code = CodeAndFlags & 0x3FF;

    if (Code <= 0x83)
    {
        Key = Key_RusLatTab [Code] [(KeyFlags >> KEY_FLAGS_RUSLAT_POS) & 1];

        if (Key <=   32) return ReturnKeyCode        ((uint_fast8_t) Key, KeyFlags);
        if (Key <  0x80) return ReturnShiftedKeyCode ((uint_fast8_t) Key, KeyFlags);
        if (Key <= 0x9F) return ReturnKeyCode        (Key_ShiftTab [Key - 0x80] [((KeyFlags >> KEY_FLAGS_LSHIFT_POS) | (KeyFlags >> KEY_FLAGS_RSHIFT_POS)) & 1], KeyFlags);
        if (Key <  0xC0) return ((Key - 0xA0) | KEY_AR2_PRESSED);

        switch (Key)
        {
            case KEY_LALT:      if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_LALT;
                                else                        KeyFlags |=  KEY_FLAGS_LALT;
                                Key_Flags = KeyFlags;
                                return KEY_UNKNOWN;

            case KEY_LSHIFT:    if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_LSHIFT;
                                else                        KeyFlags |=  KEY_FLAGS_LSHIFT;
                                Key_Flags = KeyFlags;
                                return KEY_UNKNOWN;

            case KEY_LCTRL:     if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_LCTRL;
                                else                        KeyFlags |=  KEY_FLAGS_LCTRL;
                                Key_Flags = KeyFlags;
                                return KEY_UNKNOWN;

            case KEY_CAPSLOCK:  if ((CodeAndFlags & 0x8000U) == 0) Key_Flags = KeyFlags ^ KEY_FLAGS_CAPSLOCK;
                                return KEY_UNKNOWN;

            case KEY_RSHIFT:    if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_RSHIFT;
                                else                        KeyFlags |=  KEY_FLAGS_RSHIFT;
                                Key_Flags = KeyFlags;
                                return KEY_UNKNOWN;

            case KEY_SCROLL:    if ((CodeAndFlags & 0x8000U) == 0) Key_Flags = KeyFlags ^ KEY_FLAGS_TURBO;
                                return KEY_UNKNOWN;

            case KEY_ESC:       return KEY_MENU_ESC;

            case KEY_NUMLOCK:   if ((CodeAndFlags & 0x8000U) == 0) Key_Flags = KeyFlags ^ KEY_FLAGS_NUMLOCK;
                                return KEY_UNKNOWN;

            case KEY_SPACE:     if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_BTN1;
                                else                        KeyFlags |=  KEY_FLAGS_BTN1;
                                Key_Flags = KeyFlags;
                                if ((KeyFlags & KEY_FLAGS_NUMLOCK) && (CodeAndFlags & KEY_TRANSLATE_UI) == 0) return KEY_UNKNOWN;
                                return ReturnKeyCode (32, KeyFlags);
            case KEY_A:
            case KEY_S:
            case KEY_D:
            case KEY_F:         if (CodeAndFlags & 0x8000U) KeyFlags &= ~(KEY_FLAGS_BTN2 << (Key - KEY_A));
                                else                        KeyFlags |=  (KEY_FLAGS_BTN2 << (Key - KEY_A));
                                Key_Flags = KeyFlags;
                                if ((KeyFlags & KEY_FLAGS_NUMLOCK) && (CodeAndFlags & KEY_TRANSLATE_UI) == 0) return KEY_UNKNOWN;
                                return ReturnShiftedKeyCode (Key_ASDF_RusLatTab [Key - KEY_A] [(KeyFlags >> KEY_FLAGS_RUSLAT_POS) & 1], KeyFlags);
        }

        return KEY_UNKNOWN;
    }

    switch (Code)
    {
        case PS2_R_ALT:     if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_RALT;
                            else                        KeyFlags |=  KEY_FLAGS_RALT;
                            Key_Flags = KeyFlags;
                            return KEY_UNKNOWN;

        case PS2_PRINT:     reboot (0x55AA55AA);
                            return KEY_UNKNOWN;

        case PS2_R_CTRL:    if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_RCTRL;
                            else                        KeyFlags |=  KEY_FLAGS_RCTRL;
                            Key_Flags = KeyFlags;
                            return KEY_UNKNOWN;

        case PS2_L_WIN:     return ReturnKeyCode (14, KeyFlags);
        case PS2_R_WIN:     return ReturnKeyCode (15, KeyFlags);
//      case PS2_MENU:
//      case PS2_KP_SLASH:
//      case PS2_KP_ENTER:
//      case PS2_END:

        case PS2_LEFT:      if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_LEFT;
                            else                        KeyFlags |=  KEY_FLAGS_LEFT;
                            Key_Flags = KeyFlags;
                            if ((KeyFlags & KEY_FLAGS_NUMLOCK) && (CodeAndFlags & KEY_TRANSLATE_UI) == 0) return KEY_UNKNOWN;
                            return ReturnKeyCode (8, KeyFlags);

//      case PS2_HOME:
//      case PS2_INSERT:
//      case PS2_DELETE:
        case PS2_DOWN:      if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_DOWN;
                            else                        KeyFlags |=  KEY_FLAGS_DOWN;
                            Key_Flags = KeyFlags;
                            if ((KeyFlags & KEY_FLAGS_NUMLOCK) && (CodeAndFlags & KEY_TRANSLATE_UI) == 0) return KEY_UNKNOWN;
                            return ReturnKeyCode (27, KeyFlags);

        case PS2_RIGHT:     if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_RIGHT;
                            else                        KeyFlags |=  KEY_FLAGS_RIGHT;
                            Key_Flags = KeyFlags;
                            if ((KeyFlags & KEY_FLAGS_NUMLOCK) && (CodeAndFlags & KEY_TRANSLATE_UI) == 0) return KEY_UNKNOWN;
                            return ReturnKeyCode (25, KeyFlags);

        case PS2_UP:        if (CodeAndFlags & 0x8000U) KeyFlags &= ~KEY_FLAGS_UP;
                            else                        KeyFlags |=  KEY_FLAGS_UP;
                            Key_Flags = KeyFlags;
                            if ((KeyFlags & KEY_FLAGS_NUMLOCK) && (CodeAndFlags & KEY_TRANSLATE_UI) == 0) return KEY_UNKNOWN;
                            return ReturnKeyCode (26, KeyFlags);

//      case PS2_PGDN:
//      case PS2_PGUP:
//      case PS2_PAUSE:     if ((CodeAndFlags & 0x8000U) == 0) CPU_Stop ();
//                          return KEY_UNKNOWN;
    }

    return KEY_UNKNOWN;
}

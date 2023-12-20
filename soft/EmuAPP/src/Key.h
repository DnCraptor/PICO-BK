#ifndef KEY_E_H_INCLUDE
#define KEY_E_H_INCLUDE

#include <stdint.h>

#define Key_SetRusLat(     ) Key_Flags |=  KEY_FLAGS_RUSLAT
#define Key_ClrRusLat(     ) Key_Flags &= ~KEY_FLAGS_RUSLAT
#define Key_SaveRusLat(    ) {if (Key_Flags & KEY_FLAGS_RUSLAT      ) Key_Flags |= KEY_FLAGS_RUSLAT_SAVED; else Key_Flags &= ~KEY_FLAGS_RUSLAT_SAVED;}
#define Key_RestoreRusLat( ) {if (Key_Flags & KEY_FLAGS_RUSLAT_SAVED) Key_Flags |= KEY_FLAGS_RUSLAT;       else Key_Flags &= ~KEY_FLAGS_RUSLAT;      }

enum
{
    KEY_LALT        = 0xC0,
    KEY_LSHIFT,
    KEY_LCTRL,
    KEY_CAPSLOCK,
    KEY_RSHIFT,
    KEY_SCROLL,
    KEY_NUMLOCK,
    KEY_SPACE,
    KEY_ESC,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_UNKNOWN     = 0xFF
};

#define KEY_FLAGS_RUSLAT_POS        0
#define KEY_FLAGS_RUSLAT_SAVED_POS  1
#define KEY_FLAGS_ALT_POS          2
//#define KEY_FLAGS_RALT_POS          3
#define KEY_FLAGS_SHIFT_POS        4
//#define KEY_FLAGS_RSHIFT_POS        5
#define KEY_FLAGS_CTRL_POS         6
//#define KEY_FLAGS_RCTRL_POS         7
#define KEY_FLAGS_TURBO_POS         8
#define KEY_FLAGS_NUMLOCK_POS       9
#define KEY_FLAGS_CAPSLOCK_POS     10
#define KEY_FLAGS_UP_POS           11
#define KEY_FLAGS_BTN1_POS         12
#define KEY_FLAGS_RIGHT_POS        13
#define KEY_FLAGS_DOWN_POS         14
#define KEY_FLAGS_LEFT_POS         15
#define KEY_FLAGS_BTN2_POS         16
#define KEY_FLAGS_BTN3_POS         17
#define KEY_FLAGS_BTN4_POS         18
#define KEY_FLAGS_BTN5_POS         19

#define KEY_FLAGS_RUSLAT        (1UL << KEY_FLAGS_RUSLAT_POS      )
#define KEY_FLAGS_RUSLAT_SAVED  (1UL << KEY_FLAGS_RUSLAT_SAVED_POS)
#define KEY_FLAGS_ALT          (1UL << KEY_FLAGS_ALT_POS        )
//#define KEY_FLAGS_RALT          (1UL << KEY_FLAGS_RALT_POS        )
#define KEY_FLAGS_SHIFT        (1UL << KEY_FLAGS_SHIFT_POS      )
//#define KEY_FLAGS_RSHIFT        (1UL << KEY_FLAGS_RSHIFT_POS      )
#define KEY_FLAGS_CTRL         (1UL << KEY_FLAGS_CTRL_POS       )
//#define KEY_FLAGS_RCTRL         (1UL << KEY_FLAGS_RCTRL_POS       )
#define KEY_FLAGS_CAPSLOCK      (1UL << KEY_FLAGS_CAPSLOCK_POS    )
#define KEY_FLAGS_TURBO         (1UL << KEY_FLAGS_TURBO_POS       )
#define KEY_FLAGS_NUMLOCK       (1UL << KEY_FLAGS_NUMLOCK_POS     )
#define KEY_FLAGS_UP            (1UL << KEY_FLAGS_UP_POS          )
#define KEY_FLAGS_RIGHT         (1UL << KEY_FLAGS_RIGHT_POS       )
#define KEY_FLAGS_DOWN          (1UL << KEY_FLAGS_DOWN_POS        )
#define KEY_FLAGS_LEFT          (1UL << KEY_FLAGS_LEFT_POS        )
#define KEY_FLAGS_BTN1          (1UL << KEY_FLAGS_BTN1_POS        )
#define KEY_FLAGS_BTN2          (1UL << KEY_FLAGS_BTN2_POS        )
#define KEY_FLAGS_BTN3          (1UL << KEY_FLAGS_BTN3_POS        )
#define KEY_FLAGS_BTN4          (1UL << KEY_FLAGS_BTN4_POS        )
#define KEY_FLAGS_BTN5          (1UL << KEY_FLAGS_BTN5_POS        )

#define KEY_AR2_PRESSED 0x8000

extern uint32_t Key_Flags;

#define KEY_TRANSLATE_UI 0x4000

#endif

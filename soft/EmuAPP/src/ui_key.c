#include "ps2.h"
#include "ps2_codes.h"
#include "Key.h"
#include "ui.h"

uint_fast16_t OVL_SEC(ui_GetKey) ui_GetKey (void)
{
    uint_fast16_t CodeAndFlags;
    uint_fast16_t Key;

    ps2_periodic ();

    CodeAndFlags = ps2_read ();

    if (CodeAndFlags == 0) return 0;

    if (CodeAndFlags == PS2_DELETE) return KEY_MENU_DELETE;

    Key = Key_Translate (CodeAndFlags | KEY_TRANSLATE_UI);

    ps2_leds ((Key_Flags >> KEY_FLAGS_CAPSLOCK_POS) & 1, (Key_Flags >> KEY_FLAGS_NUMLOCK_POS) & 1, (Key_Flags >> KEY_FLAGS_TURBO_POS) & 1);

    if ((CodeAndFlags & 0x8000U) || (Key == KEY_UNKNOWN)) return 0;

    if      (Key == 14) Key_SetRusLat ();
    else if (Key == 15) Key_ClrRusLat ();

//  Key &= ~KEY_AR2_PRESSED;

    if ((Key_Flags & KEY_FLAGS_RUSLAT) && (Key & 0x40)) Key ^= 0x80;

    return Key;
}

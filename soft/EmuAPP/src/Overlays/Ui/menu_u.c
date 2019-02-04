#include <string.h>
#include "reboot.h"
#include "ovl.h"
#include "menu.h"
#include "ffs.h"

#include "menu_u.h"
#include "ui_u.h"
#include "xprintf_u.h"
#include "fileman_u.h"

#define AT_OVL __attribute__((section(".ovl3_u.text")))

extern char __BUILD_NUMBER__;

int_fast16_t AT_OVL menu_fileman (uint_fast8_t Flags)
{
    AT_IROM static const char str_Menu [] =
    (
        "1.Программы\n"
        "2.Игры\n"
        "3.Утилиты\n"
        "4.Записи из эмулятора\n"
    );
    uint_fast8_t type;
    int_fast16_t n;
    
    if (Flags & MENU_FLAG_START_UI) ui_start();

again:
    ui_clear();
    ui_header_default();
    ui_draw_list (0, 4, str_Menu);
    switch (ui_select(0, 4, 4))
    {
    case 0:
        // Программы
        type=TYPE_PROG;
        break;
    
    case 1:
        // Игры
        type=TYPE_GAME;
        break;
    
    case 2:
        // Утилиты
        type=TYPE_UTIL;
        break;
    
    case 3:
        // Записи из эмулятора
        type=TYPE_TAPE;
        break;
    
    default:
        n = -1;
        goto Exit;
    }
    
    // Выбираем файл
    n=fileman(type, Flags & MENU_FLAG_LOAD_FILE);

    if (n < 0) goto again;

Exit:
    if (Flags & MENU_FLAG_START_UI) ui_stop();

    return n;
}

void AT_OVL menu (uint_fast8_t Flags)
{
    AT_IROM static const char str_Menu [] =
    (
        "1.Возврат к эмуляции\n"
        "2.Файловый менеджер\n"
        "3.Полный сброс\n"
        "4.Переключиться в режим WiFi\n"
    );
    AT_IROM static const char str_Help [] =
    (
        "Привязка клавиатуры:\n"
        "АР2      - Alt     F1 - ПОВТ\n"
        "Shift    - НР      F2 - КТ\n"
        "Ctrl     - СУ      F3 - =|=>|\n"
        "CapsLock - ЗАГЛ    F4 - |<==\n"
        "CapsLock - СТР     F5 - |==>\n"
        "Л.Win    - РУС     F6 - ИНД СУ\n"
        "П.Win    - ЛАТ     F7 - БЛОК РЕД\n"
        "Pause    - СТОП    F8 - ШАГ\n"
        "                   F9 - СБР\n"
        "\n"
        "Управление эмуляцией:\n"
        "Scroll Lock - Турбо режим\n"
        "Esc         - Меню эмулятора\n"
        "NumLock     - Включить джойстик"
    );
    AT_IROM static const char str_Build [] = "BK8266 Сборка #%d";
    char str[32];
    
    if (Flags & MENU_FLAG_START_UI) ui_start();

again:
    ui_clear();
    ui_header_default();
    ui_draw_list (0, 3, str_Menu);
    ui_draw_text (0, 8, str_Help);
    xsprintf(str, str_Build, (int)&__BUILD_NUMBER__);
    ui_draw_text (32 - strlen (str), 24, str);

    switch (ui_select(0, 3, 4))
    {
    case 0:
        // Возврат в монитор
        break;
    
    case 1:
        // Файловый менеджер
        menu_fileman (0);
        goto again;
    
    case 2:
        // Полный сброс
        reboot(0);
        break;
    
    case 3:
        // Переключиться в режим WiFi
        reboot(0x55AA55AA);
        break;
    }

    if (Flags & MENU_FLAG_START_UI) ui_stop();
}

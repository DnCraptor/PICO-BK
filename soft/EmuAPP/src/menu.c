#include "menu.h"
#include "ui.h"
#include "reboot.h"
#include "ffs.h"
#include "xprintf.h"
#include "fileman.h"

#include "CPU.h"

extern char __BUILD_NUMBER__;

int_fast16_t OVL_SEC (menu_fileman) menu_fileman (uint_fast8_t Load)
{
    uint_fast8_t type;
    int_fast16_t n;
    
    ui_start();

again:
    ui_clear();
    ui_header_default();
    ui_draw_list(
    0, 4,
    "1.Программы\n"
    "2.Игры\n"
    "3.Утилиты\n"
    "4.Записи из эмулятора\n"
    );
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
    n=fileman(type, Load);

    if (n < 0) goto again;

Exit:
    ui_stop();

    return n;
}

void OVL_SEC (menu) menu(void)
{
    char str[32];
    
    ui_start();

again:
    ui_clear();
    ui_header_default();
    ui_draw_list(
    0, 3,
    "1.Возврат к эмуляции\n"
    "2.Файловый менеджер\n"
    "3.Полный сброс\n"
    "4.Переключиться в режим WiFi\n"
    );
    ui_draw_text(0, 8,
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
    xsprintf(str, "BK8266 Сборка #%d", (int)&__BUILD_NUMBER__);
    ui_draw_text(32-ets_strlen(str), 24, str);

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

    ui_stop();
}

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
        "1.���������\n"
        "2.����\n"
        "3.�������\n"
        "4.������ �� ���������\n"
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
        // ���������
        type=TYPE_PROG;
        break;
    
    case 1:
        // ����
        type=TYPE_GAME;
        break;
    
    case 2:
        // �������
        type=TYPE_UTIL;
        break;
    
    case 3:
        // ������ �� ���������
        type=TYPE_TAPE;
        break;
    
    default:
        n = -1;
        goto Exit;
    }
    
    // �������� ����
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
        "1.������� � ��������\n"
        "2.�������� ��������\n"
        "3.������ �����\n"
        "4.������������� � ����� WiFi\n"
    );
    AT_IROM static const char str_Help [] =
    (
        "�������� ����������:\n"
        "��2      - Alt     F1 - ����\n"
        "Shift    - ��      F2 - ��\n"
        "Ctrl     - ��      F3 - =|=>|\n"
        "CapsLock - ����    F4 - |<==\n"
        "CapsLock - ���     F5 - |==>\n"
        "�.Win    - ���     F6 - ��� ��\n"
        "�.Win    - ���     F7 - ���� ���\n"
        "Pause    - ����    F8 - ���\n"
        "                   F9 - ���\n"
        "\n"
        "���������� ���������:\n"
        "Scroll Lock - ����� �����\n"
        "Esc         - ���� ���������\n"
        "NumLock     - �������� ��������"
    );
    AT_IROM static const char str_Build [] = "BK8266 ������ #%d";
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
        // ������� � �������
        break;
    
    case 1:
        // �������� ��������
        menu_fileman (0);
        goto again;
    
    case 2:
        // ������ �����
        reboot(0);
        break;
    
    case 3:
        // ������������� � ����� WiFi
        reboot(0x55AA55AA);
        break;
    }

    if (Flags & MENU_FLAG_START_UI) ui_stop();
}

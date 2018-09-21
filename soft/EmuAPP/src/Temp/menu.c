#include "menu.h"

#include "ui.h"
#include "i8080.h"
#include "i8080_hal.h"
#include "reboot.h"
#include "ffs.h"
#include "files.h"
#include "tape.h"
#include "zkg.h"
#include "xprintf.h"
#include "fileman.h"


extern char __BUILD_NUMBER__;


bool menu_fileman(void)
{
    uint8_t type;
    int16_t n;
    
again:
    ui_clear();
    ui_header("�����-86�� -->");
    ui_draw_list(
	"1.���������\n"
	"2.����\n"
	"3.�������\n"
	"4.������������� ������\n"
	);
    switch (ui_select(4))
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
	    // ������������� ������
	    type=TYPE_TAPE;
	    break;
	
	default:
	    return false;
    }
    
    // �������� ����
again2:
    n=fileman(type, "�������� ���� ��� ��������:");
    if (n < 0) goto again;
    
    // ��������� ����
    if (type != TYPE_TAPE)
    {
	// �������� ������ � ������
	int16_t addr=load_file(n);
	if (addr >= 0)
	{
	    // ��������� ����������� - ���������
	    i8080_jump(addr);
	    
	    // ������������ � ��������
	    return true;
	} else
	{
	    // ������ �������� �����
	    ui_clear();
	    ui_header("�����-86�� -->");
	    ui_draw_text(10, 10, "������ �������� ����� !");
	    ui_sleep(1000);
	    goto again2;
	}
    } else
    {
	// �������� � �����������
	tape_load(n);
	return true;
    }
}




void menu(void)
{
    char str[32];
    
again:
    ui_clear();
    ui_header("�����-86�� -->");
    ui_draw_list(
	"1.(  F11 )  ������� � ������� (��� ������� ������)\n"
	"2.(  F12 )  �������� ��������\n"
	"3.(PrnScr) ������ �����\n"
	"4.(Pause ) ������������� � ����� WiFi\n"
	);
    ui_draw_text(10, 16,
	"�������� ����������:\n"
	"�1-�4   - F1-F4          ��  - Enter\n"
	"��2     - Alt            ��  - ���. Enter\n"
	"���/��� - Caps Lock      ��  - Backspace\n"
	"��      - CTRL           \\   - Home\n"
	"��      - Shift          ��� - End/Delete\n"
	"\n"
	"\n"
	"���������� ���������:\n"
	"F5-F10      - ����� ��� E000+n*4\n"
	"Scroll Lock - ����� �����\n"
	"WIN+������  - ����� �����������\n"
	"MENU        - ������� �� �����-86��\n"
	);
    xsprintf(str, "RK8266 ������ #%d", (int)&__BUILD_NUMBER__);
    ui_draw_text(64+6-ets_strlen(str), 33, str);
    switch (ui_select(4))
    {
	case 0:
	    // ������� � �������
	    i8080_jump(0xF800);
	    break;
	
	case 1:
	    // �������� ��������
	    if (! menu_fileman()) goto again;
	    break;
	
	case 2:
	    // ������ �����
	    ets_memset(i8080_hal_memory(), 0x00, 0x8000);
	    i8080_init();
	    i8080_jump(0xF800);
	    break;
	
	case 3:
	    // ������������� � ����� WiFi
	    reboot(0x55AA55AA);
	    break;
    }
}

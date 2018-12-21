#include "ui.h"
#include "ets.h"
#include "ps2.h"
#include "ps2_codes.h"
#include "xprintf.h"
#include "CPU.h"
#include "align4.h"
#include "timer0.h"
#include "Key.h"


TUI_Data UI_Data;


void OVL_SEC (ui_clear) ui_clear(void)
{
    ets_memset(UI_Data.scr, CPU_koi8_to_zkg(' '), sizeof(UI_Data.scr));
}


#define HEADER_X    0
#define HEADER_Y    0
void OVL_SEC (ui_header) ui_header(const char *s)
{
    ui_draw_text(HEADER_X, HEADER_Y, s);
}

void OVL_SEC (ui_header_default) ui_header_default(void)
{
    ui_draw_text(HEADER_X, HEADER_Y, " ЭМУЛЯТОР БК-0010-01 НА ESP8266");
}

void OVL_SEC (ui_draw_list) ui_draw_list(uint8_t x, uint8_t y, const char *s)
{
    x += 3;

    while (*s)
    {
    uint8_t x_tmp;
    for (x_tmp=x; (*s) && ((*s)!='\n'); x_tmp++)
        UI_Data.scr[y][x_tmp]=CPU_koi8_to_zkg((uint8_t)(*s++));
    if (*s) s++;    // пропускаем '\n'
    y++;
    }
}


void OVL_SEC (ui_draw_text) ui_draw_text(uint8_t x, uint8_t y, const char *s)
{
    while (*s)
    {
    uint8_t xx;
    for (xx=x; (*s) && ((*s)!='\n'); xx++)
        UI_Data.scr[y][xx]=CPU_koi8_to_zkg((uint8_t)(*s++));
    if (*s) s++;    // пропускаем '\n'
    y++;
    }
}


int8_t OVL_SEC (ui_select) ui_select(uint8_t x, uint8_t y, uint8_t count)
{
    uint8_t n=0, prev=0;
    
    while (1)
    {
    // Очищаем предыдущую позицию
    ui_draw_text (x + (prev / 16)*16, y + (prev % 16), "   ");
    
    // Рисуем новую позицию
    ui_draw_text (x + (n    / 16)*16, y + (n    % 16), "-->");

    // Сохраняем текущую позицию как предыдущую для перерисовки
    prev=n;
    
    // Читаем клаву
    while (1)
    {
        uint_fast16_t c = OVL_CALL0 (OVL_NUM (ui_select), ui_GetKey);
        if (c==KEY_MENU_ESC)
        {
        // Отмена
        return -1;
        } else
        if (c==KEY_MENU_ENTER)
        {
            // Ввод
            return n;
        } else
        if ( (c==KEY_MENU_UP) && (n > 0) )
        {
        // Вверх
        n--;
        break;
        } else
        if ( (c==KEY_MENU_DOWN) && (n < count-1) )
        {
        // Вниз
        n++;
        break;
        } else
        if ( (c==KEY_MENU_LEFT) && (n > 0) )
        {
        // Влево
        if (n > 20) n-=20; else n=0;
        break;
        } else
        if ( (c==KEY_MENU_RIGHT) && (n < count-1) )
        {
        // Вправо
        n+=20;
        if (n >= count) n=count-1;
        break;
        } else
        if ( (c>='1') && (c<='9') )
        {
        // Выбор нужного пункта по номеру
        if (c-'1' < count)
        {
            return c-'1';
        }
        }
    }
    }
}


const char* OVL_SEC (ui_input_text) ui_input_text(const char *comment, const char *_text, uint8_t max_len)
{
    static char text[64];
    uint8_t pos;
    uint_fast16_t c;
    
#define EDIT_X  0
#define EDIT_Y  4
    if (_text)
    {
    // Есть текст
    ets_strcpy(text, _text);
    pos=ets_strlen(text);
    } else
    {
    // Нет текста - начинаем с пустой строки
    text[0]=0;
    pos=0;
    }
    
    UI_Data.CursorX=EDIT_X+pos;
    UI_Data.CursorY=EDIT_Y;
    ui_clear();
    ui_header_default();
    ui_draw_text(0, 3, comment);
    ui_draw_text(EDIT_X, EDIT_Y, text);
    while (1)
    {
    c = OVL_CALL0 (OVL_NUM (ui_input_text), ui_GetKey);
    if (c==KEY_MENU_ESC)
    {
        // Отмена
        UI_Data.CursorY=99;
        return 0;
    } else
    if ( (c==KEY_MENU_ENTER) && (pos > 0) )
    {
        // Сохранение
        text[pos]=0;
        UI_Data.CursorY=99;
        return text;
    } else
    if ( (c==KEY_MENU_BACKSPACE) && (pos > 0) )
    {
        // Забой
        UI_Data.CursorX--;
        pos--;
        UI_Data.scr[EDIT_Y][EDIT_X+pos]=CPU_koi8_to_zkg(' ');
    } else
    if ( (((c>=32) && (c<128)) || ((c>=0xC0) && (c<0x100))) && (pos < max_len) )
    {
        // Символ
        text[pos]=c;
        UI_Data.scr[EDIT_Y][EDIT_X+pos]=CPU_koi8_to_zkg(c);
        UI_Data.CursorX++;
        pos++;
    }
    }
}


int8_t OVL_SEC (ui_yes_no) ui_yes_no(const char *comment)
{
    ui_clear();
    ui_header_default();
    ui_draw_text(0, 2, comment);
    ui_draw_list(0, 4, "НЕТ\nДА\n");
    return ui_select(0, 4, 2);
}


void OVL_SEC (ui_start) ui_start(void)
{
    // Сохраняем состояние клавиатуры
    UI_Data.PrevRusLat = Key_Flags >> KEY_FLAGS_RUSLAT_POS;

    // Очищаем экран
    ui_clear();

    // Переключаем экран в режим меню
    UI_Data.CursorX=0;
    UI_Data.CursorY=99;
    UI_Data.Show=1;
}


void OVL_SEC (ui_stop) ui_stop(void)
{
    // Восстанавливаем состояние клавиатуры
    Key_Flags = (Key_Flags & ~KEY_FLAGS_RUSLAT) | ((UI_Data.PrevRusLat & 1) << KEY_FLAGS_RUSLAT_POS);

    // Возвращаем экран на место
    UI_Data.Show=0;
}


void OVL_SEC (ui_sleep) ui_sleep(uint16_t ms)
{
    uint32_t _sleep=getCycleCount()+(ms)*160000;
    while (((uint32_t)(getCycleCount() - _sleep)) & 0x80000000);
}

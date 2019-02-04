#ifndef UI_U_H
#define UI_U_H

#include <stdint.h>

typedef struct
{
    uint32_t  CursorXY;

    union
    {
        char     Ch  [64];
        uint32_t U32 [64 / 4];

    } InputText;

    union
    {
        uint8_t  U8  [32];
        uint32_t U32 [32 / 4];

    } Scr [25];

} TUI_OvlData;

extern TUI_OvlData UI_OvlData;

void          ui_clear          (void);
void          ui_header         (const char *s);
void          ui_header_default (void);
void          ui_draw_list      (uint8_t x, uint8_t y, const char *s);
void          ui_draw_text      (uint8_t x, uint8_t y, const char *s);
int8_t        ui_select         (uint8_t x, uint8_t y, uint8_t count);
const char   *ui_input_text     (const char *comment, const char *text, uint8_t max_len);
int8_t        ui_yes_no         (const char *comment);
void          ui_Resume         (void);
void          ui_Suspend        (void);
void          ui_start          (void);
void          ui_stop           (void);
void          ui_sleep          (uint16_t ms);
uint_fast16_t ui_GetKey         (void);

#endif

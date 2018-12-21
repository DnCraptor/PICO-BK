#ifndef UI_H
#define UI_H


#include "ovl.h"
#include "ets.h"

typedef struct
{
	uint8_t  Show;
	uint8_t  PrevRusLat;
	uint8_t  CursorX;
	uint8_t  CursorY;
	char     scr[25][32];

} TUI_Data;

extern TUI_Data UI_Data;


#define ui_clear_ovln OVL_NUM_UI
#define ui_clear_ovls OVL_SEC_UI
void ui_clear(void);

#define ui_header_ovln OVL_NUM_UI
#define ui_header_ovls OVL_SEC_UI
void ui_header(const char *s);

#define ui_header_default_ovln OVL_NUM_UI
#define ui_header_default_ovls OVL_SEC_UI
void ui_header_default(void);

#define ui_draw_list_ovln OVL_NUM_UI
#define ui_draw_list_ovls OVL_SEC_UI
void ui_draw_list(uint8_t x, uint8_t y, const char *s);

#define ui_draw_text_ovln OVL_NUM_UI
#define ui_draw_text_ovls OVL_SEC_UI
void ui_draw_text(uint8_t x, uint8_t y, const char *s);

#define ui_select_ovln OVL_NUM_UI
#define ui_select_ovls OVL_SEC_UI
int8_t ui_select(uint8_t x, uint8_t y, uint8_t count);

#define ui_input_text_ovln OVL_NUM_UI
#define ui_input_text_ovls OVL_SEC_UI
const char* ui_input_text(const char *comment, const char *text, uint8_t max_len);

#define ui_yes_no_ovln OVL_NUM_UI
#define ui_yes_no_ovls OVL_SEC_UI
int8_t ui_yes_no(const char *comment);

#define ui_start_ovln OVL_NUM_UI
#define ui_start_ovls OVL_SEC_UI
void ui_start(void);

#define ui_stop_ovln OVL_NUM_UI
#define ui_stop_ovls OVL_SEC_UI
void ui_stop(void);

#define ui_sleep_ovln OVL_NUM_UI
#define ui_sleep_ovls OVL_SEC_UI
void ui_sleep(uint16_t ms);

#define ui_GetKey_ovln OVL_NUM_EMU
#define ui_GetKey_ovls OVL_SEC_EMU
uint_fast16_t ui_GetKey(void);

#endif

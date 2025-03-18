#pragma once
#include <stdbool.h>
#include <inttypes.h>

#define MAX_WIDTH 128
#define MAX_HEIGHT 48
#define BYTES_PER_CHAR 2

typedef struct color_schema {
    uint8_t BACKGROUND_FIELD_COLOR;
    uint8_t FOREGROUND_FIELD_COLOR;
    uint8_t HIGHLIGHTED_FIELD_COLOR;
    uint8_t BACKGROUND_F1_12_COLOR;
    uint8_t FOREGROUND_F1_12_COLOR;
    uint8_t BACKGROUND_F_BTN_COLOR;
    uint8_t FOREGROUND_F_BTN_COLOR;
    uint8_t BACKGROUND_CMD_COLOR;
    uint8_t FOREGROUND_CMD_COLOR;
    uint8_t BACKGROUND_SEL_BTN_COLOR;
    uint8_t FOREGROUND_SELECTED_COLOR;
    uint8_t BACKGROUND_SELECTED_COLOR;
} color_schema_t;

// type of F1-F10 function pointer
typedef void (*fn_1_12_ptr)(uint8_t);

#define BTN_WIDTH 8
typedef struct fn_1_12_tbl_rec {
    char pre_mark;
    char mark;
    char name[BTN_WIDTH];
    fn_1_12_ptr action;
} fn_1_12_tbl_rec_t;

#define BTNS_COUNT 12
typedef fn_1_12_tbl_rec_t fn_1_12_tbl_t[BTNS_COUNT];

void set_color_schema(color_schema_t* pschema);

const color_schema_t* get_color_schema();

void draw_panel(int left, int top, int width, int height, char* title, char* bottom);

void draw_button(int left, int top, int width, const char* txt, bool selected);

void draw_fn_btn(fn_1_12_tbl_rec_t* prec, int left, int top);

void draw_cmd_line(int left, int top, char* cmd);

void draw_label(int left, int top, int width, char* txt, bool selected, bool highlighted);

typedef struct line {
   int8_t off;
   char* txt;
} line_t;

typedef struct lines {
   uint8_t sz;
   uint8_t toff;
   line_t* plns;
} lines_t;

void draw_box(int left, int top, int width, int height, const char* title, const lines_t* plines);
void draw_box_ex(int left, int top, int width, int height, const char* title, const char* bottom, const lines_t* plines);

int draw_selector(int left, int top, int width, int height, const char* title, const lines_t* plines, int selected_line, const char* footer);

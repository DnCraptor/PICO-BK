#ifndef MENU_H
#define MENU_H


#include "ovl.h"
#include "ets.h"

#define MENU_FLAG_START_UI  0x01
#define MENU_FLAG_LOAD_FILE 0x02

#define menu_fileman_ovln OVL_NUM_UI
#define menu_fileman_ovls OVL_SEC_UI
int_fast16_t menu_fileman (uint_fast8_t Flags);

#define menu_ovln OVL_NUM_UI
#define menu_ovls OVL_SEC_UI
void menu(uint_fast8_t Flags);


#endif

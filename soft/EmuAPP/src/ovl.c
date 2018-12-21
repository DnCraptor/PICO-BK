#include <stdarg.h>
#include "ovl.h"
#include "ets.h"

#include "ffs.h"
#include "ui.h"
#include "tape.h"
#include "menu.h"
#include "CPU.h"
#include "main.h"
#include "emu.h"

extern char _ovl_start, _ovl_end;
extern char __load_start_ovl0text, __load_stop_ovl0text, __load_size_ovl0text;
extern char __load_start_ovl1text, __load_stop_ovl1text, __load_size_ovl1text;
extern char __load_start_ovl2text, __load_stop_ovl2text, __load_size_ovl2text;
extern char __load_start_ovl3text, __load_stop_ovl3text, __load_size_ovl3text;
extern char __load_start_ovl4text, __load_stop_ovl4text, __load_size_ovl4text;
extern char __load_start_ovl5text, __load_stop_ovl5text, __load_size_ovl5text;
extern char __load_start_ovl6text, __load_stop_ovl6text, __load_size_ovl6text;
extern char __load_start_ovl7text, __load_stop_ovl7text, __load_size_ovl7text;
extern char __load_start_ovl8text, __load_stop_ovl8text, __load_size_ovl8text;
extern char __load_start_ovl9text, __load_stop_ovl9text, __load_size_ovl9text;

#define OVL_SECTION_SET( N) {(uint32_t) &__load_start_ovl##N##text - 0x40200000UL, (uint32_t) &__load_size_ovl##N##text}
#define OVL_NUM_MAX 10

const Tovl_section ovl_sections [OVL_NUM_MAX] =
{
    OVL_SECTION_SET (0),
    OVL_SECTION_SET (1),
    OVL_SECTION_SET (2),
    OVL_SECTION_SET (3),
    OVL_SECTION_SET (4),
    OVL_SECTION_SET (5),
    OVL_SECTION_SET (6),
    OVL_SECTION_SET (7),
    OVL_SECTION_SET (8),
    OVL_SECTION_SET (9)
};

typedef int (*Tovl_pFunc) (int V1, int V2, int V3, int V4, int V5);

#define OVL_FUNC_DESC( FuncName) (Tovl_pFunc) FuncName,
const Tovl_pFunc ovl_FuncTab [OVL_FUNC_TAB_SIZE] =
{
    #include "ovl_FuncDesc.h"
};
#undef OVL_FUNC_DESC

#define OVL_FUNC_DESC( FuncName) FuncName##_ovln,
const uint8_t ovl_FuncSecTab [OVL_FUNC_TAB_SIZE] =
{
    #include "ovl_FuncDesc.h"
};
#undef OVL_FUNC_DESC

uint8_t ovl_iCurN = 0xFF;

void ovl_LoadSec (int OvlN)
{
    SPIRead (ovl_sections [OvlN].Addr, &_ovl_start, ovl_sections [OvlN].Size);
    ovl_iCurN = OvlN;
    asm volatile ("isync" : : : "memory");
}

int ovl_call (int FuncN, ...)
{
    uint_fast8_t iPrevN = ovl_iCurN;
    va_list      args;
    int          Ret;

    {
        int iNewN  = ovl_FuncSecTab [FuncN];
        if (iPrevN != iNewN) ovl_LoadSec (iNewN);
        else                 iPrevN = 0xFF;
    }

    va_start (args, FuncN);

    Ret = ovl_FuncTab [FuncN] (va_arg (args, int), va_arg (args, int), va_arg (args, int), va_arg (args, int), va_arg (args, int));

    if (iPrevN < OVL_NUM_MAX) ovl_LoadSec (iPrevN);

    return Ret;
}

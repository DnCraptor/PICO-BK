#ifndef OVL_H
#define OVL_H

#include <stdint.h>

enum
{
    OVL_MODE_EMU,
    OVL_MODE_FFS,
    OVL_MODE_UI,
    OVL_MODE_INIT
};

#define OVL_FUNC_DESC( FuncName, Mode) ovl_FuncId_##FuncName,
enum
{
    #include "ovl_FuncDesc.h"
    OVL_FUNC_TAB_SIZE
};
#undef OVL_FUNC_DESC


void ovl_SwitchToMode (uint_fast8_t Mode);
int  ovl_Call         (int          iFunc, ...);

#define OVL_CALL0( FuncName)          ovl_Call (ovl_FuncId_##FuncName)
#define OVL_CALL(  FuncName, Args...) ovl_Call (ovl_FuncId_##FuncName, Args)

#endif

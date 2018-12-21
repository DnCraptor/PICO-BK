#ifndef OVL_H
#define OVL_H

#include <stdint.h>

#define OVL_NUM_0 0
#define OVL_SEC_0 __attribute__((section(".ovl0.text")))

#define OVL_NUM_1 1
#define OVL_SEC_1 __attribute__((section(".ovl1.text")))

#define OVL_NUM_2 2
#define OVL_SEC_2 __attribute__((section(".ovl2.text")))

#define OVL_NUM_3 3
#define OVL_SEC_3 __attribute__((section(".ovl3.text")))

#define OVL_NUM_4 4
#define OVL_SEC_4 __attribute__((section(".ovl4.text")))

#define OVL_NUM_5 5
#define OVL_SEC_5 __attribute__((section(".ovl5.text")))

#define OVL_NUM_6 6
#define OVL_SEC_6 __attribute__((section(".ovl6.text")))

#define OVL_NUM_7 7
#define OVL_SEC_7 __attribute__((section(".ovl7.text")))

#define OVL_NUM_8 8
#define OVL_SEC_8 __attribute__((section(".ovl8.text")))

#define OVL_NUM_9 9
#define OVL_SEC_9 __attribute__((section(".ovl9.text")))

#define OVL_SEC( Name) Name##_ovls
#define OVL_NUM( Name) Name##_ovln

#define OVL_NUM_INIT OVL_NUM_0
#define OVL_SEC_INIT OVL_SEC_0
#define OVL_NUM_EMU  OVL_NUM_1
#define OVL_SEC_EMU  OVL_SEC_1
#define OVL_NUM_UI   OVL_NUM_2
#define OVL_SEC_UI   OVL_SEC_2
#define OVL_NUM_FS   OVL_NUM_3
#define OVL_SEC_FS   OVL_SEC_3

#define OVL_FUNC_DESC( FuncName) FuncName##_ovlid,
enum
{
    #include "ovl_FuncDesc.h"
    OVL_FUNC_TAB_SIZE
};
#undef OVL_FUNC_DESC

typedef struct
{
    uint32_t Addr;
    uint32_t Size;

} Tovl_section;

const Tovl_section ovl_sections [10];

extern uint8_t ovl_iCurN;

void ovl_LoadSec (int OvlN);

int ovl_call (int FuncN, ...);
#define OVL_CALL(  OvlN, Func, Args...) ((OvlN == Func##_ovln) ? (int) Func (Args) : ovl_call (Func##_ovlid, Args))
#define OVL_CALL0( OvlN, Func)          ((OvlN == Func##_ovln) ? (int) Func (    ) : ovl_call (Func##_ovlid      ))

#define OVL_CALLV(  OvlN, Func, Args...) ((OvlN == Func##_ovln) ? Func (Args) : (void) ovl_call (Func##_ovlid, Args))
#define OVL_CALLV0( OvlN, Func)          ((OvlN == Func##_ovln) ? Func (    ) : (void) ovl_call (Func##_ovlid      ))

#endif

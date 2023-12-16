#include <stdarg.h>
#include "AnyMem.h"
#include "ets.h"
#include "spi_flash.h"
#include "ovl.h"

#include "Debug.h"
/*
extern char _ovl0_start, _ovl0_end;
extern char _ovl1_start, _ovl1_end;
extern char _ovl2_start, _ovl2_end;
extern char _ovl3_start, _ovl3_end;

extern char __load_start_ovl0_eftext, __load_stop_ovl0_eftext, __load_size_ovl0_eftext;
extern char __load_start_ovl0_utext,  __load_stop_ovl0_utext,  __load_size_ovl0_utext;
extern char __load_start_ovl0_itext,  __load_stop_ovl0_itext,  __load_size_ovl0_itext;

extern char __load_start_ovl1_eutext, __load_stop_ovl1_eutext, __load_size_ovl1_eutext;
extern char __load_start_ovl1_ftext,  __load_stop_ovl1_ftext,  __load_size_ovl1_ftext;
extern char __load_start_ovl1_itext,  __load_stop_ovl1_itext,  __load_size_ovl1_itext;

extern char __load_start_ovl2_futext, __load_stop_ovl2_futext, __load_size_ovl2_futext;
extern char __load_start_ovl2_etext,  __load_stop_ovl2_etext,  __load_size_ovl2_etext;
extern char __load_start_ovl2_itext,  __load_stop_ovl2_itext,  __load_size_ovl2_itext;

extern char __load_start_ovl3_etext,  __load_stop_ovl3_etext,  __load_size_ovl3_etext;
extern char __load_start_ovl3_ftext,  __load_stop_ovl3_ftext,  __load_size_ovl3_ftext;
extern char __load_start_ovl3_utext,  __load_stop_ovl3_utext,  __load_size_ovl3_utext;
extern char __load_start_ovl3_itext,  __load_stop_ovl3_itext,  __load_size_ovl3_itext;

enum
{
    ovl_sec_id_e = 1,
    ovl_sec_id_f,
    ovl_sec_id_u,
    ovl_sec_id_i,
    ovl_sec_id_ef,
    ovl_sec_id_eu,
    ovl_sec_id_fu
};

#define OVL_SECTION_SET( Ovl, Sec) {(uint32_t) &__load_size_##Ovl##_##Sec##text, (uint32_t) ovl_sec_id_##Sec, (uint32_t) &__load_start_##Ovl##_##Sec##text - 0x40200000UL, (uint32_t) &_##Ovl##_start}

AT_IROM const struct
{
    uint32_t Size;
    uint8_t  Id;
    uint32_t SrcAddr;
    uint32_t DstAddr;

} ovl_mode_sections [] =
{
    OVL_SECTION_SET (ovl0, ef),
    OVL_SECTION_SET (ovl1, eu),
    OVL_SECTION_SET (ovl2, e),
    OVL_SECTION_SET (ovl3, e),

    OVL_SECTION_SET (ovl0, ef),
    OVL_SECTION_SET (ovl1, f),
    OVL_SECTION_SET (ovl2, fu),
    OVL_SECTION_SET (ovl3, f),

    OVL_SECTION_SET (ovl0, u),
    OVL_SECTION_SET (ovl1, eu),
    OVL_SECTION_SET (ovl2, fu),
    OVL_SECTION_SET (ovl3, u),

    OVL_SECTION_SET (ovl0, i),
    OVL_SECTION_SET (ovl1, i),
    OVL_SECTION_SET (ovl2, i),
    OVL_SECTION_SET (ovl3, i)
};

static uint8_t ovl_CurMode;
static uint8_t ovl_CurSecId [4];

void ovl_SwitchToMode (uint_fast8_t Mode)
{
    uint_fast8_t Count;

    ovl_CurMode = (uint8_t) Mode;

    Mode &= 3;
    Mode *= 4;

    for (Count = 0; Count < 4; Count++)
    {
        uint32_t     Size;
        uint_fast8_t Id;

        Size = AnyMem_r_u32 (&ovl_mode_sections [Mode + Count].Size);
        Id   = AnyMem_r_u8  (&ovl_mode_sections [Mode + Count].Id);

        if (Size && (ovl_CurSecId [Count] != Id))
        {
            ovl_CurSecId [Count] = (uint8_t) Id;
            spi_flash_Read (AnyMem_r_u32 (&ovl_mode_sections [Mode + Count].SrcAddr), (uint32_t *) AnyMem_r_u32 (&ovl_mode_sections [Mode + Count].DstAddr), Size);
        }
    }
}

typedef int (*Tovl_pFunc) (int V1, int V2, int V3, int V4, int V5);

#include "Overlays\Ffs\ffs_f.h"
#include "Overlays\Ffs\tape_f.h"
#include "Overlays\Ui\fileman_u.h"
#include "Overlays\Ui\menu_u.h"

#define OVL_FUNC_DESC( FuncName, Mode) {Mode, (Tovl_pFunc) FuncName},
AT_IROM const struct
{
    uint32_t    Mode;
    Tovl_pFunc  pFunc;

} ovl_FuncTab [OVL_FUNC_TAB_SIZE] =
{
    #include "ovl_FuncDesc.h"
};
#undef OVL_FUNC_DESC

int ovl_Call (int FuncN, ...)
{
    uint_fast8_t PrevMode = ovl_CurMode;
    va_list      args;
    int          Ret;
    Tovl_pFunc   pFunc = (Tovl_pFunc) AnyMem_r_u32 ((uint32_t *) &ovl_FuncTab [FuncN].pFunc);

    ovl_SwitchToMode ((uint_fast8_t) AnyMem_r_u32 (&ovl_FuncTab [FuncN].Mode));

    va_start (args, FuncN);

    Ret = pFunc (va_arg (args, int), va_arg (args, int), va_arg (args, int), va_arg (args, int), va_arg (args, int));

    ovl_SwitchToMode (PrevMode);

    return Ret;
}
*/
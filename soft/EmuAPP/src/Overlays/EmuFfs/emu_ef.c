#include <stdint.h>
#include "tv.h"
#include "CPU.h"

#include "emu_ef.h"

#define AT_OVL __attribute__((section(".ovl0_ef.text")))

struct
{
    uint32_t TvStartMaxLine;

} Emu_OvlData;

static void AT_OVL emu_NewTvFrame (void)
{
    uint_fast16_t Reg          = Device_Data.SysRegs.Reg177664;
    uint_fast16_t StartMaxLine = (Reg - 0330) & 0xFF;

    if (Reg & 01000) StartMaxLine |= 256UL << 8;
    else             StartMaxLine |=  64UL << 8;

    Emu_OvlData.TvStartMaxLine = (uint32_t) StartMaxLine;
}

static int AT_OVL emu_tv (uint32_t *pBuf, uint_fast16_t Line)
{
    uint_fast8_t  Count;
    uint32_t     *pVRam;
    uint_fast32_t StartMaxLine;

    if (Line == 0)
    {
        emu_NewTvFrame ();

        return 0;
    }

    if (Line <= 30) return 0;

    Line -= 30;

    StartMaxLine = Emu_OvlData.TvStartMaxLine;

    if (Line >= (StartMaxLine >> 8)) return 0;

    pBuf += 5;

    // Рисуем строку

    pVRam = &CPU_PAGE1_MEM32 [((Line + StartMaxLine) & 0xFF) * 16];

    for (Count = 0; Count < 16; Count++)
    {
        uint_fast32_t U32;

        U32 = *pVRam++;

        U32 = (U32 & 0x55555555UL) << 1 | ((U32 >> 1) & 0x55555555UL);
        U32 = (U32 & 0x33333333UL) << 2 | ((U32 >> 2) & 0x33333333UL);
        U32 = (U32 & 0x0F0F0F0FUL) << 4 | ((U32 >> 4) & 0x0F0F0F0FUL);
        U32 = (U32 << 24) | ((U32 & 0xFF00U) << 8) | ((U32 >> 8) & 0xFF00U) | (U32 >> 24);

        *pBuf++ = (uint32_t) U32;
    }

    return 1;
}

void AT_OVL emu_OnTv (void)
{
    emu_NewTvFrame ();
    tv_SetOutFunc (emu_tv);
}

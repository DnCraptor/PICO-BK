#include "AnyMem.h"
#include "CPU.h"
#include "ovl.h"
#include "menu.h"
#include "ffs.h"

#include "tape_f.h"
#include "ffs_f.h"

#include "../EmuFfs/CPU_ef.h"
#include "../EmuFfs/emu_ef.h"
#include "../FfsUi/ffs_fu.h"

#define AT_OVL __attribute__((section(".ovl3_f.text")))

union
{
    uint32_t U32 [64];
    uint16_t U16 [64 * 2];
    uint8_t  U8  [64 * 4];

} TapeBuf;

void AT_OVL tape_ReadOp (void)
{
    uint_fast16_t ParamAdr = CPU_PAGE0_MEM16 [0306 >> 1];
    uint_fast16_t Addr     = 01000;
    uint_fast16_t Size     = 01000;
    int           iFile;
    uint_fast16_t Offset;
    uint_fast32_t CheckSum;
    uint_fast16_t Count;
    TCPU_Arg      Arg;

    CPU_PAGE0_MEM16 [0304 >> 1] = 1;

    for (Count = 0; Count < 16; Count++)
    {
        Arg = CPU_ReadMemB ((ParamAdr + 6 + Count) & 0xFFFFU); CPU_CHECK_ARG_FAULT (Arg);
        TapeBuf.U8 [Count] = (uint8_t) Arg;
    }

    ffs_cvt_name ((char *) &TapeBuf.U8 [0], (char *) &TapeBuf.U8 [16]);

    if (TapeBuf.U8 [16] == 0)
    {
	    emu_OffTv ();
        iFile = OVL_CALL (menu_fileman, MENU_FLAG_START_UI | MENU_FLAG_LOAD_FILE);
	    emu_OnTv  ();

        if (iFile < 0) goto BusFault;

        AnyMem_memcpy (&TapeBuf.U8 [0], ffs_name ((uint16_t) iFile), 16);
        for (Count = 0; (Count < 16) && (TapeBuf.U8 [Count] != 0); Count++);
        for (;          (Count < 16);                              Count++) TapeBuf.U8 [Count] = ' ';
    }
    else
    {
        iFile = ffs_find ((char *) &TapeBuf.U8 [16]);
    }

    if (iFile < 0)
    {
        CPU_PAGE0_MEM8 [0301] = 2;
    }
    else if ((ffs_GetFile (iFile), ffs_File.size) < 4)
    {
        CPU_PAGE0_MEM8 [0301] = 2;
    }
    else
    {
        ffs_read ((uint16_t) iFile, 0, (uint8_t *) &TapeBuf.U16 [8], 4);
        Addr = TapeBuf.U16 [8];
        Size = TapeBuf.U16 [9];

        if ((uint_fast32_t) Size + 4 > (ffs_GetFile (iFile), ffs_File.size))
        {
            CPU_PAGE0_MEM8 [0301] = 2;
        }
    }

    Arg = CPU_WriteMemW ((ParamAdr + 22) & 0xFFFFU, Addr); CPU_CHECK_ARG_FAULT (Arg);
    Arg = CPU_WriteMemW ((ParamAdr + 24) & 0xFFFFU, Size); CPU_CHECK_ARG_FAULT (Arg);

    for (Count = 0; Count < 16; Count++)
    {
        Arg = CPU_WriteMemB ((ParamAdr + 26 + Count) & 0xFFFFU, TapeBuf.U8 [Count]); CPU_CHECK_ARG_FAULT (Arg);
    }

    if (CPU_PAGE0_MEM8 [0301]) goto Exit;

                   Arg = CPU_ReadMemW ((ParamAdr + 24) & 0xFFFFU); CPU_CHECK_ARG_FAULT (Arg);   CPU_PAGE0_MEM16 [0266 >> 1] = Arg;
                   Arg = CPU_ReadMemW ((ParamAdr +  2) & 0xFFFFU); CPU_CHECK_ARG_FAULT (Arg);
    if (Arg == 0) {Arg = CPU_ReadMemW ((ParamAdr + 22) & 0xFFFFU); CPU_CHECK_ARG_FAULT (Arg);}  CPU_PAGE0_MEM16 [0264 >> 1] = Arg;

    Addr     = CPU_PAGE0_MEM16 [0264 >> 1];
    Size     = CPU_PAGE0_MEM16 [0266 >> 1];
    CheckSum = 0;
    Offset   = 4;

    while (Size)
    {
        uint_fast16_t TempSize;

        TempSize = Size;
        if (TempSize > sizeof (TapeBuf) / sizeof (uint8_t)) TempSize = sizeof (TapeBuf) / sizeof (uint8_t);

        ffs_read ((uint16_t) iFile, Offset, &TapeBuf.U8 [0], sizeof (TapeBuf));

        for (Count = 0; Count < TempSize; Count++)
        {
            CheckSum += (uint_fast32_t) TapeBuf.U8 [Count];
            Arg = CPU_WriteMemB (Addr, TapeBuf.U8 [Count]); CPU_CHECK_ARG_FAULT (Arg);
            Addr = (Addr + 1) & 0xFFFFU;
        }

        Offset += TempSize;
        Size   -= TempSize;
    }

    CheckSum = (CheckSum & 0xFFFFU) + (CheckSum >> 16);
    CheckSum = (CheckSum & 0xFFFFU) + (CheckSum >> 16);
    CPU_PAGE0_MEM16 [0312 >> 1] = (uint16_t) CheckSum;
    Device_Data.CPU_State.r [5] = Addr;

Exit:

    Device_Data.CPU_State.r [7] = 0116232;
    return;

BusFault:

    Device_Data.CPU_State.r [7] = CPU_PAGE0_MEM16 [04 >> 1];
    Device_Data.CPU_State.psw   = CPU_PAGE0_MEM16 [06 >> 1];
}

void AT_OVL tape_WriteOp (void)
{
    uint_fast16_t ParamAdr = Device_Data.CPU_State.r [1];
    int           iFile    = -1;
    uint_fast32_t CheckSum;
    TCPU_Arg      Arg;
    uint_fast16_t Addr;
    uint_fast16_t Size;
    uint_fast16_t offs;
    uint_fast16_t Count;
    uint_fast8_t  FirstOffs;

    Arg = CPU_ReadMemW ((ParamAdr + 2) & 0xFFFFU); CPU_CHECK_ARG_FAULT (Arg); Addr = Arg;
    Arg = CPU_ReadMemW ((ParamAdr + 4) & 0xFFFFU); CPU_CHECK_ARG_FAULT (Arg); Size = Arg;

    for (Count = 0; Count < 16; Count++)
    {
        Arg = CPU_ReadMemB ((ParamAdr + 6 + Count) & 0xFFFFU); CPU_CHECK_ARG_FAULT (Arg);
        TapeBuf.U8 [Count] = (uint8_t) Arg;
    }

    ffs_cvt_name ((char *) &TapeBuf.U8 [0], (char *) &TapeBuf.U8 [0]);
    
    if (ffs_find ((char *) &TapeBuf.U8 [0]) >= 0) goto BusFault; // Файл уже существует

    // Создаем файл
    iFile = ffs_create ((char *) &TapeBuf.U8 [0], TYPE_TAPE, Size + sizeof (uint32_t));

    if (iFile < 0) goto BusFault; // Файл не создался

    TapeBuf.U16 [0] = Addr;
    TapeBuf.U16 [1] = Size;
    FirstOffs       = 4;
    offs            = 0;
    CheckSum        = 0;

    while (Size)
    {
        uint_fast16_t RdSize;
        uint_fast16_t WrSize;
        uint8_t       *pBuf;

        pBuf   = &TapeBuf.U8 [FirstOffs];
        RdSize = sizeof (TapeBuf) / sizeof (uint8_t) - FirstOffs;
        if (RdSize > Size) RdSize = Size;
        WrSize = (RdSize + FirstOffs + 3) & ~3U;
        FirstOffs = 0;

        for (Count = 0; Count < RdSize; Count++)
        {
            Arg       = CPU_ReadMemB (Addr); CPU_CHECK_ARG_FAULT (Arg);
            *pBuf++   = Arg;
            CheckSum += Arg;
            Addr      = (Addr + 1) & 0xFFFFU;
        }

        ffs_writeData (iFile, offs, &TapeBuf.U8 [0], WrSize);

        offs += WrSize;
        Size -= RdSize;
    }

    CheckSum = (CheckSum & 0xFFFFU) + (CheckSum >> 16);
    CheckSum = (CheckSum & 0xFFFFU) + (CheckSum >> 16);
    CPU_PAGE0_MEM16 [0312 >> 1] = (uint16_t) CheckSum;

    Device_Data.CPU_State.r [7] = 0116232;
    return;

BusFault:

    if (iFile >= 0) ffs_remove (iFile);

    Device_Data.CPU_State.r [7] = CPU_PAGE0_MEM16 [04 >> 1];
    Device_Data.CPU_State.psw   = CPU_PAGE0_MEM16 [06 >> 1];
}

#undef CPU_CHECK_ARG_FAULT

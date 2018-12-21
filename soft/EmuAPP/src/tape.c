#include "tape.h"
#include "menu.h"
#include "ffs.h"
#include "CPU.h"
#include "ui.h"

union
{
    uint32_t U32 [64];
    uint16_t U16 [64 * 2];
    uint8_t  U8  [64 * 4];

} TapeBuf;

#define CPU_CHECK_ARG_FAULT( Arg) {if (CPU_IS_ARG_FAULT (Arg)) goto BusFault;}

void OVL_SEC (tape_ReadOp) tape_ReadOp (void)
{
    uint_fast16_t ParamAdr = MEM16 [0306 >> 1];
    uint_fast16_t Addr     = 01000;
    uint_fast16_t Size     = 01000;
    int           iFile;
    uint_fast16_t Offset;
    uint_fast32_t CheckSum;
    uint_fast16_t Count;
    TCPU_Arg      Arg;

    MEM16 [0304 >> 1] = 1;

    Arg = OVL_CALL (OVL_NUM (tape_ReadOp), CPU_ReadMemBuf, &TapeBuf.U8 [0], (ParamAdr + 6) & 0xFFFFU, 16); CPU_CHECK_ARG_FAULT (Arg);

    ffs_cvt_name ((char *) &TapeBuf.U8 [0], (char *) &TapeBuf.U8 [16]);

    if (TapeBuf.U8 [16] == 0)
    {
        iFile = OVL_CALL (OVL_NUM (tape_ReadOp), menu_fileman, MENU_FLAG_START_UI | MENU_FLAG_LOAD_FILE);

        if (iFile < 0) goto BusFault;

        ets_memcpy (&TapeBuf.U8 [0], ffs_name ((uint16_t) iFile), 16);
        for (Count = 0; (Count < 16) && (TapeBuf.U8 [Count] != 0); Count++);
        for (;          (Count < 16);                              Count++) TapeBuf.U8 [Count] = ' ';
    }
    else
    {
        iFile = ffs_find ((char *) &TapeBuf.U8 [16]);
    }

    if (iFile < 0)
    {
//      ets_memcpy (&TapeBuf.U8 [0], "FILE NOT FOUND  ", 16);
        MEM8 [0301] = 2;
    }
    else if (fat [iFile].size < 4)
    {
        MEM8 [0301] = 2;
    }
    else
    {
        ffs_read ((uint16_t) iFile, 0, (uint8_t *) &TapeBuf.U16 [8], 4);
        Addr = TapeBuf.U16 [8];
        Size = TapeBuf.U16 [9];

        if ((uint_fast32_t) Size + 4 > fat [iFile].size)
        {
            MEM8 [0301] = 2;
        }
    }

    Arg = OVL_CALL (OVL_NUM (tape_ReadOp), CPU_WriteMemW, (ParamAdr + 22) & 0xFFFFU, Addr); CPU_CHECK_ARG_FAULT (Arg);
    Arg = OVL_CALL (OVL_NUM (tape_ReadOp), CPU_WriteMemW, (ParamAdr + 24) & 0xFFFFU, Size); CPU_CHECK_ARG_FAULT (Arg);

    Arg = OVL_CALL (OVL_NUM (tape_ReadOp), CPU_WriteMemBuf, &TapeBuf.U8 [0], (ParamAdr + 26) & 0xFFFFU, 16); CPU_CHECK_ARG_FAULT (Arg);

    if (MEM8 [0301]) goto Exit;

                   Arg = OVL_CALL (OVL_NUM (tape_ReadOp), CPU_ReadMemW, (ParamAdr + 24) & 0xFFFFU); CPU_CHECK_ARG_FAULT (Arg);   MEM16 [0266 >> 1] = Arg;
                   Arg = OVL_CALL (OVL_NUM (tape_ReadOp), CPU_ReadMemW, (ParamAdr +  2) & 0xFFFFU); CPU_CHECK_ARG_FAULT (Arg);
    if (Arg == 0) {Arg = OVL_CALL (OVL_NUM (tape_ReadOp), CPU_ReadMemW, (ParamAdr + 22) & 0xFFFFU); CPU_CHECK_ARG_FAULT (Arg);}  MEM16 [0264 >> 1] = Arg;

    Addr     = MEM16 [0264 >> 1];
    Size     = MEM16 [0266 >> 1];
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
        }

        Arg = OVL_CALL (OVL_NUM (tape_ReadOp), CPU_WriteMemBuf, &TapeBuf.U8 [0], Addr, TempSize);
        Addr = Arg & 0xFFFFU;
        CPU_CHECK_ARG_FAULT (Arg);

        Offset += TempSize;
        Size   -= TempSize;
    }

    CheckSum = (CheckSum & 0xFFFFU) + (CheckSum >> 16);
    CheckSum = (CheckSum & 0xFFFFU) + (CheckSum >> 16);
    MEM16 [0312 >> 1] = (uint16_t) CheckSum;
    Device_Data.CPU_State.r [5] = Addr;

Exit:

    Device_Data.CPU_State.r [7] = 0116232;
    Wait_SPI_Idle (flashchip);
    return;

BusFault:

    Device_Data.CPU_State.r [7] = MEM16 [04 >> 1];
    Device_Data.CPU_State.psw   = MEM16 [06 >> 1];
    Wait_SPI_Idle (flashchip);
}

void OVL_SEC (tape_WriteOp) tape_WriteOp (void)
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

    Arg = OVL_CALL (OVL_NUM (tape_WriteOp), CPU_ReadMemW, ParamAdr + 2); CPU_CHECK_ARG_FAULT (Arg); Addr = Arg;
    Arg = OVL_CALL (OVL_NUM (tape_WriteOp), CPU_ReadMemW, ParamAdr + 4); CPU_CHECK_ARG_FAULT (Arg); Size = Arg;
    Arg = OVL_CALL (OVL_NUM (tape_WriteOp), CPU_ReadMemBuf, &TapeBuf.U8 [0], (ParamAdr + 6) & 0xFFFFU, 16); CPU_CHECK_ARG_FAULT (Arg);

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

        Arg = OVL_CALL (OVL_NUM (tape_WriteOp), CPU_ReadMemBuf, pBuf, Addr, RdSize);
        Addr = Arg & 0xFFFFU;
        CPU_CHECK_ARG_FAULT (Arg);
        for (Count = 0; Count < RdSize; Count++) CheckSum += *pBuf++;

        ffs_writeData (iFile, offs, &TapeBuf.U8 [0], WrSize);

        offs += WrSize;
        Size -= RdSize;
    }

    CheckSum = (CheckSum & 0xFFFFU) + (CheckSum >> 16);
    CheckSum = (CheckSum & 0xFFFFU) + (CheckSum >> 16);
    MEM16 [0312 >> 1] = (uint16_t) CheckSum;

    Device_Data.CPU_State.r [7] = 0116232;
    Wait_SPI_Idle (flashchip);
    return;

BusFault:

    if (iFile >= 0) ffs_remove (iFile);

    Device_Data.CPU_State.r [7] = MEM16 [04 >> 1];
    Device_Data.CPU_State.psw   = MEM16 [06 >> 1];
    Wait_SPI_Idle (flashchip);
}

#undef CPU_CHECK_ARG_FAULT

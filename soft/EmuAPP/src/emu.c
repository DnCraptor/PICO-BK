#include "ets.h"
#include "emu.h"
#include "tv.h"
#include "ps2.h"
//#include "keymap.h"
//#include "tape.h"
#include "timer0.h"
#include "ui.h"
#include "menu.h"
#include "ffs.h"
//#include "zkg.h"
#include "reboot.h"
//#include "help.h"
#include "str.h"
#include "board.h"
#include "CPU.h"
#include "Key.h"
#include "tape.h"
#include "esp8266.h"
#include "pin_mux_register.h"

void OVL_SEC (emu_start) emu_start (void)
{
    int_fast32_t  Time;
    uint_fast32_t T            = 0;
    uint_fast16_t CodeAndFlags;
    uint_fast16_t Key;
    uint_fast32_t LastKey      = 0xC00;
    uint_fast8_t  RunState     = 0;

    Time = getCycleCount ();

    // Запускаем эмуляцию
    while (1)
    {
        uint_fast8_t  Count;

        if (Key_Flags & KEY_FLAGS_TURBO)
        {
            for (Count = 0; Count < 16; Count++)
            {
                CPU_RunInstruction ();

                if (Device_Data.CPU_State.r [7] == 0116142) // 0116076
                {
                    uint_fast16_t Cmd = Device_Data.CPU_State.r [0];

                    if      (Cmd == 3) OVL_CALLV0 (OVL_NUM (emu_start), tape_ReadOp);
                    else if (Cmd == 2) OVL_CALLV0 (OVL_NUM (emu_start), tape_WriteOp);
                }
            }

            Time = getCycleCount ();
            T    = Device_Data.CPU_State.Time;
        }
        else
        {
            uint_fast32_t NewT;

            for (Count = 0; Count < 16; Count++)
            {
                if ((int32_t) (Time - getCycleCount ()) > 0) break;

                CPU_RunInstruction ();

                NewT  = Device_Data.CPU_State.Time;
                Time += (uint32_t) (NewT - T) * 53;
                T     = NewT;

                if (Device_Data.CPU_State.r [7] == 0116142) // 0116076
                {
                    uint_fast16_t Cmd = Device_Data.CPU_State.r [0];

                    if      (Cmd == 3) {OVL_CALLV0 (OVL_NUM (emu_start), tape_ReadOp);  Time = getCycleCount (); T = Device_Data.CPU_State.Time;}
                    else if (Cmd == 2) {OVL_CALLV0 (OVL_NUM (emu_start), tape_WriteOp); Time = getCycleCount (); T = Device_Data.CPU_State.Time;}
                }
            }

            NewT = getCycleCount ();
            if ((int32_t) (NewT - Time) > 0x10000L) Time = NewT - 0x10000L;
        }

        // Вся периодика

        switch (RunState++)
        {
            default:
            case 0:
                CPU_TimerRun ();
                RunState = 1;
                break;

            case 1:
                ps2_periodic ();
                break;

            case 2:
                CodeAndFlags = ps2_read ();
                if (CodeAndFlags == 0) RunState = 5;
                break;

            case 3:
                Key = Key_Translate (CodeAndFlags);
                break;

            case 4:
                ps2_leds ((Key_Flags >> KEY_FLAGS_CAPSLOCK_POS) & 1, (Key_Flags >> KEY_FLAGS_NUMLOCK_POS) & 1, (Key_Flags >> KEY_FLAGS_TURBO_POS) & 1);

                if (Key_Flags & KEY_FLAGS_NUMLOCK) Device_Data.SysRegs.RdReg177714 = (uint16_t) (Key_Flags >> KEY_FLAGS_UP_POS);
                else                               Device_Data.SysRegs.RdReg177714 = 0;

                if (CodeAndFlags & 0x8000U)
                {
                    if (((LastKey ^ CodeAndFlags) & 0x7FF) == 0)
                    {
                        Device_Data.SysRegs.RdReg177716 |= 0100;

                        LastKey = 0xC00;
                    }
                }
                else if (Key != KEY_UNKNOWN)
                {
                    if (Key == KEY_MENU_ESC)
                    {
                        OVL_CALLV (OVL_NUM (emu_start), menu, MENU_FLAG_START_UI);

                        Wait_SPI_Idle (flashchip);

                        Time = getCycleCount ();
                        T    = Device_Data.CPU_State.Time;
                    }
                    else
                    {
                        LastKey  = ((uint_fast32_t) Key << 16) | CodeAndFlags;

                        RunState = 6;
                    }
                }
                break;

            case 6:

                Device_Data.SysRegs.RdReg177716 &= ~0100;

            case 5:

                if ((LastKey & 0x800) == 0)
                {
                    if ((Device_Data.SysRegs.Reg177660 & 0200) == 0)
                    {
                        Key = (uint_fast16_t) (LastKey >> 16);

                        if      (Key == 14) Key_SetRusLat ();
                        else if (Key == 15) Key_ClrRusLat ();

                        if ((Device_Data.SysRegs.Reg177660 & 0100) == 0)
                        {
                            if (Key & KEY_AR2_PRESSED)
                            {
                                Device_Data.CPU_State.Flags &= ~CPU_FLAG_KEY_VECTOR_60;
                                Device_Data.CPU_State.Flags |=  CPU_FLAG_KEY_VECTOR_274;
                            }
                            else
                            {
                                Device_Data.CPU_State.Flags &= ~CPU_FLAG_KEY_VECTOR_274;
                                Device_Data.CPU_State.Flags |=  CPU_FLAG_KEY_VECTOR_60;
                            }
                        }

                        Device_Data.SysRegs.Reg177660 |= 0200;
                    }

                    LastKey |= 0x800;
                    Device_Data.SysRegs.Reg177662 = (uint16_t) (LastKey >> 16) & 0177;
                }

                RunState = 0;

                break;
        }
    }
}

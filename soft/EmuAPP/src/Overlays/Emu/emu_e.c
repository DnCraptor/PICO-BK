#include "ets.h"
#include "tv.h"
#include "timer0.h"
#include "reboot.h"
#include "esp8266.h"
#include "pin_mux_register.h"
#include "ovl.h"
#include "ps2_bk.h"
#include "ps2_codes.h"
#include "CPU.h"
#include "Key.h"

#include "emu_e.h"
#include "CPU_e.h"

#include "../EmuUi/Key_eu.h"
#include "../EmuUi/ps2_eu.h"

#include "Debug.h"

#define AT_OVL __attribute__((section(".ovl3_e.text")))

uint32_t ps2get_raw_code(); // TODO:

void AT_OVL emu_start (void) {
    uint64_t      cycles_cnt1  = getCycleCount ();
    int_fast32_t  Time         = (int32_t)cycles_cnt1;
    uint_fast32_t T            = 0;
    uint_fast16_t CodeAndFlags;
    uint_fast16_t Key;
    uint_fast32_t LastKey      = 0xC00;
    uint_fast8_t  RunState     = 0;
    DEBUG_PRINT(("Init Time: %d", Time));
    // Запускаем эмуляцию
    while (1) {
        if_manager(false);
        uint_fast8_t  Count;
        DEBUG_PRINT(("Key_Flags: %08Xh; (Key_Flags & KEY_FLAGS_TURBO): %d", Key_Flags, (Key_Flags & KEY_FLAGS_TURBO)));
        if (Key_Flags & KEY_FLAGS_TURBO) {
            for (Count = 0; Count < 16; Count++) {
                DEBUG_PRINT(("Count: %d", Count));
                CPU_RunInstruction ();
            }
            Time = getCycleCount ();
            T    = Device_Data.CPU_State.Time;
            DEBUG_PRINT(("Time: %d; T: %d", Time, T));
        }
        else {
            for (Count = 0; Count < 16; Count++) {
                uint64_t cycles_cnt2 = getCycleCount ();
                if (cycles_cnt2 - cycles_cnt1 < 10) {
                    DEBUG_PRINT(("break"));
                    break;
                }
                CPU_RunInstruction ();
                Time = getCycleCount ();
                T    = Device_Data.CPU_State.Time;
                DEBUG_PRINT(("Time: %d; T: %d", Time, T));
                cycles_cnt1 = cycles_cnt2;
            }
        }
        // Вся периодика
        DEBUG_PRINT(("RunState: %d", RunState));
        switch (RunState++)
        {
            default:
            case 0:
                DEBUG_PRINT(("CPU_TimerRun"));
                CPU_TimerRun ();
                RunState = 1;
                break;
            case 1:
                //ps2_periodic ();
                break;
            case 2:
                CodeAndFlags = ps2get_raw_code(); // ps2_read ();
                if (CodeAndFlags == 0) RunState = 5;
                break;
            case 3:
              //  KBD_PRINT(("CodeAndFlags: %Xh; CodeAndFlags == PS2_PAUSE: %d", CodeAndFlags, CodeAndFlags == PS2_PAUSE));
                if (CodeAndFlags == PS2_PAUSE) {
                    RunState = 5;
                    CPU_Stop ();
                }
                else {
                    Key = Key_Translate (CodeAndFlags);
                    KBD_PRINT(("CodeAndFlags: %Xh Key_Translate: %Xh", CodeAndFlags, Key));
                }
                break;
            case 4:
                // ps2_leds ((Key_Flags >> KEY_FLAGS_TURBO_POS) & 7);
                KBD_PRINT(("ps2_leds - ignored"));
                if (Key_Flags & KEY_FLAGS_NUMLOCK) Device_Data.SysRegs.RdReg177714 = (uint16_t) (Key_Flags >> KEY_FLAGS_UP_POS);
                else                               Device_Data.SysRegs.RdReg177714 = 0;
                KBD_PRINT(("Device_Data.SysRegs.RdReg177714: %Xh", Device_Data.SysRegs.RdReg177714));
                if (CodeAndFlags & 0x8000U) {
                    if (((LastKey ^ CodeAndFlags) & 0x7FF) == 0) {
                        Device_Data.SysRegs.RdReg177716 |= 0100;
                        KBD_PRINT(("Device_Data.SysRegs.RdReg177716: %Xh", Device_Data.SysRegs.RdReg177716));
                        LastKey = 0xC00;
                    }
                }
                else if (Key != KEY_UNKNOWN) {
                    if (Key == KEY_MENU_ESC) {
                        if_manager(true);
                        Time = getCycleCount ();
                        T    = Device_Data.CPU_State.Time;
                    }
                    else {
                        LastKey  = ((uint_fast32_t) Key << 16) | CodeAndFlags;
                        RunState = 6;
                    }
                }
                break;
            case 6:
                Device_Data.SysRegs.RdReg177716 &= ~0100;
                KBD_PRINT(("Device_Data.SysRegs.RdReg177716: %Xh (6)", Device_Data.SysRegs.RdReg177716));
            case 5:
                if ((LastKey & 0x800) == 0) {
                    if ((Device_Data.SysRegs.Reg177660 & 0200) == 0) {
                        Key = (uint_fast16_t) (LastKey >> 16);
KBD_PRINT(("Key: %d (5)", Key));
                        if      (Key == 14) Key_SetRusLat ();
                        else if (Key == 15) Key_ClrRusLat ();
                        if ((Device_Data.SysRegs.Reg177660 & 0100) == 0) {
                            if (Key & KEY_AR2_PRESSED) {
                                Device_Data.CPU_State.Flags &= ~CPU_FLAG_KEY_VECTOR_60;
                                Device_Data.CPU_State.Flags |=  CPU_FLAG_KEY_VECTOR_274;
                            }
                            else {
                                Device_Data.CPU_State.Flags &= ~CPU_FLAG_KEY_VECTOR_274;
                                Device_Data.CPU_State.Flags |=  CPU_FLAG_KEY_VECTOR_60;
                            }
                            KBD_PRINT(("Device_Data.CPU_State.Flags: %Xh (5)", Device_Data.CPU_State.Flags));
                        }
                        Device_Data.SysRegs.Reg177660 |= 0200;
                        KBD_PRINT(("Device_Data.SysRegs.Reg177660: %Xh (5)", Device_Data.SysRegs.Reg177660));
                    }
                    LastKey |= 0x800;
                    Device_Data.SysRegs.RdReg177662 = (uint16_t) (LastKey >> 16) & 0177;
                    KBD_PRINT(("Device_Data.SysRegs.RdReg177662: %Xh (5)", Device_Data.SysRegs.RdReg177662));
                }
                RunState = 0;
                break;
        }
    }
}

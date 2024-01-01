#include "timer0.h"
#include "reboot.h"
#include "ps2_bk.h"
#include "ps2_codes.h"
#include "CPU.h"
#include "Key.h"

#include "emu_e.h"
#include "CPU_e.h"

#include "../EmuUi/Key_eu.h"

#include "Debug.h"

#define AT_OVL __attribute__((section(".ovl3_e.text")))

uint32_t ps2get_raw_code(); // TODO:

static int_fast16_t pressed_count = 0;
inline static bool any_down(uint_fast16_t CodeAndFlags) {
    if (CodeAndFlags & 0xF000) pressed_count++;
    else pressed_count--;
    return pressed_count > 0;
}

int if_manager(bool force); // TODO:

#include "hardware/gpio.h"
#include "vga.h"

bool hw_get_bit_LOAD() {
    uint8_t out = 0;
#if LOAD_WAV_PIO
    out = gpio_get(LOAD_WAV_PIO);
#endif
    // valLoad=out*10;
    return out > 0;
};
#if LOAD_WAV_2_COVOX && LOAD_WAV_PIO
static uint8_t covox_plus = 0;
#endif

void AT_OVL emu_start () {
    uint64_t      cycles_cnt1  = getCycleCount ();
    int_fast32_t  Time         = (int32_t)cycles_cnt1;
    uint_fast32_t T            = 0;
    uint_fast16_t CodeAndFlags = 0;
    uint_fast16_t Key          = 0;
    uint_fast32_t LastKey      = 0xC00;
    uint_fast8_t  RunState     = 0;
    DEBUG_PRINT(("Init Time: %d", Time));
    KBD_PRINT(("Initial state: CodeAndFlags: %Xh Key: %Xh LastKey: %Xh CPU_State.Flags: %Xh",
                               CodeAndFlags,     Key,     LastKey, Device_Data.CPU_State.Flags));
    // Запускаем эмуляцию
    while (1) {
        int tormoz = if_manager(false);
#if LOAD_WAV_PIO
        bool bit_wav = hw_get_bit_LOAD();
        if (bit_wav) Device_Data.SysRegs.RdReg177716 |= 0b100000;
        else Device_Data.SysRegs.RdReg177716 &= ~0b100000;
#if LOAD_WAV_2_COVOX
        if (bit_wav) { covox_plus = 0x10; true_covox |= 0x0F; }
        if (!bit_wav && covox_plus) { covox_plus = 0; true_covox &= ~0x0F; }
#endif
#endif
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
                if (cycles_cnt2 - cycles_cnt1 < tormoz) { // чем выше константа, тем медленнее БК
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
                CodeAndFlags = (uint_fast16_t)(ps2get_raw_code() & 0xFFFF); // ps2_read ();
                if (CodeAndFlags == 0) RunState = 5;
/*   Адрес регистра - 177716.
   Старший байт регистра (разряды  8-15) используются для задания адреса, с которого запускается процессор при включении питания (при
этом младший байт регистра принимается равным 0). Адрес начального пуска процессора равен 100000.
   Разряды 0-3 служат для задания режимов работы процессора.
   Разряды 0-3, 8-15 доступны только по чтению.
   Разряды 4-7  доступны по чтению и по записи. При чтении из этих разрядов данные читаются из входного системного порта, а при записи в
эти разряды данные поступают в выходной системный порт.
   Назначение разрядов выходного системного порта:
   Разряд 4 используется для передачи данных на линию.
   Разряд 5 используется для передачи данных на магнитофон, либо сигнала готовности на линию.
   Разряд 6 используется для передачи данных на магнитофон и для генерации звукового сигнала.
   Разряд 7 используется для управления двигателем магнитофона ( "1" - "стоп", "0" - "пуск" ).
   Назначение разрядов входного системного порта:
   Разряд 4 используется для чтения данных с линии.
   Разряд 5 используется для чтения данных с магнитофона.
 * Разряд 6 сброшен в "0", если хотя бы одна клавиша нажата и установлен в "1", если все клавиши отжаты.
   Разряд 7 используется для чтения сигнала готовности с линии.
*/
                else { // TODO:
                //    if (any_down(CodeAndFlags)) Device_Data.SysRegs.RdReg177716 &= ~0100;
                //    else Device_Data.SysRegs.RdReg177716 |= 0100;
                //    KBD_PRINT(("2. CodeAndFlags: %04Xh RdReg177716: %oo", CodeAndFlags, Device_Data.SysRegs.RdReg177716));
                }
                break;
            case 3:
                if (CodeAndFlags == PS2_PAUSE) {
                    RunState = 5;
                    CPU_Stop ();
                }
                else {
                    Key = Key_Translate (CodeAndFlags);
                    KBD_PRINT(("3. CodeAndFlags: %04Xh RdReg177716: %oo Key: %d (%Xh / %oo)",
                                   CodeAndFlags, Device_Data.SysRegs.RdReg177716, Key, Key, Key));
                }
                break;
            case 4:
                // ps2_leds ((Key_Flags >> KEY_FLAGS_TURBO_POS) & 7);
                // джойстик
                if (Key_Flags & KEY_FLAGS_NUMLOCK) Device_Data.SysRegs.RdReg177714 = (uint16_t) (Key_Flags >> KEY_FLAGS_UP_POS);
                else                               Device_Data.SysRegs.RdReg177714 = 0;
                if (CodeAndFlags & 0x8000U) {
                    if (((LastKey ^ CodeAndFlags) & 0x7FF) == 0) {
                        Device_Data.SysRegs.RdReg177716 |= 0100;
                        KBD_PRINT(("4. RdReg177716: %oo", Device_Data.SysRegs.RdReg177716));
                        LastKey = 0xC00;
                    }
                } else if (Key != KEY_UNKNOWN) {
                    if (Key == KEY_MENU_ESC) {
                        tormoz = if_manager(true);
                        Time = getCycleCount ();
                        T    = Device_Data.CPU_State.Time;
                    }
                    else {
                        LastKey  = ((uint_fast32_t) Key << 16) | CodeAndFlags;
                        KBD_PRINT(("4. LastKey: %Xh", LastKey));
                        RunState = 6;
                    }
                }
                break;
            case 6:
                Device_Data.SysRegs.RdReg177716 &= ~0100;
                KBD_PRINT(("6. RdReg177716: %oo", Device_Data.SysRegs.RdReg177716));
            case 5:
/*   Регистр состояния клавиатуры имеет адрес 177660.
   В нем используются только два бита.
   Разряд 6 - это бит разрешения прерывания. Если в нем содержится логический ноль "0", то прерывание от клавиатуры разрешено; если нужно
запретить прерывание от клавиатуры, то в 6 разряд надо записать "1".
   Разряд 7 - это флаг готовности устройства. Он устанавливается в "1" при поступлении в регистр данных клавиатуры нового кода. Разряд
доступен только по чтению.
   Если прерывание от клавиатуры разрешено (в разряд 6 записан  "0"), то при установке разряда  7 в "1" (при поступлении в регистр данных
клавиатуры нового кода) производится прерывание от клавиатуры - читается код нажатой клавиши из регистра данных клавиатуры, выдается
звуковой сигнал и производятся действия, соответствующие нажатой клавише. При чтении регистра данных разряд 7 регистра состояния
сбрасывается в "0".*/
                if ((LastKey & 0x800) == 0) {
                    if ((Device_Data.SysRegs.Reg177660 & 0200) == 0) {
                        Key = (uint_fast16_t) (LastKey >> 16);
                        KBD_PRINT(("5. Key: %d", Key));
                        if (Key == 14) {
                            Key_SetRusLat ();
                        } else if (Key == 15) {
                            Key_ClrRusLat ();
                        }
                        if ((Device_Data.SysRegs.Reg177660 & 0100) == 0) {
                            if (Key & KEY_AR2_PRESSED) {
                                Device_Data.CPU_State.Flags &= ~CPU_FLAG_KEY_VECTOR_60;
                                Device_Data.CPU_State.Flags |=  CPU_FLAG_KEY_VECTOR_274;
                            }
                            else {
                                Device_Data.CPU_State.Flags &= ~CPU_FLAG_KEY_VECTOR_274;
                                Device_Data.CPU_State.Flags |=  CPU_FLAG_KEY_VECTOR_60;
                            }
                            KBD_PRINT(("5. CPU_State.Flags: %Xh", Device_Data.CPU_State.Flags));
                        }
                        Device_Data.SysRegs.Reg177660 |= 0200;
                        KBD_PRINT(("5. Reg177660: %oo", Device_Data.SysRegs.Reg177660));
                    }
                    LastKey |= 0x800;
                    KBD_PRINT(("5. LastKey: %Xh", LastKey));
/*   Регистр данных клавиатуры имеет адрес 177662.
   При нажатии на определенную клавишу в разрядах 0-6 этого регистра формируется соответствующий нажатой клавише семиразрядный код. Запись
нового кода в регистр не производится до тех пор, пока не будет прочитан предыдущий код.
   Разряды 7-15 не используются.*/
                    Device_Data.SysRegs.RdReg177662 = (uint16_t) (LastKey >> 16) & 0177;
                    KBD_PRINT(("5. RdReg177662: %oo", Device_Data.SysRegs.RdReg177662));
                }
                RunState = 0;
                break;
        }
    }
}

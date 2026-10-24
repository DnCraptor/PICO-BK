#include <pico/stdlib.h>

#include "reboot.h"
#include "ps2_bk.h"
#include "ps2_codes.h"
#include "CPU.h"
#include "CPU_ef.h"
#include "Key.h"
#include "emu_e.h"
#include "CPU_e.h"
#include "../EmuUi/Key_eu.h"
#include "Debug.h"
#include "ps2.h"
#include "hardware/gpio.h"
#include "vga.h"
#include "manager.h"
#include "debug.h"

#define AT_OVL __attribute__((section(".ovl3_e.text")))

static int_fast16_t pressed_count = 0;
inline static bool any_down(uint_fast16_t CodeAndFlags) {
    if (CodeAndFlags & 0xF000) pressed_count++;
    else pressed_count--;
    return pressed_count > 0;
}

bool hw_get_bit_LOAD() {
    uint8_t out = 0;
#if LOAD_WAV_PIO
    out = gpio_get(LOAD_WAV_PIO);
#endif
    // valLoad=out*10;
    return out > 0;
};
#if LOAD_WAV_2_COVOX && LOAD_WAV_PIO
static bool covox_plus = 0;
#endif

volatile bool is_kbd_joystick = false;

/*
 * Real-time pacing on the Linux host.
 *
 * The RP2350 build busy-waits after every instruction on the SysTick counter
 * until the ARM time matches the emulated BK time; the VGA interrupt raises
 * the 50 Hz frame interrupt and an alarm timer samples the sound state at
 * 44.1 kHz.  Here the host clock paces the emulation in steps of about a
 * millisecond (the thread sleeps when it is ahead), the 50 Hz interrupt
 * follows the host clock as before, and the sound state is sampled every
 * 1/44100 s of emulated time, so beeper edges keep their exact position
 * whatever the host scheduling.
 */
#define EMU_SOUND_RATE 44100u
#define EMU_AHEAD_US   1000u      /* sleep when this far ahead of real time */
#define EMU_LATE_US    100000u    /* restart pacing when this far behind */

extern void bk_audio_sample(void);   // main.cpp: one stereo sample to ALSA
extern volatile bool vsync;          // CPU_e.c: 50 Hz frame interrupt request

static uint64_t pace_anchor_us;
static uint64_t pace_ticks;
static uint32_t pace_freq;
static uint64_t sound_acc;
static uint64_t vsync_next_us;

static void pace_restart(uint64_t now) {
    pace_anchor_us = now;
    pace_ticks = 0;
    pace_freq = g_conf.cpu_freq;
}

static void pace(uint32_t dT, bool throttle) {
    // sound: one sample per 1/44100 s of emulated time
    sound_acc += (uint64_t)dT * EMU_SOUND_RATE;
    while (sound_acc >= g_conf.cpu_freq) {
        sound_acc -= g_conf.cpu_freq;
        bk_audio_sample();
    }
    uint64_t now = time_us_64();
    // 50 Hz frame interrupt, real time
    if (now >= vsync_next_us) {
        vsync = 1;
        vsync_next_us += 20000;
        if (vsync_next_us <= now) vsync_next_us = now + 20000;
    }
    if (!throttle) return;
    if (pace_freq != g_conf.cpu_freq) pace_restart(now);
    pace_ticks += dT;
    const uint64_t target = pace_anchor_us + pace_ticks * 1000000u / pace_freq;
    if (target > now + EMU_AHEAD_US) {
        sleep_us(target - now);
    } else if (now > target + EMU_LATE_US) {
        pace_restart(now);   // manager dialog, host overload: no catch-up burst
    }
}

inline static void cleanup_emu_state() {
    g_conf.cpu_freq     = g_conf.bk0010mode >= BK_0011M_FDD ? 4000000 : 3000000;
    g_conf.T            = 0;
    g_conf.CodeAndFlags = 0;
    g_conf.Key          = 0;
    g_conf.LastKey      = 0xC00;
    g_conf.RunState     = 0;
}

void AT_OVL emu_start () {
    cleanup_emu_state();
    KBD_PRINT(("Initial state: CodeAndFlags: %Xh Key: %Xh LastKey: %Xh CPU_State.Flags: %Xh",
                               g_conf.CodeAndFlags, g_conf.Key, g_conf.LastKey, Device_Data.CPU_State.Flags));
    // Запускаем эмуляцию
    pace_restart(time_us_64());
    vsync_next_us = pace_anchor_us + 20000;
    uint32_t kbd_divider = 0;
    while (1) {
        // Вызываем manager (с keyboard_tick внутри) не каждую инструкцию,
        // а ~раз в 1 мс (каждые ~256 итераций).
        if (!(kbd_divider++ & 0xFF))
            manager(false);
#if LOAD_WAV_PIO
        bool bit_wav = hw_get_bit_LOAD();
        if (bit_wav) Device_Data.SysRegs.RdReg177716 |= 0b100000;
        else Device_Data.SysRegs.RdReg177716 &= ~0b100000;
#if LOAD_WAV_2_COVOX
        if (bit_wav && covox_mix) { covox_plus = true; true_covox |= covox_mix; }
        if (!bit_wav && covox_plus && covox_mix) { covox_plus = false; true_covox &= ~covox_mix; }
        if (covox_plus && !covox_mix) { covox_plus = false; true_covox = 0; }
#endif
#endif
        uint_fast8_t  Count;
        DEBUG_PRINT(("Key_Flags: %08Xh; (Key_Flags & KEY_FLAGS_TURBO): %d", Key_Flags, (Key_Flags & KEY_FLAGS_TURBO)));
        if (Key_Flags & KEY_FLAGS_TURBO) {
            for (Count = 0; Count < 16; Count++) {
                DEBUG_PRINT(("Count: %d", Count));
                CPU_RunInstruction ();
            }
            uint32_t dT = Device_Data.CPU_State.Time - g_conf.T; // BM1 emulated ticks passed
            if (dT < 0x7FFFFFFF) pace(dT, false);
            g_conf.T    = Device_Data.CPU_State.Time;
        }
        else {
            CPU_RunInstruction ();
            uint32_t dT = Device_Data.CPU_State.Time - g_conf.T; // BM1 emulated ticks passed
            if (dT < 0x7FFFFFFF) { // W/A for overload
                // wait for the same time as for emulated CPU
                pace(dT, true);
            }
            g_conf.T    = Device_Data.CPU_State.Time;
        }
        // Вся периодика
        DEBUG_PRINT(("RunState: %d", g_conf.RunState));
        switch (g_conf.RunState++)
        {
            default:
            case 0:
                DEBUG_PRINT(("CPU_TimerRun"));
                CPU_TimerRun ();
                g_conf.RunState = 1;
                break;
            case 1:
                //ps2_periodic ();
                break;
            case 2: {
                uint_fast16_t prev = g_conf.CodeAndFlags;
                g_conf.CodeAndFlags = (uint_fast16_t)(ps2get_raw_code() & 0xFFFF); // ps2_read ();
                if (prev != g_conf.CodeAndFlags) {
                    KBD_PRINT(("2. 0x%04X (prev: 0x%04X)", g_conf.CodeAndFlags, prev));
                }
                if (g_conf.CodeAndFlags == 0) { // nothig pressed, so do nothig
                    g_conf.RunState = 0; /// 5;
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
                } else {
                    if (g_conf.PrevCAF == g_conf.CodeAndFlags) { // nothing changed, so do nothing (suppress auto-repeat)
                        g_conf.RunState = 0;
                    } else {
                        g_conf.PrevCAF = g_conf.CodeAndFlags;
                    }
                     // TODO:
                //    if (any_down(CodeAndFlags)) Device_Data.SysRegs.RdReg177716 &= ~0100;
                //    else Device_Data.SysRegs.RdReg177716 |= 0100;
                //    KBD_PRINT(("2. CodeAndFlags: %04Xh RdReg177716: %oo", CodeAndFlags, Device_Data.SysRegs.RdReg177716));
                }
                break;
            }
            case 3:
                if (g_conf.CodeAndFlags == PS2_PAUSE) {
                    g_conf.RunState = 5;
                    CPU_Stop ();
                }
                else if (g_conf.CodeAndFlags == PS2_HOME) {
                    manager(true);
                    g_conf.Key = KEY_UNKNOWN;
                    g_conf.T    = Device_Data.CPU_State.Time;
                }
                else {
                    g_conf.Key = Key_Translate (g_conf.CodeAndFlags);
                    KBD_PRINT(("3. CodeAndFlags: %04Xh RdReg177716: %oo Key: %d (%Xh / %oo)",
                        g_conf.CodeAndFlags, Device_Data.SysRegs.RdReg177716, g_conf.Key, g_conf.Key, g_conf.Key));
                }
                break;
            case 4:
                // ps2_leds ((Key_Flags >> KEY_FLAGS_TURBO_POS) & 7);
                if (g_conf.CodeAndFlags & 0x8000U) {
                    if (((g_conf.LastKey ^ g_conf.CodeAndFlags) & 0x7FF) == 0) {
                        Device_Data.SysRegs.RdReg177716 |= 0100;
                        KBD_PRINT(("4. RdReg177716: %oo", Device_Data.SysRegs.RdReg177716));
                        g_conf.LastKey = 0xC00;
                    }
                } else if (g_conf.Key != KEY_UNKNOWN) {
                    g_conf.LastKey  = ((uint_fast32_t) g_conf.Key << 16) | g_conf.CodeAndFlags;
                    KBD_PRINT(("4. LastKey: %Xh", g_conf.LastKey));
                    g_conf.RunState = 6;
                }
                break;
            case 6:
                Device_Data.SysRegs.RdReg177716 &= ~0100;
                KBD_PRINT(("6. RdReg177716: %oo", Device_Data.SysRegs.RdReg177716));
                // fall through
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
                if ((g_conf.LastKey & 0x800) == 0) {
                    if ((Device_Data.SysRegs.Reg177660 & 0200) == 0) {
                        g_conf.Key = (uint_fast16_t) (g_conf.LastKey >> 16);
                        KBD_PRINT(("5. Key: %d", g_conf.Key));
                        if (g_conf.Key == 14) {
                            Key_SetRusLat ();
                        } else if (g_conf.Key == 15) {
                            Key_ClrRusLat ();
                        }
                        if ((Device_Data.SysRegs.Reg177660 & 0100) == 0) {
                            if (g_conf.Key & KEY_AR2_PRESSED) {
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
                    g_conf.LastKey |= 0x800;
                    KBD_PRINT(("5. LastKey: %Xh", g_conf.LastKey));
/*   Регистр данных клавиатуры имеет адрес 177662.
   При нажатии на определенную клавишу в разрядах 0-6 этого регистра формируется соответствующий нажатой клавише семиразрядный код. Запись
нового кода в регистр не производится до тех пор, пока не будет прочитан предыдущий код.
   Разряды 7-15 не используются.*/
                    Device_Data.SysRegs.RdReg177662 = (uint16_t) (g_conf.LastKey >> 16) & 0177;
                    KBD_PRINT(("5. RdReg177662: %oo", Device_Data.SysRegs.RdReg177662));
                }
                g_conf.RunState = 0;
                break;
        }
    }
}

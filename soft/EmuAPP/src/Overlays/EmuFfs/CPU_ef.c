#include <stdlib.h> 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "AnyMem.h"
#include "CPU.h"
#include "CPU_ef.h"
#include "vga.h"
#include "fdd.h"
#include "aySoundSoft.h"

#define AT_OVL __attribute__((section(".ovl0_ef.text")))

/*
таймер

177706 -- Регистр начального значения таймера. Доступен по чтению и записи.
177710 -- Реверсивный счётчик. Доступен по чтению, запись в регистр игнорируется.
177712 -- Программируемый таймер-- регистр управления.
(001)бит 0: STOP: "1" - остановка
При установке запрещает счёт и переписывает в регистр счётчика константу из регистра начального значения таймера
(002)бит 1: WRAPAROUND: "1" - Режим непрерывного счёта, отменяет действие битов EXPENABLE и ONESHOT
Запрещает фиксацию перехода счётчика через 0. Досчитав до нуля, таймер продолжает вычитание.
(004)бит 2: EXPENABLE: "1" - разрешение установки сигнала EXPIRY ("конец счета")
Установка включает индикацию, при очередном переходе таймера через 0 устанавливается в 1 бит EXPIRY, если ранее он был сброшен.
Нужно учитывать, что при первом (после включения ЭВМ или системного сброса) запуске таймера в данном режиме индикация
срабатывает только после ВТОРОГО перехода счётчика через 0, причём независимо от того, работал ли таймер до этого в
других режимах
(010)бит 3: ONESHOT: режим одновибратора
При установке запрещает повторный счёт, после первого досчёта до 0 сбрасывается бит RUN. Установка данного режима не отменяет
режим индикации (бит EXPENABLE)
(020)бит 4: RUN: запуск счётчика, запись "1"-- загружает счётчик из регистра 177706 и начинает отсчёт
В этом режиме при досчёте до нуля в счётчик заново заносится константа из регистра начального значения, следовательно
счёт всегда ведётся от константы до нуля (если только не запрещена фиксация перехода через 0 битом WRAPAROUND). При сбросе
в 0 в счётчик переписывается начальное значение из регистра начального значения
(040)бит 5: делитель на 4, "1"-- включён
Установка снижает скорость счёта в 4 раза (режим умножения времени на 4)
(100)бит 6: делитель на 16, "1"-- включён
Установка снижает скорость счёта в 16 раз (режим умножения времени на 16)
при одновременной установке скорость снижается, соответственно в 64 раза
(200)бит 7: EXPIRY: флаг окончания счета, устанавливается в "1" при достижении счётчиком нуля, сбрасывается только программно
биты 8-15 не используются, "1".
177662 Разряд 14 - управляет включением системного таймера. При значении 0 таймер выключен, при 1 - включен.
*/

void AT_OVL CPU_TimerRun (void)
{
    DEBUG_PRINT(("CPU_TimerRun"));
    if (!(Device_Data.SysRegs.WrReg177662 & (1ul << 14))) {
        return;
    }
    uint_fast16_t Cfg = Device_Data.SysRegs.Reg177712;
    //если счётчик остановлен
    if (Cfg & 1) {
        Device_Data.SysRegs.Reg177710 = Device_Data.SysRegs.Reg177706; //проинициализируем регистр счётчика
    }
    else if (Cfg & 020) { //если счётчик запущен
        uint_fast32_t CurT = Device_Data.CPU_State.Time >> 7;
        uint_fast32_t T    = Device_Data.Timer.T + (((CurT - Device_Data.Timer.PrevT) & 0xFFFFFF) << Device_Data.Timer.Div);
        int_fast32_t  Cntr = Device_Data.SysRegs.Reg177710;
        Device_Data.Timer.PrevT = CurT;
        Device_Data.Timer.T  = T & 0x3F;
        T >>= 6;
        if (Cntr == 0 || (Cfg & 2)) { //если счетчик уже был равен 0 или режим WRAPAROUND
            Cntr -= T;
        }
        else
        {
            Cntr -= T;
            if (Cntr <= 0) {
                uint_fast16_t InitVal = Device_Data.SysRegs.Reg177706;
                if (Cfg & 4) { //разрешение установки сигнала "конец счёта" ?
                    Cfg |= 0200;    //да, установим сигнал
                }
                if (Cfg & 010) { //установлен режим одновибратора?
                    Cfg &= ~020;    //тогда сбросим бит 4
                    Cntr = InitVal; //проинициализируем регистр счётчика
                }
                else if (InitVal) {
                    do {
                        Cntr += InitVal; //проинициализируем регистр счётчика
                    } while (Cntr <= 0);
                }
                Device_Data.SysRegs.Reg177712 = Cfg;
            }
        }
        Device_Data.SysRegs.Reg177710 = (uint16_t) Cntr;
    }
}

TCPU_Arg AT_OVL CPU_ReadMemW (TCPU_Arg Adr) {
    DEBUG_PRINT(("CPU_ReadMemW Adr: %oo (%Xh) Page: (#%d)", Adr, Adr, Adr >> 12));
    uint16_t *pReg;
    if (Adr >= 0177130 && is_fdd_suppored()) {
        switch (Adr >> 1) {
            case (0177130 >> 1):
                Device_Data.SysRegs.RdReg177130 = GetState();
                DSK_PRINT(("W RdReg177130: %04Xh", Device_Data.SysRegs.RdReg177130));
                return Device_Data.SysRegs.RdReg177130;
            case (0177132 >> 1):
                Device_Data.SysRegs.Reg177132 = GetData();
                DSK_PRINT(("W Reg177132: %04Xh", Device_Data.SysRegs.Reg177132));
                return Device_Data.SysRegs.Reg177132;
            }
    }
    if (Adr < CPU_START_IO_ADR) {
        uintptr_t Page = Device_Data.MemPages [(Adr) >> 12];
        DEBUG_PRINT(("CPU_ReadMemW Adr: %oo (%Xh) Page: %08Xh (#%d)", Adr, Adr, Page, Adr >> 12));
        if (Page) {
            uintptr_t t = (Page + ((Adr) & 0x0FFE));
            DEBUG_PRINT(("CPU_ReadMemW (Page + ((Adr) & 0x0FFE)): %08Xh", t));
            uint16_t r = AnyMem_r_u16 ((uint16_t *)t);
            DEBUG_PRINT(("CPU_ReadMemW AnyMem_r_u16(%08X): %oo", t, r));
            return r;
        }
        return CPU_ARG_READ_ERR;
    }
    switch (Adr >> 1) {
        case (0177660 >> 1): pReg = &Device_Data.SysRegs.Reg177660;   break;
        case (0177662 >> 1): Device_Data.SysRegs.Reg177660 &= ~0200;
                             pReg = &Device_Data.SysRegs.RdReg177662; break;
        case (0177664 >> 1): pReg = &Device_Data.SysRegs.Reg177664;   break;
        case (0177706 >> 1): pReg = &Device_Data.SysRegs.Reg177706;   break;
        case (0177710 >> 1): CPU_TimerRun ();
                             pReg = &Device_Data.SysRegs.Reg177710;   break;
        case (0177712 >> 1): CPU_TimerRun ();
                             pReg = &Device_Data.SysRegs.Reg177712;   break;
        case (0177714 >> 1): pReg = &Device_Data.SysRegs.RdReg177714; break;
        case (0177716 >> 1): {
            pReg = &Device_Data.SysRegs.RdReg177716;
            break;
        }
        default: return CPU_ARG_READ_ERR;
    }
    return *pReg;
}

TCPU_Arg AT_OVL CPU_ReadMemB (TCPU_Arg Adr) {
    uint16_t *pReg;
    DEBUG_PRINT(("CPU_ReadMemB Adr: %oo (%Xh) Page: (#%d)", Adr, Adr, Adr >> 12));
    if (Adr >= 0177130 && is_fdd_suppored()) {
        switch (Adr >> 1) {
            case (0177130 >> 1):
                Device_Data.SysRegs.RdReg177130 = GetState();
                if (0177131 == Adr) { // ?
                    DSK_PRINT(("B RdReg177131: %02Xh", (Device_Data.SysRegs.RdReg177130 >> 8) & 0xff));
                    return (Device_Data.SysRegs.RdReg177130 >> 8) & 0xff;
                }
                DSK_PRINT(("B RdReg177130: %02Xh", Device_Data.SysRegs.RdReg177130 & 0xff));
                return Device_Data.SysRegs.RdReg177130 & 0xff;
            case (0177132 >> 1):
                Device_Data.SysRegs.Reg177132 = GetData();
                DSK_PRINT(("B Reg177132: %04Xh",  Device_Data.SysRegs.Reg177132));
                return Device_Data.SysRegs.Reg177132 & 0xff;
        }
    }
    if (Adr < CPU_START_IO_ADR) {
        uintptr_t Page = Device_Data.MemPages [Adr >> 12];
        if (Page) return AnyMem_r_u8 ((uint8_t *) (Page + (Adr & 0x0FFF)));
        return CPU_ARG_READ_ERR;
    }
    switch (Adr >> 1)  {
        case (0177660 >> 1): pReg = &Device_Data.SysRegs.Reg177660;   break;
        case (0177662 >> 1): Device_Data.SysRegs.Reg177660 &= ~0200;
                             pReg = &Device_Data.SysRegs.RdReg177662; break;
        case (0177664 >> 1): pReg = &Device_Data.SysRegs.Reg177664;   break;
        case (0177706 >> 1): pReg = &Device_Data.SysRegs.Reg177706;   break;
        case (0177710 >> 1): CPU_TimerRun ();
                             pReg = &Device_Data.SysRegs.Reg177710;   break;
        case (0177712 >> 1): CPU_TimerRun ();
                             pReg = &Device_Data.SysRegs.Reg177712;   break;
        case (0177714 >> 1): pReg = &Device_Data.SysRegs.RdReg177714; break;
        case (0177716 >> 1): {
            pReg = &Device_Data.SysRegs.RdReg177716;
            break;
        }
        default: return CPU_ARG_READ_ERR;
    }
    return ((uint8_t *) pReg) [Adr & 1];
}

static const uintptr_t PageTab [8*4] = {
    CPU_PAGE11_MEM_ADR,
    CPU_PAGE12_MEM_ADR,
    CPU_PAGE13_MEM_ADR,
    CPU_PAGE14_MEM_ADR,

    CPU_PAGE51_MEM_ADR,
    CPU_PAGE52_MEM_ADR,
    CPU_PAGE53_MEM_ADR,
    CPU_PAGE54_MEM_ADR,

    CPU_PAGE21_MEM_ADR,
    CPU_PAGE22_MEM_ADR,
    CPU_PAGE23_MEM_ADR,
    CPU_PAGE24_MEM_ADR,

    CPU_PAGE31_MEM_ADR,
    CPU_PAGE32_MEM_ADR,
    CPU_PAGE33_MEM_ADR,
    CPU_PAGE34_MEM_ADR,

    CPU_PAGE41_MEM_ADR,
    CPU_PAGE42_MEM_ADR,
    CPU_PAGE43_MEM_ADR,
    CPU_PAGE44_MEM_ADR,

    CPU_PAGE71_MEM_ADR,
    CPU_PAGE72_MEM_ADR,
    CPU_PAGE73_MEM_ADR,
    CPU_PAGE74_MEM_ADR,

    CPU_PAGE01_MEM_ADR,
    CPU_PAGE02_MEM_ADR,
    CPU_PAGE03_MEM_ADR,
    CPU_PAGE04_MEM_ADR,

    CPU_PAGE61_MEM_ADR,
    CPU_PAGE62_MEM_ADR,
    CPU_PAGE63_MEM_ADR,
    CPU_PAGE64_MEM_ADR
}; // {1, 5, 2, 3, 4, 7, 0, 6};

static inline void select_11_page(uint16_t Word) {
    Device_Data.SysRegs.Wr1Reg177716 = (uint16_t) Word;
    uint8_t p = ((Word >> 12) & 7) * 4;
    // DBGM_PRINT(("select_11_page(0%o) p: %d", Word, p));
    Device_Data.MemPages [4] = PageTab [p];
    Device_Data.MemPages [5] = PageTab [p + 1];
    Device_Data.MemPages [6] = PageTab [p + 2];
    Device_Data.MemPages [7] = PageTab [p + 3];
    if (Word & 01) {
        Device_Data.MemPages [8]  = CPU_PAGE81_MEM_ADR;
        Device_Data.MemPages [9]  = CPU_PAGE82_MEM_ADR;
        Device_Data.MemPages [10] = CPU_PAGE83_MEM_ADR;
        Device_Data.MemPages [11] = CPU_PAGE84_MEM_ADR;
    } else if (Word & 02) {
        Device_Data.MemPages [8]  = CPU_PAGE91_MEM_ADR;
        Device_Data.MemPages [9]  = CPU_PAGE92_MEM_ADR;
        Device_Data.MemPages [10] = CPU_PAGE93_MEM_ADR;
        Device_Data.MemPages [11] = CPU_PAGE94_MEM_ADR;
    } else if (Word & 010) {
        Device_Data.MemPages [8]  = CPU_PAGE101_MEM_ADR;
        Device_Data.MemPages [9]  = CPU_PAGE102_MEM_ADR;
        Device_Data.MemPages [10] = CPU_PAGE103_MEM_ADR;
        Device_Data.MemPages [11] = CPU_PAGE104_MEM_ADR;
    } else if (Word & 020) {
        Device_Data.MemPages [8]  = CPU_PAGE111_MEM_ADR;
        Device_Data.MemPages [9]  = CPU_PAGE112_MEM_ADR;
        Device_Data.MemPages [10] = CPU_PAGE113_MEM_ADR;
        Device_Data.MemPages [11] = CPU_PAGE114_MEM_ADR;
    } else {
        p = ((Word >> 8) & 7) * 4;
        // DBGM_PRINT(("select_11_page(0%o) p2: %d", Word, p));
        Device_Data.MemPages [8]  = PageTab [p];
        Device_Data.MemPages [9]  = PageTab [p + 1];
        Device_Data.MemPages [10] = PageTab [p + 2];
        Device_Data.MemPages [11] = PageTab [p + 3];
    }
}

TCPU_Arg AT_OVL CPU_WriteW (TCPU_Arg Adr, uint_fast16_t Word) {
    uint_fast16_t PrevWord;
    DEBUG_PRINT(("CPU_WriteW(%oo, %oo) Page: #%d", Adr, Word, Adr >> 12));
    if (CPU_IS_ARG_REG (Adr)) {
        DEBUG_PRINT(("R%d <= %oo", CPU_GET_ARG_REG_INDEX (Adr), Word));
        Device_Data.CPU_State.r [CPU_GET_ARG_REG_INDEX (Adr)] = (uint16_t) Word;
        return CPU_ARG_WRITE_OK;
    }
    if (get_bk0010mode() == BK_FDD) {
        if (Adr < 0160000) {
            uintptr_t Page = Device_Data.MemPages[Adr >> 12];
            auto addr = Page + (Adr & 0x0FFE);
            DEBUG_PRINT(("CPU_WriteW(%oo, %oo) Page: %08X (#%d)", Adr, Word, Page, Adr >> 12));
            if (Page && addr < CPU_PAGE01_MEM_ADR + RAM_PAGES_SIZE) {
                AnyMem_w_u16 ((uint16_t *)addr, Word);
                return CPU_ARG_WRITE_OK;
            }
            return CPU_ARG_WRITE_ERR;
        }
    }
    else if (Adr < 0140000) {
        uintptr_t Page = Device_Data.MemPages [Adr >> 12];
        auto addr = Page + (Adr & 0x0FFE);
        DEBUG_PRINT(("CPU_WriteW(%oo, %oo) Page: %08X (#%d)", Adr, Word, Page, Adr >> 12));
        if (Page && addr < CPU_PAGE01_MEM_ADR + RAM_PAGES_SIZE) {
            AnyMem_w_u16 ((uint16_t *)addr, Word);
            return CPU_ARG_WRITE_OK;
        }
        return CPU_ARG_WRITE_ERR;
    }
    switch (Adr >> 1) {
        case (0177130 >> 1):
            if (is_fdd_suppored()) {
                DSK_PRINT(("W WrReg177130 <- %04Xh", Word));
                Device_Data.SysRegs.WrReg177130 = (uint16_t) Word;
                SetCommand(Word);
            } else {
                return CPU_ARG_WRITE_ERR;
            }
            break;
        case (0177132 >> 1):
            if (is_fdd_suppored()) {
                DSK_PRINT(("W Reg177132 <- %04Xh", Word));
                Device_Data.SysRegs.Reg177132 = (uint16_t) Word;
                WriteData(Word);
            } else {
                return CPU_ARG_WRITE_ERR;
            }
            break;
        case (0177660 >> 1):
            PrevWord = Device_Data.SysRegs.Reg177660;
            Device_Data.SysRegs.Reg177660 = (uint16_t) ((Word & 0100) | (PrevWord & ~0100));
            break;
        case (0177662 >> 1):
            Device_Data.SysRegs.WrReg177662 = (uint16_t) Word;
            // Разряд 14 - управляет включением системного таймера. При значении 0 таймер выключен, при 1 - включен.
            // TODO: if 0011
            graphics_set_page(Word & 0100000 ? CPU_PAGE61_MEM_ADR : CPU_PAGE51_MEM_ADR, (Word >> 8) & 15);
            break;
        case (0177664 >> 1):
            PrevWord = Device_Data.SysRegs.Reg177664;
            Word = ((Word & 01377) | (PrevWord & ~01377));
            graphics_shift_screen(Word);
            Device_Data.SysRegs.Reg177664 = (uint16_t) Word;
            break;
        case (0177706 >> 1):
            CPU_TimerRun ();
            Device_Data.SysRegs.Reg177706 = (uint16_t) Word;
            break;
//      case (0177710 >> 1):
        case (0177712 >> 1):
            Word |= 0xFF00U;
            Device_Data.SysRegs.Reg177712 = (uint16_t) Word;
            Device_Data.SysRegs.Reg177710 = Device_Data.SysRegs.Reg177706;
            Device_Data.Timer.PrevT = Device_Data.CPU_State.Time >> 7;
            Device_Data.Timer.T     = 0;
            Device_Data.Timer.Div   = (~Word >> 4) & 6;
            break;
        case (0177714 >> 1):
			/*177714
			Регистр параллельного программируемого порта ввода вывода - два регистра, входной по чтению и выходной по записи.
			из выходного нельзя ничего прочитать т.к., читается оно из входного,
			во входной невозможно ничего записать, т.к. записывается оно в выходной.
			*/
            Device_Data.SysRegs.WrReg177714 = (uint16_t) Word;
            {
                uint_fast32_t Reg = (Word & 0xFF) >> 1;
                if (Device_Data.SysRegs.WrReg177716 & 0100) Reg += 0x80;
#ifdef COVOX
                true_covox = Device_Data.SysRegs.WrReg177716;
#endif
#ifdef AYSOUND
                AY_write_address(Device_Data.SysRegs.WrReg177716);
#endif
            }
            break;
        case (0177716 >> 1):
            DEBUG_PRINT(("case (0177716 >> 1) on CPU_WriteW - switch pages: %d", (Word >> 12) & 7));
            if (Word & (1U << 11) && is_bk0011mode()) {
                select_11_page((uint16_t) Word);
            }
            else {
                Device_Data.SysRegs.WrReg177716 = (uint16_t) Word;
                uint_fast32_t Reg = *(uint8_t *) &Device_Data.SysRegs.WrReg177714 >> 1;
                if (Word & 0100) Reg += 0x80;
                // ???
                true_covox = Device_Data.SysRegs.WrReg177716;
            }
            break;
        default:
            return CPU_ARG_WRITE_ERR;
    }
    return CPU_ARG_WRITE_OK;
}

TCPU_Arg AT_OVL CPU_WriteB (TCPU_Arg Adr, uint_fast8_t Byte) {
    uint_fast16_t Word;
    uint_fast16_t PrevWord;
    DEBUG_PRINT(("CPU_WriteB(%08Xh, %oo) Page: #%d", Adr, Word, Adr >> 12));
    if (CPU_IS_ARG_REG (Adr)) {
        *(uint8_t *) &Device_Data.CPU_State.r [CPU_GET_ARG_REG_INDEX (Adr)] = (uint8_t) Byte;
        return CPU_ARG_WRITE_OK;
    }
    if (get_bk0010mode() == BK_FDD) {
        if (Adr < 0160000) {
            uintptr_t Page = Device_Data.MemPages[Adr >> 12];
            auto addr = Page + ((Adr) & 0x0FFF);
            if (Page && addr < CPU_PAGE01_MEM_ADR + RAM_PAGES_SIZE) {
                AnyMem_w_u8 ((uint8_t *)addr, Byte);
                return CPU_ARG_WRITE_OK;
            }
            return CPU_ARG_WRITE_ERR;
        }       
    }
    if (Adr < 0140000) {
        uintptr_t Page = Device_Data.MemPages [Adr >> 12];
        auto addr = Page + ((Adr) & 0x0FFF);
        if (Page && addr < CPU_PAGE01_MEM_ADR + RAM_PAGES_SIZE) {
            AnyMem_w_u8 ((uint8_t *)addr, Byte);
            return CPU_ARG_WRITE_OK;
        }
        return CPU_ARG_WRITE_ERR;
    }
    Word = Byte;
    if (Adr & 1) Word <<= 8;
    switch (Adr >> 1) {
        case (0177130 >> 1):
            if (is_fdd_suppored()) {
                DSK_PRINT(("B WrReg177130 <- %04Xh", Word));
                Device_Data.SysRegs.WrReg177130 = (uint16_t) Word;
                SetCommand(Word & 0xFF);
            } else {
                return CPU_ARG_WRITE_ERR;
            }
            break;
        case (0177132 >> 1):
            if (is_fdd_suppored()) {
                DSK_PRINT(("B Reg177132 <- %04Xh", Word));
                Device_Data.SysRegs.Reg177132 = (uint16_t) Word;
                WriteData(Word & 0xFF);
            } else {
                return CPU_ARG_WRITE_ERR;
            }
            break;
        case (0177660 >> 1):
            PrevWord = Device_Data.SysRegs.Reg177660;
            Device_Data.SysRegs.Reg177660 = (uint16_t) ((Word & 0100) | (PrevWord & ~0100));
            break;
        case (0177662 >> 1):
            Device_Data.SysRegs.WrReg177662 = (uint16_t) Word;
            // TODO: if 0011 ?
            graphics_set_page(Word & 0100000 ? CPU_PAGE61_MEM_ADR : CPU_PAGE51_MEM_ADR, (Word >> 8) & 15);
            break;
        case (0177664 >> 1):
            PrevWord = Device_Data.SysRegs.Reg177664;
            Word = ((Word & 01377) | (PrevWord & ~01377));
            Device_Data.SysRegs.Reg177664 = (uint16_t) Word;
            graphics_shift_screen(Word);
            break;
        case (0177706 >> 1):
            CPU_TimerRun ();
            Device_Data.SysRegs.Reg177706 = (uint16_t) Word;
            break;
//      case (0177710 >> 1):
        case (0177712 >> 1):
            Word |= 0xFF00U;
            Device_Data.SysRegs.Reg177712 = (uint16_t) Word;
            Device_Data.SysRegs.Reg177710 = Device_Data.SysRegs.Reg177706;
            Device_Data.Timer.PrevT = Device_Data.CPU_State.Time >> 7;
            Device_Data.Timer.T     = 0;
            Device_Data.Timer.Div   = (~Word >> 4) & 6;
            break;
        case (0177714 >> 1):
            Device_Data.SysRegs.WrReg177714 = (uint16_t) Word;
            {
                uint_fast32_t Reg = (Word & 0xFF) >> 1;
                if (Device_Data.SysRegs.WrReg177716 & 0100) Reg += 0x80;
#ifdef COVOX
                true_covox = Device_Data.SysRegs.WrReg177716;
#endif
#ifdef AYSOUND
				if (Adr & 1) {
					Word >>= 8; // работаем со старшим байтом
				}
                AY_set_reg(Word & 0377);
#endif
            }
            break;
        case (0177716 >> 1):
            if (Word & (1U << 11) && is_bk0011mode()) {
                DBGM_PRINT(("CPU_WriteB -> select_11_page(0%o) ", Word));
                select_11_page((uint16_t) Word);
            }
            else {
                Device_Data.SysRegs.WrReg177716 = (uint16_t) Word;
                {
                    uint_fast32_t Reg = *(uint8_t *) &Device_Data.SysRegs.WrReg177714 >> 1;
                    if (Word & 0100) Reg += 0x80;
                    /// ???
                    true_covox = Device_Data.SysRegs.WrReg177716;
                }
            }
            break;
        default:
            return CPU_ARG_WRITE_ERR;
    }
    return CPU_ARG_WRITE_OK;
}

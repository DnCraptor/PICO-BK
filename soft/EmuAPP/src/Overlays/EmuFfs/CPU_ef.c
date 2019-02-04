#include <stdlib.h> 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "gpio_lib.h"
#include "ets.h"
#include "esp8266.h"
#include "AnyMem.h"
#include "CPU.h"

#include "CPU_ef.h"

#define AT_OVL __attribute__((section(".ovl0_ef.text")))

/*
������

177706�-- ������� ���������� �������� �������. �������� �� ������ � ������.
177710�-- ����������� �������. �������� �� ������, ������ � ������� ������������.
177712�-- ��������������� ������-- ������� ����������.
(001)��� 0: STOP: "1" - ���������
��� ��������� ��������� ���� � ������������ � ������� �������� ��������� �� �������� ���������� �������� �������
(002)��� 1: WRAPAROUND: "1" - ����� ������������ �����, �������� �������� ����� EXPENABLE � ONESHOT
��������� �������� �������� �������� ����� 0. �������� �� ����, ������ ���������� ���������.
(004)��� 2: EXPENABLE: "1" - ���������� ��������� ������� EXPIRY ("����� �����")
��������� �������� ���������, ��� ��������� �������� ������� ����� 0 ��������������� � 1 ��� EXPIRY, ���� ����� �� ��� �������.
����� ���������, ��� ��� ������ (����� ��������� ��� ��� ���������� ������) ������� ������� � ������ ������ ���������
����������� ������ ����� ������� �������� �������� ����� 0, ������ ���������� �� ����, ������� �� ������ �� ����� �
������ �������
(010)��� 3: ONESHOT: ����� �������������
��� ��������� ��������� ��������� ����, ����� ������� ������� �� 0 ������������ ��� RUN. ��������� ������� ������ �� ��������
����� ��������� (��� EXPENABLE)
(020)��� 4: RUN: ������ ��������, ������ "1"-- ��������� ������� �� �������� 177706 � �������� ������
� ���� ������ ��� ������� �� ���� � ������� ������ ��������� ��������� �� �������� ���������� ��������, �������������
���� ������ ������ �� ��������� �� ���� (���� ������ �� ��������� �������� �������� ����� 0 ����� WRAPAROUND). ��� ������
� 0 � ������� �������������� ��������� �������� �� �������� ���������� ��������
(040)��� 5: �������� �� 4, "1"-- �������
��������� ������� �������� ����� � 4 ���� (����� ��������� ������� �� 4)
(100)��� 6: �������� �� 16, "1"-- �������
��������� ������� �������� ����� � 16 ��� (����� ��������� ������� �� 16)
��� ������������� ��������� �������� ���������, �������������� � 64 ����
(200)��� 7: EXPIRY: ���� ��������� �����, ��������������� � "1" ��� ���������� ��������� ����, ������������ ������ ����������
���� 8-15 �� ������������, "1".
*/

void AT_OVL CPU_TimerRun (void)
{
    uint_fast16_t Cfg = Device_Data.SysRegs.Reg177712;

    //���� ������� ����������
    if (Cfg & 1)
    {
        Device_Data.SysRegs.Reg177710 = Device_Data.SysRegs.Reg177706; //����������������� ������� ��������
    }
    else if (Cfg & 020) //���� ������� �������
    {
        uint_fast32_t CurT = Device_Data.CPU_State.Time >> 7;
        uint_fast32_t T    = Device_Data.Timer.T + (((CurT - Device_Data.Timer.PrevT) & 0xFFFFFF) << Device_Data.Timer.Div);
        int_fast32_t  Cntr = Device_Data.SysRegs.Reg177710;

        Device_Data.Timer.PrevT = CurT;
        Device_Data.Timer.T  = T & 0x3F;

        T >>= 6;

        if (Cntr == 0 || (Cfg & 2)) //���� ������� ��� ��� ����� 0 ��� ����� WRAPAROUND
        {
            Cntr -= T;
        }
        else
        {
            Cntr -= T;

            if (Cntr <= 0)
            {
                uint_fast16_t InitVal = Device_Data.SysRegs.Reg177706;

                if (Cfg & 4) //���������� ��������� ������� "����� �����" ?
                {
                    Cfg |= 0200;    //��, ��������� ������
                }

                if (Cfg & 010) //���������� ����� �������������?
                {
                    Cfg &= ~020;    //����� ������� ��� 4

                    Cntr = InitVal; //����������������� ������� ��������
                }
                else if (InitVal)
                {
                    do
                    {
                        Cntr += InitVal; //����������������� ������� ��������

                    } while (Cntr <= 0);
                }

                Device_Data.SysRegs.Reg177712 = Cfg;
            }
        }

        Device_Data.SysRegs.Reg177710 = (uint16_t) Cntr;
    }
}

TCPU_Arg AT_OVL CPU_ReadMemW (TCPU_Arg Adr)
{
    uint16_t *pReg;

    if (Adr < 0177600) return AnyMem_r_u16 (CPU_GET_PAGE_ADR_MEM16 (Adr));

    switch (Adr >> 1)
    {
        case (0177660 >> 1): pReg = &Device_Data.SysRegs.Reg177660;   break;
        case (0177662 >> 1): Device_Data.SysRegs.Reg177660 &= ~0200;
                             pReg = &Device_Data.SysRegs.Reg177662;   break;
        case (0177664 >> 1): pReg = &Device_Data.SysRegs.Reg177664;   break;
        case (0177706 >> 1): pReg = &Device_Data.SysRegs.Reg177706;   break;
        case (0177710 >> 1): CPU_TimerRun ();
                             pReg = &Device_Data.SysRegs.Reg177710;   break;
        case (0177712 >> 1): CPU_TimerRun ();
                             pReg = &Device_Data.SysRegs.Reg177712;   break;
        case (0177714 >> 1): pReg = &Device_Data.SysRegs.RdReg177714; break;
        case (0177716 >> 1): pReg = &Device_Data.SysRegs.RdReg177716; break;

        default: return CPU_ARG_READ_ERR;
    }

    return *pReg;
}

TCPU_Arg AT_OVL CPU_ReadMemB (TCPU_Arg Adr)
{
    uint16_t *pReg;

    if (Adr < 0177600) return AnyMem_r_u8 (CPU_GET_PAGE_ADR_MEM8 (Adr));

    switch (Adr >> 1)
    {
        case (0177660 >> 1): pReg = &Device_Data.SysRegs.Reg177660;   break;
        case (0177662 >> 1): Device_Data.SysRegs.Reg177660 &= ~0200;
                             pReg = &Device_Data.SysRegs.Reg177662;   break;
        case (0177664 >> 1): pReg = &Device_Data.SysRegs.Reg177664;   break;
        case (0177706 >> 1): pReg = &Device_Data.SysRegs.Reg177706;   break;
        case (0177710 >> 1): CPU_TimerRun ();
                             pReg = &Device_Data.SysRegs.Reg177710;   break;
        case (0177712 >> 1): CPU_TimerRun ();
                             pReg = &Device_Data.SysRegs.Reg177712;   break;
        case (0177714 >> 1): pReg = &Device_Data.SysRegs.RdReg177714; break;
        case (0177716 >> 1): pReg = &Device_Data.SysRegs.RdReg177716; break;

        default: return CPU_ARG_READ_ERR;
    }

    return ((uint8_t *) pReg) [Adr & 1];
}

TCPU_Arg AT_OVL CPU_WriteW (TCPU_Arg Adr, uint_fast16_t Word)
{
    uint_fast16_t PrevWord;

    if (CPU_IS_ARG_REG (Adr))
    {
        Device_Data.CPU_State.r [CPU_GET_ARG_REG_INDEX (Adr)] = (uint16_t) Word;

        return CPU_ARG_WRITE_OK;
    }

    if (Adr < 0177600)
    {
        uint16_t *pMem = CPU_GET_PAGE_ADR_MEM16 (Adr);

        if ((uintptr_t) pMem >= 0x40200000UL) return CPU_ARG_WRITE_ERR;

        AnyMem_w_u16 (pMem, Word);

        return CPU_ARG_WRITE_OK;
    }

    switch (Adr >> 1)
    {
        case (0177660 >> 1):

            PrevWord = Device_Data.SysRegs.Reg177660;

            Device_Data.SysRegs.Reg177660 = (uint16_t) ((Word & 0100) | (PrevWord & ~0100));

            break;

//      case (0177662 >> 1):
            
        case (0177664 >> 1):

            PrevWord = Device_Data.SysRegs.Reg177664;

            Word = ((Word & 01377) | (PrevWord & ~01377));

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

            Device_Data.SysRegs.WrReg177714 = (uint16_t) Word;

            {
                uint_fast32_t Reg = (Word & 0xFF) >> 1;
                if (Device_Data.SysRegs.WrReg177716 & 0100) Reg += 0x80;
                WRITE_PERI_REG (GPIO_SIGMA_DELTA_ADDRESS,   SIGMA_DELTA_ENABLE
                                                          | (Reg << SIGMA_DELTA_TARGET_S)
                                                          | (1 << SIGMA_DELTA_PRESCALAR_S));
            }

            break;

        case (0177716 >> 1):

            Device_Data.SysRegs.WrReg177716 = (uint16_t) Word;

            {
                uint_fast32_t Reg = *(uint8_t *) &Device_Data.SysRegs.WrReg177714 >> 1;
                if (Word & 0100) Reg += 0x80;
                WRITE_PERI_REG (GPIO_SIGMA_DELTA_ADDRESS,   SIGMA_DELTA_ENABLE
                                                          | (Reg << SIGMA_DELTA_TARGET_S)
                                                          | (1 << SIGMA_DELTA_PRESCALAR_S));
            }

            break;

        default:

            return CPU_ARG_WRITE_ERR;
    }

    return CPU_ARG_WRITE_OK;
}

TCPU_Arg AT_OVL CPU_WriteB (TCPU_Arg Adr, uint_fast8_t Byte)
{
    uint_fast16_t Word;
    uint_fast16_t PrevWord;

    if (CPU_IS_ARG_REG (Adr))
    {
        *(uint8_t *) &Device_Data.CPU_State.r [CPU_GET_ARG_REG_INDEX (Adr)] = (uint8_t) Byte;

        return CPU_ARG_WRITE_OK;
    }

    if (Adr < 0177600)
    {
        uint8_t *pMem = CPU_GET_PAGE_ADR_MEM8 (Adr);

        if ((uintptr_t) pMem >= 0x40200000UL) return CPU_ARG_WRITE_ERR;

        AnyMem_w_u8 (pMem, Byte);

        return CPU_ARG_WRITE_OK;
    }

    Word = Byte;

    if (Adr & 1) Word <<= 8;

    switch (Adr >> 1)
    {
        case (0177660 >> 1):

            PrevWord = Device_Data.SysRegs.Reg177660;

            Device_Data.SysRegs.Reg177660 = (uint16_t) ((Word & 0100) | (PrevWord & ~0100));

            break;

//      case (0177662 >> 1):
            
        case (0177664 >> 1):

            PrevWord = Device_Data.SysRegs.Reg177664;

            Word = ((Word & 01377) | (PrevWord & ~01377));

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

            Device_Data.SysRegs.WrReg177714 = (uint16_t) Word;

            {
                uint_fast32_t Reg = (Word & 0xFF) >> 1;
                if (Device_Data.SysRegs.WrReg177716 & 0100) Reg += 0x80;
                WRITE_PERI_REG (GPIO_SIGMA_DELTA_ADDRESS,   SIGMA_DELTA_ENABLE
                                                          | (Reg << SIGMA_DELTA_TARGET_S)
                                                          | (1 << SIGMA_DELTA_PRESCALAR_S));
            }

            break;

        case (0177716 >> 1):

            Device_Data.SysRegs.WrReg177716 = (uint16_t) Word;

            {
                uint_fast32_t Reg = *(uint8_t *) &Device_Data.SysRegs.WrReg177714 >> 1;
                if (Word & 0100) Reg += 0x80;
                WRITE_PERI_REG (GPIO_SIGMA_DELTA_ADDRESS,   SIGMA_DELTA_ENABLE
                                                          | (Reg << SIGMA_DELTA_TARGET_S)
                                                          | (1 << SIGMA_DELTA_PRESCALAR_S));
            }

            break;

        default:

            return CPU_ARG_WRITE_ERR;
    }

    return CPU_ARG_WRITE_OK;
}

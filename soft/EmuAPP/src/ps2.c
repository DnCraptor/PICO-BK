#include <stdint.h>

#include "gpio_lib.h"
#include "timer0.h"
#include "ps2_bk.h"
#include "board.h"

Tps2_Data ps2_Data;

void gpio_int (void)
{
    uint32_t      dT;
    uint_fast16_t IntState;

    GPIO_REG_WRITE (GPIO_STATUS_W1TC_ADDRESS, GPIO_REG_READ (GPIO_STATUS_ADDRESS));
    
    {
        // �������� ����� �� �������� ����, ���� ��� ������� ������� - ������� ������� ������
        uint32_t T = getCycleCount ();

        dT                = T - ps2_Data.IntPrevT;
        ps2_Data.IntPrevT = T;
    }

    if (dT < (50 * 160)) return;  // ���� ����� ������ 50��� (�� ��������� 60-100���), �� ��� ������ (���������� ��)

    IntState = ps2_Data.IntState;

    if (IntState & PS2_INT_STATE_FLAG_TX)
    {
        if (IntState >= (10U << PS2_INT_STATE_BITN_POS))
        {
            // ��������� ACK - ��������� � �����
            if (gpio_in (PS2_DATA)) IntState |= PS2_INT_STATE_FLAG_TXACK;

            IntState &= ~(PS2_INT_STATE_FLAG_TX | PS2_INT_STATE_BITN_MASK);
        }
        else
        {
            if (IntState >= (9U << PS2_INT_STATE_BITN_POS))
            {
                // �������� 8 ��� ������ � 1 ��� ��������

                gpio_init_input_pu (PS2_DATA);

            } else
            {
                uint_fast16_t TxData = ps2_Data.TxData;

                // �� � ������ ��������
                if (TxData & 1) gpio_on  (PS2_DATA);
                else            gpio_off (PS2_DATA);

                ps2_Data.TxData = TxData >> 1;
            }

            IntState += 1U << PS2_INT_STATE_BITN_POS;
        }
    }
    else
    {
        uint_fast16_t RxData;
        // �� � ������ ������

        if (dT > (120 * 160)) IntState &= ~PS2_INT_STATE_BITN_MASK; // 120��� ������� - ���������� ��������

        // ��������� ���
        RxData = ps2_Data.RxData;

        if (gpio_in (PS2_DATA)) RxData |= 0x400;

        IntState += 1U << PS2_INT_STATE_BITN_POS;
       
        if (IntState < (11U << PS2_INT_STATE_BITN_POS)) // ��������� �� ����� �����
        {
            ps2_Data.RxData = RxData >> 1;
        }
        else
        {
            // ������� 11 ���
            if (((RxData & 0x001) == 0) && (RxData & 0x400))  // �������� ������� ����� � ���� �����
            {
                uint_fast16_t Code;
                // ������� ��������� ���
                RxData >>= 1;
                
                // �������� ���
                Code = RxData & 0xff;
                
                // ������� ��������
                RxData ^= RxData >> 4;
                RxData ^= RxData >> 2;
                RxData ^= RxData >> 1;
                RxData ^= RxData >> 8;
                
                if ((RxData & 1) == 0)
                {
                    // ��� ��������� !
                    if      (Code == 0xE0) IntState |= PS2_INT_STATE_FLAG_E0;
                    else if (Code == 0xE1) IntState |= PS2_INT_STATE_FLAG_E1;
                    else if (Code == 0xF0) IntState |= PS2_INT_STATE_FLAG_F0;
                    else if (Code == 0xFA) IntState |= PS2_INT_STATE_FLAG_ACK;
                    else if (Code == 0xFE) IntState |= PS2_INT_STATE_FLAG_RESEND;
                    else if (Code == 0xAA) IntState |= PS2_INT_STATE_FLAG_BAT;
                    else
                    {
                        uint_fast8_t iRxBufWr = ps2_Data.iRxBufWr & (PS2_RX_BUF_SIZE - 1);
                        // ����������� ������ � �������
                        Code |= (IntState & (PS2_INT_STATE_FLAG_E0 | PS2_INT_STATE_FLAG_E1 | PS2_INT_STATE_FLAG_F0)) << 8;
                        // ������ � �����
                        ps2_Data.RxBuf [iRxBufWr++] = (uint16_t) Code;

                        ps2_Data.iRxBufWr = (uint8_t) iRxBufWr;
                        
                        // ���������� �����
                        IntState &= ~(PS2_INT_STATE_FLAG_E0 | PS2_INT_STATE_FLAG_E1 | PS2_INT_STATE_FLAG_F0);
                    }
                }
            }
            
            // ���������� ��������
            IntState &= ~PS2_INT_STATE_BITN_MASK;
        }
    }

    ps2_Data.IntState = IntState;
}

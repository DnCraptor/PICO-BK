#include <stdint.h>

#include "ets.h"
#include "gpio_lib.h"
#include "timer0.h"
#include "ps2.h"
#include "ps2_eu.h"
#include "board.h"

#include "Debug.h"

#define AT_OVL __attribute__((section(".ovl1_eu.text")))

uint_fast16_t AT_OVL ps2_read (void)
{
    uint_fast8_t  iRxBufRd = ps2_Data.iRxBufRd & (PS2_RX_BUF_SIZE - 1);
    uint_fast16_t Ret;

    if (((iRxBufRd ^ ps2_Data.iRxBufWr) & (PS2_RX_BUF_SIZE - 1)) == 0) return 0;

    Ret = ps2_Data.RxBuf [iRxBufRd++];

    ps2_Data.iRxBufRd = (uint8_t) iRxBufRd;

    if (((ps2_Data.PrevCode & 0x3FF) == 0x214) && ((Ret & 0x3FF) == 0x77)) Ret = 0;

    ps2_Data.PrevCode = Ret;

    return Ret;
}

void AT_OVL ps2_leds (uint_fast8_t LedsNew)
{
    ps2_Data.LedsNew = LedsNew;
}

void AT_OVL ps2_periodic (void)
{
    uint_fast8_t State = ps2_Data.State;

    switch (State)
    {
        default:

            State = 0;

        case 0:

            ps2_Data.LedsLast = 0xFF;
            ps2_Data.TxByte   = 0xFF; // Отправляем команду "Reset"
            State++;
            break;

        case 1:
        case 7:
        case 12:

            gpio_off         (PS2_CLK); // PS2_CLK вниз
            gpio_init_output (PS2_CLK);
            ps2_Data.PrevT = getCycleCount ();
            State++;
            break;

        case 2:
        case 8:
        case 13:

            if ((getCycleCount () - ps2_Data.PrevT) < (100 * 160)) break;

            gpio_off         (PS2_DATA); // PS2_DATA вниз (старт бит)
            gpio_init_output (PS2_DATA);
            ps2_Data.PrevT = getCycleCount ();
            State++;
            break;

        case 3:
        case 9:
        case 14:

            if ((getCycleCount () - ps2_Data.PrevT) < (200 * 160)) break;

            {
                uint_fast8_t  Parity = ps2_Data.TxByte;
                uint_fast16_t TxData = Parity;

                Parity ^= Parity >> 4;
                Parity ^= Parity >> 2;
                Parity ^= Parity >> 1;

                if ((Parity & 1) == 0) TxData |= 0x100;

                ps2_Data.TxData = TxData;
            }
            {
                uint_fast8_t Mask = PS2_INT_STATE_FLAG_E0 | PS2_INT_STATE_FLAG_E1 | PS2_INT_STATE_FLAG_F0;

                if (State != 9) Mask |= PS2_INT_STATE_FLAG_BAT;

                MEMORY_BARRIOR ();
                ps2_Data.IntState = (uint8_t) ((ps2_Data.IntState & Mask) | PS2_INT_STATE_FLAG_TX);
                MEMORY_BARRIOR ();
            }
            gpio_init_input_pu (PS2_CLK); // Отпускаем PS2_CLK
            ps2_Data.PrevT = getCycleCount ();
            State++;
            break;

        case 4:
        case 10:
        case 15:

            if ((getCycleCount () - ps2_Data.PrevT) < (5000 * 160)) break; // Ждем завершения передачи

            if (ps2_Data.IntState & PS2_INT_STATE_FLAG_RESEND) State -= 3; // Если нужно повторяем отправку
            else                                               State++;
            break;

        case 5:

            if (ps2_Data.IntState & PS2_INT_STATE_FLAG_ACK) State++;
            else                                            State = 0; // Если нужно повторяем Reset
            break;

        case 6:

            if (((ps2_Data.LedsNew & 7) == ps2_Data.LedsLast) && ((ps2_Data.IntState & PS2_INT_STATE_FLAG_BAT) == 0)) break;

            ps2_Data.TxByte = 0xED; // Отправляем команду "Set/Reset LEDs"
            State++;
            break;

        case 11:

            ps2_Data.TxByte = ps2_Data.LedsNew & 7; // Отправляем состояние светодиодов
            if (ps2_Data.IntState & PS2_INT_STATE_FLAG_ACK) State++;
            else                                            State = 6; // Если нужно начинаем с начала
            break;

        case 16:

            if (ps2_Data.IntState & PS2_INT_STATE_FLAG_ACK) ps2_Data.LedsLast = ps2_Data.TxByte;
            State = 6;
            break;
    }

    ps2_Data.State = State;
}

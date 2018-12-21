#include "tv.h"

#include "timer0.h"
#include "gpio_lib.h"
#include "i2s.h"
#include "board.h"
#include "CPU.h"
#include "ui.h"

#define N_BUFS      8   // должно быть больше 4
#define T_DELAY_N   6

#define SYNC_PROGRAM_2_30   0UL
#define SYNC_PROGRAM_4_28   2UL
#define SYNC_PROGRAM_28_4   3UL

#define LINES_IN_FIELD1 8
#define SYNC_PROGRAM_FIELD1 ((SYNC_PROGRAM_4_28 << (30 -  0 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 -  1 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 -  2 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 -  3 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 -  4 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 -  5 * 2 )) |    \
                                                                        \
                             (SYNC_PROGRAM_28_4 << (30 -  6 * 2 )) |    \
                             (SYNC_PROGRAM_28_4 << (30 -  7 * 2 )) |    \
                             (SYNC_PROGRAM_28_4 << (30 -  8 * 2 )) |    \
                             (SYNC_PROGRAM_28_4 << (30 -  9 * 2 )) |    \
                             (SYNC_PROGRAM_28_4 << (30 - 10 * 2 )) |    \
                                                                        \
                             (SYNC_PROGRAM_2_30 << (30 - 11 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 - 12 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 - 13 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 - 14 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 - 15 * 2 ))   )


#define LINES_IN_FIELD2 7
#define SYNC_PROGRAM_FIELD2 ((SYNC_PROGRAM_2_30 << (30 -  0 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 -  1 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 -  2 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 -  3 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 -  4 * 2 )) |    \
                                                                        \
                             (SYNC_PROGRAM_28_4 << (30 -  5 * 2 )) |    \
                             (SYNC_PROGRAM_28_4 << (30 -  6 * 2 )) |    \
                             (SYNC_PROGRAM_28_4 << (30 -  7 * 2 )) |    \
                             (SYNC_PROGRAM_28_4 << (30 -  8 * 2 )) |    \
                             (SYNC_PROGRAM_28_4 << (30 -  9 * 2 )) |    \
                                                                        \
                             (SYNC_PROGRAM_2_30 << (30 - 10 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 - 11 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 - 12 * 2 )) |    \
                             (SYNC_PROGRAM_2_30 << (30 - 13 * 2 ))   )

/*

Обычная строка:

    54 бит синхро
    66 бит пауза минимум
    данные (599 бит)
    17 бит пауза минимум

Бит Слово.бит
  0     0. 0       54 бит синхро
 54     1.22      106 бит пауза
160     5. 0      512 бит данные
672    21. 0       64 бит пауза
736    23. 0

Кадровые синхро импульсы (полустроки)

    Вариант 1 (4_28):
  0     0. 0       54 бит синхро
 54     1.22      314 бит пауза
368    11.16       54 бит синхро
422    13. 6      314 бит пауза
736    23.00

    Вариант 2 (2_30):
  0     0. 0       27 бит синхро
 27     0.27      341 бит пауза
368    11.16       27 бит синхро
395    12.11      341 бит пауза
736    23.00

    Вариант 3 (28_4):
  0     0. 0      314 бит синхро
314     9.26       54 бит пауза
368    11.16      314 бит синхро
682    21.10       54 бит пауза
736    23.00

           2_30 (00)    4_28 (10)    28_4 (11)
[0]      = 0xFFFFFFE0   0xFFFFFFFF   0xFFFFFFFF
[1]      = 0x00000000   0xFFFFFC00   0xFFFFFFFF
[2..8]   = 0x00000000   0x00000000   0xFFFFFFFF
[9]      = 0x00000000   0x00000000   0xFFFFFFC0
[10]     = 0x00000000   0x00000000   0x00000000

[11]     = 0x0000FFFF   0x0000FFFF   0x0000FFFF
[12]     = 0xFFE00000   0xFFFFFFFF   0xFFFFFFFF
[13]     = 0x00000000   0xFC000000   0xFFFFFFFF
[14..20] = 0x00000000   0x00000000   0xFFFFFFFF
[21]     = 0x00000000   0x00000000   0xFFC00000
[22]     = 0x00000000   0x00000000   0x00000000

    pBuf [    10] = 0;  
    pBuf [    11] = 0x0000FFFF;
    pBuf [    22] = 0;  

    U32 = 0xFFFFFFFF;

    if (SyncProgram >= 0) U32 <<=  5; pBuf [     0] = U32;
    if (SyncProgram >= 0) U32   =  0;                             SyncProgram <<= 1;
    if (SyncProgram >= 0) U32 <<= 10; pBuf [     1] = U32;
    if (SyncProgram >= 0) U32   =  0; pBuf [ 2...8] = U32;
                          U32 <<=  6; pBuf [     9] = U32;        SyncProgram <<= 1;
    U32 = 0xFFFFFFFF;

    if (SyncProgram >= 0) U32 <<= 21; pBuf [    12] = U32;        SyncProgram <<= 1;
    if (SyncProgram >= 0) U32 <<= 26; pBuf [    13] = U32;
    if (SyncProgram >= 0) U32   =  0; pBuf [14..20] = U32;
                          U32 <<= 22; pBuf [    21] = U32;        SyncProgram <<= 1;

*/

static struct
{
    uint8_t  iTdelay;
    uint8_t  iBuf;
    uint8_t  SyncFieldLine;
    uint8_t  BkLine;
    uint8_t  MenuY;
    uint8_t  ShowMenu;
    uint16_t MinLine;
    uint16_t Line;
    int32_t  SyncProgram;

    volatile uint8_t  TimerState;
    volatile uint32_t TimerToff;

    uint32_t Tdelay [T_DELAY_N]; // линия задержки синхронизации (т.к. I2S имеет задержку при выводе)
    uint32_t Buf    [N_BUFS] [23];

} TV_Data;

static void t0_int(void)
{
    if (TV_Data.TimerState)
    {
        // Отключаем синхру

        gpio_off     (TV_SYNC);
        timer0_write (getCycleCount () + 100 * 160);
    }
    else
    {
        // Включаем синхронизацию

        gpio_on      (TV_SYNC);
        timer0_write (TV_Data.TimerToff);

        TV_Data.TimerState = 1;
    }

}

static const uint32_t* tv_i2s_cb(void)
{
    uint32_t   *pBuf;
    uint32_t   *T;

    // Настраиваем таймер на отключение синхры
    {
        uint_fast8_t iTdelay = TV_Data.iTdelay + 1;

        if (iTdelay >= T_DELAY_N) iTdelay = 0;
        TV_Data.iTdelay = (uint8_t) iTdelay;
        T = &TV_Data.Tdelay [iTdelay];

        {
            uint32_t CurT     = getCycleCount ();
            uint32_t TimerTon = CurT + 160 * 30;

            TV_Data.TimerToff  = TimerTon + (*T);
            timer0_write (TimerTon);

            TV_Data.TimerState = 0;
        }
    }

    // Берем следующий буфер

    pBuf = TV_Data.Buf [(TV_Data.iBuf++) & (N_BUFS - 1)];

    {
        uint_fast16_t Line = TV_Data.Line;

        if (Line)
        {
            // Идет графика

            pBuf [ 0] = 0xFFFFFFFFUL;
            pBuf [ 1] = 0xFFFFFC00UL;
            pBuf [ 2] = 0x00000000UL;
            pBuf [ 3] = 0x00000000UL;
            pBuf [ 4] = 0x00000000UL;
            pBuf [21] = 0x00000000UL;

            TV_Data.Line = (uint16_t) (Line - 1);

            (*T) = 8 * 160; // 2+4+2 мкс - строчная синхра

            if ((Line < TV_Data.MinLine) || (Line >= 275))
            {
                uint_fast8_t Count;

                // Пустые строки в начале и в конце кадра

                for (Count = 5; Count <= 20; Count++) pBuf [Count] = 0;

            }
            else
            {
                // Рисуем строку
                uint32_t     *pTempBuf = &pBuf [5];
                uint_fast8_t BkLine    = TV_Data.BkLine;
                uint_fast8_t Count;

				if (TV_Data.ShowMenu)
            	{
            		uint_fast8_t y       =  TV_Data.MenuY;
	                char         *pText  =  UI_Data.scr [y];
	                uint8_t      *pZkg   = &MEM8 [CPU_ZKG_OFFSET + BkLine];
	                int_fast8_t  CursorX = -1;

	                if (y == UI_Data.CursorY) CursorX = UI_Data.CursorX;

            		if (++BkLine >= 10)
            		{
            			BkLine        = 0;
            			TV_Data.MenuY = (uint8_t) (y + 1);
            		}

	                TV_Data.BkLine = (uint8_t) BkLine;

            		for (Count = 0; Count < 16; Count++)
            		{
            			uint_fast32_t U32;

            			U32  = pZkg [(*pText++) * 10] << 16;
            			U32 |= pZkg [(*pText++) * 10];

            			if ((CursorX >> 1) == Count)
            			{
	            			if (CursorX & 1) U32 ^= 0x000000FFUL;
	            			else             U32 ^= 0x00FF0000UL;
            			}

            			U32 = (U32 & 0x000F000FUL) << 8 | ((U32 >> 4) & 0x000F000FUL);
            			U32 = (U32 & 0x03030303UL) << 4 | ((U32 >> 2) & 0x03030303UL);
            			U32 = (U32 & 0x11111111UL) << 2 | ((U32 >> 1) & 0x11111111UL);
						U32 |= U32 << 1;

                        *pTempBuf++ = (uint32_t) U32;
            		}
            	}
            	else
            	{
	                uint32_t *pVRam = &MEM32 [0x1000 + BkLine * 16];

	                TV_Data.BkLine = (uint8_t) (BkLine + 1);

                    for (Count = 0; Count < 16; Count++)
                    {
                        uint_fast32_t U32;

                        U32 = *pVRam++;

                        U32 = (U32 & 0x55555555UL) << 1 | ((U32 >> 1) & 0x55555555UL);
                        U32 = (U32 & 0x33333333UL) << 2 | ((U32 >> 2) & 0x33333333UL);
                        U32 = (U32 & 0x0F0F0F0FUL) << 4 | ((U32 >> 4) & 0x0F0F0F0FUL);
                        U32 = (U32 << 24) | ((U32 & 0xFF00U) << 8) | ((U32 >> 8) & 0xFF00U) | (U32 >> 24);

                        *pTempBuf++ = (uint32_t) U32;
                    }
            	}
            }

            return pBuf;
        }
    }

    // Идет синхронизация

    {
        uint_fast8_t  SyncFieldLine = TV_Data.SyncFieldLine;

        if (SyncFieldLine & 0x7F)
        {
            int32_t       SyncProgram = TV_Data.SyncProgram;
            uint_fast8_t  Count;
            uint_fast32_t U32;

            (*T)=68*160;    // 2+64+2 мкс - кадровая синхра

            TV_Data.SyncFieldLine = (uint8_t) (SyncFieldLine - 1);

            pBuf [10] = 0;  
            pBuf [11] = 0x0000FFFFUL;
            pBuf [22] = 0;  

            U32 = 0xFFFFFFFFUL;
            if (SyncProgram >= 0) U32 <<=  5;                                        pBuf [    0] = U32;
            if (SyncProgram >= 0) U32   =  0;                                                             SyncProgram <<= 1;
            if (SyncProgram >= 0) U32 <<= 10;                                        pBuf [    1] = U32;
            if (SyncProgram >= 0) U32   =  0; for (Count =  2; Count <=  8; Count++) pBuf [Count] = U32;
                                  U32 <<=  6;                                        pBuf [    9] = U32;  SyncProgram <<= 1;
            U32 = 0xFFFFFFFFUL;
            if (SyncProgram >= 0) U32 <<= 21;                                        pBuf [   12] = U32;  SyncProgram <<= 1;
            if (SyncProgram >= 0) U32 <<= 26;                                        pBuf [   13] = U32;
            if (SyncProgram >= 0) U32   =  0; for (Count = 14; Count <= 20; Count++) pBuf [Count] = U32;
                                  U32 <<= 22;                                        pBuf [   21] = U32;  SyncProgram <<= 1;

            TV_Data.SyncProgram = SyncProgram;

            return pBuf;
        }

        // Конец синхронизации - даем пустую строку перед графикой

        (*T)=8*160; // 2+4+2 мкс - строчная синхра

        TV_Data.Line = 304;

        if (UI_Data.Show)
        {
        	TV_Data.ShowMenu = 1;
        	TV_Data.MinLine  = 25;
            TV_Data.BkLine   = 0;
            TV_Data.MenuY    = 0;
        }
        else
        {
            uint_fast16_t Reg = Device_Data.SysRegs.Reg177664;

            TV_Data.BkLine  =  Reg - 0330;

            if (Reg & 01000) TV_Data.MinLine = 19;
            else             TV_Data.MinLine = 19 + 192;

        	TV_Data.ShowMenu = 0;
        }

        // Меняем поле

        SyncFieldLine ^= 0x80;

        if (SyncFieldLine & 0x80)
        {
            // Field 2

            TV_Data.SyncFieldLine = SyncFieldLine + LINES_IN_FIELD2;
            TV_Data.SyncProgram   = SYNC_PROGRAM_FIELD2;

            pBuf [0] = 0xFFFFFFFFUL;
            pBuf [1] = 0xFFFFFC00UL;

        }
        else
        {
            // Field 1

            TV_Data.SyncFieldLine = SyncFieldLine + LINES_IN_FIELD1;
            TV_Data.SyncProgram   = SYNC_PROGRAM_FIELD1;

            pBuf [0] = 0xFFFFFFE0UL; // После сихронизации Field 2 первая строка с укороченным строчным синхроимпульсом
            pBuf [1] = 0x00000000UL;
        }

        {
            uint_fast8_t Count;

            for (Count = 2; Count <= 21; Count++) pBuf [Count] = 0;
        }
    }

    return pBuf;
}


void OVL_SEC (tv_init) tv_init (void)
{
    // Инициализируем неизменяемую часть буферов
    {
        uint_fast8_t Count;

        for (Count = 0; Count < N_BUFS; Count++) TV_Data.Buf [Count] [22] = 0;
    }

    // Инитим порт SYNC

    gpio_init_output (TV_SYNC);
}


void OVL_SEC (tv_start) tv_start(void)
{
    // Настраиваем прерывание по таймеру

    timer0_isr_init();
    timer0_attachInterrupt(t0_int);
    
    // Запускаем I2S

    i2s_init(tv_i2s_cb, 92);
    i2s_start();
}

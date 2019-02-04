#include <stdint.h>
#include "gpio_lib.h"
#include "timer0.h"
#include "tv.h"
#include "board.h"

#include "tv_i.h"
#include "i2s_i.h"

#define AT_OVL __attribute__((section(".ovl3_i.text")))

void AT_OVL tv_init (void)
{
    // Инициализируем неизменяемую часть буферов
    {
        uint_fast8_t Count;

        for (Count = 0; Count < TV_N_BUFS; Count++) TV_Data.Buf [Count] [22] = 0;
    }

    // Инитим порт SYNC

    gpio_init_output (TV_SYNC);
}


void AT_OVL tv_start (void)
{
    // Настраиваем прерывание по таймеру

    ETS_INTR_ENABLE (ETS_CCOMPARE0_INUM);

    // Запускаем I2S

    i2s_init  (92);
    i2s_start ();
}

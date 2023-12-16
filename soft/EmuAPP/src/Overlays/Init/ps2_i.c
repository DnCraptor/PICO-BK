#include "gpio_lib.h"
#include "ps2_bk.h"
#include "ps2_i.h"
#include "board.h"

#define AT_OVL __attribute__((section(".ovl3_i.text")))

void AT_OVL ps2_init (void)
{
    // Переключаем PS2_DATA и PS2_CLK в GPIO
    gpio_init_input_pu (PS2_DATA);
    gpio_init_input_pu (PS2_CLK);

    // Настраиваем прерывание по низкому фронту на PS2_CLK
    gpio_pin_intr_state_set (PS2_CLK, GPIO_PIN_INTR_NEGEDGE);

    // Настраиваем прерывание по GPIO
    ETS_GPIO_INTR_ENABLE ();
}

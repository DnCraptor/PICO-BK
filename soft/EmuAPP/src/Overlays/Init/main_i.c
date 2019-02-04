#include <stdint.h>

#include "pin_mux_register.h"

#include "CPU_i.h"
#include "i2s_i.h"
#include "main_i.h"
#include "spi_flash_i.h"
#include "tv_i.h"
#include "ps2_i.h"
#include "ffs_i.h"

#define AT_OVL __attribute__((section(".ovl3_i.text")))

extern uint32_t _bss_start[], _bss_end[], VecBase;

void AT_OVL main_init (void)
{
    if(rom_i2c_readReg(103,4,1) != 136) // 8: 40MHz, 136: 26MHz
    {
        //if(get_sys_const(sys_const_crystal_26m_en) == 1) // soc_param0: 0: 40MHz, 1: 26MHz, 2: 24MHz
        {
            // set 80MHz PLL CPU
            rom_i2c_writeReg(103,4,1,0x88);
            rom_i2c_writeReg(103,4,2,0x91);
        }
    }
    
    // Переключаемся на 160МГц
    REG_SET_BIT(0x3ff00014, BIT(0));
    ets_update_cpu_frequency(160);
    // Инитим UART на 115200
    uart_div_modify(0, 80000000/115200);
    
    // Инитим BSS
    uint8_t *ptr=(uint8_t*)_bss_start;
    while (ptr < (uint8_t*)_bss_end)
    (*ptr++)=0;
    
     ets_isr_mask(0x3FE); // запретить прерывания 1..9

    // Меняем менеджер прерываний на свой, реетерабельный с приоритетами. Прерывания в порядке убывания приоритета: Timer, I2S, GPIO.
    __asm__ __volatile__ ("wsr.vecbase %0; rsync" : : "a" (&VecBase));

    spi_flash_Init ();

    // Инитим файловую систему
    ffs_init ();

    // Инитим экран
    tv_init  ();
    tv_start ();
    // Инитим клавиатуру
    ps2_init ();
    
    // Инитим процессор
    CPU_Init ();
}

#include <stdint.h>

#include "pin_mux_register.h"
#include "esp8266.h"

#include "CPU_i.h"
#include "i2s_i.h"
#include "main_i.h"
#include "spi_flash_i.h"
#include "tv_i.h"
#include "ps2_i.h"
#include "ffs_i.h"

#include "Debug.h"

#define AT_OVL __attribute__((section(".ovl3_i.text")))

extern uint32_t VecBase;

void AT_OVL main_init (void)
{
    while ((ESP8266_UART0->STATUS >> 16) & 0xFF);

    if(rom_i2c_readReg(103,4,1) != 136) // 8: 40MHz, 136: 26MHz
    {
        //if(get_sys_const(sys_const_crystal_26m_en) == 1) // soc_param0: 0: 40MHz, 1: 26MHz, 2: 24MHz
        {
            // set 80MHz PLL CPU
            rom_i2c_writeReg(103,4,1,0x88);
            rom_i2c_writeReg(103,4,2,0x91);
        }
    }
    
    // ������������� �� 160���
    REG_SET_BIT(0x3ff00014, BIT(0));
    ets_update_cpu_frequency(160);
    // ������ UART �� 115200
    uart_div_modify(0, 80000000/115200);

    ets_isr_mask(0x3FE); // ��������� ���������� 1..9

    // ������ �������� ���������� �� ����, �������������� � ������������. ���������� � ������� �������� ����������: Timer, I2S, GPIO.
  //  __asm__ __volatile__ ("wsr.vecbase %0; rsync" : : "a" (&VecBase));

//  spi_flash_Init ();

    // ������ �������� �������
    ffs_init ();

#ifndef DEBUG
    // ������ �����
    tv_init  ();

    tv_start ();

#endif

    // ������ ����������
    ps2_init ();
    
    // ������ ���������
    CPU_Init ();
}

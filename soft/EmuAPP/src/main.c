#include "main.h"
#include "ets.h"
#include "tv.h"
#include "ps2.h"
#include "ffs.h"
#include "CPU.h"
#include "esp8266.h"
#include "pin_mux_register.h"

void OVL_SEC (main_init) main_init (void)
{
    Wait_SPI_Idle (flashchip);

	ESP8266_SPI0->USER |= 0x20; // +1 такт перед CS = 0x80000064
	SET_PERI_REG_MASK (PERIPHS_IO_MUX_CONF_U, SPI0_CLK_EQU_SYS_CLK); // QSPI = 80 MHz
	ESP8266_SPI0->CTRL = (ESP8266_SPI0->CTRL & ~0x1FFF) | 0x1000;

    Wait_SPI_Idle (flashchip);

    // Инитим файловую систему
    OVL_CALLV0 (OVL_NUM (main_init), ffs_init);
    
    // Инитим экран
    tv_init  ();
    tv_start ();
    // Инитим клавиатуру
    ps2_init ();
    
    // Инитим процессор
    CPU_Init ();

    Wait_SPI_Idle (flashchip);
}

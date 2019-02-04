#include "esp8266.h"
#include "spi_flash.h"
#include "pin_mux_register.h"
#include "ets.h"

#include "spi_flash_i.h"

#define AT_OVL __attribute__((section(".ovl3_i.text")))

void AT_OVL spi_flash_Init (void)
{
    Wait_SPI_Idle (&spi_flash_ChipData);

    ESP8266_SPI0->USER |= 0x20; // +1 такт перед CS = 0x80000064
    SET_PERI_REG_MASK (PERIPHS_IO_MUX_CONF_U, SPI0_CLK_EQU_SYS_CLK); // QSPI = 80 MHz
    ESP8266_SPI0->CTRL = (ESP8266_SPI0->CTRL & ~0x1FFF) | 0x1000;

    Wait_SPI_Idle (&spi_flash_ChipData);
}

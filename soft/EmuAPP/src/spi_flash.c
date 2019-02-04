#include <stdint.h>
#include "spi_flash.h"

SpiFlashChip spi_flash_ChipData;

SpiFlashOpResult spi_flash_Read (uint32_t faddr, uint32_t *dst, uint32_t size)
{
    return SPI_read_data (&spi_flash_ChipData, faddr, dst, size);
}

extern SpiFlashChip *flashchip;

void __attribute__((section(".startup.text"))) spi_flash_Startup (void)
{
    spi_flash_ChipData = *flashchip;
}

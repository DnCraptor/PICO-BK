#include <stdint.h>
#include <stddef.h>
#include "spi_flash.h"

uint32_t OVL_SEC (spi_flash_unlock) spi_flash_unlock (void)
{
    uint32_t Status;

    if (SPI_read_status (flashchip, &Status) != 0 || Status == 0) return 1;

    Wait_SPI_Idle        (flashchip);
    if (SPI_write_enable (flashchip)         != 0) return 0;
    if (SPI_write_status (flashchip, Status) != 0) return 0;
    Wait_SPI_Idle        (flashchip);

    return 1;
}

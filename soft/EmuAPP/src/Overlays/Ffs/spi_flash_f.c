#include <stdint.h>
#include "spi_flash.h"

#include "spi_flash_f.h"

#define AT_OVL __attribute__((section(".ovl3_f.text")))

uint32_t AT_OVL spi_flash_Unlock (void)
{
    uint32_t Status;

    if (SPI_read_status  (&spi_flash_ChipData, &Status) != 0 || Status == 0) return SPI_FLASH_RESULT_OK;
    Wait_SPI_Idle        (&spi_flash_ChipData);
    if (SPI_write_enable (&spi_flash_ChipData)          != 0)                return SPI_FLASH_RESULT_ERR;
    if (SPI_write_status (&spi_flash_ChipData,  Status) != 0)                return SPI_FLASH_RESULT_ERR;
    Wait_SPI_Idle        (&spi_flash_ChipData);

    return SPI_FLASH_RESULT_OK;
}

SpiFlashOpResult AT_OVL spi_flash_Write (uint32_t des_addr, const uint32_t *src_addr, uint32_t size)
{
    uint32_t PageSize = spi_flash_ChipData.page_size;

    if (des_addr + size > spi_flash_ChipData.chip_size) return SPI_FLASH_RESULT_ERR;

    while (size)
    {
        uint32_t TempSize;

        TempSize = PageSize - (des_addr % PageSize);

        if (TempSize > size) TempSize = size;


        if (SPI_page_program (&spi_flash_ChipData, des_addr, src_addr, TempSize))
        {
            Wait_SPI_Idle (&spi_flash_ChipData);

            return SPI_FLASH_RESULT_ERR;
        }

        src_addr  = (uint32_t *) ((uintptr_t) src_addr + TempSize);
        des_addr += TempSize;
        size     -= TempSize;
    }

    Wait_SPI_Idle (&spi_flash_ChipData);

    return SPI_FLASH_RESULT_OK;
}

SpiFlashOpResult AT_OVL spi_flash_EraseSector (uint32_t sectornum)
{
    if (spi_flash_Unlock () != SPI_FLASH_RESULT_OK)                                                                goto ErrExit;
    if (sectornum > spi_flash_ChipData.chip_size / spi_flash_ChipData.sector_size)                                 goto ErrExit;
    if (SPI_write_enable (&spi_flash_ChipData) != SPI_FLASH_RESULT_OK)                                             goto ErrExit;
    if (SPI_sector_erase (&spi_flash_ChipData, spi_flash_ChipData.sector_size * sectornum) != SPI_FLASH_RESULT_OK) goto ErrExit;

    Wait_SPI_Idle (&spi_flash_ChipData);

    return SPI_FLASH_RESULT_OK;

ErrExit:

    Wait_SPI_Idle (&spi_flash_ChipData);

    return SPI_FLASH_RESULT_ERR;
}

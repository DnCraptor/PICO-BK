#ifndef SPI_FLASH_I_H
#define SPI_FLASH_I_H

#include <stdint.h>
#include "spi_flash.h"

void             spi_flash_Init          (void);
SpiFlashOpResult spi_flash_unlock_i      (void);
SpiFlashOpResult spi_flash_Write_i       (uint32_t des_addr, const uint32_t *src_addr, uint32_t size);
SpiFlashOpResult spi_flash_EraseSector_i (uint32_t sectornum);

#endif

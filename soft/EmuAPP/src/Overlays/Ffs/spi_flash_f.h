#ifndef SPI_FLASH_F_H_INCLUDE
#define SPI_FLASH_F_H_INCLUDE

#include <stdint.h>
#include "spi_flash.h"

SpiFlashOpResult spi_flash_unlock       (void);
SpiFlashOpResult spi_flash_Write        (uint32_t des_addr, const uint32_t *src_addr, uint32_t size);
SpiFlashOpResult spi_flash_EraseSector  (uint32_t sectornum);

#endif

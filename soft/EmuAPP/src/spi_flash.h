#ifndef SPI_FLASH_H_INCLUDE
#define SPI_FLASH_H_INCLUDE

#include <inttypes.h>
#include "ovl.h"
#include "ets.h"

#define spi_flash_unlock_ovln OVL_NUM_FS
#define spi_flash_unlock_ovls OVL_SEC_FS
uint32_t spi_flash_unlock (void);

#endif

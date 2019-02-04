#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include <stdint.h>

typedef struct
{
    uint32_t    deviceId;       //+00
    uint32_t    chip_size;      //+04 chip size in byte
    uint32_t    block_size;     //+08
    uint32_t    sector_size;    //+0c
    uint32_t    page_size;      //+10
    uint32_t    status_mask;    //+14

} SpiFlashChip;

typedef enum
{
    SPI_FLASH_RESULT_OK,
    SPI_FLASH_RESULT_ERR,
    SPI_FLASH_RESULT_TIMEOUT

} SpiFlashOpResult;

extern SpiFlashOpResult SPI_read_status  (SpiFlashChip *sflashchip, uint32_t *sta);
extern SpiFlashOpResult SPI_write_status (SpiFlashChip *sflashchip, uint32_t  sta);
extern SpiFlashOpResult SPI_write_enable (SpiFlashChip *sflashchip);
extern SpiFlashOpResult Wait_SPI_Idle    (SpiFlashChip *sflashchip);
extern SpiFlashOpResult SPI_read_data    (SpiFlashChip *sflashchip, uint32_t src_addr,        uint32_t *dest_addr, uint32_t size);
extern SpiFlashOpResult SPI_page_program (SpiFlashChip *sflashchip, uint32_t dest_addr, const uint32_t *src_addr,  uint32_t size);
extern SpiFlashOpResult SPI_sector_erase (SpiFlashChip *sflashchip, uint32_t addr);


//extern uint32_t         SPIRead          (uint32_t addr, void *outptr, uint32_t len);
//extern uint32_t         SPIEraseSector   (int);
//extern uint32_t         SPIWrite         (uint32_t addr, const void *inptr, uint32_t len);

extern SpiFlashChip spi_flash_ChipData;

SpiFlashOpResult spi_flash_Read (uint32_t faddr, uint32_t *dst, uint32_t size);

#endif

#include "spi_flash.h"

extern uint8_t      _bss_start;
extern uint8_t      _bss_end;
extern uint32_t     _stack_limit;
extern uint32_t     _stack_init;
extern SpiFlashChip *flashchip;

void __attribute__((section(".startup.text"))) Startup_LoInit (void)
{
    {
        // Инитим BSS
        uint8_t *ptr = &_bss_start;

        while (ptr < &_bss_end) (*ptr++) = 0;
    }

    {
        // Инитим стэк
        uint32_t *ptr = &_stack_limit;

        while (ptr < &_stack_init) (*ptr++) = 0x55A5A5AA;
    }

    spi_flash_ChipData = *flashchip;
}
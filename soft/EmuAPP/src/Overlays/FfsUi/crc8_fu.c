#include "AnyMem.h"

#include "crc8_fu.h"

#define AT_OVL __attribute__((section(".ovl2_fu.text")))

uint8_t AT_OVL CRC8 (uint8_t crc, const uint8_t *data, uint8_t len)
{
    while (len--)
    {
        uint8_t i;
    
        crc ^= AnyMem_r_u8 (data++);
        for (i = 0; i < 8; i++) crc = (crc << 1) ^ ((crc & 0x80) ? 0x2F : 0x00);
    }
    
    return crc;
}

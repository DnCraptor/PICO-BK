#include "AnyMem.h"

#include "crc8_i.h"

#define AT_OVL __attribute__((section(".ovl3_i.text")))

uint8_t AT_OVL CRC8_i (uint8_t crc, const uint8_t *data, uint8_t len)
{
    while (len--)
    {
        uint8_t i;
    
        crc ^= AnyMem_r_u8 (data++);
        for (i = 0; i < 8; i++) crc = (crc << 1) ^ ((crc & 0x80) ? 0x2F : 0x00);
    }
    
    return crc;
}

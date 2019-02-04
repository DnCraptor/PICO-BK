#ifndef ESP8266_H
#define ESP8266_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t CMD;
    volatile uint32_t ADDR;
    volatile uint32_t CTRL;
    volatile uint32_t CTRL1;
    volatile uint32_t RD_STATUS;
    volatile uint32_t CTRL2;
    volatile uint32_t CLOCK;
    volatile uint32_t USER;
    volatile uint32_t USER1;
    volatile uint32_t USER2;
    volatile uint32_t WR_STATUS;
    volatile uint32_t PIN;
    volatile uint32_t SLAVE;
    volatile uint32_t SLAVE1;
    volatile uint32_t SLAVE2;
    volatile uint32_t SLAVE3;
    volatile uint32_t FIFO [16];

    uint32_t Empty [28];

    volatile uint32_t EXT0;
    volatile uint32_t EXT1;
    volatile uint32_t EXT2;
    volatile uint32_t EXT3;

} TESP8266_SPI;

#define ESP8266_SPI0 ((TESP8266_SPI *) 0x60000200UL)

#endif

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

typedef struct
{
    volatile uint32_t FIFO;
    volatile uint32_t INT_RAW;
    volatile uint32_t INT_ST;
    volatile uint32_t INT_ENA;
    volatile uint32_t INT_CLR;
    volatile uint32_t CLKDIV;
    volatile uint32_t AUTOBAUD;
    volatile uint32_t STATUS;
    volatile uint32_t CONF0;
    volatile uint32_t CONF1;
    volatile uint32_t LOWPULSE;
    volatile uint32_t HIGHPULSE;
    volatile uint32_t PULSE_NUM;

    uint32_t Empty [17];

    volatile uint32_t DATE;
    volatile uint32_t ID;

} TESP8266_UART;

#define ESP8266_UART0 ((TESP8266_UART *) 0x60000000UL)
#define ESP8266_UART1 ((TESP8266_UART *) 0x60000F00UL)
#define ESP8266_SPI0  ((TESP8266_SPI  *) 0x60000200UL)

#endif

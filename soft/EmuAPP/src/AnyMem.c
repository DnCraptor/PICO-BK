#include "AnyMem.h"
#include "spi_flash.h"

#include "ram_page.h"
extern uint8_t RAM[RAM_SIZE];

uint8_t  AnyMem_r_u8  (const uint8_t  *Adr) {return *(uint8_t  *) RAM[*Adr];}
uint16_t AnyMem_r_u16 (const uint16_t *Adr) {return *(uint16_t *) RAM[*Adr];}
uint32_t AnyMem_r_u32 (const uint32_t *Adr) {return *(uint32_t *) RAM[*Adr];}
void     AnyMem_w_u8  (uint8_t  *Adr, uint_fast8_t  Data) {*(uint8_t  *) RAM[*Adr] = (uint8_t ) Data;}
void     AnyMem_w_u16 (uint16_t *Adr, uint_fast16_t Data) {*(uint16_t *) RAM[*Adr] = (uint16_t) Data;}
void     AnyMem_w_u32 (uint32_t *Adr, uint_fast32_t Data) {*(uint32_t *) RAM[*Adr] = (uint32_t) Data;}

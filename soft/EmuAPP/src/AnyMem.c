#include "AnyMem.h"

#include "ram_page.h"
extern uint8_t RAM[RAM_SIZE];

uint8_t  AnyMem_r_u8  (const uint8_t  *Adr) {return *Adr;}
uint16_t AnyMem_r_u16 (const uint16_t *Adr) {return *Adr;}
uint32_t AnyMem_r_u32 (const uint32_t *Adr) {return *Adr;}
void     AnyMem_w_u8  (uint8_t  *Adr, uint_fast8_t  Data) {*Adr = (uint8_t ) Data;}
void     AnyMem_w_u16 (uint16_t *Adr, uint_fast16_t Data) {*Adr = (uint16_t) Data;}
void     AnyMem_w_u32 (uint32_t *Adr, uint_fast32_t Data) {*Adr = (uint32_t) Data;}

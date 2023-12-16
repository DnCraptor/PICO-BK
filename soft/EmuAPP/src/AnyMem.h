#ifndef ANY_MEM_H
#define ANY_MEM_H

#include <stdint.h>

typedef struct
{
    uintptr_t Adr;
    uint32_t  Buf;
    uint32_t  FlashBuf [32 / 4];

} TAnyMem_Data;

extern TAnyMem_Data AnyMem_Data;

uint8_t  AnyMem_r_u8   (const uint8_t  *Adr);
uint16_t AnyMem_r_u16  (const uint16_t *Adr);
uint32_t AnyMem_r_u32  (const uint32_t *Adr);
void     AnyMem_w_u8   (      uint8_t  *Adr, uint_fast8_t  Data);
void     AnyMem_w_u16  (      uint16_t *Adr, uint_fast16_t Data);
void     AnyMem_w_u32  (      uint32_t *Adr, uint_fast32_t Data);

#define AnyMem_r_c(  Adr) ((         char) AnyMem_r_u8 ((uint8_t *) (Adr)))
#define AnyMem_r_uc( Adr) ((unsigned char) AnyMem_r_u8 ((uint8_t *) (Adr)))
#define AnyMem_w_c(  Adr, Data)            AnyMem_w_u8 ((uint8_t *) (Adr), (uint_fast8_t) (Data))
#define AnyMem_w_uc( Adr, Data)            AnyMem_w_u8 ((uint8_t *) (Adr), (uint_fast8_t) (Data))

#endif

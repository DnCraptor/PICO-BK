#include "AnyMem.h"
#include "spi_flash.h"

static struct
{
    uintptr_t Adr;
    uint32_t  Buf;
    uint32_t  FlashBuf [32 / 4];

} AnyMem_Data;


void *AnyMem_r_Buf (const void *Adr)
{
    uintptr_t Offset;

    if ((uintptr_t) Adr < 0x40200000UL)
    {
        Offset = (uintptr_t) Adr & 3;
        AnyMem_Data.Buf = *(uint32_t *) ((uintptr_t) Adr - Offset);
        return (void *) ((uintptr_t) &AnyMem_Data.Buf + Offset);
    }

    Offset = (uintptr_t) Adr - AnyMem_Data.Adr;

    if (Offset >= 32)
    {
        uintptr_t NewAdr;
        Offset = (uintptr_t) Adr & 3;
        NewAdr = (uintptr_t) Adr - Offset;
        AnyMem_Data.Adr = NewAdr;
        spi_flash_Read (NewAdr - 0x40200000UL, &AnyMem_Data.FlashBuf [0], sizeof (AnyMem_Data.FlashBuf));
    }

    return (void *) ((uintptr_t) &AnyMem_Data.FlashBuf [0] + Offset);
}

void AnyMem_w_Buf (void *Adr)
{
    if ((uintptr_t) Adr < 0x40200000UL)
    {
        *(uint32_t *) ((uintptr_t) Adr & ~3) = AnyMem_Data.Buf;
    }
}

uint8_t  AnyMem_r_u8  (const uint8_t  *Adr) {return *(uint8_t  *) AnyMem_r_Buf (Adr);}
uint16_t AnyMem_r_u16 (const uint16_t *Adr) {return *(uint16_t *) AnyMem_r_Buf (Adr);}
uint32_t AnyMem_r_u32 (const uint32_t *Adr) {return *(uint32_t *) AnyMem_r_Buf (Adr);}
void     AnyMem_w_u8  (uint8_t  *Adr, uint_fast8_t  Data) {*(uint8_t  *) AnyMem_r_Buf (Adr) = (uint8_t ) Data; AnyMem_w_Buf (Adr);}
void     AnyMem_w_u16 (uint16_t *Adr, uint_fast16_t Data) {*(uint16_t *) AnyMem_r_Buf (Adr) = (uint16_t) Data; AnyMem_w_Buf (Adr);}
void     AnyMem_w_u32 (uint32_t *Adr, uint_fast32_t Data) {*(uint32_t *) AnyMem_r_Buf (Adr) = (uint32_t) Data; AnyMem_w_Buf (Adr);}

void *AnyMem_memcpy (void *to, const void *from, uint32_t size)
{
    uint8_t       *to_   = to;
    const uint8_t *from_ = from;
    while (size--) AnyMem_w_u8 (to_++, AnyMem_r_u8 (from_++));
    return to;
}

char *AnyMem_strcpy (char *to, const char *from)
{
    char       *to_   = to;
    const char *from_ = from;
    char        Ch;
    while ((Ch = AnyMem_r_c (from_++)) != 0) AnyMem_w_c (to_++, Ch);
    return to;
}

void *AnyMem_memset (void *block, int c, uint32_t size)
{
    uint8_t *block_ = block;
    while (size--) AnyMem_w_u8 (block_++, c);
    return block;
}

#ifndef ANY_MEM_H
#define ANY_MEM_H

#include <stdint.h>

void    *AnyMem_r_Buf  (const void     *Adr);
void     AnyMem_w_Buf  (void           *Adr);
uint8_t  AnyMem_r_u8   (const uint8_t  *Adr);
uint16_t AnyMem_r_u16  (const uint16_t *Adr);
uint32_t AnyMem_r_u32  (const uint32_t *Adr);
void     AnyMem_w_u8   (      uint8_t  *Adr, uint_fast8_t  Data);
void     AnyMem_w_u16  (      uint16_t *Adr, uint_fast16_t Data);
void     AnyMem_w_u32  (      uint32_t *Adr, uint_fast32_t Data);
void    *AnyMem_memcpy (void *to, const void *from, uint32_t size);
char    *AnyMem_strcpy (char *to, const char *from);
void    *AnyMem_memset (void *block, int c, uint32_t size);

#define AnyMem_r_c(  Adr) ((         char) AnyMem_r_u8 ((uint8_t *) (Adr)))
#define AnyMem_r_uc( Adr) ((unsigned char) AnyMem_r_u8 ((uint8_t *) (Adr)))
#define AnyMem_w_c(  Adr, Data)            AnyMem_w_u8 ((uint8_t *) (Adr), (uint_fast8_t) (Data))
#define AnyMem_w_uc( Adr, Data)            AnyMem_w_u8 ((uint8_t *) (Adr), (uint_fast8_t) (Data))

#endif

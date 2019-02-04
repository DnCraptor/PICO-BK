#include "str_u.h"
#include "AnyMem.h"

#define AT_OVL __attribute__((section(".ovl3_u.text")))

int AT_OVL str_nlen (const char *s, int maxlen)
{
    int l=0;
    for ( ; AnyMem_r_c (s) && (maxlen > 0); maxlen--, l++, s++);
    return l;
}

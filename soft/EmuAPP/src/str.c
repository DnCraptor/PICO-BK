#include "str.h"

#include "ets.h"
#include "xprintf.h"

int to_upper(int c)
{
    if ( (c>='a') && (c<='z') )
	return (c-'a')+'A'; else
    if ((c >= 0xC0) && (c <= 0xDF)) return  c + 0x20; else  // Русские символы КОИ8-Р
	return c;
}

int OVL_SEC (strnlen) strnlen(const char *s, int maxlen)
{
    int l=0;
    for ( ; (*s) && (maxlen > 0); maxlen--, l++, s++);
    return l;
}

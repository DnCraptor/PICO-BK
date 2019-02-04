#include "str_fu.h"

#define AT_OVL __attribute__((section(".ovl2_fu.text")))

int AT_OVL to_upper (int c)
{
    if ( (c>='a') && (c<='z') )
    return (c-'a')+'A'; else
    if ((c >= 0xC0) && (c <= 0xDF)) return  c + 0x20; else  // Русские символы КОИ8-Р
    return c;
}

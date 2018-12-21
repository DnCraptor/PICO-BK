#ifndef XPRINTF_H
#define XPRINTF_H


#include <stdarg.h>
#include "ovl.h"


#ifdef __cplusplus
extern "C" {
#endif


#define xvsprintf_ovln OVL_NUM_UI
#define xvsprintf_ovls OVL_SEC_UI
int xvsprintf(char *buf, const char *fmt, va_list args);

#define xsprintf_ovln OVL_NUM_UI
#define xsprintf_ovls OVL_SEC_UI
int xsprintf(char *buf, const char *fmt, ...);


#ifdef __cplusplus
}
#endif


#endif

#ifndef XPRINTF_U_H
#define XPRINTF_U_H

#include <stdarg.h>

int xvsprintf(char *buf, const char *fmt, va_list args);
int xsprintf(char *buf, const char *fmt, ...);

#endif

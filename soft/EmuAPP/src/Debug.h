#ifndef DEBUG_H
#define DEBUG_H

//#define DEBUG

#ifdef DEBUG

    #include <stdarg.h>

    void Debug_Printf (const char *fmt, ...);

#else

    #define Debug_Printf( Args...)

#endif

#endif

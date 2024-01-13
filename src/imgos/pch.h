#pragma once
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define ASSERT(x)

inline static int max(int a, int b) {
	return a > b ? a : b;
}

inline static int min(int a, int b) {
	return a < b ? a : b;
}

inline static error_t strcpy_s(char* dst, size_t size, const char* src)
{
    if(!dst || !src) return EINVAL;

    for(;size > 0;--size)
    {
        if(!(*dst++ = *src++)) return 0;
    }

    return ERANGE;
}

#define _snwprintf(...) snprintf(__VA_ARGS__)

inline static error_t _tsplitpath_s(
        const char*   _FullPath,
        char*         _Drive, // out
        size_t        _DriveCount,
        char*         _Dir, // out
        size_t        _DirCount,
        char*         _Filename, // out
        size_t        _FilenameCount,
        char*         _Ext, // out
        size_t        _ExtCount
) {
    const size_t sz = strlen(_FullPath);
    size_t first_point = 0;
    size_t first_slash = 0;
    size_t last_slash = 0;
    size_t i = sz - 1;
    while(i--) {
        const char c = _FullPath[i];
        if (!first_point && c == '.') {
            first_point = i;
            continue;
        }
        bool slash = (c == '/' || c == '\\');
        if (!first_slash && slash) {
            first_slash = i;
            continue;
        }
        if (slash) {
            last_slash = i;
        }
    }
    if (first_point == 0) first_point = sz - 1;
	if (_DriveCount && _Drive) strncpy(_Drive, _FullPath, min(last_slash, _DriveCount));
    if (_DirCount && _Dir) strncpy(_Dir, _FullPath + last_slash + 1, min(first_slash - last_slash, _DirCount));
    if (_FilenameCount && _Filename) strncpy(_Filename, _FullPath + first_slash + 1, min(first_point - first_slash, _FilenameCount));
    if (_ExtCount && _Ext) strncpy(_Filename, _FullPath + first_point + 1, min(sz - first_point, _ExtCount));
    return 0;
}

inline static void gmtime_s(tm* pctm, const time_t* ptt) {
    tm * tmInfo = gmtime( ptt );
    *pctm = *tmInfo;
}


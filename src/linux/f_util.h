#ifndef ORANGE_BK_F_UTIL_H
#define ORANGE_BK_F_UTIL_H

/* FatFs result codes as text (the SD card driver's f_util.h). */

#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *FRESULT_str(FRESULT i);

#ifdef __cplusplus
}
#endif

#endif /* ORANGE_BK_F_UTIL_H */

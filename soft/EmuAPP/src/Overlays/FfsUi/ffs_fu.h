#ifndef FFS_FU_H
#define FFS_FU_H

#include <stdint.h>
#include "ffs.h"

extern FILE ffs_File;
extern char ffs_TempFileName [16+1];

void        ffs_GetFile (uint_fast16_t iFile);
const char* ffs_name    (uint_fast16_t n);   // × ffs_TempFileName

#endif

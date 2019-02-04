#ifndef EMU_EF_H
#define EMU_EF_H

#include <stddef.h>
#include "tv.h"

void emu_OnTv (void);

#define emu_OffTv() tv_SetOutFunc (NULL)

#endif

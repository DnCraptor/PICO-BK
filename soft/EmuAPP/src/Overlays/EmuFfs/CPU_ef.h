#ifndef CPU_EF_H_INCLUDE
#define CPU_EF_H_INCLUDE

#include <stdint.h>
#include "CPU.h"

void CPU_TimerRun (void);

TCPU_Arg CPU_ReadMemW (TCPU_Arg Adr);
TCPU_Arg CPU_ReadMemB (TCPU_Arg Adr);
TCPU_Arg CPU_WriteW   (TCPU_Arg Adr, uint_fast16_t Word);
TCPU_Arg CPU_WriteB   (TCPU_Arg Adr, uint_fast8_t  Byte);

#define CPU_WriteMemW CPU_WriteW
#define CPU_WriteMemB CPU_WriteB

#endif

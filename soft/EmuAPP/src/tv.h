#ifndef TV_H
#define TV_H

#include <stdint.h>

#define TV_N_BUFS   8   // должно быть больше 4
#define T_DELAY_N   6

typedef int (* Ttv_SetOutFunc) (uint32_t *pBuf, uint_fast16_t Line);

typedef struct
{
    uint8_t             iTdelay;
    uint8_t             iBuf;
    uint8_t             SyncFieldLine;
    volatile uint8_t    TimerState;
    uint16_t            Line;
    int32_t             SyncProgram;
    Ttv_SetOutFunc      pOutFunc;
    volatile uint32_t   TimerToff;

    uint32_t            Tdelay [T_DELAY_N]; // линия задержки синхронизации (т.к. I2S имеет задержку при выводе)
    uint32_t            Buf    [TV_N_BUFS] [23];

} TTV_Data;

extern TTV_Data TV_Data;

void            timer0_isr_handler (void);
const uint32_t* tv_i2s_cb          (void);

#define tv_SetOutFunc( pFunc) asm volatile ("" : : : "memory"); TV_Data.pOutFunc = (pFunc); asm volatile ("" : : : "memory")

#endif

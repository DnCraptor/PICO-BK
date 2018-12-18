#ifndef CPU_H_INCLUDE
#define CPU_H_INCLUDE

#include <stdint.h>

// #define DEBUG_PRINT( X) printf X
#define DEBUG_PRINT( X)

#define  CPU_ARG_REG        0x40000000UL
#define  CPU_ARG_FAULT      0x80000000UL
#define  CPU_ARG_WRITE_OK   0
#define  CPU_ARG_WRITE_ERR  CPU_ARG_FAULT
#define  CPU_ARG_READ_ERR   CPU_ARG_FAULT

#define  CPU_IS_ARG_FAULT(        Arg) (Arg &  CPU_ARG_FAULT)
#define  CPU_IS_ARG_REG(          Arg) (Arg &  CPU_ARG_REG)
#define  CPU_IS_ARG_FAULT_OR_REG( Arg) (Arg & (CPU_ARG_FAULT | CPU_ARG_REG))

typedef uint_fast32_t TCPU_Arg;
typedef uint_fast32_t TCPU_Psw;

#define CPU_FLAG_KEY_VECTOR_274 0x00000001UL
#define CPU_FLAG_KEY_VECTOR_60  0x00000002UL

typedef struct
{
    uint16_t r [8];
    TCPU_Psw psw;
    uint32_t Flags;
    uint32_t Time;

} TCPU_State;

typedef struct
{
    TCPU_State CPU_State;

    struct
    {
        uint32_t PrevT;
        uint8_t  T;
        uint8_t  Div;

    } Timer;

    struct
    {
        uint16_t Reg177660;
        uint16_t Reg177662;
        uint16_t Reg177664;
        uint16_t Reg177706;
        uint16_t Reg177710;
        uint16_t Reg177712;
        uint16_t RdReg177714;
        uint16_t WrReg177714;
        uint16_t RdReg177716;
        uint16_t WrReg177716;

    } SysRegs;

} TDevice_Data;

extern TDevice_Data Device_Data;

void CPU_Init           (void);
void CPU_RunInstruction (void);
void CPU_Stop           (void);
void CPU_TimerRun       (void);
char CPU_koi8_to_zkg    (char);

TCPU_Arg CPU_ReadMemW  (TCPU_Arg Adr);
TCPU_Arg CPU_ReadMemB  (TCPU_Arg Adr);
TCPU_Arg CPU_WriteW    (TCPU_Arg Adr, uint_fast16_t Word);
TCPU_Arg CPU_WriteB    (TCPU_Arg Adr, uint_fast8_t  Byte);

#define CPU_WriteMemW CPU_WriteW
#define CPU_WriteMemB CPU_WriteB

#define MEM8  ((uint8_t  *) 0x3FFE8000)
#define MEM16 ((uint16_t *) 0x3FFE8000)
#define MEM32 ((uint32_t *) 0x3FFE8000)

#define CPU_ZKG_OFFSET 0x94BE // (соответствует пробелу)

#endif

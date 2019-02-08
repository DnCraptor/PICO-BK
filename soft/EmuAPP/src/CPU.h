#ifndef CPU_H_INCLUDE
#define CPU_H_INCLUDE

#include <stdint.h>

// #define DEBUG_PRINT( X) printf X
#define DEBUG_PRINT( X)

#define CPU_PAGE0_MEM_ADR 0x3FFEC000
#define CPU_PAGE1_MEM_ADR 0x3FFF0000
#define CPU_PAGE2_MEM_ADR 0x40104000
#define CPU_PAGE3_MEM_ADR 0x40108000
#define CPU_PAGE4_MEM_ADR 0x4010С000
#define CPU_PAGE5_MEM_ADR 0x3FFFC000
#define CPU_PAGE6_MEM_ADR 0x3FFF4000
#define CPU_PAGE7_MEM_ADR 0x3FFF8000

#define CPU_PAGE0_MEM8  ((uint8_t  *) CPU_PAGE0_MEM_ADR)
#define CPU_PAGE0_MEM16 ((uint16_t *) CPU_PAGE0_MEM_ADR)
#define CPU_PAGE0_MEM32 ((uint32_t *) CPU_PAGE0_MEM_ADR)
#define CPU_PAGE1_MEM32 ((uint32_t *) CPU_PAGE1_MEM_ADR)
#define CPU_PAGE5_MEM32 ((uint32_t *) CPU_PAGE5_MEM_ADR)
#define CPU_PAGE6_MEM32 ((uint32_t *) CPU_PAGE6_MEM_ADR)

#define R     Device_Data.CPU_State.r
#define PSW   Device_Data.CPU_State.psw
#define SP    Device_Data.CPU_State.r [6]
#define PC    Device_Data.CPU_State.r [7]

#define  CPU_ARG_REG        0x40000000UL
#define  CPU_ARG_FAULT      0x80000000UL
#define  CPU_ARG_WRITE_OK   0
#define  CPU_ARG_READ_OK    0
#define  CPU_ARG_WRITE_ERR  CPU_ARG_FAULT
#define  CPU_ARG_READ_ERR   CPU_ARG_FAULT

#define  CPU_IS_ARG_FAULT(        Arg) (Arg &  CPU_ARG_FAULT)
#define  CPU_IS_ARG_REG(          Arg) (Arg &  CPU_ARG_REG)
#define  CPU_IS_ARG_FAULT_OR_REG( Arg) (Arg & (CPU_ARG_FAULT | CPU_ARG_REG))
#define  CPU_CHECK_ARG_FAULT(     Arg) {if (CPU_IS_ARG_FAULT (Arg)) goto BusFault;}

#define  CPU_GET_ARG_REG_INDEX(   Arg) (Arg & 7)

#define CPU_CALC_TIMING( T) {Device_Data.CPU_State.Time += T;}

#define CPU_GET_PSW( Arg)                     \
{                                             \
    Arg = (TCPU_Arg) Psw;                     \
    DEBUG_PRINT (("  PSW=%o", (int) (Arg)));  \
}

#define CPU_SET_PSW( Arg)                     \
{                                             \
    Psw = (TCPU_Psw) (Arg);                   \
    DEBUG_PRINT (("  %o=>PSW", (int) (Arg))); \
}

#define CPU_INST_PUSH( Arg)                                          \
{                                                                    \
    TCPU_Arg sp = SP - 2; SP = sp;                                   \
    CPU_CHECK_ARG_FAULT (CPU_WriteMemW (sp, (uint_fast16_t) (Arg))); \
    DEBUG_PRINT (("  (-SP=%o)<=%o", (int) sp, (int) (Arg)));         \
}

#define CPU_INST_POP( Arg)                                         \
{                                                                  \
    TCPU_Arg sp = SP; SP = sp + 2;                                 \
    Arg = CPU_ReadMemW (sp);                                       \
    CPU_CHECK_ARG_FAULT (Arg);                                     \
    DEBUG_PRINT (("  (SP+=%o)=%o", (int) sp, (int) (Arg)));        \
}

#define CPU_INST_INTERRUPT( Vec)                                                                       \
{                                                                                                      \
    CPU_GET_PSW   (ArgS);                                                                              \
    CPU_INST_PUSH (ArgS);                                                                              \
    CPU_INST_PUSH (PC);                                                                                \
    PC   = CPU_PAGE0_MEM16 [ (Vec) >> 1     ];                                                         \
    ArgS = CPU_PAGE0_MEM16 [((Vec) >> 1) + 1] & 0377;                                                  \
    CPU_SET_PSW (ArgS);                                                                                \
                                                                                                       \
    DEBUG_PRINT (("  (%o)=%o=>PC  (%o)=%o=>PSW", (int) (Vec), (int) PC, (int) (Vec) + 2, (int) ArgS)); \
}

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

    uintptr_t MemPages [4];

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

// CPU timing в тактах
#define CPU_TIMING_BASE        12
#define CPU_TIMING_HALT        68
#define CPU_TIMING_WAIT        12
#define CPU_TIMING_RTI         40
#define CPU_TIMING_RESET     1140
#define CPU_TIMING_BR_BASE     16
#define CPU_TIMING_RTS         32
#define CPU_TIMING_MARK        56
#define CPU_TIMING_EMT         64
#define CPU_TIMING_IOT        104
#define CPU_TIMING_SOB         20
#define CPU_TIMING_INT         40
#define CPU_TIMING_BUS_ERROR   64
#define CPU_TIMING_STOP        64

//однооперандные расчётные
extern const uint8_t CPU_timing_OneOps_TST  [8];
extern const uint8_t CPU_timing_OneOps_CLR  [8];
extern const uint8_t CPU_timing_OneOps_MTPS [8];
extern const uint8_t CPU_timing_OneOps_XOR  [8];
extern const uint8_t CPU_timing_OneOps_JMP  [8];
extern const uint8_t CPU_timing_OneOps_JSR  [8];
//двухоперандные расчётные
extern const uint8_t CPU_timing_TwoOps_MOV [64];
extern const uint8_t CPU_timing_TwoOps_CMP [64];
extern const uint8_t CPU_timing_TwoOps_BIS [64];

#define CPU_GET_PAGE_ADR_MEM16( Adr) ((uint16_t *) (Device_Data.MemPages [(Adr) >> 14] + ((Adr) & 0x3FFE)))
#define CPU_GET_PAGE_ADR_MEM8(  Adr) ((uint8_t  *) (Device_Data.MemPages [(Adr) >> 14] + ((Adr) & 0x3FFF)))

//#define MEM8  ((uint8_t  *) 0x3FFE8000)
//#define MEM16 ((uint16_t *) 0x3FFE8000)
//#define MEM32 ((uint32_t *) 0x3FFE8000)

#endif

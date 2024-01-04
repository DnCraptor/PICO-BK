#include <stdlib.h> 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "AnyMem.h"
#include "CPU.h"
#include "CPU_e.h"
#include "../EmuFfs/CPU_ef.h"

#include "Debug.h"

#define AT_OVL __attribute__((section(".ovl3_e.text")))

static bool call_ET100 = false;
void Call_ET100() {
    call_ET100 = true;
}

#define CPU_CALC_TIMING_D(  Tab) {Device_Data.CPU_State.Time += Tab [(OpCode >> 3) & 07];}
#define CPU_CALC_TIMING_SD( Tab) {Device_Data.CPU_State.Time += Tab [((OpCode >> 6) & 070) | ((OpCode >> 3) & 07)];}

#define CPU_CHECK_ARG_FAULT_OR_REG( Arg) {if (CPU_IS_ARG_FAULT_OR_REG (Arg)) goto BusFault;}

#define CPU_READ_ADRD_JMP()                 \
{                                           \
    AdrD = CPU_GetArgAdrW (OpCode);         \
    CPU_CHECK_ARG_FAULT_OR_REG (AdrD);      \
}

#define CPU_READ_ADRDW()                    \
{                                           \
    AdrD = CPU_GetArgAdrW (OpCode);         \
                                            \
    CPU_CHECK_ARG_FAULT (AdrD);             \
}

#define CPU_READ_ADRSW()                    \
{                                           \
    AdrS = CPU_GetArgAdrW (OpCode >> 6);    \
                                            \
    CPU_CHECK_ARG_FAULT (AdrS);             \
}

#define CPU_READ_ADRDB()                    \
{                                           \
    AdrD = CPU_GetArgAdrB (OpCode);         \
                                            \
    CPU_CHECK_ARG_FAULT (AdrD);             \
}

#define CPU_READ_ADRSB()                    \
{                                           \
    AdrS = CPU_GetArgAdrB (OpCode >> 6);    \
                                            \
    CPU_CHECK_ARG_FAULT (AdrS);             \
}

#define CPU_READ_DW()                       \
{                                           \
    CPU_READ_ADRDW ();                      \
                                            \
    ArgD = CPU_ReadW (AdrD);                \
                                            \
    CPU_CHECK_ARG_FAULT (ArgD);             \
                                            \
    DEBUG_PRINT (("=%o", (int) ArgD));      \
}

#define CPU_READ_DB()                       \
{                                           \
    CPU_READ_ADRDB ();                      \
                                            \
    ArgD = CPU_ReadB (AdrD);                \
                                            \
    CPU_CHECK_ARG_FAULT (ArgD);             \
                                            \
    DEBUG_PRINT (("=%o", (int) ArgD));      \
}

#define CPU_READ_SW()                       \
{                                           \
    CPU_READ_ADRSW ();                      \
                                            \
    ArgS = CPU_ReadW (AdrS);                \
                                            \
    CPU_CHECK_ARG_FAULT (ArgS);             \
                                            \
    DEBUG_PRINT (("=%o", (int) ArgS));      \
}

#define CPU_READ_SB()                       \
{                                           \
    CPU_READ_ADRSB ();                      \
                                            \
    ArgS = CPU_ReadB (AdrS);                \
                                            \
    CPU_CHECK_ARG_FAULT (ArgS);             \
                                            \
    DEBUG_PRINT (("=%o", (int) ArgS));      \
}

#define CPU_READ_SDW()                      \
{                                           \
    CPU_READ_SW ();                         \
    DEBUG_PRINT ((", "));                   \
    CPU_READ_DW ();                         \
}

#define CPU_READ_SDB()                      \
{                                           \
    CPU_READ_SB ();                         \
    DEBUG_PRINT ((", "));                   \
    CPU_READ_DB ();                         \
}

#define CPU_WRITE_DW()                                                                              \
{                                                                                                   \
    if (CPU_IS_ARG_FAULT (CPU_WriteW (AdrD, Res))) goto BusFault;                                   \
                                                                                                    \
    if (AdrD & CPU_ARG_REG) {DEBUG_PRINT (("  %o=>R%d",  (int) (uint16_t) Res, (int) (AdrD & 7)));} \
    else                    {DEBUG_PRINT (("  %o=>(%o)", (int) (uint16_t) Res, (int)  AdrD     ));} \
}

#define CPU_WRITE_DB()                                                                             \
{                                                                                                  \
    if (CPU_IS_ARG_FAULT (CPU_WriteB (AdrD, Res))) goto BusFault;                                  \
                                                                                                   \
    if (AdrD & CPU_ARG_REG) {DEBUG_PRINT (("  %o=>R%d",  (int) (uint8_t) Res, (int) (AdrD & 7)));} \
    else                    {DEBUG_PRINT (("  %o=>(%o)", (int) (uint8_t) Res, (int)  AdrD     ));} \
}

#define CPU_WRITE_DW1()                                                                             \
{                                                                                                   \
    CPU_READ_ADRDW ();                                                                              \
    CPU_WRITE_DW   ();                                                                              \
}

#define CPU_WRITE_DB1()                                                                            \
{                                                                                                  \
    CPU_READ_ADRDB ();                                                                             \
    CPU_WRITE_DB   ();                                                                             \
}

#define CPU_WRITE_DB1_MOVE()                                                 \
{                                                                            \
    CPU_READ_ADRDB ();                                                       \
    if (AdrD & CPU_ARG_REG)                                                  \
    {                                                                        \
        R [AdrD & 7] = (int16_t) (int8_t) Res;                               \
        DEBUG_PRINT (("  %o=>R%d",  (int) (uint8_t) Res, (int) (AdrD & 7))); \
    }                                                                        \
    else                                                                     \
    {                                                                        \
        if (CPU_IS_ARG_FAULT (CPU_WriteB (AdrD, Res))) goto BusFault;        \
        DEBUG_PRINT (("  %o=>(%o)", (int) (uint8_t) Res, (int)  AdrD     )); \
    }                                                                        \
}

#define CPU_PSW_T_POS 4
#define CPU_PSW_N_POS 3
#define CPU_PSW_Z_POS 2
#define CPU_PSW_V_POS 1
#define CPU_PSW_C_POS 0

#define CPU_PSW_N (1 << CPU_PSW_N_POS)
#define CPU_PSW_Z (1 << CPU_PSW_Z_POS)
#define CPU_PSW_V (1 << CPU_PSW_V_POS)
#define CPU_PSW_C (1 << CPU_PSW_C_POS)
#define CPU_PSW_T (1 << CPU_PSW_T_POS)

#define CPU_SET_PSW_MTPS( Arg)                                             \
{                                                                          \
    DEBUG_PRINT (("  PSW=%o&177420|%o", (int) Psw, (int) ((Arg) & 0357))); \
    Psw = (Psw & 0177420) | ((Arg) & 0357);                                \
}

#define CPU_SET_PSW_FLAGS_W_SET_Z_CLR_NVC( )     \
{                                                \
    Psw |=   CPU_PSW_Z;                          \
    Psw &= ~(CPU_PSW_N | CPU_PSW_V | CPU_PSW_C); \
}

#define CPU_SET_PSW_FLAGS_B_SET_Z_CLR_NVC   CPU_SET_PSW_FLAGS_W_SET_Z_CLR_NVC

#define CPU_SET_PSW_FLAGS_W_NZ_SET_C_CLR_V( )                 \
{                                                             \
    Psw |=   CPU_PSW_C;                                       \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V);              \
                                                              \
    if ( Res               & 0x8000U)       Psw |= CPU_PSW_N; \
    if ((Res               & 0xFFFFU) == 0) Psw |= CPU_PSW_Z; \
}

#define CPU_SET_PSW_FLAGS_B_NZ_SET_C_CLR_V( )                 \
{                                                             \
    Psw |=   CPU_PSW_C;                                       \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V);              \
                                                              \
    if ( Res               & 0x80U)       Psw |= CPU_PSW_N;   \
    if ((Res               & 0xFFU) == 0) Psw |= CPU_PSW_Z;   \
}

#define CPU_SET_PSW_FLAGS_W_NZ_CLR_VC( )                      \
{                                                             \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V | CPU_PSW_C);  \
                                                              \
    if ( Res               & 0x8000U)       Psw |= CPU_PSW_N; \
    if ((Res               & 0xFFFFU) == 0) Psw |= CPU_PSW_Z; \
}

#define CPU_SET_PSW_FLAGS_B_NZ_CLR_VC( )                      \
{                                                             \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V | CPU_PSW_C);  \
                                                              \
    if ( Res               & 0x80U)       Psw |= CPU_PSW_N;   \
    if ((Res               & 0xFFU) == 0) Psw |= CPU_PSW_Z;   \
}

#define CPU_SET_PSW_FLAGS_W_NZ_CLR_V( )                       \
{                                                             \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V);              \
                                                              \
    if ( Res               & 0x8000U)       Psw |= CPU_PSW_N; \
    if ((Res               & 0xFFFFU) == 0) Psw |= CPU_PSW_Z; \
}

#define CPU_SET_PSW_FLAGS_B_NZ_CLR_V( )                       \
{                                                             \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V);              \
                                                              \
    if ( Res               & 0x80U)       Psw |= CPU_PSW_N;   \
    if ((Res               & 0xFFU) == 0) Psw |= CPU_PSW_Z;   \
}

#define CPU_SET_PSW_FLAGS_W_NZV( )                            \
{                                                             \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V);              \
                                                              \
    if ( Res               & 0x8000U)       Psw |= CPU_PSW_N; \
    if ((Res               & 0xFFFFU) == 0) Psw |= CPU_PSW_Z; \
    if ((Res ^ (Res >> 1)) & 0x8000U)       Psw |= CPU_PSW_V; \
}

#define CPU_SET_PSW_FLAGS_B_NZV( )                            \
{                                                             \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V);              \
                                                              \
    if ( Res               & 0x80U)       Psw |= CPU_PSW_N;   \
    if ((Res               & 0xFFU) == 0) Psw |= CPU_PSW_Z;   \
    if ((Res ^ (Res >> 1)) & 0x80U)       Psw |= CPU_PSW_V;   \
}

#define CPU_SET_PSW_FLAGS_W_NZVC( )                             \
{                                                               \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V | CPU_PSW_C);    \
                                                                \
    if ( Res               &  0x8000U )       Psw |= CPU_PSW_N; \
    if ((Res               &  0xFFFFU ) == 0) Psw |= CPU_PSW_Z; \
    if ((Res ^ (Res >> 1)) &  0x8000U )       Psw |= CPU_PSW_V; \
    if ( Res               & 0x20000UL)       Psw |= CPU_PSW_C; \
}

#define CPU_SET_PSW_FLAGS_B_NZVC( )                             \
{                                                               \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V | CPU_PSW_C);    \
                                                                \
    if ( Res               &  0x80U)       Psw |= CPU_PSW_N;    \
    if ((Res               &  0xFFU) == 0) Psw |= CPU_PSW_Z;    \
    if ((Res ^ (Res >> 1)) &  0x80U)       Psw |= CPU_PSW_V;    \
    if ( Res               & 0x200U)       Psw |= CPU_PSW_C;    \
}

#define CPU_SET_PSW_FLAGS_W_Z_CLR_V( )           \
{                                                \
    Psw &= ~(CPU_PSW_Z | CPU_PSW_V);             \
    if ((Res & 0xFFFFU ) == 0) Psw |= CPU_PSW_Z; \
}

#define CPU_SET_PSW_FLAGS_B_Z_CLR_V( )           \
{                                                \
    Psw &= ~(CPU_PSW_Z | CPU_PSW_V);             \
    if ((Res & 0xFFU ) == 0) Psw |= CPU_PSW_Z;   \
}

#define CPU_SET_PSW_FLAGS_W_FOR_LEFT_SHIFT( )                   \
{                                                               \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V | CPU_PSW_C);    \
    if ( Res               &  0x8000U )       Psw |= CPU_PSW_N; \
    if ((Res               &  0xFFFFU ) == 0) Psw |= CPU_PSW_Z; \
    if ( ArgD              &  0x8000U )       Psw |= CPU_PSW_C; \
    if ((Res ^ ArgD)       &  0x8000U )       Psw |= CPU_PSW_V; \
}

#define CPU_SET_PSW_FLAGS_W_FOR_RIGHT_SHIFT( )                      \
{                                                                   \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V | CPU_PSW_C);        \
    if ( Res                 &  0x8000U )       Psw |= CPU_PSW_N;   \
    if ((Res                 &  0xFFFFU ) == 0) Psw |= CPU_PSW_Z;   \
    if ( ArgD                &       1  )       Psw |= CPU_PSW_C;   \
    if ((Res ^ (ArgD << 15)) &  0x8000U )       Psw |= CPU_PSW_V;   \
}

#define CPU_SET_PSW_FLAGS_B_FOR_LEFT_SHIFT( )                   \
{                                                               \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V | CPU_PSW_C);    \
    if ( Res               &  0x80U )       Psw |= CPU_PSW_N;   \
    if ((Res               &  0xFFU ) == 0) Psw |= CPU_PSW_Z;   \
    if ( ArgD              &  0x80U )       Psw |= CPU_PSW_C;   \
    if ((Res ^ ArgD)       &  0x80U )       Psw |= CPU_PSW_V;   \
}

#define CPU_SET_PSW_FLAGS_B_FOR_RIGHT_SHIFT( )                  \
{                                                               \
    Psw &= ~(CPU_PSW_N | CPU_PSW_Z | CPU_PSW_V | CPU_PSW_C);    \
    if ( Res                &  0x80U )       Psw |= CPU_PSW_N;  \
    if ((Res                &  0xFFU ) == 0) Psw |= CPU_PSW_Z;  \
    if ( ArgD               &     1  )       Psw |= CPU_PSW_C;  \
    if ((Res ^ (ArgD << 7)) &  0x80U )       Psw |= CPU_PSW_V;  \
}

#define CPU_PREP_FLAGS_DW()  {ArgD |= (ArgD & 0x8000U) << 1;}
#define CPU_PREP_FLAGS_DB()  {ArgD |= (ArgD & 0x80)    << 1;}
#define CPU_PREP_FLAGS_SW()  {ArgS |= (ArgS & 0x8000U) << 1;}
#define CPU_PREP_FLAGS_SB()  {ArgS |= (ArgS & 0x80)    << 1;}
#define CPU_PREP_FLAGS_SDW() {CPU_PREP_FLAGS_SW (); CPU_PREP_FLAGS_DW ();}
#define CPU_PREP_FLAGS_SDB() {CPU_PREP_FLAGS_SB (); CPU_PREP_FLAGS_DB ();}

// Z == 0
#define CPU_IS_FLAGS_NE( ) ((Psw & CPU_PSW_Z) == 0)
// Z == 1
#define CPU_IS_FLAGS_EQ( ) ((Psw & CPU_PSW_Z) != 0)
// N ^ V == 0
#define CPU_IS_FLAGS_GE( ) ((((Psw >> CPU_PSW_V_POS) ^ (Psw >> CPU_PSW_N_POS)) & 1) == 0)
// N ^ V == 1
#define CPU_IS_FLAGS_LT( ) ((((Psw >> CPU_PSW_V_POS) ^ (Psw >> CPU_PSW_N_POS)) & 1) != 0)
// Z | (N ^ V) == 0
#define CPU_IS_FLAGS_GT( ) ((((Psw >> CPU_PSW_Z_POS) | ((Psw >> CPU_PSW_V_POS) ^ (Psw >> CPU_PSW_N_POS))) & 1) == 0)
// Z | (N ^ V) == 1
#define CPU_IS_FLAGS_LE( ) ((((Psw >> CPU_PSW_Z_POS) | ((Psw >> CPU_PSW_V_POS) ^ (Psw >> CPU_PSW_N_POS))) & 1) != 0)
// N == 0
#define CPU_IS_FLAGS_PL( ) ((Psw & CPU_PSW_N) == 0)
// N == 1
#define CPU_IS_FLAGS_MI( ) ((Psw & CPU_PSW_N) != 0)
// C | Z == 0
#define CPU_IS_FLAGS_HI( ) ((Psw & (CPU_PSW_C | CPU_PSW_Z)) == 0)
// C | Z == 1
#define CPU_IS_FLAGS_LOS() ((Psw & (CPU_PSW_C | CPU_PSW_Z)) != 0)
// V == 0
#define CPU_IS_FLAGS_VC( ) ((Psw & CPU_PSW_V) == 0)
// V == 1
#define CPU_IS_FLAGS_VS( ) ((Psw & CPU_PSW_V) != 0)
// C == 0
#define CPU_IS_FLAGS_CC( ) ((Psw & CPU_PSW_C) == 0)
// C == 1
#define CPU_IS_FLAGS_CS( ) ((Psw & CPU_PSW_C) != 0)

#define CPU_INST_BR( )                                                  \
{                                                                       \
    DEBUG_PRINT (("  PC=%o+%o", (int) PC, (int) (int8_t) OpCode * 2));  \
    PC = (uint16_t) (PC + (int16_t) (int8_t) OpCode * 2);               \
    DEBUG_PRINT (("=%o=>PC", (int) PC));                                \
}

static TCPU_Arg AT_OVL CPU_ReadW (TCPU_Arg Adr)
{
    if (CPU_IS_ARG_REG (Adr)) return R [CPU_GET_ARG_REG_INDEX (Adr)];

    return CPU_ReadMemW (Adr);
}

static TCPU_Arg AT_OVL CPU_ReadB (TCPU_Arg Adr)
{
    if (CPU_IS_ARG_REG (Adr)) return (*(uint8_t *) &R [CPU_GET_ARG_REG_INDEX (Adr)]);

    return CPU_ReadMemB (Adr);
}

static TCPU_Arg AT_OVL CPU_GetArgAdrW (uint_fast8_t SrcCode)
{
    TCPU_Arg       Adr  = SrcCode & 07;
    uint16_t      *pReg = &R [Adr];
    uint_fast16_t  Reg;
    uint_fast16_t  pc;

    switch ((SrcCode >> 3) & 07)
    {
        case 00:

            DEBUG_PRINT (("R%d", (int) Adr));

            return Adr | CPU_ARG_REG;

        case 01:

            Reg = *pReg;

            DEBUG_PRINT (("(R%d=%o)", (int) Adr, (int) Reg));

            return Reg;

        case 02:

            Reg   = *pReg;
            *pReg = (uint16_t) (Reg + 2);

            DEBUG_PRINT (("(R%d+=%o)", (int) Adr, (int) Reg));

            return Reg;

        case 03:

            Reg   = *pReg;
            *pReg = (uint16_t) (Reg + 2);

            DEBUG_PRINT (("((R%d+=%o)", (int) Adr, (int) Reg));

            Adr = CPU_ReadMemW (Reg);

            DEBUG_PRINT (("=%o)", (int) Adr));

            return Adr;

        case 04:

            Reg   = *pReg;
            Reg   = (uint16_t) (Reg - 2);
            *pReg = (uint16_t) Reg;

            DEBUG_PRINT (("(-R%d=%o)", (int) Adr, (int) Reg));

            return Reg;

        case 05:

            Reg   = *pReg;
            Reg   = (uint16_t) (Reg - 2);
            *pReg = (uint16_t) Reg;

            DEBUG_PRINT (("((-R%d=%o)", (int) Adr, (int) Reg));

            Adr = CPU_ReadMemW (Reg);

            DEBUG_PRINT (("=%o)", (int) Adr));

            return Adr;

        case 06:

            pc  = PC;
            PC  = (uint16_t) (pc + 2);
            Reg = *pReg;

            DEBUG_PRINT (("(R%d=%o", (int) Adr, (int) Reg));

            Adr = CPU_ReadMemW (pc);

            if (CPU_IS_ARG_FAULT (Adr)) return Adr;

            DEBUG_PRINT (("+%o", (int) Adr));

            Adr = (uint16_t) (Reg + Adr);

            DEBUG_PRINT (("=%o)", (int) Adr));

            return Adr;

        case 07:

            pc  = PC;
            PC  = (uint16_t) (pc + 2);
            Reg = *pReg;

            DEBUG_PRINT (("((R%d=%o", (int) Adr, (int) Reg));

            Adr = CPU_ReadMemW (pc);

            if (CPU_IS_ARG_FAULT (Adr)) return Adr;

            DEBUG_PRINT (("+%o", (int) Adr));

            Adr = (uint16_t) (Reg + Adr);

            DEBUG_PRINT (("=%o)", (int) Adr));

            Adr = CPU_ReadMemW (Adr);

            DEBUG_PRINT (("=%o)", (int) Adr));

            return Adr;
    }

    return 0;
}

static TCPU_Arg AT_OVL CPU_GetArgAdrB (uint_fast8_t SrcCode)
{
    TCPU_Arg    Adr = SrcCode & 07;
    uint16_t      *pReg = &R [Adr];
    uint_fast16_t  Reg;
    uint_fast16_t  pc;

    switch ((SrcCode >> 3) & 07)
    {
        case 00:

            DEBUG_PRINT (("R%d", (int) Adr));

            return Adr | CPU_ARG_REG;

        case 01:

            Reg = *pReg;

            DEBUG_PRINT (("(R%d=%o)", (int) Adr, (int) Reg));

            return Reg;

        case 02:

            Reg = *pReg;

            if (Adr >= 6) *pReg = (uint16_t) (Reg + 2);
            else          *pReg = (uint16_t) (Reg + 1);

            DEBUG_PRINT (("(R%d+=%o)", (int) Adr, (int) Reg));

            return Reg;

        case 03:

            Reg   = *pReg;
            *pReg = (uint16_t) (Reg + 2);

            DEBUG_PRINT (("((R%d+=%o)", (int) Adr, (int) Reg));

            Adr = CPU_ReadMemW (Reg);

            DEBUG_PRINT (("=%o)", (int) Adr));

            return Adr;

        case 04:

            Reg = *pReg;

            if (Adr >= 6) Reg = (uint16_t) (Reg - 2);
            else          Reg = (uint16_t) (Reg - 1);

            *pReg = (uint16_t) Reg;

            DEBUG_PRINT (("(-R%d=%o)", (int) Adr, (int) Reg));

            return Reg;

        case 05:

            Reg   = *pReg;
            Reg   = (uint16_t) (Reg - 2);
            *pReg = (uint16_t) Reg;

            DEBUG_PRINT (("((-R%d=%o)", (int) Adr, (int) Reg));

            Adr = CPU_ReadMemW (Reg);

            DEBUG_PRINT (("=%o)", (int) Adr));

            return Adr;

        case 06:

            pc  = PC;
            PC  = (uint16_t) (pc + 2);
            Reg = *pReg;

            DEBUG_PRINT (("(R%d=%o", (int) Adr, (int) Reg));

            Adr = CPU_ReadMemW (pc);

            if (CPU_IS_ARG_FAULT (Adr)) return Adr;

            DEBUG_PRINT (("+%o", (int) Adr));

            Adr = (uint16_t) (Reg + Adr);

            DEBUG_PRINT (("=%o)", (int) Adr));

            return Adr;

        case 07:

            pc  = PC;
            PC  = (uint16_t) (pc + 2);
            Reg = *pReg;

            DEBUG_PRINT (("((R%d=%o", (int) Adr, (int) Reg));

            Adr = CPU_ReadMemW (pc);

            if (CPU_IS_ARG_FAULT (Adr)) return Adr;

            DEBUG_PRINT (("+%o", (int) Adr));

            Adr = (uint16_t) (Reg + Adr);

            DEBUG_PRINT (("=%o)", (int) Adr));

            Adr = CPU_ReadMemW (Adr);

            DEBUG_PRINT (("=%o)", (int) Adr));

            return Adr;
    }

    return 0;
}

void AT_OVL CPU_Stop (void)
{
    TCPU_Arg ArgS;
    TCPU_Psw Psw = PSW;

    DEBUG_PRINT (("\nSTOP"));

  BusFault:

    CPU_INST_INTERRUPT (04);
    PSW = Psw;
    DEBUG_PRINT (("  PSW=%o\n", (int) Psw));
    CPU_CALC_TIMING (CPU_TIMING_STOP);
    return;
}

#include "fdd.h"
#include "CPU_i.h"
static uint16_t m_nFDDCatchAddr, m_nFDDExitCatchAddr = -1;

#if DSK_DEBUG
static bk_mode_t _bk0010mode = BK_FDD;
#else
static bk_mode_t _bk0010mode = BK_0010_01;
#endif

bk_mode_t get_bk0010mode() {
    return _bk0010mode;
}

bool is_fdd_suppored() {
    return _bk0010mode == BK_FDD || _bk0010mode == BK_0011M_FDD;
}

static inline uint16_t GetWordIndirect(uint16_t addr) {
	return CPU_ReadMemW(addr);
}

void set_bk0010mode(bk_mode_t mode) {
    _bk0010mode = mode;
    switch (mode)
    {
    case BK_0011M_FDD:
    case BK_FDD:
		if (0167 == GetWordIndirect(0160016))
		{
			// 326 прошивка
			m_nFDDCatchAddr = 0160372;
			m_nFDDExitCatchAddr = 0161564;
            DSK_PRINT(("EmulateFDD 326 m_nFDDCatchAddr: 0%o", m_nFDDCatchAddr));
		}
		else
		{
			// 253 прошивка, там нету перехода к эмулятору EIS/FIS
			m_nFDDCatchAddr = 0160422;
			m_nFDDExitCatchAddr = 0161540;
            DSK_PRINT(("EmulateFDD 253 m_nFDDCatchAddr: 0%o", m_nFDDCatchAddr));
		}
		// дополнительная проверка на правильную прошивку.
		if (GetWordIndirect(m_nFDDCatchAddr)       == 0010663
		        && GetWordIndirect(m_nFDDCatchAddr + 2)   == 0000050
		        && GetWordIndirect(m_nFDDCatchAddr + 4)   == 0106763
		        && GetWordIndirect(m_nFDDCatchAddr + 6)   == 0000052
		        && GetWordIndirect(m_nFDDCatchAddr + 010) == 0012700
		        && GetWordIndirect(m_nFDDCatchAddr + 012) == 0000004
		        && GetWordIndirect(m_nFDDCatchAddr + 014) == 0011063
		        && GetWordIndirect(m_nFDDCatchAddr + 016) == 0000046
		        && GetWordIndirect(m_nFDDCatchAddr + 020) == 0010710
		        && GetWordIndirect(m_nFDDCatchAddr + 022) == 0062710
		   )
		{
			// всё ок
            DSK_PRINT(("EmulateFDD m_nFDDCatchAddr: 0%o OK", m_nFDDCatchAddr));
		}
		else
		{
			// нестандартная прошивка
			m_nFDDCatchAddr = 0177777;
			m_nFDDExitCatchAddr = 0177777;
            DSK_PRINT(("EmulateFDD ??? m_nFDDCatchAddr: 0%o", m_nFDDCatchAddr));
		}
        break;
    default:
		// нестандартная прошивка
		m_nFDDCatchAddr = 0177777;
		m_nFDDExitCatchAddr = 0177777;
        DSK_PRINT(("EmulateFDD ??? m_nFDDCatchAddr: 0%o", m_nFDDCatchAddr));
        break;
    }
}

void AT_OVL CPU_RunInstruction (void) {
    if ((PC & 0177776) == m_nFDDCatchAddr && is_fdd_suppored()) {
        EmulateFDD();
        PC = m_nFDDExitCatchAddr;
    }
    Periodic();
    TCPU_Arg OpCode;
    TCPU_Arg AdrS;
    TCPU_Arg AdrD;
    TCPU_Arg ArgS;
    TCPU_Arg ArgD;
    TCPU_Arg Res;
    TCPU_Psw Psw = PSW;

    if ((Psw & 0200) == 0 && (Device_Data.CPU_State.Flags & (CPU_FLAG_KEY_VECTOR_60 | CPU_FLAG_KEY_VECTOR_274)))
    {
        if (Device_Data.CPU_State.Flags & CPU_FLAG_KEY_VECTOR_60)
        {
            Device_Data.CPU_State.Flags &= ~CPU_FLAG_KEY_VECTOR_60;
            DEBUG_PRINT (("               INTERRUPT VEC=60"));
            CPU_INST_INTERRUPT (060);
            CPU_CALC_TIMING    (CPU_TIMING_INT);
            goto Exit;
        }
        else if (Device_Data.CPU_State.Flags & CPU_FLAG_KEY_VECTOR_274)
        {
            Device_Data.CPU_State.Flags &= ~CPU_FLAG_KEY_VECTOR_274;
            DEBUG_PRINT (("               INTERRUPT VEC=274"));
            CPU_INST_INTERRUPT (0274);
            CPU_CALC_TIMING    (CPU_TIMING_INT);
            goto Exit;
        }
    }
    if (call_ET100) {
        call_ET100 = false;
        DEBUG_PRINT (("  !!!SYSTEM TIMER!!!"));
        CPU_INST_INTERRUPT (0100);
        PSW = Psw;
        DEBUG_PRINT (("  PSW=%o\n", (int) Psw));
        CPU_CALC_TIMING (CPU_TIMING_INT);
        //  exit (1);
        return;
    }

    DEBUG_PRINT (("%06o: ", (int) PC));

    OpCode = CPU_ReadMemW (PC); PC += 2;

    CPU_CHECK_ARG_FAULT (OpCode);

    DEBUG_PRINT (("%06o ", (int) OpCode));

    switch (OpCode >> 12)
    {
        case 000: switch ((OpCode >> 6) & 077)
        {
          case 000: switch (OpCode & 077)
          {
            case 000: // 000000   HALT
                    DEBUG_PRINT (("HALT  "));
                    CPU_INST_INTERRUPT (04);
                    CPU_CALC_TIMING    (CPU_TIMING_HALT);
                    break;

            case 001: // 000001   WAIT
                    DEBUG_PRINT (("WAIT  "));
                    CPU_CALC_TIMING (CPU_TIMING_WAIT);
//                  exit (1);
                    break;

            case 002: // 000002   RTI
                    DEBUG_PRINT (("RTI   "));
                    CPU_INST_POP   (ArgS);
                    DEBUG_PRINT (("=>PC"));
                    PC = (uint16_t) ArgS;
                    CPU_INST_POP   (ArgS);
                    DEBUG_PRINT (("=>PSW"));
                    CPU_SET_PSW    (ArgS & 0377);
                    CPU_CALC_TIMING (CPU_TIMING_RTI);
                    break;

            case 003: // 000003   BPT
                    DEBUG_PRINT (("BPT   "));
                    CPU_INST_INTERRUPT (014);
                    CPU_CALC_TIMING (CPU_TIMING_IOT);
                    break;

            case 004: // 000004   IOT
                    DEBUG_PRINT (("IOT   "));
                    CPU_INST_INTERRUPT (020);
                    CPU_CALC_TIMING (CPU_TIMING_IOT);
                    break;

            case 005: // 000005   RESET
                    DEBUG_PRINT (("RESET "));
                    CPU_CALC_TIMING (CPU_TIMING_RESET);
                    break;

            case 006: // 000006   RTT
                    DEBUG_PRINT (("RTT   "));
                    CPU_INST_POP   (ArgS);
                    DEBUG_PRINT (("=>PC"));
                    PC = (uint16_t) ArgS;
                    CPU_INST_POP   (ArgS);
                    DEBUG_PRINT (("=>PSW"));
                    CPU_SET_PSW    (ArgS & 0377);
                    CPU_CALC_TIMING (CPU_TIMING_RTI);
                    break;

            default:  goto InvalidOpCode;
          }
          break;

          case 001: // 0001DD   JMP
                    DEBUG_PRINT (("JMP   "));
                    CPU_READ_ADRD_JMP ();
                    DEBUG_PRINT (("=>PC"));
                    PC = AdrD;
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_JMP);
                    break;

          case 002:

            if ((OpCode & 070) == 0)
            {
                    // 00020R    RTS
                    DEBUG_PRINT (("RTS   "));
                    PC = R [OpCode & 07];
                    DEBUG_PRINT (("R%d=%o=>PC", (int) (OpCode & 07), (int) PC));
                    CPU_INST_POP   (ArgS);
                    R [OpCode & 07] = (uint16_t) ArgS;
                    DEBUG_PRINT (("=>R%d", (int) (OpCode & 07)));
                    CPU_CALC_TIMING (CPU_TIMING_RTS);
                    break;
            }
            else if ((OpCode & 070) >= 040)
            {
                    // 000240-000277  PSW
                    DEBUG_PRINT (("PSW   "));
                    CPU_GET_PSW (ArgS)
                    if (OpCode & 020)
                    {
                        DEBUG_PRINT (("  SET %o", (int) (OpCode & 017)));
                        ArgS |= OpCode & 017;
                    }
                    else
                    {
                        DEBUG_PRINT (("  CLR %o", (int) (OpCode & 017)));
                        ArgS &= ~(TCPU_Arg) (OpCode & 017);
                    }
                    CPU_SET_PSW (ArgS);
                    CPU_CALC_TIMING (CPU_TIMING_BASE);
                    break;
            }
            else goto InvalidOpCode;

            break;

          case 003: // 0003DD   SWAB
                    DEBUG_PRINT  (("SWAB  "));
                    CPU_READ_DW  ();
                    Res = (uint16_t) (ArgD << 8) | (ArgD >> 8);
                    CPU_SET_PSW_FLAGS_B_NZ_CLR_VC ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 004: // 000400-000777   BR
          case 005:
          case 006:
          case 007:
                    DEBUG_PRINT  (("BR    "));
                    CPU_INST_BR ();
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 010: // 001000-001377   BNE
          case 011:
          case 012:
          case 013:
                    DEBUG_PRINT  (("BNE   "));
                    if (CPU_IS_FLAGS_NE ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 014: // 001400-001777   BEQ
          case 015:
          case 016:
          case 017:
                    DEBUG_PRINT  (("BEQ   "));
                    if (CPU_IS_FLAGS_EQ ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 020: // 002000-002377   BGE
          case 021:
          case 022:
          case 023:
                    DEBUG_PRINT  (("BGE   "));
                    if (CPU_IS_FLAGS_GE ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 024: // 002400-002777   BLT
          case 025:
          case 026:
          case 027:
                    DEBUG_PRINT  (("BLT   "));
                    if (CPU_IS_FLAGS_LT ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 030: // 003000-003377   BGT
          case 031:
          case 032:
          case 033:
                    DEBUG_PRINT  (("BGT   "));
                    if (CPU_IS_FLAGS_GT ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 034: // 003400-003777   BLE
          case 035:
          case 036:
          case 037:
                    DEBUG_PRINT  (("BLE   "));
                    if (CPU_IS_FLAGS_LE ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 040: // 004RDD   JSR
          case 041:
          case 042:
          case 043:
          case 044:
          case 045:
          case 046:
          case 047:
                {
                    uint16_t *pReg = &R [(OpCode >> 6) & 7];

                    DEBUG_PRINT (("JSR   "));
                    CPU_READ_ADRD_JMP ();
                    ArgS = *pReg;
                    DEBUG_PRINT (("R%d=%o", (int) ((OpCode >> 6) & 7), (int) ArgS));
                    CPU_INST_PUSH (ArgS);
                    *pReg = PC;
                    DEBUG_PRINT (("  PC=%o=>R%d", (int) PC, (int) ((OpCode >> 6) & 7)));
                    PC    = AdrD;
                    DEBUG_PRINT (("  %o=>PC", (int) PC));
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_JSR);
                    break;
                }

          case 050: // 0050DD   CLR
                    DEBUG_PRINT (("CLR   "));
                    Res = 0;
                    CPU_SET_PSW_FLAGS_W_SET_Z_CLR_NVC ();
                    CPU_WRITE_DW1 ()
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 051: // 0051DD   COM
                    DEBUG_PRINT (("COM   "));
                    CPU_READ_DW ();
                    Res = ~ArgD;
                    CPU_SET_PSW_FLAGS_W_NZ_SET_C_CLR_V ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 052: // 0052DD   INC
                    DEBUG_PRINT (("INC   "));
                    CPU_READ_DW ();
                    CPU_PREP_FLAGS_DW ();
                    Res = ArgD + 1;
                    CPU_SET_PSW_FLAGS_W_NZV ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 053: // 0053DD   DEC
                    DEBUG_PRINT (("DEC   "));
                    CPU_READ_DW ();
                    CPU_PREP_FLAGS_DW ();
                    Res = ArgD - 1;
                    CPU_SET_PSW_FLAGS_W_NZV ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 054: // 0054DD   NEG
                    DEBUG_PRINT (("NEG   "));
                    CPU_READ_DW ();
                    CPU_PREP_FLAGS_DW ();
                    Res = -ArgD;
                    CPU_SET_PSW_FLAGS_W_NZVC ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 055: // 0055DD   ADC
                    DEBUG_PRINT (("ADC   "));
                    CPU_READ_DW ();
                    CPU_PREP_FLAGS_DW ();
                    Res = ArgD;
                    if (CPU_IS_FLAGS_CS ()) Res += 1;
                    CPU_SET_PSW_FLAGS_W_NZVC ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 056: // 0056DD   SBC
                    DEBUG_PRINT (("SBC   "));
                    CPU_READ_DW ();
                    CPU_PREP_FLAGS_DW ();
                    Res = ArgD;
                    if (CPU_IS_FLAGS_CS ()) Res -= 1;
                    CPU_SET_PSW_FLAGS_W_NZVC ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 057: // 0057DD   TST
                    DEBUG_PRINT  (("TST   "));
                    CPU_READ_DW  ();
                    Res = ArgD;
                    CPU_SET_PSW_FLAGS_W_NZ_CLR_VC ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_TST);
                    break;

          case 060: // 0060DD   ROR
                    DEBUG_PRINT (("ROR   "));
                    CPU_READ_DW ();
                    Res = ArgD >> 1;
                    if (CPU_IS_FLAGS_CS ()) Res |= 0100000;
                    CPU_SET_PSW_FLAGS_W_FOR_RIGHT_SHIFT ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 061: // 0061DD   ROL
                    DEBUG_PRINT (("ROL   "));
                    CPU_READ_DW ();
                    Res = ArgD << 1;
                    if (CPU_IS_FLAGS_CS ()) Res |= 1;
                    CPU_SET_PSW_FLAGS_W_FOR_LEFT_SHIFT ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 062: // 0062DD   ASR
                    DEBUG_PRINT (("ASR   "));
                    CPU_READ_DW ();
                    Res = (uint16_t) ((int16_t) ArgD >> 1);
                    CPU_SET_PSW_FLAGS_W_FOR_RIGHT_SHIFT ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 063: // 0063DD   ASL
                    DEBUG_PRINT (("ASL   "));
                    CPU_READ_DW ();
                    Res = ArgD << 1;
                    CPU_SET_PSW_FLAGS_W_FOR_LEFT_SHIFT ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 064: // 0064NN   MARK
                    DEBUG_PRINT (("MARK  "));
                    ArgS = (OpCode & 077) << 1;
                    SP   = (uint16_t) (PC + ArgS);
                    PC   = R [5];
                    CPU_INST_POP (ArgS);
                    R [5] = ArgS;
                    CPU_CALC_TIMING (CPU_TIMING_MARK);
                    break;

          case 067: // 0067DD   SXT
                    DEBUG_PRINT (("SXT   "));
                    if (CPU_IS_FLAGS_MI ()) Res = 0177777;
                    else                    Res = 0;
                    CPU_SET_PSW_FLAGS_W_Z_CLR_V ();
                    CPU_WRITE_DW1 ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          default: goto InvalidOpCode;
        }
        break;

        case 001:   // 01SSDD   MOV
                    DEBUG_PRINT (("MOV   "));
                    CPU_READ_SW ();
                    Res = ArgS;
                    CPU_SET_PSW_FLAGS_W_NZ_CLR_V ();
                    CPU_WRITE_DW1 ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_MOV);
                    break;

        case 002:   // 02SSDD   CMP
                    DEBUG_PRINT (("CMP   "));
                    CPU_READ_SDW ();
                    CPU_PREP_FLAGS_SDW ();
                    Res = ArgS - ArgD;
                    CPU_SET_PSW_FLAGS_W_NZVC ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_CMP);
                    break;

        case 003:   // 03SSDD   BIT - Bit Test
                    DEBUG_PRINT (("BIT   "));
                    CPU_READ_SDW ();
                    Res = ArgS & ArgD;
                    CPU_SET_PSW_FLAGS_W_NZ_CLR_V ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_CMP);
                    break;

        case 004:   // 04SSDD   BIC - Bit Clear
                    DEBUG_PRINT (("BIC   "));
                    CPU_READ_SDW ();
                    Res = (~ArgS) & ArgD;
                    CPU_SET_PSW_FLAGS_W_NZ_CLR_V ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_BIS);
                    break;

        case 005:   // 05SSDD   BIS - Bit Set
                    DEBUG_PRINT (("BIS   "));
                    CPU_READ_SDW ();
                    Res = ArgS | ArgD;
                    CPU_SET_PSW_FLAGS_W_NZ_CLR_V ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_BIS);
                    break;

        case 006:   // 06SSDD   ADD
                    DEBUG_PRINT (("ADD   "));
                    CPU_READ_SDW ();
                    CPU_PREP_FLAGS_SDW ();
                    Res = ArgS + ArgD;
                    CPU_SET_PSW_FLAGS_W_NZVC ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_BIS);
                    break;

        case 007:

            if ((OpCode & 07000) == 04000)
            {
                    // 074RDD   XOR
                    DEBUG_PRINT (("XOR   "));
                    AdrS = (OpCode >> 6) & 7;
                    ArgS = R [AdrS];
                    DEBUG_PRINT (("R%d=%o, ", (int) AdrS, (int) ArgS));
                    CPU_READ_DW ();
                    Res = ArgS ^ ArgD;
                    CPU_SET_PSW_FLAGS_W_NZ_CLR_V ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_XOR);
                    break;
            }
            else if ((OpCode & 07000) == 07000)
            {
                    // 077R00   SOB
                    DEBUG_PRINT (("SOB   "));
                    AdrD = (OpCode >> 6) & 7;
                    ArgD = R [AdrD];
                    Res  = ArgD - 1;
                    DEBUG_PRINT (("R%d=%o-1=%o=>R%d", (int) AdrD, (int) ArgD, (int) Res, (int) AdrD));
                    R [AdrD] = (uint16_t) Res;
                    if (Res)
                    {
                        DEBUG_PRINT (("PC=%o", (int) PC));
                        PC = PC - (OpCode & 077) * 2;
                        DEBUG_PRINT (("-%o=%o=>PC", (int) ((OpCode & 077) * 2), (int) PC));
                    }
                    CPU_CALC_TIMING (CPU_TIMING_SOB);
                    break;
            }
            else goto InvalidOpCode;

            break;

        case 010: switch ((OpCode >> 6) & 077)
        {
          case 000: // 100000-100377   BPL
          case 001:
          case 002:
          case 003:
                    DEBUG_PRINT  (("BPL   "));
                    if (CPU_IS_FLAGS_PL ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 004: // 100400-100777   BMI
          case 005:
          case 006:
          case 007:
                    DEBUG_PRINT  (("BMI   "));
                    if (CPU_IS_FLAGS_MI ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 010: // 101000-101377   BHI
          case 011:
          case 012:
          case 013:
                    DEBUG_PRINT  (("BHI   "));
                    if (CPU_IS_FLAGS_HI ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 014: // 101400-101777   BLOS
          case 015:
          case 016:
          case 017:
                    DEBUG_PRINT  (("BLOS  "));
                    if (CPU_IS_FLAGS_LOS ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 020: // 102000-102377   BVC
          case 021:
          case 022:
          case 023:
                    DEBUG_PRINT  (("BVC   "));
                    if (CPU_IS_FLAGS_VC ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 024: // 102400-102777   BVS
          case 025:
          case 026:
          case 027:
                    DEBUG_PRINT  (("BVS   "));
                    if (CPU_IS_FLAGS_VS ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 030: // 103000-103377   BHIS(BCC)
          case 031:
          case 032:
          case 033:
                    DEBUG_PRINT  (("BCC   "));
                    if (CPU_IS_FLAGS_CC ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 034: // 103400-103777   BLO(BCS)
          case 035:
          case 036:
          case 037:
                    DEBUG_PRINT  (("BCS   "));
                    if (CPU_IS_FLAGS_CS ()) {CPU_INST_BR ();}
                    CPU_CALC_TIMING (CPU_TIMING_BR_BASE);
                    break;

          case 040: // 104000-104377   EMT
          case 041:
          case 042:
          case 043:
                    DEBUG_PRINT (("EMT   "));
                    CPU_INST_INTERRUPT (030);
                    CPU_CALC_TIMING (CPU_TIMING_EMT);
                    break;

          case 044: // 104400-104777   TRAP
          case 045:
          case 046:
          case 047:
                    DEBUG_PRINT (("TRAP  "));
                    CPU_INST_INTERRUPT (034);
                    CPU_CALC_TIMING (CPU_TIMING_EMT);
                    break;

          case 050: // *050DD   CLRB - Clear
                    DEBUG_PRINT (("CLRB  "));
                    Res = 0;
                    CPU_SET_PSW_FLAGS_B_SET_Z_CLR_NVC ();
                    CPU_WRITE_DB1 ()
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 051: // *051DD   COMB - Complement
                    DEBUG_PRINT (("COMB  "));
                    CPU_READ_DB ();
                    Res = ~ArgD;
                    CPU_SET_PSW_FLAGS_B_NZ_SET_C_CLR_V ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 052: // *052DD   INCB - Increment
                    DEBUG_PRINT (("INCB  "));
                    CPU_READ_DB ();
                    CPU_PREP_FLAGS_DB ();
                    Res = ArgD + 1;
                    CPU_SET_PSW_FLAGS_B_NZV ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 053: // *053DD   DECB - Decrement
                    DEBUG_PRINT (("DECB  "));
                    CPU_READ_DB ();
                    CPU_PREP_FLAGS_DB ();
                    Res = ArgD - 1;
                    CPU_SET_PSW_FLAGS_B_NZV ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 054: // *054DD   NEGB - Negate
                    DEBUG_PRINT (("NEGB  "));
                    CPU_READ_DB ();
                    CPU_PREP_FLAGS_DB ();
                    Res = -ArgD;
                    CPU_SET_PSW_FLAGS_B_NZVC ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 055: // *055DD   ADCB - Add Carry
                    DEBUG_PRINT (("ADCB  "));
                    CPU_READ_DB ();
                    CPU_PREP_FLAGS_DB ();
                    Res = ArgD;
                    if (CPU_IS_FLAGS_CS ()) Res += 1;
                    CPU_SET_PSW_FLAGS_B_NZVC ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 056: // *056DD   SBCB - Subtract Carry
                    DEBUG_PRINT (("SBCB  "));
                    CPU_READ_DB ();
                    CPU_PREP_FLAGS_DB ();
                    Res = ArgD;
                    if (CPU_IS_FLAGS_CS ()) Res -= 1;
                    CPU_SET_PSW_FLAGS_B_NZVC ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 057: // *057DD   TSTB - Test
                    DEBUG_PRINT  (("TSTB  "));
                    CPU_READ_DB  ()
                    Res = ArgD;
                    CPU_SET_PSW_FLAGS_B_NZ_CLR_VC ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_TST);
                    break;

          case 060: // *060DD   RORB - Rotate Right
                    DEBUG_PRINT (("RORB  "));
                    CPU_READ_DB ();
                    Res = ArgD >> 1;
                    if (CPU_IS_FLAGS_CS ()) Res |= 0200;
                    CPU_SET_PSW_FLAGS_B_FOR_RIGHT_SHIFT ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 061: // *061DD   ROLB - Rotate Left
                    DEBUG_PRINT (("ROLB  "));
                    CPU_READ_DB ();
                    Res = ArgD << 1;
                    if (CPU_IS_FLAGS_CS ()) Res |= 1;
                    CPU_SET_PSW_FLAGS_B_FOR_LEFT_SHIFT ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 062: // *062DD   ASRB - Arithmetic Shift Right
                    DEBUG_PRINT (("ASRB  "));
                    CPU_READ_DB ();
                    Res = (uint8_t) ((int8_t) ArgD >> 1);
                    CPU_SET_PSW_FLAGS_B_FOR_RIGHT_SHIFT ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 063: // *063DD   ASLB - Arithmetic Shift Left
                    DEBUG_PRINT (("ASLB  "));
                    CPU_READ_DB ();
                    Res = ArgD << 1;
                    CPU_SET_PSW_FLAGS_B_FOR_LEFT_SHIFT ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          case 064: // 1064DD   MTPS - Move To PSW (Processor Status Word)
                    DEBUG_PRINT (("MTPS  "));
                    CPU_READ_DB ();
                    CPU_SET_PSW_MTPS (ArgD);
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_MTPS);
                    break;

          case 067: // 1067DD   MFPS - Move From PSW
                    DEBUG_PRINT (("MFPS  "));
                    CPU_GET_PSW (ArgS)
                    Res = ArgS & 0377;
                    CPU_SET_PSW_FLAGS_B_NZ_CLR_V ();
                    CPU_WRITE_DB1_MOVE ()
                    CPU_CALC_TIMING_D (CPU_timing_OneOps_CLR);
                    break;

          default:  goto InvalidOpCode;          
                    // 1065-1066
                    // 1070-1077
        }
        break;

        case 011:   // 11SSDD   MOVB
                    DEBUG_PRINT (("MOVB  "));
                    CPU_READ_SB ();
                    Res = ArgS;
                    CPU_SET_PSW_FLAGS_B_NZ_CLR_V ();
                    CPU_WRITE_DB1_MOVE ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_MOV);
                    break;

        case 012:   // 12SSDD   CMPB
                    DEBUG_PRINT (("CMPB  "));
                    CPU_READ_SDB ();
                    CPU_PREP_FLAGS_SDB ();
                    Res = ArgS - ArgD;
                    CPU_SET_PSW_FLAGS_B_NZVC ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_CMP);
                    break;

        case 013:   // 13SSDD   BITB - Bit Test
                    DEBUG_PRINT (("BITB  "));
                    CPU_READ_SDB ();
                    Res = ArgS & ArgD;
                    CPU_SET_PSW_FLAGS_B_NZ_CLR_V ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_CMP);
                    break;

        case 014:   // 14SSDD   BICB - Bit Clear
                    DEBUG_PRINT (("BICB  "));
                    CPU_READ_SDB ();
                    Res = (~ArgS) & ArgD;
                    CPU_SET_PSW_FLAGS_B_NZ_CLR_V ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_BIS);
                    break;

        case 015:   // 15SSDD   BISB - Bit Set
                    DEBUG_PRINT (("BISB  "));
                    CPU_READ_SDB ();
                    Res = ArgS | ArgD;
                    CPU_SET_PSW_FLAGS_B_NZ_CLR_V ();
                    CPU_WRITE_DB ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_BIS);
                    break;

        case 016:   // 16SSDD   SUB
                    DEBUG_PRINT (("SUB   "));
                    CPU_READ_SDW ();
                    CPU_PREP_FLAGS_SDW ();
                    Res = ArgD - ArgS;
                    CPU_SET_PSW_FLAGS_W_NZVC ();
                    CPU_WRITE_DW ();
                    CPU_CALC_TIMING_SD (CPU_timing_TwoOps_BIS);
                    break;

        case 017:   goto InvalidOpCode;
    }

  Exit:

    PSW = Psw;
    DEBUG_PRINT (("  PSW=%o\n", (int) Psw));
    return;

  InvalidOpCode:
    DEBUG_PRINT (("  !!!INVALID OPERATION CODE!!!"));
    CPU_INST_INTERRUPT (04);
    PSW = Psw;
    DEBUG_PRINT (("  PSW=%o\n", (int) Psw));
    CPU_CALC_TIMING (CPU_TIMING_INT);
//  exit (1);
    return;

  BusFault:
    DEBUG_PRINT (("\n!!!BUS FAULT!!!"));
    CPU_INST_INTERRUPT (04);
    PSW = Psw;
    DEBUG_PRINT (("  PSW=%o\n", (int) Psw));
    CPU_CALC_TIMING (CPU_TIMING_BUS_ERROR);
    return;
}

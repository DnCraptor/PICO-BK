#ifndef CPU_H_INCLUDE
#define CPU_H_INCLUDE

#include <stdint.h>
#include "ram_page.h"
#include "ROM10.h"
#include "ROM11.h"
#include "FDDROM.h"
#include "DISK_327ROM.h"
#include "Focal.h"
#include "Tests.h"

#ifdef BOOT_DEBUG || RAM_DEBUG
extern void logMsg(char* msg);
#define printf(...) { char tmp[80]; snprintf(tmp, 80, __VA_ARGS__); logMsg(tmp); }
#ifdef BOOT_DEBUG
#define DEBUG_PRINT( X) printf X
#endif
#ifdef RAM_DEBUG
#define RAM_PRINT( X) printf X
#endif
#else
#define DEBUG_PRINT( X)
#endif
#ifdef KBD_DEBUG
extern void logMsg(char* msg);
#define printf(...) { char tmp[80]; snprintf(tmp, 80, __VA_ARGS__); logMsg(tmp); }
#define KBD_PRINT( X) printf X
#else
#define KBD_PRINT( X)
#endif

typedef enum BK_MODE {
    BK_FDD,
    BK_0010,
    BK_0010_01,
    BK_0011M
} bk_mode_t;

extern bk_mode_t bk0010mode;
void init_rom();

#define RAM_PAGES_SIZE (sizeof RAM)
#define CPU_PAGE0_1_MEM_ADR  &RAM[0x00000] /* RAM Page 0.1 0..40000 it was 0x3FFEC000 */
#define CPU_PAGE0_2_MEM_ADR  &RAM[0x02000] /* RAM Page 0.2 0..40000 it was 0x3FFEC000 */
#define CPU_PAGE1_1_MEM_ADR  &RAM[0x04000] /* RAM Page 1.1          it was 0x3FFF0000 */
#define CPU_PAGE1_2_MEM_ADR  &RAM[0x06000] /* RAM Page 1.2          it was 0x3FFF0000 */
#define CPU_PAGE2_1_MEM_ADR  &RAM[0x14000] /* RAM Page 5.1          it was 0x40104000 */
#define CPU_PAGE2_2_MEM_ADR  &RAM[0x16000] /* RAM Page 5.2          it was 0x40104000 */
#define CPU_PAGE3_1_MEM_ADR  &RAM[0x18000] /* RAM Page 6.1          it was 0x40108000 */
#define CPU_PAGE3_2_MEM_ADR  &RAM[0x1A000] /* RAM Page 6.2          it was 0x40108000 */
#define CPU_PAGE4_1_MEM_ADR  &RAM[0x1C000] /* RAM Page 7.1          it was 0x4010C000 - system */
#define CPU_PAGE4_2_MEM_ADR  &RAM[0x1E000] /* RAM Page 7.2          it was 0x4010C000 - system */
#define CPU_PAGE5_1_MEM_ADR  &RAM[0x10000] /* RAM Page 4.1          it was 0x3FFFC000 - video page 0 */
#define CPU_PAGE5_2_MEM_ADR  &RAM[0x12000] /* RAM Page 4.2          it was 0x3FFFC000 - video page 0 */
#define CPU_PAGE6_1_MEM_ADR  &RAM[0x08000] /* RAM Page 2.1          it was 0x3FFF4000 - video page 1 */
#define CPU_PAGE6_2_MEM_ADR  &RAM[0x0A000] /* RAM Page 2.2          it was 0x3FFF4000 - video page 1 */
#define CPU_PAGE7_1_MEM_ADR  &RAM[0x0C000] /* RAM Page 3.1          it was 0x3FFF8000 */
#define CPU_PAGE7_2_MEM_ADR  &RAM[0x0E000] /* RAM Page 3.2          it was 0x3FFF8000 */
#define CPU_PAGE8_1_MEM_ADR  &ROM11[0x00000] /* ROM11 Page 0.1          it was 0x40250000 - bk11m_328_basic2.rom */
#define CPU_PAGE8_2_MEM_ADR  &ROM11[0x02000] /* ROM11 Page 0.2          it was 0x40250000 - bk11m_329_basic3.rom  */
#define CPU_PAGE9_1_MEM_ADR  &ROM11[0x04000] /* ROM11 Page 1.1          it was 0x40254000 - bk11m_327_basic1.rom */
#define CPU_PAGE9_2_MEM_ADR  &ROM11[0x06000] /* ROM11 Page 1.2          it was 0x40254000 - bk11m_325_ext.rom */
#define CPU_PAGEA_1_MEM_ADR 0 /* &DISK_327ROM[0x00000] /* КНГМД для БК-0011М // &FDDROM[0x00000] КНГМД для БК-0010 */
#define CPU_PAGEA_2_MEM_ADR 0 /* &DISK_327ROM[0x00000] /* КНГМД для БК-0011М // &FDDROM[0x00000] КНГМД для БК-0010 */
#define CPU_PAGEB_1_MEM_ADR 0 /* ? */
#define CPU_PAGEB_2_MEM_ADR 0 /* ? */
#define CPU_PAGEC_1_MEM_ADR &ROM11[0x08000] /* ROM11 Page 2.1          it was 0x40258000 - bk11m_324_bos.rom */
#define CPU_PAGEC_2_MEM_ADR &ROM11[0x0A000] /* ROM11 Page 2.2          it was 0x40258000 - bk11m_330_mstd.rom */

#define CPU_PAGE_0010_01_1_MEM_ADR &ROM10[0x00000] /* monitor 0010-01 */
#define CPU_PAGE_0010_01_2_MEM_ADR &ROM10[0x02000] /* Basic 0010-01 */
#define CPU_PAGE_0010_01_3_MEM_ADR &ROM10[0x04000] /* ... ROM10 0010-01 Basic */
#define CPU_PAGE_0010_01_4_MEM_ADR &ROM10[0x06000] /* ... ROM10 0010-01 Basic */

#define CPU_PAGE_0010_1_MEM_ADR &ROM10[0x00000] /* monitor 0010-01 */
#define CPU_PAGE_0010_2_MEM_ADR &ROM10F[0x00000] /* Focal 0010 */
#define CPU_PAGE_0010_3_MEM_ADR 0 /* not used */
#define CPU_PAGE_0010_4_MEM_ADR &TESTS_ROM[0x00000] /* Tests 0010 */

#define CPU_PAGE_0010N_1_MEM_ADR &ROM10[0x00000] /* monitor 0010-01 */
#define CPU_PAGE_0010N_2_MEM_ADR &RAM[0x04000] /* RAM 8k page 1.1 */
#define CPU_PAGE_0010N_3_MEM_ADR &RAM[0x06000] /* RAM 8k page 1.2 */
#define CPU_PAGE_0010N_4_MEM_ADR &DISK_327ROM[0x00000] /* DISK_327.ROM */

#define CPU_START_IO_ADR 0177600

//#define CPU_PAGE0_MEM8  ((uint8_t  *) CPU_PAGE0_1_MEM_ADR)
#define CPU_PAGE0_MEM16 ((uint16_t *) CPU_PAGE0_1_MEM_ADR)
//#define CPU_PAGE0_MEM32 ((uint32_t *) CPU_PAGE0_1_MEM_ADR)
//#define CPU_PAGE1_MEM32 ((uint32_t *) CPU_PAGE1_1_MEM_ADR)
#define CPU_PAGE5_MEM32 ((uint32_t *) CPU_PAGE5_1_MEM_ADR)
#define CPU_PAGE6_MEM32 ((uint32_t *) CPU_PAGE6_1_MEM_ADR)

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
    uintptr_t MemPages [8];
    struct
    {
        uint32_t PrevT;
        uint8_t  T;
        uint8_t  Div;

    } Timer;
    struct
    {
        uint16_t RdReg177130; // Регистр управления НГМД по чтению
        uint16_t WrReg177130; // Регистр управления НГМД по записи
        uint16_t Reg177132; // Регистр данных НГМД доступен по чтению и записи
        uint16_t Reg177660;
        uint16_t RdReg177662;
        uint16_t WrReg177662;
        uint16_t Reg177664;
        uint16_t Reg177706;
        uint16_t Reg177710;
        uint16_t Reg177712;
        uint16_t RdReg177714;
        uint16_t WrReg177714;
        uint16_t RdReg177716;
        uint16_t WrReg177716;
        uint16_t Wr1Reg177716;
    } SysRegs;

} TDevice_Data;

extern TDevice_Data Device_Data;

// CPU timing � ������
#define CPU_TIMING_BASE         16
#define CPU_TIMING_HALT         96
#define CPU_TIMING_WAIT         16
#define CPU_TIMING_RTI          48
#define CPU_TIMING_RESET      1140
#define CPU_TIMING_BR_BASE      16
#define CPU_TIMING_RTS          32
#define CPU_TIMING_MARK         56
#define CPU_TIMING_EMT          80
#define CPU_TIMING_IOT         128 
#define CPU_TIMING_SOB          21
#define CPU_TIMING_INT          48
#define CPU_TIMING_BUS_ERROR    64
#define CPU_TIMING_STOP         64

//�������������� ���ޣ����
extern const uint8_t CPU_timing_OneOps_TST  [8];
extern const uint8_t CPU_timing_OneOps_CLR  [8];
extern const uint8_t CPU_timing_OneOps_MTPS [8];
extern const uint8_t CPU_timing_OneOps_XOR  [8];
extern const uint8_t CPU_timing_OneOps_JMP  [8];
extern const uint8_t CPU_timing_OneOps_JSR  [8];
//�������������� ���ޣ����
extern const uint8_t CPU_timing_TwoOps_MOV [64];
extern const uint8_t CPU_timing_TwoOps_CMP [64];
extern const uint8_t CPU_timing_TwoOps_BIS [64];

#define CPU_GET_PAGE_ADR_MEM16( Adr) ((uint16_t *) (Device_Data.MemPages [Adr >> 13] + (Adr & 0x1FFE)))
#define CPU_GET_PAGE_ADR_MEM8(  Adr) ((uint8_t  *) (Device_Data.MemPages [Adr >> 13] + (Adr & 0x1FFF)))

//#define MEM8  ((uint8_t  *) 0x3FFE8000)
//#define MEM16 ((uint16_t *) 0x3FFE8000)
//#define MEM32 ((uint32_t *) 0x3FFE8000)

#endif

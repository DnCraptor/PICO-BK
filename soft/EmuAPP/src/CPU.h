#ifndef CPU_H_INCLUDE
#define CPU_H_INCLUDE

#include <stdint.h>
#include "ram_page.h"
#include "ROM10.h"
#include "ROM11.h"
#include "FDDROM.h"
#include "BOS_N_DISK_327.h"
#include "018_bk0010_focal.h"
#include "Tests.h"

#if BOOT_DEBUG || DSK_DEBUG || MNGR_DEBUG || KBD_DEBUG
#include "stdio.h"
extern void logMsg(char* msg);
#define printf(...) { char tmp[80]; snprintf(tmp, 80, __VA_ARGS__); logMsg(tmp); }
#if MNGR_DEBUG
#define DBGM_PRINT( X) printf X
#else
#define DBGM_PRINT( X)
#endif
#if BOOT_DEBUG
#define DEBUG_PRINT( X) printf X
#else
#define DEBUG_PRINT( X)
#endif
#if DSK_DEBUG
#define DSK_PRINT( X) printf X
#else
#define DSK_PRINT( X)
#endif
#else
#define DEBUG_PRINT( X)
#define DSK_PRINT( X)
#define DBGM_PRINT( X)
#endif
#ifdef KBD_DEBUG
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

bk_mode_t get_bk0010mode();
void set_bk0010mode(bk_mode_t mode);
bool is_fdd_suppored();
void init_rom();

#define RAM_PAGES_SIZE (sizeof RAM)
#define CPU_PAGE01_MEM_ADR  &RAM[0x00000] /* RAM Page 0.1 0..40000 */
#define CPU_PAGE02_MEM_ADR  &RAM[0x01000] /* RAM Page 0.2 0..40000 */
#define CPU_PAGE03_MEM_ADR  &RAM[0x02000] /* RAM Page 0.3 0..40000 */
#define CPU_PAGE04_MEM_ADR  &RAM[0x03000] /* RAM Page 0.4 0..40000 */

#define CPU_PAGE11_MEM_ADR  &RAM[0x04000] /* RAM Page 1.1          */
#define CPU_PAGE12_MEM_ADR  &RAM[0x05000] /* RAM Page 1.2          */
#define CPU_PAGE13_MEM_ADR  &RAM[0x06000] /* RAM Page 1.3          */
#define CPU_PAGE14_MEM_ADR  &RAM[0x07000] /* RAM Page 1.4          */

#define CPU_PAGE21_MEM_ADR  &RAM[0x14000] /* RAM Page 5.1          */
#define CPU_PAGE22_MEM_ADR  &RAM[0x15000] /* RAM Page 5.2          */
#define CPU_PAGE23_MEM_ADR  &RAM[0x16000] /* RAM Page 5.3          */
#define CPU_PAGE24_MEM_ADR  &RAM[0x17000] /* RAM Page 5.4          */

#define CPU_PAGE31_MEM_ADR  &RAM[0x18000] /* RAM Page 6.1          */
#define CPU_PAGE32_MEM_ADR  &RAM[0x19000] /* RAM Page 6.2          */
#define CPU_PAGE33_MEM_ADR  &RAM[0x1A000] /* RAM Page 6.3          */
#define CPU_PAGE34_MEM_ADR  &RAM[0x1B000] /* RAM Page 6.4          */

#define CPU_PAGE41_MEM_ADR  &RAM[0x1C000] /* RAM Page 7.1          - system */
#define CPU_PAGE42_MEM_ADR  &RAM[0x1D000] /* RAM Page 7.2          - system */
#define CPU_PAGE43_MEM_ADR  &RAM[0x1E000] /* RAM Page 7.3          - system */
#define CPU_PAGE44_MEM_ADR  &RAM[0x1F000] /* RAM Page 7.4          - system */

#define CPU_PAGE51_MEM_ADR  &RAM[0x10000] /* RAM Page 4.1          - video page 0 */
#define CPU_PAGE52_MEM_ADR  &RAM[0x11000] /* RAM Page 4.2          - video page 0 */
#define CPU_PAGE53_MEM_ADR  &RAM[0x12000] /* RAM Page 4.3          - video page 0 */
#define CPU_PAGE54_MEM_ADR  &RAM[0x13000] /* RAM Page 4.4          - video page 0 */

#define CPU_PAGE61_MEM_ADR  &RAM[0x08000] /* RAM Page 2.1          - video page 1 */
#define CPU_PAGE62_MEM_ADR  &RAM[0x09000] /* RAM Page 2.2          - video page 1 */
#define CPU_PAGE63_MEM_ADR  &RAM[0x0A000] /* RAM Page 2.3          - video page 1 */
#define CPU_PAGE64_MEM_ADR  &RAM[0x0B000] /* RAM Page 2.4          - video page 1 */

#define CPU_PAGE71_MEM_ADR  &RAM[0x0C000] /* RAM Page 3.1          */
#define CPU_PAGE72_MEM_ADR  &RAM[0x0D000] /* RAM Page 3.2          */
#define CPU_PAGE73_MEM_ADR  &RAM[0x0E000] /* RAM Page 3.3          */
#define CPU_PAGE74_MEM_ADR  &RAM[0x0F000] /* RAM Page 3.4          */

#define CPU_PAGE81_MEM_ADR  &ROM11[0x00000] /* ROM11 Page 0.1      - bk11m_328_basic2.rom  */
#define CPU_PAGE82_MEM_ADR  &ROM11[0x01000] /* ROM11 Page 0.2      - bk11m_328_basic2.rom  */
#define CPU_PAGE83_MEM_ADR  &ROM11[0x02000] /* ROM11 Page 0.3      - bk11m_329_basic3.rom  */
#define CPU_PAGE84_MEM_ADR  &ROM11[0x03000] /* ROM11 Page 0.4      - bk11m_329_basic3.rom  */

#define CPU_PAGE91_MEM_ADR  &ROM11[0x04000] /* ROM11 Page 1.1      - bk11m_327_basic1.rom */
#define CPU_PAGE92_MEM_ADR  &ROM11[0x05000] /* ROM11 Page 1.2      - bk11m_327_basic1.rom */
#define CPU_PAGE93_MEM_ADR  &ROM11[0x06000] /* ROM11 Page 1.3      - bk11m_325_ext.rom */
#define CPU_PAGE94_MEM_ADR  &ROM11[0x07000] /* ROM11 Page 1.4      - bk11m_325_ext.rom */

#define CPU_PAGE101_MEM_ADR 0 /* страница ПЗУ3.1 3.2 */
#define CPU_PAGE102_MEM_ADR 0 /* страница ПЗУ3.1 3.2 */
#define CPU_PAGE103_MEM_ADR 0 /* страница ПЗУ3.1 3.2 */
#define CPU_PAGE104_MEM_ADR 0 /* страница ПЗУ3.1 3.2 */
#define CPU_PAGE111_MEM_ADR 0 /* страница ПЗУ4.1 4.1 */
#define CPU_PAGE112_MEM_ADR 0 /* страница ПЗУ4.1 4.1 */
#define CPU_PAGE113_MEM_ADR 0 /* страница ПЗУ4.1 4.1 */
#define CPU_PAGE114_MEM_ADR 0 /* страница ПЗУ4.1 4.1 */
#if NO_FDD_0011M
#define CPU_PAGE12_MEM_ADR &ROM11[0x08000] /* ROM11 Page 2      - bk11m_324_bos.rom + bk11m_330_mstd.rom */
#else
#define CPU_PAGE121_MEM_ADR &BOS_N_DISK_327[0x00000] /* bk11m_324_bos.rom */
#define CPU_PAGE122_MEM_ADR &BOS_N_DISK_327[0x01000] /* bk11m_324_bos.rom */
#define CPU_PAGE123_MEM_ADR &BOS_N_DISK_327[0x02000] /* КНГМД для БК-0011М */
#define CPU_PAGE124_MEM_ADR &BOS_N_DISK_327[0x03000] /* КНГМД для БК-0011М */
#endif
#define CPU_PAGE13x_MEM_ADR TODO /* 16кб: ОЗУ А16М */

#define CPU_PAGE141_MEM_ADR &ROM10[0x00000] /* monitor 0010-01 */
#define CPU_PAGE142_MEM_ADR &ROM10[0x01000] /* monitor 0010-01 */
#define CPU_PAGE143_MEM_ADR &ROM10[0x02000] /* ROM10 Basic 0010-01 */
#define CPU_PAGE144_MEM_ADR &ROM10[0x03000] /* ROM10 Basic 0010-01 */

#define CPU_PAGE151_MEM_ADR &ROM10[0x04000] /* ... ROM10 0010-01 Basic */
#define CPU_PAGE152_MEM_ADR &ROM10[0x05000] /* ... ROM10 0010-01 Basic */
#define CPU_PAGE153_MEM_ADR &ROM10[0x06000] /* ... ROM10 0010-01 Basic */
#define CPU_PAGE154_MEM_ADR &ROM10[0x07000] /* ... ROM10 0010-01 Basic */

#define CPU_PAGE161_MEM_ADR &ROM10[0x00000] /* monitor 0010 */
#define CPU_PAGE162_MEM_ADR &ROM10[0x01000] /* monitor 0010 */
#define CPU_PAGE163_MEM_ADR &FOCAL_018[0x00000] /* Focal 0010 */
#define CPU_PAGE164_MEM_ADR &FOCAL_018[0x01000] /* Focal 0010 */

#define CPU_PAGE171_MEM_ADR 0 // &BK0010_ROM[0x04000] /* not used 8k */
#define CPU_PAGE172_MEM_ADR 0 // &BK0010_ROM[0x05000] /* not used 8k */
#define CPU_PAGE173_MEM_ADR &TESTS_ROM[0x00000] /* Tests 0010 */
#define CPU_PAGE174_MEM_ADR &TESTS_ROM[0x01000] /* Tests 0010 */

#define CPU_PAGE191_MEM_ADR 0 // &FDDROM[0x00000] /* not used */
#define CPU_PAGE192_MEM_ADR 0 // &FDDROM[0x01000] /* not used */
#define CPU_PAGE193_MEM_ADR &FDDROM[0x00000] /* FDD.ROM */
#define CPU_PAGE194_MEM_ADR &FDDROM[0x01000] /* FDD.ROM */

#define CPU_START_IO_ADR 0177600

#define CPU_PAGE01_MEM8  ((uint8_t  *) CPU_PAGE01_MEM_ADR)
#define CPU_PAGE02_MEM8  ((uint8_t  *) CPU_PAGE02_MEM_ADR)
#define CPU_PAGE03_MEM8  ((uint8_t  *) CPU_PAGE03_MEM_ADR)
#define CPU_PAGE04_MEM8  ((uint8_t  *) CPU_PAGE04_MEM_ADR)

#define CPU_PAGE01_MEM16 ((uint16_t *) CPU_PAGE01_MEM_ADR)
#define CPU_PAGE02_MEM16 ((uint16_t *) CPU_PAGE02_MEM_ADR)
#define CPU_PAGE03_MEM16 ((uint16_t *) CPU_PAGE03_MEM_ADR)
#define CPU_PAGE04_MEM16 ((uint16_t *) CPU_PAGE04_MEM_ADR)

#define CPU_PAGE01_MEM32 ((uint32_t *) CPU_PAGE01_MEM_ADR)
#define CPU_PAGE02_MEM32 ((uint32_t *) CPU_PAGE02_MEM_ADR)
#define CPU_PAGE03_MEM32 ((uint32_t *) CPU_PAGE03_MEM_ADR)
#define CPU_PAGE04_MEM32 ((uint32_t *) CPU_PAGE04_MEM_ADR)

#define CPU_PAGE11_MEM32 ((uint32_t *) CPU_PAGE11_MEM_ADR)
#define CPU_PAGE12_MEM32 ((uint32_t *) CPU_PAGE12_MEM_ADR)
#define CPU_PAGE13_MEM32 ((uint32_t *) CPU_PAGE13_MEM_ADR)
#define CPU_PAGE14_MEM32 ((uint32_t *) CPU_PAGE14_MEM_ADR)

#define CPU_PAGE51_MEM32 ((uint32_t *) CPU_PAGE51_MEM_ADR)
#define CPU_PAGE52_MEM32 ((uint32_t *) CPU_PAGE52_MEM_ADR)
#define CPU_PAGE53_MEM32 ((uint32_t *) CPU_PAGE53_MEM_ADR)
#define CPU_PAGE54_MEM32 ((uint32_t *) CPU_PAGE54_MEM_ADR)

#define CPU_PAGE61_MEM32 ((uint32_t *) CPU_PAGE61_MEM_ADR)
#define CPU_PAGE62_MEM32 ((uint32_t *) CPU_PAGE62_MEM_ADR)
#define CPU_PAGE63_MEM32 ((uint32_t *) CPU_PAGE63_MEM_ADR)
#define CPU_PAGE64_MEM32 ((uint32_t *) CPU_PAGE64_MEM_ADR)

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
    PC   = CPU_PAGE01_MEM16 [ (Vec) >> 1     ];                                                         \
    ArgS = CPU_PAGE01_MEM16 [((Vec) >> 1) + 1] & 0377;                                                  \
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
    uintptr_t MemPages [16];
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

// CPU timing в тактах
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

// однооперандные расчётные
extern const uint8_t CPU_timing_OneOps_TST  [8];
extern const uint8_t CPU_timing_OneOps_CLR  [8];
extern const uint8_t CPU_timing_OneOps_MTPS [8];
extern const uint8_t CPU_timing_OneOps_XOR  [8];
extern const uint8_t CPU_timing_OneOps_JMP  [8];
extern const uint8_t CPU_timing_OneOps_JSR  [8];
// двухоперандные расчётные
extern const uint8_t CPU_timing_TwoOps_MOV [64];
extern const uint8_t CPU_timing_TwoOps_CMP [64];
extern const uint8_t CPU_timing_TwoOps_BIS [64];

#define CPU_GET_PAGE_ADR_MEM16( Adr) ((uint16_t *) (Device_Data.MemPages [Adr >> 12] + ((Adr) & 0x0FFE)))
#define CPU_GET_PAGE_ADR_MEM8(  Adr) ((uint8_t  *) (Device_Data.MemPages [Adr >> 12] + ((Adr) & 0x0FFF)))

#endif

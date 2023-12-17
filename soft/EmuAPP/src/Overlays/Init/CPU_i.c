#include <string.h>
#include "board.h"
#include "gpio_lib.h"
#include "ets.h"

#include "CPU.h"
#include "CPU_i.h"

//#define AT_OVL __attribute__((section(".ovl3_i.text")))

void /*AT_OVL*/ CPU_Init (void)
{
    memset (&Device_Data, 0, sizeof (Device_Data));

//  Device_Data.SysRegs.Reg177660   = 0;
//  Device_Data.SysRegs.RdReg177662 = 0;
    Device_Data.SysRegs.Reg177664   = 01330;
//  Device_Data.SysRegs.Reg177706   = 0;
//  Device_Data.SysRegs.Reg177710   = 0177777;
//  Device_Data.SysRegs.Reg177712   = 0177400;
//  Device_Data.SysRegs.RdReg177714 = 0;
    Device_Data.SysRegs.RdReg177716 = (0140000 & 0177400) | 0300;
    Device_Data.SysRegs.WrReg177662  = 047400;
    Device_Data.SysRegs.Wr1Reg177716 = (1 << 12) | 1;

    Device_Data.MemPages [0] = CPU_PAGE0_MEM_ADR; /* RAM Page 0 */
    Device_Data.MemPages [1] = CPU_PAGE5_MEM_ADR; /* RAM Page 4 video 0 */
    Device_Data.MemPages [2] = CPU_PAGE8_MEM_ADR; /* ROM Page 0 - bk11m_328_basic2.rom + bk11m_329_basic3.rom */
    Device_Data.MemPages [3] = CPU_PAGE12_MEM_ADR; /* ROM Page 1 - bk11m_324_bos.rom + bk11m_330_mstd.rom */

    Device_Data.CPU_State.psw   = 0340;
    Device_Data.CPU_State.r [7] = Device_Data.SysRegs.RdReg177716 & 0177400;

    //============================================================================
    //STEP 1: SIGMA-DELTA CONFIG;REG SETUP

    //WRITE_PERI_REG (GPIO_SIGMA_DELTA_ADDRESS,   SIGMA_DELTA_ENABLE
    //                                          | (0x80 << SIGMA_DELTA_TARGET_S)
    //                                          | (1 << SIGMA_DELTA_PRESCALAR_S));

    //============================================================================
    //STEP 2: PIN FUNC CONFIG :SET PIN TO GPIO MODE AND ENABLE OUTPUT

    // ������ ���� �������
 //   gpio_init_output(BEEPER);
//  gpio_on         (BEEPER);

    //============================================================================
    //STEP 3: CONNECT SIGNAL TO GPIO PAD
   // WRITE_PERI_REG (PERIPHS_GPIO_BASEADDR + (10 + BEEPER) * 4, READ_PERI_REG (PERIPHS_GPIO_BASEADDR + (10 + BEEPER) * 4) | 1);
}

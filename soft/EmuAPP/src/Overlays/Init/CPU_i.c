#include <string.h>
#include "board.h"
#include "gpio_lib.h"
#include "ets.h"

#include "CPU.h"
#include "CPU_i.h"

#define AT_OVL __attribute__((section(".ovl3_i.text")))

bk_mode_t bk0010mode = BK_FDD;

void init_rom() {
    switch (bk0010mode) {
        case BK_FDD:
            Device_Data.MemPages [2] = CPU_PAGE16_MEM_ADR; // monitor 8k + focal 8k (masked out)
            Device_Data.MemPages [3] = CPU_PAGE18_MEM_ADR; // masked out 8k + fdd rom
            break;
        case BK_0010:
            Device_Data.MemPages [2] = CPU_PAGE16_MEM_ADR; // monitor 8k + focal 8k
            Device_Data.MemPages [3] = CPU_PAGE17_MEM_ADR; // empty 8k + tests 7.5k
            break;
        case BK_0010_01:
            Device_Data.MemPages [2] = CPU_PAGE14_MEM_ADR; // monitor 8k + basic 8k
            Device_Data.MemPages [3] = CPU_PAGE15_MEM_ADR; // + basic 16k
            break;
        case BK_0011M:
            Device_Data.MemPages [2] = CPU_PAGE8_MEM_ADR; /* ROM Page 0 - bk11m_328_basic2.rom + bk11m_329_basic3.rom */
            Device_Data.MemPages [3] = CPU_PAGE12_MEM_ADR; /* ROM Page 2 - bk11m_324_bos.rom + bk11m_330_mstd.rom */
            break;
    }
}

void AT_OVL CPU_Init (void)
{
    memset (&Device_Data, 0, sizeof (Device_Data));

//  Device_Data.SysRegs.Reg177660   = 0;
//  Device_Data.SysRegs.RdReg177662 = 0;
    Device_Data.SysRegs.Reg177664   = 01330;
//  Device_Data.SysRegs.Reg177706   = 0;
//  Device_Data.SysRegs.Reg177710   = 0177777;
//  Device_Data.SysRegs.Reg177712   = 0177400;
//  Device_Data.SysRegs.RdReg177714 = 0;
    Device_Data.SysRegs.RdReg177716 = ((bk0010mode != BK_0011M ? 0100000 : 0140000) & 0177400) | 0300;
    Device_Data.SysRegs.WrReg177662  = 047400;
    Device_Data.SysRegs.Wr1Reg177716 = (1 << 12) | 1;
/*
 * бит 0: признак 0-ой дорожки
 * бит 1: готовность к работе
 * бит 2: защита от записи
 * бит 7: запрос на чтение/запись данных из регистра данных
 * бит 14: признак записи циклического контрольного кода на диск
 * бит 15: признак 0-го сектора (индексного отверстия)
*/
    Device_Data.SysRegs.RdReg177130 = 0;
/*
 * биты 0-3: выбор накопителя; бит 3: см. примечание (АльтПро)
 * бит 4: включение мотора
 * бит 5: выбор головки, "0" -- верхняя, "1" -- нижняя.
 * бит 6: направление перемещения головок
 * бит 7: шаг (40/80 дорожек?)
 * бит 8: признак "начало чтения"
 * бит 9: признак "запись маркера"
 * бит 10: включение схемы предкоррекции
(АльтПро) При установленном бите 010 при записи в регистр 0177130 для контроллеров с количеством ДОЗУ 64 или 128Кб подключается ПЗУ Бейсика.
 Для остальных контроллеров Бейсик подключается полным его копированием в одну из страниц.
 Однако бит 010 всё равно надо записать в 0177130 (иначе при чтении по адресам портов контроллера будет читаться не Бейсик, а содержимое портов)
*/
    Device_Data.SysRegs.WrReg177130 = 0;
    Device_Data.SysRegs.Reg177132   = 0;

    Device_Data.MemPages [0] = CPU_PAGE0_MEM_ADR; /* RAM Page 0 */
    Device_Data.MemPages [1] = CPU_PAGE5_MEM_ADR; /* RAM Page 4 video 0 */
    init_rom();
    Device_Data.MemPages [4] = CPU_PAGE1_MEM_ADR; // temporary W/A for FDD case

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

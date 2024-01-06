#include <string.h>
#include "CPU_bk.h"
#include "CPU_i.h"

#define AT_OVL __attribute__((section(".ovl3_i.text")))

void init_rom() {
    switch (get_bk0010mode()) {
        case BK_FDD:
            Device_Data.MemPages [8]  = CPU_PAGE161_MEM_ADR; // monitor 8k
            Device_Data.MemPages [9]  = CPU_PAGE162_MEM_ADR; // monitor 8k
            Device_Data.MemPages [10] = CPU_PAGE11_MEM_ADR; // W/A RAM from page 1
            Device_Data.MemPages [11] = CPU_PAGE12_MEM_ADR; // W/A
            Device_Data.MemPages [12] = CPU_PAGE13_MEM_ADR; // W/A
            Device_Data.MemPages [13] = CPU_PAGE14_MEM_ADR; // W/A
            Device_Data.MemPages [14] = CPU_PAGE193_MEM_ADR; // masked out 8k + fdd rom
            Device_Data.MemPages [15] = CPU_PAGE194_MEM_ADR; // masked out 8k + fdd rom
            break;
        case BK_0010:
            Device_Data.MemPages [8]  = CPU_PAGE161_MEM_ADR; // monitor 8k
            Device_Data.MemPages [9]  = CPU_PAGE162_MEM_ADR; // monitor 8k
            Device_Data.MemPages [10] = CPU_PAGE163_MEM_ADR; // focal 8k
            Device_Data.MemPages [11] = CPU_PAGE164_MEM_ADR; // focal 8k
            Device_Data.MemPages [12] = CPU_PAGE171_MEM_ADR; // empty 8k
            Device_Data.MemPages [13] = CPU_PAGE172_MEM_ADR; // empty 8k
            Device_Data.MemPages [14] = CPU_PAGE173_MEM_ADR; // tests 7.5k
            Device_Data.MemPages [15] = CPU_PAGE174_MEM_ADR; // tests 7.5k
            break;
        case BK_0010_01:
            Device_Data.MemPages [8]  = CPU_PAGE141_MEM_ADR; // monitor 8k
            Device_Data.MemPages [9]  = CPU_PAGE142_MEM_ADR; // monitor 8k
            Device_Data.MemPages [10] = CPU_PAGE143_MEM_ADR; // basic 8k
            Device_Data.MemPages [11] = CPU_PAGE144_MEM_ADR; // basic 8k
            Device_Data.MemPages [12] = CPU_PAGE151_MEM_ADR; // + basic 16k
            Device_Data.MemPages [13] = CPU_PAGE152_MEM_ADR; // + basic 16k
            Device_Data.MemPages [14] = CPU_PAGE153_MEM_ADR; // + basic 16k
            Device_Data.MemPages [15] = CPU_PAGE154_MEM_ADR; // + basic 16k
            break;
        case BK_0011M_FDD:
            Device_Data.MemPages [8]  = CPU_PAGE81_MEM_ADR; /* ROM Page 0.1 - bk11m_328_basic2.rom */
            Device_Data.MemPages [9]  = CPU_PAGE82_MEM_ADR; /* ROM Page 0.2 - bk11m_328_basic2.rom */
            Device_Data.MemPages [10] = CPU_PAGE83_MEM_ADR; /* ROM Page 0.3 - bk11m_329_basic3.rom */
            Device_Data.MemPages [11] = CPU_PAGE84_MEM_ADR; /* ROM Page 0.4 - bk11m_329_basic3.rom */
            Device_Data.MemPages [12] = CPU_PAGE121_MEM_ADR; /* ROM Page 2.1 - bk11m_324_bos.rom */
            Device_Data.MemPages [13] = CPU_PAGE122_MEM_ADR; /* ROM Page 2.2 - bk11m_324_bos.rom */
            Device_Data.MemPages [14] = CPU_PAGE123_MEM_ADR; /* ROM Page 2.3 - КНГМД */
            Device_Data.MemPages [15] = CPU_PAGE124_MEM_ADR; /* ROM Page 2.4 - КНГМД */
            break;
        case BK_0011M:
            Device_Data.MemPages [8]  = CPU_PAGE81_MEM_ADR; /* ROM Page 0.1 - bk11m_328_basic2.rom */
            Device_Data.MemPages [9]  = CPU_PAGE82_MEM_ADR; /* ROM Page 0.2 - bk11m_328_basic2.rom */
            Device_Data.MemPages [10] = CPU_PAGE83_MEM_ADR; /* ROM Page 0.3 - bk11m_329_basic3.rom */
            Device_Data.MemPages [11] = CPU_PAGE84_MEM_ADR; /* ROM Page 0.4 - bk11m_329_basic3.rom */
            Device_Data.MemPages [12] = CPU_PAGE131_MEM_ADR; /* ROM Page 2.1 - bk11m_324_bos.rom */
            Device_Data.MemPages [13] = CPU_PAGE132_MEM_ADR; /* ROM Page 2.2 - bk11m_324_bos.rom */
            Device_Data.MemPages [14] = CPU_PAGE133_MEM_ADR; /* ROM Page 2.3 - bk11m_330_mstd.rom */
            Device_Data.MemPages [15] = CPU_PAGE134_MEM_ADR; /* ROM Page 2.4 - bk11m_330_mstd.rom */
            break;
    }

    // ensure initialized mode
    set_bk0010mode(get_bk0010mode());
}

extern volatile uint16_t true_covox; // vga.h
extern bool m_bIRQ2rq;
void init_system_timer(uint16_t* pReg, bool enable);

void AT_OVL CPU_Init (void) {
    memset (&Device_Data, 0, sizeof (Device_Data));
    bool bk11m = is_bk0011mode();
    m_bIRQ2rq = false;

//  Device_Data.SysRegs.Reg177660   = 0;
//  Device_Data.SysRegs.RdReg177662 = 0;
    Device_Data.SysRegs.Reg177664   = 01330;
//  Device_Data.SysRegs.Reg177706   = 0;
//  Device_Data.SysRegs.Reg177710   = 0177777;
//  Device_Data.SysRegs.Reg177712   = 0177400;
//  Device_Data.SysRegs.RdReg177714 = 0;
    Device_Data.SysRegs.RdReg177716 = ((!bk11m ? 0100000 : 0140000) & 0177400) | 0300;
    Device_Data.SysRegs.WrReg177662  = 047400;
    if (bk11m) Device_Data.SysRegs.WrReg177662 &= ~(1 << 14); // TODO: ensure
    Device_Data.SysRegs.Wr1Reg177716 = (1 << 12) | 1;
    // TODO:^ (200)бит 7: включение двигателя магнитофона, "1" -- стоп, "0" -- пуск. Начальное состояние "1".
    true_covox = 0;

    init_system_timer(&Device_Data.SysRegs.WrReg177662, bk11m);

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

    Device_Data.MemPages [0] = CPU_PAGE01_MEM_ADR; /* RAM Page 0.1 */
    Device_Data.MemPages [1] = CPU_PAGE02_MEM_ADR; /* RAM Page 0.2 */
    Device_Data.MemPages [2] = CPU_PAGE03_MEM_ADR; /* RAM Page 0.3 */
    Device_Data.MemPages [3] = CPU_PAGE04_MEM_ADR; /* RAM Page 0.4 */
    Device_Data.MemPages [4] = CPU_PAGE51_MEM_ADR; /* RAM Page 4.1 video 0 */
    Device_Data.MemPages [5] = CPU_PAGE52_MEM_ADR; /* RAM Page 4.2 video 0 */
    Device_Data.MemPages [6] = CPU_PAGE53_MEM_ADR; /* RAM Page 4.3 video 0 */
    Device_Data.MemPages [7] = CPU_PAGE54_MEM_ADR; /* RAM Page 4.4 video 0 */
    init_rom();

    Device_Data.CPU_State.psw   = 0340;
    Device_Data.CPU_State.r [7] = Device_Data.SysRegs.RdReg177716 & 0177400;
}

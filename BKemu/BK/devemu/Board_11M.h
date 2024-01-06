// Board_11M.h: interface for the CMotherBoard_11M class.
//

#pragma once

#include "Board.h"

constexpr auto BRD_11_BASIC2_BNK = 32;
constexpr auto BRD_11_BASIC3_BNK = 34;
constexpr auto BRD_11_BASIC1_BNK = 36;
constexpr auto BRD_11_EXT_BNK    = 38;
constexpr auto BRD_11_BOS_BNK    = 48;
constexpr auto BRD_11_TST_BNK    = 50;
constexpr auto BRD_11_PG12_1_BNK = 40;
constexpr auto BRD_11_PG12_2_BNK = 42;
constexpr auto BRD_11_PG13_1_BNK = 44;
constexpr auto BRD_11_PG13_2_BNK = 46;


class CMotherBoard_11M : public CMotherBoard
{
	protected:
		static const int    m_arPageNums[8];
		static const int    m_arPageCodes[8];

		// Initialize memory
		virtual bool        InitMemoryModules() override;
		virtual void        InitMemoryValues() override;
		virtual void        MemoryManager() override;
		bool                LoadRomModule11(int iniRomNameIndex, int bank);   // загрузка нужного модуля ПЗУ по заданному адресу

		virtual bool        RestoreMemory(CMSFManager &msf) override;

		virtual bool        SetSystemRegister(uint16_t addr, uint16_t src, UINT dwFlags = GSSR_NONE) override;

		virtual bool        Interception() override; // Called after each command

		virtual int         GetScreenPage() const override;

		bool                EmulateLoadTape11();
		bool                EmulateSaveTape11();

		virtual bool        FillWndVectorPtr(int nMemSize) override;

	public:
		CMotherBoard_11M(BK_DEV_MPI model = BK_DEV_MPI::BK0011M);
		virtual ~CMotherBoard_11M() override;

		virtual MSF_CONF    GetConfiguration() override;

		virtual void        StopInterrupt() override;
		virtual void        UnStopInterrupt() override;
		virtual void        ChangePalette() override;
		virtual bool        RestoreState(CMSFManager &msf, HBITMAP hScreenshot) override;
		virtual void        OnReset() override;

		virtual void        Set177716RegMem(uint16_t w) override;
		virtual void        Set177716RegTap(uint16_t w) override;
		virtual void        SetMemPages(int pg0, int pg1) override;
		virtual void        RestoreMemPages() override;

};

/*
    Организация массива памяти (делается один большой массив, чтобы преобразовывать
    адрес из диапазона 64к, в зависимости от того, какие страницы подключены в окна)
    128кб: ОЗУ
    000000--банк 0 ----страница 0 --- 0
    010000--банк 1
    020000--банк 2
    030000--банк 3
    040000--банк 4 ----страница 1 --- 16384
    050000--банк 5
    060000--банк 6
    070000--банк 7
    100000--банк 8 ----страница 2 --- 32768
    110000--банк 9
    120000--банк 10
    130000--банк 11
    140000--банк 12 ---страница 3 --- 49152
    160000--банк 14
    170000--банк 15
    200000--банк 16 ---страница 4 --- 65536
    210000--банк 17
    220000--банк 18
    230000--банк 19
    240000--банк 20 --страница 5 --- 81920 экран 0
    250000--банк 21
    260000--банк 22
    270000--банк 23
    300000--банк 24 --страница 6 --- 98304 экран 1
    310000--банк 25
    320000--банк 26
    330000--банк 27
    340000--банк 28 --страница 7 --- 114688
    350000--банк 29
    360000--банк 30
    370000--банк 31
    80кб: ПЗУ
    400000--банк 32 --страница 8 --- 131072 - Бейсик bk11m_328_basic2.rom
    410000--банк 33
    420000--банк 34 ------------------------- бейсик bk11m_329_basic3.rom
    430000--банк 35
    440000--банк 36 --страница 9 --- 147456 - бейсик bk11m_327_basic1.rom
    450000--банк 37
    460000--банк 38 ------------------------- расширение БОС bk11m_325_ext.rom
    470000--банк 39
    500000--банк 40 --страница 10 -- 163840 - страница ПЗУ3.1 (по умолчанию - пусто, обращение вызывает прерывание по вектору 4)
    510000--банк 41
    520000--банк 42 ------------------------- страница ПЗУ3.2 (по умолчанию - пусто, обращение вызывает прерывание по вектору 4)
    530000--банк 43
    540000--банк 44 --страница 11 -- 180224 - страница ПЗУ4.1 (по умолчанию - пусто, обращение вызывает прерывание по вектору 4)
    550000--банк 45
    560000--банк 46 ------------------------- страница ПЗУ4.2 (по умолчанию - пусто, обращение вызывает прерывание по вектору 4)
    570000--банк 47
    600000--банк 48 --страница 12 -- 196608 - основной БОС bk11m_324_bos.rom
    610000--банк 49
    620000--банк 50 ------------------------- ПЗУ дисковода или МСТД bk11m_330_mstd.rom
    630000--банк 51
    16кб: ОЗУ А16М
    640000--банк 52 --страница 13--- 212992 - ОЗУ контроллера А16М, банк А00
    650000--банк 53                                                 банк А01
    660000--банк 54 --------------------------                      банк А10
    670000--банк 55                                                 банк А11
    700000----------------------- 229376 ---- конец

    стр 10 - две пустые колодки, куда можно вставить 2 пзушки по 8 кб, в корпусе БК,
            а так же сигнал ПЗУ3 на МПИ
    стр 11 - выбирается внешнее ПЗУ в блоках расширения, сигнал ПЗУ4 на МПИ


    не стоит забывать, что логический номер страницы и реальный номер (маска) не совпадают.
    страница    маска
    -----------------
    0           6
    1           0
    2           2
    3           3
    4           4
    5           1
    6           7
    7           5

    как делается ремап
    000000 --- банк 0 - страница 0 ------ нет ремапа
    010000 --- банк 1 - страница 0
    020000 --- банк 2 - страница 0
    030000 --- банк 3 - страница 0
    040000 --- банк 4 - страница 0..7 --- номер стр. окна 0 * 040000
    050000 --- банк 5 - страница 0..7
    060000 --- банк 6 - страница 0..7
    070000 --- банк 7 - страница 0..7
    100000 --- банк 8 - страница 0..7 --- номер стр. окна 1 * 040000, или пзу в зависимости от битов
    110000 --- банк 9 - страница 0..7
    120000 --- банк 10- страница 0..7
    130000 --- банк 11- страница 0..7
    140000 --- банк 12- БОС ------------- банк 48
    150000 --- банк 13- БОС ------------- банк 49
    160000 --- банк 14- пусто или ПЗУ дисковода
    170000 --- банк 15- пусто. или зависит от режима контроллера А16М

*/

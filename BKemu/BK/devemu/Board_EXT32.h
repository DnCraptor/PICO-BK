// Board_EXT32.h: interface for the CMotherBoard_EXT32 class.
//

#pragma once

#include "Board.h"


class CMotherBoard_EXT32 : public CMotherBoard
{
	protected:
		DWORD               m_nPage;

		void                SetMemoryPage(int nPage);

		// Initialize memory
		virtual bool        InitMemoryModules() override;
		virtual void        MemoryManager() override; // подключаем нужный нам банк Доп Озу в 5й банк озу
		virtual bool        FillWndVectorPtr(int nMemSize) override;

	public:
		CMotherBoard_EXT32();
		virtual ~CMotherBoard_EXT32() override;

		virtual MSF_CONF    GetConfiguration() override;

		virtual void        OnReset() override;

		// Methods for loading and saving emulator state
		virtual bool        RestoreState(CMSFManager &msf, HBITMAP hScreenshot) override;

		virtual void        SetByteT(const uint16_t addr, uint8_t value, int &nTC) override;
		virtual void        SetWordT(const uint16_t addr, uint16_t value, int &nTC) override;
		virtual uint8_t    *GetAddMemory() const override;
};

/*
Организация массива памяти (делается один большой массив)
32кб: ОЗУ
000000--банк 0 ---страница 0 --- 0
010000--банк 1
020000--банк 2
030000--банк 3
040000--банк 4 ---страница 1 --- 16384
050000--банк 5
060000--банк 6
070000--банк 7
8кб: ПЗУ
100000--банк 8 ---страница 2 --- 32768 - монитор MONIT10.ROM
110000--банк 9
8кб: доп ОЗУ
120000--банк 10 ------------------------- сюда подключаются дополнительные 8 банков ОЗУ, по две штуки за раз
130000--банк 11
16кб: пусто
140000--банк 12 ---страница 3 --- 49152 - пусто
150000--банк 13
160000--банк 14 ------------------------- пусто
170000--банк 15
32кб: ОЗУ, которое мапится в 10й и 11й банки
200000--банк 16 --- 0 стр
210000--банк 17
220000--банк 18 --- 1 стр
230000--банк 19
240000--банк 20 --- 2 стр
250000--банк 21
260000--банк 22 --- 3 стр
270000--банк 23
300000--конец

проще делать ремап 5го банка в соответствующий банк доп озу, а 5й банк считать пустым
*/
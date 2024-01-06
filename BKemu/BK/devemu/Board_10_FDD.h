// Board_10_FDD.h: interface for the CMotherBoard_10_FDD class.
//


#pragma once

#include "Board.h"


class CMotherBoard_10_FDD : public CMotherBoard
{
		uint16_t m_nFDDCatchAddr, m_nFDDExitCatchAddr;

		BKMEMBank_t         m_MemMapBasic[6];   // специально для бейсика придётся ввести ещё один массив. Тут будут храниться данные, которые замещаются ПЗУ Бейсика
		bool                m_bBasicOn;         // флаг, что бейсик подключён
	protected:
		// Initialize memory
		virtual bool        InitMemoryModules() override;
		virtual void        MemoryManager() override;
		// Methods for loading and saving emulator state
		virtual bool        RestoreMemory(CMSFManager &msf) override;
		// Methods emulate registers behaviour
		virtual bool        SetSystemRegister(uint16_t addr, uint16_t src, UINT dwFlags = GSSR_NONE) override;
		virtual bool        GetSystemRegister(uint16_t addr, void *pDst, UINT dwFlags = GSSR_NONE) override;

		virtual bool        Interception() override; // Called after each command
		virtual bool        FillWndVectorPtr(int nMemSize) override;

	public:
		CMotherBoard_10_FDD();
		virtual ~CMotherBoard_10_FDD() override;

		virtual MSF_CONF    GetConfiguration() override;

		virtual void        OnReset() override;

		virtual void        SetFDDType(BK_DEV_MPI model, bool bInit = true) override;
		virtual BK_DEV_MPI  GetFDDType() override;
		virtual uint8_t    *GetAddMemory() const override;
};

/*
Организация массива памяти (делается один большой массив)
32кб: ОЗУ
000000--банк 0 ---страница 0 --- 0
020000--банк 1
040000--банк 2 ---страница 1 --- 16384
060000--банк 3
32кб: ПЗУ
100000--банк 4 ---страница 2 --- 32768 - монитор MONIT10.ROM
120000--банк 5 ------------------------- бейсик Basic10_1.rom
140000--банк 6 ---страница 3 --- 49152 - бейсик Basic10_2.rom
160000--банк 7 ------------------------- бейсик и регистры Basic10_2.rom
8кб: ПЗУ
200000--банк 8
16кб: ОЗУ
220000--банк 9 дополнительные 16 кб озу
240000--банк 10
260000 - конец
*/
// Board_11M_FDD.h: interface for the CMotherBoard_11M_FDD class.
//


#pragma once

#include "Board_11M.h"
/*
    Организация массива памяти (делается один большой массив, чтобы преобразовывать
    адрес из диапазона 64к, в зависимости от того, какие страницы подключены в окна)

    см. в файле Board_11M.h, там сразу под дисковод рассчитывалось
*/

class CMotherBoard_11M_FDD : public CMotherBoard_11M
{
		uint16_t m_nFDDCatchAddr, m_nFDDExitCatchAddr;
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

	public:
		CMotherBoard_11M_FDD();
		virtual ~CMotherBoard_11M_FDD() override;

		virtual MSF_CONF    GetConfiguration() override;

		virtual void        OnReset() override;

		virtual void        SetFDDType(BK_DEV_MPI model, bool bInit = true) override;
		virtual BK_DEV_MPI  GetFDDType() override;
		virtual uint8_t    *GetAddMemory() const override;
};

#pragma once
#include "emu2149.h"

class CAYSnd
{
	protected:
		CEMU2149            m_ay8910[AY_NUMS];      // объект сопроцессор Ay8910-3
		int                 m_nChipSel;             // номер выбранного чипа в данный момент

		// -----------------------------
		// логирование регистров
		bool m_bLog; // флаг работы логирования
		bool m_bLogRegOut;
		FILE *m_pLOGFile;
		// -----------------------------

	public:
		CAYSnd();
		~CAYSnd();
		void    EnableSound(bool bEnable);
		bool    IsSoundEnabled() const;
		void    SetFilter(bool bEnable);
		bool    IsFilter() const;
		void    SetDCOffsetCorrect(bool bEnable);
		bool    IsDCOffsetCorrect() const;
		void    SetStereo(bool bEnable);
		void    ReInit();
		void    Reset();
		void    write_address(uint16_t word);
		void    write_data(uint8_t byte);
		void    GetSample1(sOneSample *pSm);
		void    GetSample2(sOneSample *pSm);


		bool                isLogEnabled() const
		{
			return m_bLog;
		}
		// фиксируем тик таймера 50Гц
		void                log_timerTick();
		// запускаем логирование
		void                log_start(const CString &strUniq);
		// завершаем логирование
		void                log_done();


};

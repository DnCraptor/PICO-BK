// BKSoundDevice.h: interface for the CBKSoundDevice class.
//


#pragma once

#include "BKSound_Defines.h"
//#include "libdspl-2.0.h"
#include "BKMessageBox.h"


constexpr auto DCOFFSET_BUFFER_MASK = 0xff; // степень двойки минус 1
constexpr auto DCOFFSET_BUFFER_LEN = (DCOFFSET_BUFFER_MASK + 1); // степень двойки

// длина ких фильтра. нечётная. чтоб симметричную характеристику строить
constexpr auto FIR_LENGTH = 127;

class CBKSoundDevice
{
	protected:
		bool            m_bEnableSound;     // флаг разрешения звука
		bool            m_bFilter;          // флаг включения фильтрации звука
		bool            m_bDCOffset;        // включить функцию корректировки смещения
		bool            m_bStereo;

		// переменные для вычисления смещения DC.
		int             m_nBufferPosL, m_nBufferPosR;
		double          m_dAvgL, m_dAvgR;
		std::unique_ptr<double[]> m_pdBufferL;
		std::unique_ptr<double[]> m_pdBufferR;

		std::unique_ptr<double[]> m_pH;     // коэффициенты КИХ
		int             m_nFirLength;       // размер буфера коэффициентов
		std::unique_ptr<double[]> m_pLeftBuf;  // буферы для фильтра левый канал
		std::unique_ptr<double[]> m_pRightBuf; // правый канал
		int             m_nLeftBufPos;      // позиции в буферах фильтра
		int             m_nRightBufPos;

		// накопители значения сэмпла, значимо только последнее на момент выборки значение
		double          m_dLeftAcc, m_dRightAcc;

		// эмулятор конденсатора
		struct sRCFparam
		{
			bool        bRCProc;    // флаг, обозначает, что идёт в данный момент false - разряд true - заряд
			double      ft;         // аккумулятор временного интервала
			SAMPLE_INT  fmaxvol;
			SAMPLE_INT  fminvol;
			SAMPLE_INT  fdeltavol;
			SAMPLE_INT  fUi_prev;
			sRCFparam()
				: bRCProc(false)
				, ft(0.0)
				, fmaxvol(0.0)
				, fminvol(0.0)
				, fdeltavol(0.0)
				, fUi_prev(0.0)
			{};
		};
		sRCFparam       m_RCFL, m_RCFR;
		double          m_RCFVal;

		void            RCFilter(sRCFparam &p, const double fAcc, const double fTime) const;
		inline SAMPLE_INT   RCFilterCalc(sRCFparam &p) const;
		void            SetFCFilterValue(const double v)
		{
			m_RCFVal = v;
		}

		double          DCOffset(double sample, double &fAvg, double *pBuf, int &nBufPos) const;
		double          FIRFilter(double sample, double *pBuf, int &nBufPos) const;

		bool            CreateFIRBuffers(int nLen);
	public:
		CBKSoundDevice();
		virtual ~CBKSoundDevice();
		virtual void    ReInit() {}
		virtual void    Reset() {}

		inline void     EnableSound(bool bEnable)
		{
			m_bEnableSound = bEnable;

			if (!bEnable)
			{
				m_dLeftAcc = m_dRightAcc = 0.0;
			}
		}
		inline bool     IsSoundEnabled() const
		{
			return m_bEnableSound;
		}

		inline void     SetFilter(bool bEnable)
		{
			m_bFilter = bEnable;
		}
		inline bool     IsFilter() const
		{
			return m_bFilter;
		}
		inline void     SetDCOffsetCorrect(bool bEnable)
		{
			m_bDCOffset = bEnable;
		}
		inline bool     IsDCOffsetCorrect() const
		{
			return m_bDCOffset;
		}
		inline void     SetStereo(bool bEnable)
		{
			m_bStereo = bEnable;
		};
		inline bool     IsStereo() const
		{
			return m_bStereo;
		};

		virtual void    SetData(uint16_t inVal) {}

		virtual void    GetSample(sOneSample *pSm) {}
		void            RCFilterL(const double fTime)
		{
			RCFilter(m_RCFL, m_dLeftAcc, fTime);
		}
		void            RCFilterR(const double fTime)
		{
			RCFilter(m_RCFR, m_dRightAcc, fTime);
		}
		void            RCFilterLF(const double fTime)
		{
			if (m_bFilter)
			{
				RCFilter(m_RCFL, m_dLeftAcc, fTime);
			}
		}
		void            RCFilterRF(const double fTime)
		{
			if (m_bFilter)
			{
				RCFilter(m_RCFR, m_dRightAcc, fTime);
			}
		}
};


#pragma once

#include "BKSoundDevice.h"
#include "Config.h"

class CEMU2149 : public CBKSoundDevice
{
		int m_nChip; // номер чипа

#pragma pack(push)
#pragma pack(1) // всё должно быть именно так, несмотря на предупреждения всяких анализаторов. Я лучше знаю.
		struct AYREGS
		{
			uint16_t fA, fB, fC;   // 0,1; 2,3; 4,5;
			uint8_t noise, mix;    // 6; 7;
			uint8_t vA, vB, vC;    // 8; 9; 10;
			uint16_t envT;         // 11,12;
			uint8_t env;           // 13;
			uint8_t portA, portB;  // 14; 15;
		};

		union
		{
			struct AYREGS m_r; // обозначения регистров для более удобного доступа к их значениям.
			uint8_t m_reg[16]; // содержимое регистров
		};

#pragma pack(pop)

		static const int    m_voltbl[2][32];
		double              m_vols[32];
		double              m_dPanKoeff[AY_PAN_BASE + 1];

		int                 m_nClock, m_nRate;
		uint32_t            m_nBaseIncrement;
		uint32_t            m_nBaseCount;

		/* m_nRate converter*/
		double              m_dRealStep;
		double              m_dPSGTime;
		double              m_dPSGStep;

		bool                m_bHQ;              // флаг работы в высоком качестве (всегда true)
		uint8_t             m_nSynthReg;        // номер последнего выбранного регистра для обмена данными

		struct SoundChannel
		{
			int     nCount;
			int     nVolume;    // громкость звучания канала
			int     nFreq;      // частота ноты канала
			bool    bEnv;       // флаг, что включена огибающая на канале
			bool    bToneToggle;
			bool    bToneMask;  // флаг звучания ноты
			bool    bNoiseMask; // флаг звучания шума
			// указатели на переменные из конфига, для того, чтобы на лету всё это менялось
			double *pVolume;    // громкость канала
			int    *pPanL;      // панорамирование влево
			int    *pPanR;      // панорамирование вправо
		};

		struct SoundChannel m_Channel[AY_CHANS];

		int                 m_nEnvPtr;
		int                 m_nDEnv;
		bool                m_bEnvLValue;
		bool                m_bEnvAttack;
		bool                m_bEnvHold;
		bool                m_bEnvAlt;
		int                 m_nEnvFreq;
		int                 m_nEnvCount;

		uint32_t            m_nNoiseSeed;
		int                 m_nNoiseCount;
		int                 m_nNoiseFreq;

		/* I/O Ctrl*/
		uint8_t             m_nAddr;

		double              m_dMixL, m_dMixR;
		void                calc();


	public:
		CEMU2149();
		void                setChip(int nChip);
		virtual ~CEMU2149() override;
		virtual void        ReInit() override;
		virtual void        Reset() override;

		virtual void        GetSample(sOneSample *pSm) override;

		void                PSG_reset();
		void                PSG_init(int c, int r);
		void                PSG_set_quality(bool q);
		void                PSG_setVolumeMode();
		void                PSG_writeReg(uint8_t reg, uint8_t val);
		void                PSG_writeIO(uint8_t nAddr, uint8_t val);
		uint8_t             PSG_readReg(uint8_t reg) const;
		uint8_t             PSG_readIO() const;
		void                PSG_calc(SAMPLE_INT &L, SAMPLE_INT &R);

		// Writing address
		void                synth_write_address(uint8_t addr);

		// Writing data
		void                synth_write_data(uint8_t byte);

};



#pragma once

#include "BKSoundDevice.h"
#include "Config.h"

class CEMU8930 : public CBKSoundDevice
{
		static const double m_dutycycle[9];
		int m_nChip;

#pragma pack(push)
#pragma pack(1) // всё должно быть именно так, несмотря на предупреждения всяких анализаторов. Я лучше знаю.
		struct AYREGS_8910
		{
			uint16_t    fA, fB, fC;    // 0,1; 2,3; 4,5;
			uint8_t     noise, mix;    // 6; 7;
			uint8_t     vA, vB, vC;    // 8; 9; 10;
			uint16_t    envT;          // 11,12;
			uint8_t     env;           // 13;
			uint8_t     portA, portB;  // 14; 15;
		};
		struct AYREGS_8930
		{
			uint16_t    fA, fB, fC;    // 0,1; 2,3; 4,5;
			uint8_t     noise, mix;    // 6; 7;
			uint8_t     vA, vB, vC;    // 8; 9; 10;
			uint16_t    envTA;         // 11,12;
			uint8_t     envA;          // 13;
			uint8_t     portA, portB;  // 14; 15;
			uint16_t    envTB;         // 16,17;
			uint16_t    envTC;         // 18,19;
			uint8_t     envB;          // 20;
			uint8_t     envC;          // 21;
			uint8_t     dutyCycleA;    // 22;
			uint8_t     dutyCycleB;    // 23;
			uint8_t     dutyCycleC;    // 24;
			uint8_t     noiseANDMask;  // 25;
			uint8_t     noiseORMask;   // 26;
			uint8_t     reserved13b;   // 27*;
			uint8_t     reserved14b;   // 28*;
			uint8_t     envA15b;       // 29; - дубль, или тут значимы только 4 старших бита
			uint8_t     reserved16b;   // 30*;
			uint8_t     test;          // 31*; *-регистры недоступны
		};

		union
		{
			struct AYREGS_8910 m_RLegacy; // обозначения регистров для более удобного доступа к их значениям.
			struct AYREGS_8930 m_RExpand;
			uint8_t m_reg[32]; // содержимое регистров
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

		bool                m_bHQ;
		uint8_t             m_nSynthReg;
		bool                m_bExpandedMode;
		int                 m_nExpandedBank;

		struct SoundChannel
		{
			int     nCount;
			int     nVolume;        // громкость звучания канала
			int     nFreq;          // частота ноты канала
			bool    bEnv;           // флаг, что включена огибающая на канале
			bool    bToneToggle;
			bool    bToneMask;      // флаг звучания ноты
			bool    bNoiseMask;     // флаг звучания шума
			int     nDutyCycle;     // скважность
			// всё для огибающей, тут их три на каждый канал.
			int     nEnvPtr;
			int     nDEnv;
			bool    bEnvLValue;
			bool    bEnvAttack;
			bool    bEnvHold;
			bool    bEnvAlt;
			int     nEnvFreq;
			int     nEnvCount;
			// для duty cycle
			double  frqT[2];
			int     frqT_t;
			int     phaseT;

			double  frqE[2];
			int     frqE_t;
			int     phaseE;

			// указатели на переменные из конфига
			double *pVolume;        // громкость канала
			int    *pPanL;          // панорамирование влево
			int    *pPanR;          // панорамирование вправо
		};

		struct SoundChannel m_Channel[AY_CHANS];

		uint32_t            m_nNoiseSeed, m_nNoiseValue;
		int                 m_nNoiseCount;
		int                 m_nNoiseFreq;
		uint8_t             m_nNoiseORMask, m_nNoiseANDMask;

		/* I/O Ctrl*/
		uint8_t             m_nAddr;

		double              m_dMixL, m_dMixR;
		void                calc();
		void                calcDutyE(SoundChannel &ch); //расчёт частоты с учётом dutyCycle для огибающей
		void                calcDutyT(SoundChannel &ch); //расчёт частоты с учётом dutyCycle для тона
	protected:

	public:
		CEMU8930();
		void                setChip(int nChip);
		virtual ~CEMU8930() override;
		virtual void        ReInit() override;
		virtual void        Reset() override;

		virtual void        GetSample(sOneSample *pSm) override;

		void                PSG_reset();
		void                PSG_init(int c, int r);
		void                PSG_set_quality(bool q);
		void                PSG_setVolumeMode();
		void                PSG_writeReg(uint8_t reg, uint8_t val);
		void                PSG_writeIO(uint8_t nAddr, uint8_t val);
		uint8_t             PSG_readReg(int reg);
		uint8_t             PSG_readIO();
		void                PSG_calc(SAMPLE_INT &L, SAMPLE_INT &R);

		// Writing address
		void                synth_write_address(uint8_t addr)
		{
			m_nSynthReg = addr;
		}

		// Writing data
		void                synth_write_data(uint8_t byte)
		{
			PSG_writeReg(m_nSynthReg, byte);
		}
};

/*
Проверил работу AY8930.
Таки да. Она не абсолютно совместима с AY8910.
В AY8910 миксер не влияет на амплитуду волнового пакета, она там выставляется независимо от разрешения каналов.
А в AY8930 очень даже влияет. И если в миксере запретить тон и шум в канале, то и волновой пакет тоже не работает,
т.е. может зависит только от тона, настолько тщательно я не проверял.
В общем, различие только в способе звучания чистого волнового пакета, во всём остальном - никакой разницы,
работает 1:1, только генератор шума генерирует более приятно звучащий шум.
для AY8910 традиционно чистый волпак включался так:

movb #77,mixer
movb #20,chan_x_amp ;выставляем амплитуду канала, в котором будет звучать чистый волновой пакет
mov #period,period_vp
movb #form,form_vp

И AY8910 начинает жужжать, так, как велит ему волпак. А AY8930 при этом молчит, как партизан.
Но есть способ заставить его воспроизвести чистый волновой пакет.
Это делается так:

mov #0,chan_b_tone ;в регистре тона обязательно должен быть 0
movb #75,mixer ;включаем звучание тона, разрешая этим волпак, в канале B
movb #20,chan_b_amp ;выставляем амплитуду канала, в котором будет звучать чистый волновой пакет
mov #period,period_vp
movb #form,form_vp

И AY8930 начинает жужжать точно так же как и AY8910.
Но при этом способе уже AY8910 молчит как партизан.
Таким образом, использовать AY8930 можно. Но не вместо AY8910, а как отдельный муз.сопр., для которого надо
будет немного подправить код плееров.
*/

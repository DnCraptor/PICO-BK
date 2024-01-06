#pragma once

#include "BKSoundDevice.h"


class Counter8253
{
		int         m_nMode;            // режим работы счётчика
		int         m_nRL;              // режим чтения/записи
		bool        m_bBCD;             // тип счёта: true - двоично-десятичный, false - двоичный
		uint16_t    m_nValue;           // текущее значение счётчика ("CE" in Intel docs)
		uint16_t    m_nCounter;         // новое значение счётчика ("CR" in Intel docs)
		bool        m_bwmsb;            // true = следующая запись будет старший байт 16-битного значения
		int         m_phase;            // see phase definition tables in simulate2(), below
		uint8_t     m_lowcount;         // младший байт нового значения счётчика для 16-битной записи
		bool        m_gate;             // разрешение выхода (0 = low, 1 = high)
		bool        m_nOutput;          // значение на выходе

		// загрузка нового значения счётчика
		void        load_count(uint16_t newcount);
		// установка выходного значения счётчика
		void        set_output(bool v);
		//
		void        load_counter_value();
		int         adjusted_count();
		void        decrease_counter_value(int32_t cycles);

	public:
		Counter8253() : m_nMode(0), m_nRL(0), m_bBCD(false), m_nValue(0), m_nCounter(0)
			, m_bwmsb(false), m_phase(0), m_lowcount(0), m_gate(false), m_nOutput(true)
		{}
		~Counter8253() = default;
		void        reset();
		void        simulate_1();
		void        simulate_0();
		void        SetControl(uint8_t ctrl);
		void        SetCount(uint8_t cnt);
		void        SetGate(bool state);
		bool        GetOutput();
};

class CMenestrel : public CBKSoundDevice
{
		bool        m_bGatePrev;
		bool        m_bWrPrev;

		int         m_nClock, m_nRate;

		/* m_nRate converter*/
		double      m_dRealStep;
		double      m_dPSGTime;
		double      m_dPSGStep;

		Counter8253 LeftCh[3];
		Counter8253 RightCh[3];

		void        calc();

	public:
		CMenestrel();
		virtual ~CMenestrel() override;

		void            init(int c, int r);
		virtual void    ReInit() override;
		virtual void    Reset() override;

		// запись данных. мл.байт - данные, ст.байт - адрес и управление
		virtual void    SetData(uint16_t inVal) override;
		// получение текущего сэмпла
		virtual void    GetSample(sOneSample *pSm) override;

};


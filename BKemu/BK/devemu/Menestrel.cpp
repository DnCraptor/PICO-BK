#include "pch.h"
#include "Menestrel.h"
#include "Config.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// установка режимов работы счётчика
void Counter8253::SetControl(uint8_t ctrl)
{
	// по мотивам алгоритмов из MAME
	int nRL = (ctrl >> 4) & 3;

	if (nRL == 0)
	{
		/* Experimentally verified: this command does not affect the mode control */
	}
	else
	{
		m_bBCD = !!(ctrl & 1);
		m_nRL = nRL;
		m_nMode = (ctrl >> 1) & 7;

		if (m_nMode > 5)
		{
			m_nMode &= 3;
		}

		m_bwmsb = false;
		m_phase = 0; /* Phase 0 is always the phase after a mode control write */
		set_output(m_nMode ? true : false);
	}
}

void Counter8253::load_count(uint16_t newcount)
{
	// алгоритм взят из MAME
	/* Значение счётчика 1 запрещено в режимах 2 и 3. Что происходит в этом случае
	определено экспериментально. */
	switch (m_nMode)
	{
		case 2:
			if (newcount == 1)
			{
				newcount++;
			}

			if (m_phase == 0)
			{
				m_phase = 1;
			}

			break;

		case 3:
			if (newcount == 1)
			{
				newcount = 0;
			}

			if (m_phase == 0)
			{
				m_phase = 1;
			}

			break;

		case 0:
		case 4:
			m_phase = 1;
			break;
	}

	m_nCounter = newcount;
}

void Counter8253::set_output(bool v)
{
	// алгоритм взят из MAME
	m_nOutput = v;
}

void Counter8253::load_counter_value()
{
	// алгоритм взят из MAME
	m_nValue = m_nCounter;
}

int Counter8253::adjusted_count()
{
	// алгоритм взят из MAME
	uint16_t val = m_nValue;

	if (!m_bBCD)
	{
		return (val == 0) ? 0x10000 : val;
	}

	if (val == 0)
	{
		return 10000;
	}

	/* In BCD mode, a nybble loaded with value A-F counts down the same as in
	   binary mode, but wraps around to 9 instead of F after 0, so loading the
	   count with 0xFFFF gives a period of
	          0xF  - for the units to count down to 0
	   +   10*0xF  - for the tens to count down to 0
	   +  100*0xF  - for the hundreds to count down to 0
	   + 1000*0xF  - for the thousands to count down to 0
	   = 16665 cycles
	*/
	return
	    ((val >> 12) & 0xF) * 1000 +
	    ((val >> 8) & 0xF) * 100 +
	    ((val >> 4) & 0xF) * 10 +
	    (val & 0xF);
}

/* This function subtracts 1 from m_nValue "cycles" times, taking into
   account binary or BCD operation, and wrapping around from 0 to 0xFFFF or
   0x9999 as necessary. */
void Counter8253::decrease_counter_value(int32_t cycles)
{
	// алгоритм взят из MAME
	if (!m_bBCD)
	{
		m_nValue -= (cycles & 0xffff);
		return;
	}

	uint8_t units = m_nValue & 0xf;
	uint8_t tens = (m_nValue >> 4) & 0xf;
	uint8_t hundreds = (m_nValue >> 8) & 0xf;
	uint8_t thousands = (m_nValue >> 12) & 0xf;

	if (cycles <= units)
	{
		units -= cycles;
	}
	else
	{
		cycles -= units;
		units = (10 - cycles % 10) % 10;
		cycles = (cycles + 9) / 10; /* the +9 is so we get a carry if cycles%10 wasn't 0 */

		if (cycles <= tens)
		{
			tens -= cycles;
		}
		else
		{
			cycles -= tens;
			tens = (10 - cycles % 10) % 10;
			cycles = (cycles + 9) / 10;

			if (cycles <= hundreds)
			{
				hundreds -= cycles;
			}
			else
			{
				cycles -= hundreds;
				hundreds = (10 - cycles % 10) % 10;
				cycles = (cycles + 9) / 10;
				thousands = (10 + thousands - cycles % 10) % 10;
			}
		}
	}

	m_nValue = (thousands << 12) | (hundreds << 8) | (tens << 4) | units;
}

void Counter8253::reset()
{
	SetGate(false);
	set_output(true);
	m_phase = 0;
}

// на входе 1 такт
void Counter8253::simulate_1()
{
	// алгоритм взят из MAME
	switch (m_nMode)
	{
		case 0:

			/* Mode 0: (Interrupt on Terminal Count)

			          +------------------
			          |
			----------+
			  <- n+1 ->

			  ^
			  +- counter load

			phase|output|length  |value|next|comment
			-----+------+--------+-----+----+----------------------------------
			    0|low   |infinity|     |1   |waiting for count
			    1|low   |1       |     |2   |internal delay when counter loaded
			    2|low   |n       |n..1 |3   |counting down
			    3|high  |infinity|0..1 |3   |counting down

			Gate level sensitive only. Low disables counting, high enables it. */
			switch (m_phase)
			{
				case 1:
					/* Counter load cycle */
					m_phase = 2;
					load_counter_value();
					break;

				case 2:
					if (m_gate)
					{
						if (adjusted_count() <= 1)
						{
							/* Counter wrapped, output goes high */
							m_phase = 3;
							m_nValue = 0;
							set_output(true);
						}
						else
						{
							decrease_counter_value(1);
						}
					}

					break;

				case 3:
					if (m_gate)
					{
						decrease_counter_value(1);
					}

					break;
			}

			break;

		case 1:

			/* Mode 1: (Hardware Retriggerable One-Shot a.k.a. Programmable One-Shot)

			-----+       +------------------
			     |       |
			     +-------+
			     <-  n  ->

			  ^
			  +- trigger

			phase|output|length  |value|next|comment
			-----+------+--------+-----+----+----------------------------------
			    0|high  |infinity|     |1   |counting down
			    1|high  |1       |     |2   |internal delay to load counter
			    2|low   |n       |n..1 |3   |counting down
			    3|high  |infinity|0..1 |3   |counting down

			Gate rising-edge sensitive only.
			Rising edge initiates counting and resets output after next clock. */
			switch (m_phase)
			{
				case 1:
					/* Counter load cycle, output goes low */
					m_phase = 2;
					load_counter_value();
					set_output(false);
					break;

				case 2:
					if (adjusted_count() <= 1)
					{
						/* Counter wrapped, output goes high */
						m_phase = 3;
						set_output(true);
					}

					decrease_counter_value(1);
					break;

				case 0:
				case 3:
					decrease_counter_value(1);
					break;
			}

			break;

		case 2:

			/* Mode 2: (Rate Generator)

			--------------+ +---------+ +----
			              | |         | |
			              +-+         +-+
			   <-    n    -X-    n    ->
			              <1>
			^
			+- counter load or trigger

			phase|output|length  |value|next|comment
			-----+------+--------+-----+----+----------------------------------
			    0|high  |infinity|     |1   |waiting for count
			    1|high  |1       |     |2   |internal delay to load counter
			    2|high  |n       |n..2 |3   |counting down
			    3|low   |1       |1    |2   |reload counter

			Counter rewrite has no effect until repeated

			Gate rising-edge and level sensitive.
			Gate low disables counting and sets output immediately high.
			Rising-edge reloads count and initiates counting
			Gate high enables counting. */
			if (!m_gate)
			{
				/* Gate low or mode control write forces output high */
				set_output(true);
			}
			else
			{
				switch (m_phase)
				{
					case 0:
						/* Gate low or mode control write forces output high */
						set_output(true);
						break;

					case 1:
						m_phase = 2;
						load_counter_value();
						break;

					case 2:
						if (adjusted_count() <= 2)
						{
							/* Counter hits 1, output goes low */
							m_phase = 3;
							set_output(false);
						}

						/* Calculate counter value */
						decrease_counter_value(1);
						break;

					case 3:
						m_phase = 2;
						load_counter_value();
						set_output(true);
						break;
				}
			}

			break;

		case 3:

			/* Mode 3: (Square Wave Generator)

			----------------+           +-----------+           +----
			                |           |           |           |
			                +-----------+           +-----------+
			    <- (n+1)/2 -X-   n/2   ->
			 ^
			 +- counter load or trigger

			phase|output|length  |value|next|comment
			-----+------+--------+-----+----+----------------------------------
			    0|high  |infinity|     |1   |waiting for count
			    1|high  |1       |     |2   |internal delay to load counter
			    2|high  |n/2(+1) |n..0 |3   |counting down double speed, reload counter
			    3|low   |n/2     |n..0 |2   |counting down double speed, reload counter

			Counter rewrite has no effect until repeated (output falling or rising)

			Gate rising-edge and level sensitive.
			Gate low disables counting and sets output immediately high.
			Rising-edge reloads count and initiates counting
			Gate high enables counting. */
			if (!m_gate)
			{
				/* Gate low or mode control write forces output high */
				set_output(true);
			}
			else
			{
				switch (m_phase)
				{
					case 0:
						/* Gate low or mode control write forces output high */
						set_output(true);
						break;

					case 1:
						m_phase = 2;
						load_counter_value();
						break;

					case 2:
						if (((adjusted_count() + 1) >> 1) <= 1)
						{
							/* High phase expired, output goes low */
							m_phase = 3;
							load_counter_value();
							set_output(false);
						}

						decrease_counter_value(1);
						break;

					case 3:
						if ((adjusted_count() >> 1) <= 1)
						{
							/* Low phase expired, output goes high */
							m_phase = 2;
							load_counter_value();
							m_nValue &= 0xfffe; // в этом месте нужно скорректировать значение
							set_output(true);
						}

						decrease_counter_value(1);
						break;
				}
			}

			break;

		case 4:
		case 5:

			/* Mode 4: (Software Trigger Strobe)
			   Mode 5: (Hardware Trigger Strobe)

			--------------+ +--------------------
			              | |
			              +-+
			    <-  n+1  ->
			    ^         <1>
			    +- counter load (mode 4) or trigger (mode 5)

			phase|output|length  |value|next|comment
			-----+------+--------+-----+----+----------------------------------
			    0|high  |infinity|0..1 |0   |waiting for count/counting down
			    1|high  |1       |     |2   |internal delay when counter loaded
			    2|high  |n       |n..1 |3   |counting down
			    3|low   |1       |0    |0   |strobe

			Mode 4 only: counter rewrite loads new counter
			Mode 5 only: count not reloaded immediately.
			Mode control write doesn't stop count but sets output high

			Mode 4 only: Gate level sensitive only. Low disables counting, high enables it.
			Mode 5 only: Gate rising-edge sensitive only. Rising edge initiates counting */
			if (m_gate || m_nMode == 5)
			{
				switch (m_phase)
				{
					case 0:
						break;

					case 1:
					{
						m_phase = 2;
						load_counter_value();
						int adjusted_value = (m_nValue == 0) ? 0 : adjusted_count();

						if (adjusted_value <= 0)
						{
							/* Counter has hit zero, set output to low */
							m_phase = 3;
							m_nValue = 0;
							set_output(false);
						}
					}
					break;

					case 2:
					{
						int adjusted_value = (m_nValue == 0) ? 0 : adjusted_count();
						int elapsed_cycles = 1;

						if (adjusted_value <= 1)
						{
							/* Counter has hit zero, set output to low */
							elapsed_cycles -= adjusted_value;
							m_phase = 3;
							m_nValue = 0;
							set_output(false);
						}

						decrease_counter_value(elapsed_cycles);
					}
					break;

					case 3:
						m_phase = 0;
						decrease_counter_value(1);
						set_output(true);
						break;
				}
			}

			break;
	}
}

// на входе 0 тактов
// обычно тут делается только загрузка нового значения счётчика
// но иногда - не только.
void Counter8253::simulate_0()
{
	// алгоритм взят из MAME
	switch (m_nMode)
	{
		case 0:

			/* Mode 0: (Interrupt on Terminal Count)

			          +------------------
			          |
			----------+
			  <- n+1 ->

			  ^
			  +- counter load

			phase|output|length  |value|next|comment
			-----+------+--------+-----+----+----------------------------------
			    0|low   |infinity|     |1   |waiting for count
			    1|low   |1       |     |2   |internal delay when counter loaded
			    2|low   |n       |n..1 |3   |counting down
			    3|high  |infinity|0..1 |3   |counting down

			Gate level sensitive only. Low disables counting, high enables it. */
			switch (m_phase)
			{
				case 1:
					/* Counter load cycle */
					load_counter_value();
					break;

				case 2:
					if (m_gate)
					{
						if (adjusted_count() <= 0)
						{
							/* Counter wrapped, output goes high */
							m_phase = 3;
							m_nValue = 0;
							set_output(true);
						}
					}

					break;
			}

			break;

		case 1:

			/* Mode 1: (Hardware Retriggerable One-Shot a.k.a. Programmable One-Shot)

			-----+       +------------------
			     |       |
			     +-------+
			     <-  n  ->

			  ^
			  +- trigger

			phase|output|length  |value|next|comment
			-----+------+--------+-----+----+----------------------------------
			    0|high  |infinity|     |1   |counting down
			    1|high  |1       |     |2   |internal delay to load counter
			    2|low   |n       |n..1 |3   |counting down
			    3|high  |infinity|0..1 |3   |counting down

			Gate rising-edge sensitive only.
			Rising edge initiates counting and resets output after next clock. */
			switch (m_phase)
			{
				case 1:
					/* Counter load cycle, output goes low */
					load_counter_value();
					set_output(false);
					break;

				case 2:
					if (adjusted_count() <= 0)
					{
						/* Counter wrapped, output goes high */
						m_phase = 3;
						set_output(true);
					}

					break;
			}

			break;

		case 2:

			/* Mode 2: (Rate Generator)

			--------------+ +---------+ +----
			              | |         | |
			              +-+         +-+
			   <-    n    -X-    n    ->
			              <1>
			^
			+- counter load or trigger

			phase|output|length  |value|next|comment
			-----+------+--------+-----+----+----------------------------------
			    0|high  |infinity|     |1   |waiting for count
			    1|high  |1       |     |2   |internal delay to load counter
			    2|high  |n       |n..2 |3   |counting down
			    3|low   |1       |1    |2   |reload counter

			Counter rewrite has no effect until repeated

			Gate rising-edge and level sensitive.
			Gate low disables counting and sets output immediately high.
			Rising-edge reloads count and initiates counting
			Gate high enables counting. */
			if (!m_gate)
			{
				/* Gate low or mode control write forces output high */
				set_output(true);
			}
			else
			{
				switch (m_phase)
				{
					case 0:
						/* Gate low or mode control write forces output high */
						set_output(true);
						break;

					case 1:
						load_counter_value();
						break;

					case 2:
						if (adjusted_count() <= 1)
						{
							/* Counter hits 1, output goes low */
							m_phase = 3;
							set_output(false);
						}

						break;
				}
			}

			break;

		case 3:

			/* Mode 3: (Square Wave Generator)

			----------------+           +-----------+           +----
			                |           |           |           |
			                +-----------+           +-----------+
			    <- (n+1)/2 -X-   n/2   ->
			 ^
			 +- counter load or trigger

			phase|output|length  |value|next|comment
			-----+------+--------+-----+----+----------------------------------
			    0|high  |infinity|     |1   |waiting for count
			    1|high  |1       |     |2   |internal delay to load counter
			    2|high  |n/2(+1) |n..0 |3   |counting down double speed, reload counter
			    3|low   |n/2     |n..0 |2   |counting down double speed, reload counter

			Counter rewrite has no effect until repeated (output falling or rising)

			Gate rising-edge and level sensitive.
			Gate low disables counting and sets output immediately high.
			Rising-edge reloads count and initiates counting
			Gate high enables counting. */
			if (!m_gate || m_phase == 0)
			{
				/* Gate low or mode control write forces output high */
				set_output(true);
			}
			else if (m_phase == 1)
			{
				load_counter_value();
			}

			break;

		case 4:
		case 5:

			/* Mode 4: (Software Trigger Strobe)
			   Mode 5: (Hardware Trigger Strobe)

			--------------+ +--------------------
			              | |
			              +-+
			    <-  n+1  ->
			    ^         <1>
			    +- counter load (mode 4) or trigger (mode 5)

			phase|output|length  |value|next|comment
			-----+------+--------+-----+----+----------------------------------
			    0|high  |infinity|0..1 |0   |waiting for count/counting down
			    1|high  |1       |     |2   |internal delay when counter loaded
			    2|high  |n       |n..1 |3   |counting down
			    3|low   |1       |0    |0   |strobe

			Mode 4 only: counter rewrite loads new counter
			Mode 5 only: count not reloaded immediately.
			Mode control write doesn't stop count but sets output high

			Mode 4 only: Gate level sensitive only. Low disables counting, high enables it.
			Mode 5 only: Gate rising-edge sensitive only. Rising edge initiates counting */
			if (m_gate || m_nMode == 5)
			{
				switch (m_phase)
				{
					case 1:
						load_counter_value();
						break;

					case 2:
						if (((m_nValue == 0) ? 0 : adjusted_count()) <= 0)
						{
							/* Counter has hit zero, set output to low */
							m_phase = 3;
							m_nValue = 0;
							set_output(false);
						}

						break;
				}
			}

			break;
	}
}

void Counter8253::SetCount(uint8_t cnt)
{
	switch (m_nRL)
	{
		case 0:
			// какая-то хрень. и она игнорируется.
			break;

		case 1:
			// запись только мл.байта
			load_count(cnt);
			simulate_0();

			if (m_nMode == 0)
			{
				set_output(false);
			}

			break;

		case 2:
			// запись только старшего байта
			load_count(cnt << 8);
			simulate_0();

			if (m_nMode == 0)
			{
				set_output(false);
			}

			break;

		case 3:

			// запись сперва мл. затем старшего байта
			if (m_bwmsb)
			{
				load_count(m_lowcount | (cnt << 8));
				simulate_0();
			}
			else
			{
				m_lowcount = cnt;

				if (m_nMode == 0)
				{
					/* The Intel docs say that writing the MSB in mode 0, phase
					   2 won't stop the count, but this was experimentally
					   determined to be false. */
					m_phase = 0;
					set_output(false);
				}
			}

			m_bwmsb = !m_bwmsb;
			break;
	}
}

void Counter8253::SetGate(bool state)
{
	if (state != m_gate)
	{
		m_gate = state;

		if (state && (m_nMode == 1 || m_nMode == 2 || m_nMode == 5))
		{
			m_phase = 1;
		}
	}
}

bool Counter8253::GetOutput()
{
	return m_gate ? !m_nOutput : false;
}

CMenestrel::CMenestrel()
	: m_nClock(0)
	, m_nRate(0)
	, m_bWrPrev(false)
{
	SetFCFilterValue(1e-7 * 9000);

	if (CreateFIRBuffers(FIR_LENGTH))
	{
		ReInit();
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}

CMenestrel::~CMenestrel()
    = default;

void CMenestrel::init(int c, int r)
{
	m_nClock = c;
	m_nRate = r;
	m_dRealStep = 1.0 / double(m_nRate);
	m_dPSGStep = 1.0 / double(m_nClock);
}

void CMenestrel::ReInit()
{
	// 1000000 - тактовая частота для i8253
	init(1000000, g_Config.m_nSoundSampleRate);
	Reset();
	double w0 = 2 * 11000.0 / double(g_Config.m_nSoundSampleRate);
	double w1 = 0.0;
	int res = fir_linphase(m_nFirLength, w0, w1, FIR_FILTER::LOWPASS,
	                       FIR_WINDOW::BLACKMAN_HARRIS, true, 0.0, m_pH.get());
}

void CMenestrel::Reset()
{
	m_dPSGTime = 0;
	m_bWrPrev = false;
	m_bGatePrev = false;

	for (auto &cnt : LeftCh)
	{
		cnt.reset();
	}

	for (auto &cnt : RightCh)
	{
		cnt.reset();
	}
}


void CMenestrel::SetData(uint16_t inVal)
{
	// в inVal в старшем байте управляющие биты, а в младшем - данные
	inVal = ~inVal; // проинвертируем
	bool bGate = !!(inVal & 0100000); // старший бит - разрешение выхода всех счётчиков

	if (bGate != m_bGatePrev)
	{
		LeftCh[0].SetGate(bGate);
		LeftCh[1].SetGate(bGate);
		LeftCh[2].SetGate(bGate);
		RightCh[0].SetGate(bGate);
		RightCh[1].SetGate(bGate);
		RightCh[2].SetGate(bGate);
		m_bGatePrev = bGate;
	}

	bool bWr = !!(inVal & 010000);

	if (m_bWrPrev && !bWr) // строб переднего фронта (инверсно, поэтому - строб заднего)
	{
		int nReg = (inVal >> 8) & 3;
		bool bSelLeft = !!(inVal & 02000);
		bool bSelRight = !!(inVal & 04000);
		inVal &= 0xff;

		if (bSelLeft)
		{
			if (nReg < 3)
			{
				LeftCh[nReg].SetCount(inVal);
			}
			else
			{
				// задаём режим управления счётчиком
				int nCnt = (inVal >> 6) & 3;

				if (nCnt < 3)
				{
					LeftCh[nCnt].SetControl(inVal);
				}
			}
		}

		if (bSelRight)
		{
			if (nReg < 3)
			{
				RightCh[nReg].SetCount(inVal);
			}
			else
			{
				// задаём режим управления счётчиком
				int nCnt = (inVal >> 6) & 3;

				if (nCnt < 3)
				{
					RightCh[nCnt].SetControl(inVal);
				}
			}
		}
	}

	m_bWrPrev = bWr;
}


void CMenestrel::calc()
{
	constexpr double b = double(MAX_SAMPLE) / FLOAT_BASE;
	m_dLeftAcc = m_dRightAcc = 0.0;

	// дальше надо считать периоды отдельных счётчиков
	for (auto &cnt : LeftCh)
	{
		cnt.simulate_1();
//      double vol = cnt.GetOutput() ? b : 0.0;
//      m_dLeftAcc = m_dLeftAcc + vol - m_dLeftAcc * vol;
		double vol = cnt.GetOutput() ? b / 3.0 : 0.0;
		m_dLeftAcc += vol;
	}

	for (auto &cnt : RightCh)
	{
		cnt.simulate_1();
//      double vol = cnt.GetOutput() ? b : 0.0;
//      m_dRightAcc = m_dRightAcc + vol - m_dRightAcc * vol;
		double vol = cnt.GetOutput() ? b / 3.0 : 0.0;
		m_dRightAcc += vol;
	}
}


void CMenestrel::GetSample(sOneSample *pSm)
{
	SAMPLE_INT l = 0.0;
	SAMPLE_INT r = 0.0;

	if (m_bEnableSound)
	{
//      double f_mixL1 = 0.0;
//      double f_mixR1 = 0.0;
//      int n = 0;

		/* Simple m_nRate converter */
		do
		{
			calc();
			RCFilterL(m_dPSGStep);
			RCFilterR(m_dPSGStep);
//          f_mixL1 += RCFilterCalc(m_RCFL);
//          f_mixR1 += RCFilterCalc(m_RCFR);
//          n++;
			m_dPSGTime += m_dPSGStep;
		}
		while (m_dPSGTime < m_dRealStep);

		m_dPSGTime -= m_dRealStep;
//      l = SAMPLE_INT(f_mixL1 / n);
//      r = SAMPLE_INT(f_mixR1 / n);
		l = RCFilterCalc(m_RCFL);
		r = RCFilterCalc(m_RCFR);
	}

	if (m_bDCOffset)
	{
		// для чистоты огибающей это лучше не включать.
		l = DCOffset(l, m_dAvgL, m_pdBufferL.get(), m_nBufferPosL);
		r = DCOffset(r, m_dAvgR, m_pdBufferR.get(), m_nBufferPosR);
	}

	// фильтр
	pSm->s[OSL] = FIRFilter(l, m_pLeftBuf.get(), m_nLeftBufPos);
	pSm->s[OSR] = FIRFilter(r, m_pRightBuf.get(), m_nRightBufPos);
}


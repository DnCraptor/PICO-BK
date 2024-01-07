/****************************************************************************

emu2149.c -- YM2149/AY-3-8910 emulator by Mitsutaka Okazaki 2001

2001 04-28 : Version 1.00beta -- 1st Beta Release.
2001 08-14 : Version 1.10
2001 10-03 : Version 1.11     -- Added PSG_set_quality().

References:
psg.vhd        -- 2000 written by Kazuhiro Tsujikawa.
s_fme7.c       -- 1999,2000 written by Mamiya (NEZplug).
ay8910.c       -- 1998-2001 Author unknown (MAME).
MSX-Datapack   -- 1991 ASCII Corp.
AY-3-8910 data sheet

теперь уже мало что осталось от оригинала, добавлены более качественные
алгоритмы из ZX Speccy

Все расчёты внутри делаются в double
так что нет никаких ограничений против того, чтобы сохранять результаты в любом
формате, хоть 24 бит, хоть 32, хоть fp64, с частотой дискретизации хоть 192кГц
была бы только необходимость
*****************************************************************************/
#include "pch.h"
#include "emu2149.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

constexpr auto GETA_BITS = 24;

CEMU2149::CEMU2149()
	: m_nChip(0)
	, m_nClock(0)
	, m_nRate(0)
	, m_nSynthReg(0xff)
{
	if (CreateFIRBuffers(FIR_LENGTH))
	{
		ReInit();
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}

void CEMU2149::setChip(int nChip)
{
	m_nChip = nChip;
}

CEMU2149::~CEMU2149()
    = default;

void CEMU2149::ReInit()
{
	PSG_init(g_Config.m_nSoundChipFrequency * 2, g_Config.m_nSoundSampleRate);
	PSG_reset(); // использует значения, заданные в PSG_init
	PSG_setVolumeMode();
	double w0 = 2 * 12000.0 / double(g_Config.m_nSoundSampleRate);
	double w1 = 0.0;
///	int res = fir_linphase(m_nFirLength, w0, w1, FIR_FILTER::LOWPASS,
	///                       FIR_WINDOW::BLACKMAN_HARRIS, true, 0.0, m_pH.get());
}

void CEMU2149::PSG_init(int c, int r)
{
	m_nClock = c;
	m_nRate = r;
	PSG_set_quality(true);
}

void CEMU2149::PSG_set_quality(bool q)
{
	m_bHQ = q;

	if (m_bHQ && m_nRate) // если включено высшее качество и задана частота дискретизации
	{
		m_nBaseIncrement = 1 << GETA_BITS;
		m_dRealStep = 1.0 / double(m_nRate);
		m_dPSGStep = 16.0 / double(m_nClock);
	}
	else
	{
		// если нет чего-то одного из этого, то режим HQ невозможен
		m_bHQ = false;
		m_dRealStep = 0.0;
		m_dPSGStep = 0.0;

		if (m_nRate)
		{
			m_nBaseIncrement = int(double(1 << GETA_BITS) * double(m_nClock) / double(m_nRate << 4));
		}
		else
		{
			m_nBaseIncrement = int(double(1 << GETA_BITS) * double(m_nClock) / double(1 << 4));
		}
	}
}


const int CEMU2149::m_voltbl[2][32] =
{
	{
		0x0000, 0x0000, 0x0340, 0x0340, 0x04C0, 0x04C0, 0x06F2, 0x06F2, 0x0A44, 0x0A44, 0x0F13, 0x0F13, 0x1510, 0x1510, 0x227E, 0x227E,
		0x289F, 0x289F, 0x414E, 0x414E, 0x5B21, 0x5B21, 0x7258, 0x7258, 0x905E, 0x905E, 0xB550, 0xB550, 0xD7A0, 0xD7A0, 0xFFFF, 0xFFFF
	},
	{
		0x0000, 0x0000, 0x00EF, 0x01D0, 0x0290, 0x032A, 0x03EE, 0x04D2, 0x0611, 0x0782, 0x0912, 0x0A36, 0x0C31, 0x0EB6, 0x1130, 0x13A0,
		0x1751, 0x1BF5, 0x20E2, 0x2594, 0x2CA1, 0x357F, 0x3E45, 0x475E, 0x5502, 0x6620, 0x7730, 0x8844, 0xA1D2, 0xC102, 0xE0A2, 0xFFFF
	}
};


void CEMU2149::PSG_setVolumeMode()
{
	int type = g_Config.m_nSoundChipModel[m_nChip];
	auto p = &g_Config.m_AYParam[m_nChip];
	//указатели на переменные в конфиге, чтобы громкость и панорамирование применялась на лету
	m_Channel[CHAN_A].pVolume = &(p->V[CHAN_A]);
	m_Channel[CHAN_A].pPanL =   &(p->nPL[CHAN_A]);
	m_Channel[CHAN_A].pPanR =   &(p->nPR[CHAN_A]);
	m_Channel[CHAN_B].pVolume = &(p->V[CHAN_B]);
	m_Channel[CHAN_B].pPanL =   &(p->nPL[CHAN_B]);
	m_Channel[CHAN_B].pPanR =   &(p->nPR[CHAN_B]);
	m_Channel[CHAN_C].pVolume = &(p->V[CHAN_C]);
	m_Channel[CHAN_C].pPanL =   &(p->nPL[CHAN_C]);
	m_Channel[CHAN_C].pPanR =   &(p->nPR[CHAN_C]);

	if (type > MODEL_YM2149)
	{
		type = MODEL_YM2149;
	}

	for (int i = 0; i < 32; ++i)
	{
		m_vols[i] = double(m_voltbl[type][i]) / double(0x10000 * AY_CHANS);
	}

	// рассчитаем таблицу значений коэффициентов умножения амплитуды при позиционировании
	// 0 - паннинг в противоположный канал,
	// 100 - паннинг в рабочий канал,
	// промежуточные значения - где-то между каналами.
	// 100 - это количество позиций слайдера, его масштаб
	// 0.25 - усиление для центрального канала, чтобы слишком тихо не казалось
	// 50 - половина от 100, 2500 - квадрат половины от 100
	// расчёт по простенькому квадратному уравнению.
	// хотя там наверное логарифм должен быть

	for (int n = 0; n <= AY_PAN_BASE; ++n)
	{
		m_dPanKoeff[n] = double(n) / double(AY_PAN_BASE);
		// и поправочный коэффициент
		int m = n - AY_PAN_BASE / 2;
		double k = 0.25 * (double(m * m) / double(AY_PAN_BASE * AY_PAN_BASE) - 1.0); //отрицательный
		m_dPanKoeff[n] -= k; // поэтому вычитаем, чтобы увеличить
	}
}


void CEMU2149::PSG_reset()
{
	m_nBaseCount = 0;
	m_dPSGTime = 0;
	m_nAddr = 0;
	// чистим массив регистров
	memset(m_reg, 0, 16);

	for (auto &i : m_Channel)
	{
		i.nCount = 0;
		i.nVolume = 0;
		i.nFreq = 1;
		i.bEnv = false;
		i.bToneToggle = false;
		i.bNoiseMask = false;
		i.bToneMask = false;
	}

	m_nNoiseSeed = 1111;
	m_nNoiseCount = 0;
	m_nNoiseFreq = 0;
	m_nEnvPtr = 0;
	m_nDEnv = 0;
	m_nEnvFreq = 1;
	m_nEnvCount = 0;
	m_bEnvAttack = false;
	m_bEnvLValue = false;
	m_bEnvHold = false;
	m_bEnvAlt = false;
}

uint8_t CEMU2149::PSG_readIO() const
{
	return m_reg[m_nAddr];
}

uint8_t CEMU2149::PSG_readReg(uint8_t r) const
{
	return m_reg[r & 0x0F];
}

void CEMU2149::PSG_writeIO(uint8_t nAddr, uint8_t val)
{
	if (nAddr & 1)   // если адрес регистра нечётный
	{
		PSG_writeReg(m_nAddr, val);    // то запись значения по адресу, определяемому 4-мя мл. битами
	}
	else
	{
		m_nAddr = val & 0x0f;    // а иначе - значение и есть адрес, который потом используется в PSG_readIO
	}

	// это наверное какой-то алгоритм, используемый в Z80
}

void CEMU2149::PSG_writeReg(uint8_t rg, uint8_t val)
{
	if (rg > 0x0f)
	{
		// будем игнорировать номера регистров больше 15.
		return;
	}

	//rg &= 0x0F;
	uint32_t t = (1 << rg);

	if (t & ((1 << 1) | (1 << 3) | (1 << 5) | (1 << 13)))
	{
		val &= 0x0F;
	}
	else if (t & ((1 << 6) | (1 << 8) | (1 << 9) | (1 << 10)))
	{
		val &= 0x1F;
	}

	m_reg[rg] = val;

	switch (rg)
	{
		// 0,2,4 - младшие 8 бит частоты каналов A,B,C
		// 1,3,5 - старшие 4 бита частоты каналов A,B,C
		case 0:
		case 1:
			m_Channel[CHAN_A].nFreq = m_r.fA;

			if (!m_Channel[CHAN_A].nFreq)
			{
				m_Channel[CHAN_A].bToneToggle = false;
			}

			break;

		case 2:
		case 3:
			m_Channel[CHAN_B].nFreq = m_r.fB;

			if (!m_Channel[CHAN_B].nFreq)
			{
				m_Channel[CHAN_B].bToneToggle = false;
			}

			break;

		case 4:
		case 5:
			m_Channel[CHAN_C].nFreq = m_r.fC;

			if (!m_Channel[CHAN_C].nFreq)
			{
				m_Channel[CHAN_C].bToneToggle = false;
			}

			break;

		// регистр генератор шума
		case 6:
			m_nNoiseFreq = (val << 1);   // 0..3e
			break;

		// регистр управления смесителем, 0 - разрешено, 1 - запрещено
		// биты 2..0 - флаг разрешения тона каналов C,B,A
		// биты 5..3 - флаг разрешения шума каналов C,B,A
		// биты 7..6 - флаг ввода/вывода в порты B,A: 0 - in, 1 - out
		/*
		    на AY8910 если регистром-миксером отключить тон и шум то при записи
		    потоковых данных в регистр громкости AY превращается в 4-х битный
		    ЦАП. таким образом на нем проигрывают оцифрованные записи.
		    Если в регистре громкости выставить в 1 пятый бит, то на выход
		    попрёт форма огибающей, чем успешно пользуются музыканты.
		    т.е. если в регистре громкости 020, то огибающая будет звучать
		    независимо от того, включён ли канал в миксере.
		    В AY8930 эту особенность судя по всему не учли.
		*/
		case 7:
			m_Channel[CHAN_A].bToneMask  = !(val & (1 << 0)); // тон true - включён, false - выключен
			m_Channel[CHAN_B].bToneMask  = !(val & (1 << 1));
			m_Channel[CHAN_C].bToneMask  = !(val & (1 << 2));
			m_Channel[CHAN_A].bNoiseMask = !(val & (1 << 3)); // шум true - включён, false - выключен
			m_Channel[CHAN_B].bNoiseMask = !(val & (1 << 4));
			m_Channel[CHAN_C].bNoiseMask = !(val & (1 << 5));
			break;

		// амплитуда каналов A,B,C, мл 4 бита - амплитуда, бит 5 - V - флаг выбора волнового пакета,
		case 8:
		case 9:
		case 10:
			rg -= 8;
			m_Channel[rg].bEnv = !!(val & 0x10);
			m_Channel[rg].nVolume = ((val & 0x0f) << 1) + 1;
			break;

		// 11 - младшие 8 бит периода волнового пакета
		// 12 - старшие 8 бит периода волнового пакета
		case 11:
		case 12:
			m_nEnvFreq = int(m_r.envT);
			break;

		/*форма волнового пакета
		    бит 4 - флаг цикличности, 1 - действует в зависимости от бита 1, один цикл или бесконечно ,
		        0 - в конце цикла счётчик сбросится в 0 и зафиксирует это значение
		    бит 3 - атака, 1 - возрастание, счёт от 0000 до 1111, 0 - затухание, счёт от 1111 до 0000
		    бит 2 - alternate, 1 - счётчик огибающей меняет направление счёта каждый цикл.
		    бит 1 - удержание, 1 - ограничивается одним циклом, зафиксировав последнее значение счётчика
		    огибающей, 0000 или 1111, зависит от того, какой был отсчёт, по восходящей или по нисходящей
		*/
		case 13:
			if (val < 4)
			{
				val = 9;
			}
			else if (val < 8)
			{
				val = 15;
			}

			m_nEnvCount = 0;
			m_bEnvHold = !!(val & 1);
			m_bEnvAlt = !!(val & 2);
			m_bEnvAttack = !!(val & 4);
			m_bEnvLValue = m_bEnvAttack ^ m_bEnvAlt;
			m_bEnvAlt = m_bEnvAlt && !m_bEnvHold;

			if (m_bEnvAttack)
			{
				m_nEnvPtr = 0;
				m_nDEnv = 1;
			}
			else
			{
				m_nEnvPtr = 0x1f;
				m_nDEnv = -1;
			}

			break;

		case 14:
		case 15:
		default:
			break;
	}
}

void CEMU2149::calc()
{
	/* по даташиту  частота огибающей fE = fmaster / (256*EP) = (fmaster/16) / (EP << 4);
	*               частота тона      fT = fmaster / (16*TP)  = (fmaster/16) / TP;
	*               частота шума      fN = fmaster / (16*NP)  = (fmaster/16) / NP;
	* fmaster - частота работы чипа,
	* TP - значение регистра тона (12бит)
	* NP - значение регистра шума (5бит)
	* EP - значение регистра огибающей (16бит)
	*
	* т.е. видим, что тон расширяется до 16 битного значения
	*  шум расширяется до 9 битного значения
	*  и огибающая - до 24 битного
	*
	* А тут что-то такого не видно. И по ощущениям, тож не делается 256*EP
	* т.е. m_nPSGStep рассчитывается с учётом что базовая частота поделена на 16
	* а где тут ещё 1/16 в счётчике огибающей тогда? И кто врёт?

	 * Если включён HQ, то с каждым вызовом incr всегда будет равен 1
	 * если выключен - всегда больше 1, чтобы с большим шагом считать и меньшей достоверностью.
	 */
	m_nBaseCount += m_nBaseIncrement;
	const int incr = (m_nBaseCount >> GETA_BITS);
	m_nBaseCount &= (1 << GETA_BITS) - 1;
	/* Считаем огибающую волнового пакета*/
	m_nEnvCount += incr;

	if (m_nEnvCount >= m_nEnvFreq)
	{
		m_nEnvCount = 0;
		m_nEnvPtr += m_nDEnv;

		if (m_nEnvPtr & ~0x1f) // Цикл кончился?
		{
			if (m_bEnvAlt)  // если бит 2
			{
				// меняем направление счёта
				m_nDEnv = -m_nDEnv;
				m_nEnvPtr += m_nDEnv;
			}
			else
			{
				// или начинаем заново
				m_nEnvPtr &= 0x1f;
			}

			if (m_bEnvHold) // если бит 1 - то прекращаем счёт
			{
				m_nDEnv = 0;
				m_nEnvPtr = m_bEnvLValue ? 0x1f : 0;
			}
		}
	}

	/* Считаем шум */
	m_nNoiseCount += incr;

	if (m_nNoiseCount >= m_nNoiseFreq)
	{
		m_nNoiseCount = 0;
		// шум, вычисляемый по методу из aymi
		const uint32_t bit0x3 = (m_nNoiseSeed ^ (m_nNoiseSeed >> 2)) & 1;
		m_nNoiseSeed >>= 1;
		m_nNoiseSeed |= (bit0x3 << 16);
	}

	const bool bNoiseToggle = !!(m_nNoiseSeed & 2);
	m_dMixL = 0.0;
	m_dMixR = 0.0;

	for (auto &i : m_Channel)
	{
		i.nCount += incr;

		if (i.nCount >= i.nFreq)
		{
			i.nCount = 0;

			if (i.nFreq)
			{
				i.bToneToggle ^= true;
			}
		}

		// пользуемся тем фактом, что true == 0x1, false == 0x0
		const bool isON = (i.bToneToggle & i.bToneMask) | (bNoiseToggle & i.bNoiseMask);

		if (!isON)
		{
			double v = m_vols[i.bEnv ? m_nEnvPtr : i.nVolume];

			if (v)
			{
				v *= (*i.pVolume);
				// классическое микширование суммированием сигнала, но звук тихий получается
				m_dMixL += v * m_dPanKoeff[(*i.pPanL)];
				m_dMixR += v * m_dPanKoeff[(*i.pPanR)];
			}
		}
	}
}
/*
Z=A+B-AB.  - микширование двух сигналов
T=Z+C-ZC=A+B+C-AB-AC-BC+ABC. - микширование трёх сигналов
при микшировании каналов, если у них максимальная амплитуда - получается пердёж и сильные искажения

*/


void CEMU2149::PSG_calc(SAMPLE_INT &L, SAMPLE_INT &R)
{
	if (m_bHQ)
	{
		double f_mixL1 = 0.0;
		double f_mixR1 = 0.0;
		int n = 0;

		/* Simple m_nRate converter */
		do
		{
			calc();
			f_mixL1 += m_dMixL;
			f_mixR1 += m_dMixR;
			n++;
			m_dPSGTime += m_dPSGStep;
		}
		while (m_dPSGTime < m_dRealStep);

		m_dPSGTime -= m_dRealStep;
		L = SAMPLE_INT(f_mixL1 / n); // если выбирать среднее арифметическое, то шум становится мягким
		R = SAMPLE_INT(f_mixR1 / n);
		// L = m_dMixL; // а если выбирать отдельные сэмплы, качество на слух не страдает,
		// R = m_dMixR; // но шумовые эффекты гораздо резче выражены
		// В общем, сейчас уже без разницы как, всё одинаково звучит, но второе более производительно (на будущее)
	}
	else
	{
		calc();
		L = SAMPLE_INT(m_dMixL);
		R = SAMPLE_INT(m_dMixR);
	}
}

void CEMU2149::Reset()
{
	PSG_reset();
}
/*
 АУшка использует только верхний полупериод сэмпла
*/
void CEMU2149::GetSample(sOneSample *pSm)
{
	SAMPLE_INT l = 0.0;
	SAMPLE_INT r = 0.0;

	if (m_bEnableSound)
	{
		PSG_calc(l, r);
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

void CEMU2149::synth_write_address(uint8_t addr)
{
	m_nSynthReg = addr;
}

void CEMU2149::synth_write_data(uint8_t byte)
{
	PSG_writeReg(m_nSynthReg, byte);
}


#include "pch.h"
#include "AYSnd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CAYSnd::CAYSnd()
	: m_nChipSel(AY1)
	, m_bLog(false)
	, m_bLogRegOut(false)
	, m_pLOGFile(nullptr)
{
	m_ay8910[AY1].setChip(AY1);
	m_ay8910[AY2].setChip(AY2);
}

CAYSnd::~CAYSnd()
    = default;

void CAYSnd::EnableSound(bool bEnable)
{
	m_ay8910[AY1].EnableSound(bEnable);
	m_ay8910[AY2].EnableSound(g_Config.m_b2ndAY8910 && bEnable);
}

bool CAYSnd::IsSoundEnabled() const
{
	return m_ay8910[AY1].IsSoundEnabled();
}

void CAYSnd::SetFilter(bool bEnable)
{
	m_ay8910[AY1].SetFilter(bEnable);
	m_ay8910[AY2].SetFilter(bEnable);
}

bool CAYSnd::IsFilter() const
{
	return m_ay8910[AY1].IsFilter();
}

void CAYSnd::SetDCOffsetCorrect(bool bEnable)
{
	m_ay8910[AY1].SetDCOffsetCorrect(bEnable);
	m_ay8910[AY2].SetDCOffsetCorrect(bEnable);
}

bool CAYSnd::IsDCOffsetCorrect() const
{
	return m_ay8910[AY1].IsDCOffsetCorrect();
}

void CAYSnd::SetStereo(bool bEnable)
{
	m_ay8910[AY1].SetStereo(bEnable);
	m_ay8910[AY2].SetStereo(bEnable);
}

void CAYSnd::ReInit()
{
	m_ay8910[AY1].ReInit();
	m_ay8910[AY2].ReInit();
}

void CAYSnd::Reset()
{
	switch (g_Config.m_n2AYWorkMode)
	{
		case MODE_GRYPHON:

			// в этом режиме m_nChipSel - не номер, а битовая маска
			if (g_Config.m_b2ndAY8910)
			{
				m_nChipSel = 0;     // если включено 2 AY, то по умолчанию ни один чип не выбран
			}
			else
			{
				m_nChipSel = 0100000; // если выключено 2 AY, то по умолчанию выбирается первый чип
			}

			break;

		default:
		case MODE_GID:
		case MODE_TURBOSOUND:
			m_nChipSel = AY1; // по умолчанию первый
			break;
	}

	m_ay8910[AY1].Reset();
	m_ay8910[AY2].Reset();
}

void CAYSnd::GetSample1(sOneSample *pSm)
{
	m_ay8910[AY1].GetSample(pSm);
}

void CAYSnd::GetSample2(sOneSample *pSm)
{
	m_ay8910[AY2].GetSample(pSm);
}


#pragma warning(disable:4996)

static uint8_t log_data[2] = { 0, 0 };
static uint8_t log_psghdr[] = { 'P', 'S', 'G', 0x1a, 10, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t log_ff = 0xff;

/*
Структура PSG - формата
Offset Number of byte Description
+ 0 3 Identifier 'PSG'
+ 3 1 Marker “End of Text”(1Ah)
+ 4 1 Version number
+ 5 1 Player frequency(for versions 10 + )
+ 6 10 Data

Data — последовательности пар байтов записи в регистр.
Первый байт — номер регистра(от 0 до 0x0F), второй — значение.
Вместо номера регистра могут быть специальные маркеры : 0xFF, 0xFE или 0xFD
0xFD — конец композиции.
0xFF — ожидание 20 мс.
0xFE — следующий байт показывает сколько раз выждать по 80 мс.

!!! двух чиповая музыка ломает логику PSG дампа
непонятно. То ли надо делать по дампу на каждый чип,
то ли придумывать расширенный формат какой-нибудь.
Но я в интернете вообще еле нашёл описание формата psg дампа.

*/

void CAYSnd::write_address(uint16_t word)
{
	uint8_t addr = ~word & 0xff;

	switch (g_Config.m_n2AYWorkMode)
	{
		case MODE_GRYPHON:

			/*
			Вариант 1: Маски разрядов 14. и 15.
			*/
			if (g_Config.m_b2ndAY8910) // если включено 2 AY
			{
				m_nChipSel = ~word & 0140000; // выделим из адреса маски
			}
			else // если выключено
			{
				m_nChipSel = 0100000; // то зададим первый чип по умолчанию.
			}

			addr &= 0x0f; // теперь выделяем адрес

			// а далее воможны варианты - или пишем в нужный чип, или в оба, если заданы оба бита
			// или никуда не пишем, если ничего не задано
			if (m_nChipSel & 0100000)
			{
				m_ay8910[AY1].synth_write_address(addr);
				log_data[0] = addr;
			}

			if (m_nChipSel & 040000)
			{
				m_ay8910[AY2].synth_write_address(addr);
				log_data[0] = addr;
			}

			break;

		case MODE_TURBOSOUND:

			/*
			Вариант 2: Стандарт TurboSound.
			*/
			if (g_Config.m_b2ndAY8910) // если включено 2 AY
			{
				switch (addr)   // проверяем на код выбора чипа
				{
					case 0xff:
						m_nChipSel = AY1;
						return; // если адрес - код выбора, то выходим и ничего не делаем.

					case 0xfe:
						m_nChipSel = AY2;
						return;
				}
			}
			else // если выключено
			{
				m_nChipSel = AY1; // то зададим первый чип по умолчанию.
			}

			addr &= 0x0f; // выделяем номер адреса регистра
			m_ay8910[m_nChipSel].synth_write_address(addr);
			log_data[0] = addr;
			break;

		default:
		case MODE_GID:

			/*
			мой, ни с чем не совместимый формат, когда номер чипа задаётся числом в битах
			7..4 т.е можно до 16 AYшек адресовать
			*/
			if (g_Config.m_b2ndAY8910) // если включено 2 AY
			{
				addr &= 0x1f; // адрес регистра вместе с номером чипа
				// поскольку их у нас только два - номер всего из одного бита 4
				m_nChipSel = (addr >> 4) & 0x0f;
			}
			else // если выключено
			{
				m_nChipSel = AY1; // то зададим первый чип по умолчанию.
			}

			m_ay8910[m_nChipSel].synth_write_address(addr & 0x0f);
			log_data[0] = addr;
			break;
	}
}

void CAYSnd::write_data(uint8_t byte)
{
	switch (g_Config.m_n2AYWorkMode)
	{
		case MODE_GRYPHON:
			if (m_nChipSel & 0100000)
			{
				m_ay8910[AY1].synth_write_data(~byte);
			}

			if (m_nChipSel & 040000)
			{
				m_ay8910[AY2].synth_write_data(~byte);
			}

			if (m_nChipSel & 0140000)
			{
				log_data[1] = ~byte;

				if (m_bLog)
				{
					fwrite(log_data, 1, 2, m_pLOGFile);
					m_bLogRegOut = true;
				}
			}

			break;

		case MODE_TURBOSOUND:
		case MODE_GID:
		default:
			m_ay8910[m_nChipSel].synth_write_data(~byte);
			log_data[1] = ~byte;

			if (m_bLog)
			{
				fwrite(log_data, 1, 2, m_pLOGFile);
				m_bLogRegOut = true;
			}

			break;
	}
}

void CAYSnd::log_timerTick()
{
	if (m_bLog && m_bLogRegOut)
	{
		fwrite(&log_ff, 1, 1, m_pLOGFile);
	}
}

void CAYSnd::log_start(const CString &strUniq)
{/***
	CString strName = _T("AYlog_") + strUniq + _T(".psg");
	fs::path strPathName = g_Config.m_strSavesPath / strName.GetString();
	m_pLOGFile = _tfopen(strPathName.c_str(), _T("wbST"));

	if (m_pLOGFile)
	{
		m_bLog = true;
		fwrite(log_psghdr, 1, 16, m_pLOGFile);  // запишем заголовок
		fwrite(&log_ff, 1, 1, m_pLOGFile);  // запишем первый тик
		m_bLogRegOut = false; // флаг, что не надо пока тики фиксировать.
		// чтобы перед началом музыки много пустых тиков не было
	}***/
}

void CAYSnd::log_done()
{
	if (m_bLog)
	{
		fclose(m_pLOGFile);
		m_bLog = false;
	}
}



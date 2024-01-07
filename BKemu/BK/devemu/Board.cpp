// Chip.cpp: implementation of the CMotherBoard class.
//


#include "pch.h"
#include "Ini.h"
#include "Board.h"
#include "BKMessageBox.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

extern "C" {
#include "ram_page.h"
extern uint8_t RAM[RAM_SIZE];
}

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CMotherBoard::CMotherBoard(BK_DEV_MPI model)
	:/// m_pParent(nullptr)   // Указатель на окно, в которое посылаются оповещающие сообщения
///	, 
	m_pSpeaker(nullptr)
	, m_pCovox(nullptr)
	, m_pMenestrel(nullptr)
	, m_pAYSnd(nullptr)
	, m_pDebugger(nullptr)
	, m_bBreaked(false)
	, m_bRunning(false)
	, m_bKillTimerEvent(false)
	, m_nLowBound(1)
	, m_nHighBound(100000000)
	, m_nCPUFreq_prev(0)
	  // переменные,задающие конкретную модель
	, m_BoardModel(model)
	, m_nStartAddr(0100000)
	, m_nBKPortsIOArea(BK_PURE_PORTSIO_AREA)
	, m_nKeyCleanEvent(0)
{
	DBGM_PRINT(("CMotherBoard::CMotherBoard(BK_DEV_MPI model = %d)", model));
	m_sTV.init();
	SetCPUBaseFreq(CPU_SPEED_BK10); // частота задаётся этой константой
	ZeroMemory(m_MemoryMap, sizeof(m_MemoryMap));
	/////m_cpu.AttachBoard(this);
	////m_fdd.AttachParent(this);
	////m_fdd.init_A16M_10(&m_ConfBKModel, ALTPRO_A16M_STD10_MODE);

	// Инициализация модуля памяти
	if (!FillWndVectorPtr(0200000))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}

CMotherBoard::~CMotherBoard()
{
	//// m_vWindows.clear();
}

MSF_CONF CMotherBoard::GetConfiguration()
{
	return MSF_CONF::BK1001;
}

BK_DEV_MPI CMotherBoard::GetBoardModel()
{
	return m_BoardModel;
}

bool CMotherBoard::FillWndVectorPtr(int nMemSize) {
	////m_vWindows.clear();
	InitMemoryValues();
	return true;
}
/***
void CMotherBoard::AttachWindow(CMainFrame *pParent)
{
	m_pParent = pParent;
}
**/
void CMotherBoard::AttachSound(CBkSound *pSnd)
{
	m_pSound = pSnd;
}

void CMotherBoard::AttachSpeaker(CSpeaker *pDevice)
{
	m_pSpeaker = pDevice;
}

void CMotherBoard::AttachCovox(CCovox *pDevice)
{
	m_pCovox = pDevice;
}

void CMotherBoard::AttachMenestrel(CMenestrel *pDevice)
{
	m_pMenestrel = pDevice;
}

void CMotherBoard::AttachAY8910(CAYSnd *pDevice)
{
	m_pAYSnd = pDevice;
}

void CMotherBoard::AttachDebugger(CDebugger *pDevice)
{
	m_pDebugger = pDevice;
}

void CMotherBoard::InitMemoryValues() {
	uint16_t val = 0;
	uint8_t flag = 0;
	for (size_t bank = 0; bank < 4; ++bank) {
		m_MemoryMap[bank].bReadable = true;
		m_MemoryMap[bank].bWritable = true;
		m_MemoryMap[bank].nBank = bank;
		m_MemoryMap[bank].nOffset = bank << 12;
		m_MemoryMap[bank].nPage = bank >> 2;
		m_MemoryMap[bank].addr = &RAM[bank << 12];
	}
	for (size_t bank = 4; bank < 8; ++bank) {
		m_MemoryMap[bank].bReadable = true;
		m_MemoryMap[bank].bWritable = true;
		m_MemoryMap[bank].nBank = bank;
		m_MemoryMap[bank].nOffset = bank << 12 + 0x10000;
		m_MemoryMap[bank].nPage = bank >> 2;
		m_MemoryMap[bank].addr = &RAM[(bank << 12) + 0x10000];  // Page5 from БК0011М для плавного сопряжения перекл. режимов
	}
	for (size_t bank = 0; bank < 8; ++bank) {
        BKMEMBank_t& b = m_MemoryMap[bank];
		if (!b.addr || !b.bWritable) continue;
		auto pPtr = reinterpret_cast<uint16_t *>(b.addr);
    	for (int i = 0; i < PAGE_SIZE_BYTES / 2; i++, flag--)	{
	    	pPtr[i] = val;
		    val = ~val;
    		if (flag == 192) {
			    val = ~val;
			    flag = 0;
		    }
		}
	}
}

void CMotherBoard::OnReset()
{
///	m_pParent->GetScreen()->SetPalette(0);
///	m_pParent->GetScreen()->SetRegister(m_reg177664);
	m_sTV.clear();
	m_nCPUFreq_prev = 0; // принудительно заставим применить изменения параметров фрейма
	FrameParam();
}

/*
В функциях GetByte, GetWord, SetByte, SetWord доступ к регистрам 0177700..0177712
не обрабатывается, т.к. эти функции должны вызываться только из CPU, и доступ к
регистрам обрабатывается там.
*/

void CMotherBoard::GetByte(const uint16_t addr, uint8_t *pValue)
{
	*pValue = GetByte(addr);
}

uint8_t CMotherBoard::GetByte(const uint16_t addr)
{
	int nTC = 0;
	return GetByteT(addr, nTC);
}

uint8_t CMotherBoard::GetByteT(const uint16_t addr, int &nTC) {
	const int nBank = (addr >> 12) & 0x0f;
	BKMEMBank_t &mbPtr = m_MemoryMap[nBank];
	uint8_t m = *(mbPtr.addr + (addr & 007777));
	uint8_t v;

	// Сперва проверим, на системные регистры
	if (nBank == 15)
	{
		const BK_DEV_MPI nBKFddType = GetFDDType();

		if (addr >= m_nBKPortsIOArea)
		{
			switch (nBKFddType)
			{
				case BK_DEV_MPI::SMK512:
					switch (GetAltProMode())
					{
						case ALTPRO_SMK_SYS_MODE:
						case ALTPRO_SMK_ALL_MODE:
							// в режиме SYS и ALL в этом диапазоне можно читать
							goto gblb1;

						default:
							if (GetSystemRegister(addr, &v, GSSR_BYTEOP))
							{
								nTC += REG_TIMING_CORR_VALUE;
								return v;
							}
					}

					// иначе, в этом диапазоне действуют обычные правила
					break;

				case BK_DEV_MPI::A16M:
					switch (GetAltProMode())
					{
						case ALTPRO_A16M_START_MODE:
							// в режиме Start в этом диапазоне можно читать
gblb1:
							nTC += mbPtr.nTimingCorrection;

							if (GetSystemRegister(addr, &v, GSSR_BYTEOP))
							{
								return v | m;
							}

							return m;

						case ALTPRO_A16M_BASIC_MODE:

							// ещё одно исключение. в режиме бейсика m_nBKPortsIOArea == BK_PURE_PORTSIO_AREA целых 0600 байтов ещё
							if ((addr < BK_PURE_PORTSIO_AREA) && (GetAltProExtCode() & 010)) // если бейсик подключён
							{
								goto gblb1;
							}

							// тут break; не нужен, переходим к выполнению кода из секции default свитча выше уровнем
					}

					[[fallthrough]]; // тут break; не нужен!!!
				// иначе, как в обычном режиме, смотрим порты
				default:

					// в обычном режиме тут только регистры могут быть
					if (GetSystemRegister(addr, &v, GSSR_BYTEOP))
					{
						nTC += REG_TIMING_CORR_VALUE;
						return v;
					}
					else
					{
						throw CExceptionHalt(addr, _T("Can't read this address."));
					}

					break;
			}
		}
		else
		{
			// по адресам 170000-177000 для SMK если режим не SYS - всегда ОЗУ
			if (nBKFddType == BK_DEV_MPI::SMK512 && GetAltProMode() != ALTPRO_A16M_START_MODE)
			{
				nTC += mbPtr.nTimingCorrection;
				return m;
			}
		}
	}

	// теперь проверим на чтение из несуществующего банка
	if (mbPtr.bReadable)
	{
		nTC += mbPtr.nTimingCorrection;
		// если читать можно, возвращаем значение
		return m;
	}

	throw CExceptionHalt(addr, _T("Can't read this address."));
}


void CMotherBoard::GetWord(const uint16_t addr, uint16_t *pValue)
{
	*pValue = GetWord(addr);
}

uint16_t CMotherBoard::GetWord(const uint16_t addr)
{
	int nTC = 0;
	return GetWordT(addr, nTC);
}

uint16_t CMotherBoard::GetWordT(const uint16_t addr, int &nTC)
{
	const int nBank = (addr >> 12) & 0x0f;
	BKMEMBank_t &mbPtr = m_MemoryMap[nBank];
	uint16_t m = *(uint16_t *)(mbPtr.addr + (addr & 007776));
	uint16_t v;

	// Сперва проверим, на системные регистры
	if (nBank == 15)
	{
		const BK_DEV_MPI nBKFddType = GetFDDType();

		if (addr >= m_nBKPortsIOArea)
		{
			switch (nBKFddType)
			{
				case BK_DEV_MPI::SMK512:
					switch (GetAltProMode())
					{
						case ALTPRO_SMK_SYS_MODE:
						case ALTPRO_SMK_ALL_MODE:
							// в режиме SYS и ALL в этом диапазоне можно читать
							goto gwlb1;

						default:
							if (GetSystemRegister(addr, &v, GSSR_NONE))
							{
								nTC += REG_TIMING_CORR_VALUE;
								return v;
							}
					}

					// иначе, в этом диапазоне действуют обычные правила
					break;

				case BK_DEV_MPI::A16M:
					switch (GetAltProMode())
					{
						case ALTPRO_A16M_START_MODE:
							// в режиме Start в этом диапазоне можно читать
gwlb1:
							nTC += mbPtr.nTimingCorrection;

							if (GetSystemRegister(addr, &v, GSSR_NONE))
							{
								return v | m;
							}

							return m;

						case ALTPRO_A16M_BASIC_MODE:

							// ещё одно исключение. в режиме бейсика m_nBKPortsIOArea == BK_PURE_PORTSIO_AREA целых 0600 байтов ещё
							if ((addr < BK_PURE_PORTSIO_AREA) && (GetAltProExtCode() & 010)) // если бейсик подключён
							{
								goto gwlb1;
							}
					}

					[[fallthrough]]; // тут break; не нужен!!!
				// иначе, как в обычном режиме, смотрим порты
				default:

					// в обычном режиме тут только регистры могут быть
					if (GetSystemRegister(addr, &v, GSSR_NONE))
					{
						nTC += REG_TIMING_CORR_VALUE;
						return v;
					}
					else
					{
						throw CExceptionHalt(addr, _T("Can't read this address."));
					}

					break;
			}
		}
		else
		{
			// по адресам 170000-177000 для SMK если режим не SYS - всегда ОЗУ
			if (nBKFddType == BK_DEV_MPI::SMK512 && GetAltProMode() != ALTPRO_A16M_START_MODE)
			{
				nTC += mbPtr.nTimingCorrection;
				return m;
			}
		}
	}

	// теперь проверим на чтение из несуществующего банка
	if (mbPtr.bReadable)
	{
		nTC += mbPtr.nTimingCorrection;
		// если читать можно
		return m;
	}

	throw CExceptionHalt(addr, _T("Can't read this address."));
}


void CMotherBoard::SetByte(const uint16_t addr, uint8_t value)
{
	int nTC = 0;
	SetByteT(addr, value, nTC);
}

void CMotherBoard::SetByteT(const uint16_t addr, uint8_t value, int &nTC)
{
	const int nBank = (addr >> 12) & 0x0f;
	BKMEMBank_t &mbPtr = m_MemoryMap[nBank];
	uint8_t *pDst = mbPtr.addr + (addr & 07777);

	// Сперва проверим, на системные регистры
	if (nBank == 15)
	{
		const BK_DEV_MPI nBKFddType = GetFDDType();

		if (addr >= m_nBKPortsIOArea)
		{
			switch (nBKFddType)
			{
				case BK_DEV_MPI::SMK512:
					switch (GetAltProMode())
					{
						case ALTPRO_SMK_HLT11_MODE:
						case ALTPRO_SMK_HLT10_MODE:
							// case ALTPRO_SMK_OZU10_MODE:
							// в режиме Hlt11 и Hlt10 в этом диапазоне можно писать, а для старой версии, ещё и в режиме ОЗУ10
							goto sblb1;

						default:
							if (SetSystemRegister(addr, value, GSSR_BYTEOP))
							{
								nTC += REG_TIMING_CORR_VALUE;
								return;
							}
					}

					// иначе, в этом диапазоне действуют обычные правила
					break;

				case BK_DEV_MPI::A16M:

					// в режиме Hlt11 в этом диапазоне можно писать
					if (GetAltProMode() == ALTPRO_A16M_HLT11_MODE)
					{
sblb1:
						SetSystemRegister(addr, value, GSSR_BYTEOP);
						*pDst = value;
						nTC += mbPtr.nTimingCorrection;
						return;
					}

					[[fallthrough]];
				// иначе, как в обычном режиме, смотрим порты
				default:

					// в обычном режиме тут только регистры могут быть
					if (SetSystemRegister(addr, value, GSSR_BYTEOP))
					{
						nTC += REG_TIMING_CORR_VALUE;
						return;
					}
					else
					{
						throw CExceptionHalt(addr, _T("Can't write this address."));
					}

					break;
			}
		}
		else
		{
			if (nBKFddType == BK_DEV_MPI::SMK512 && GetAltProMode() != ALTPRO_A16M_START_MODE)
			{
				*pDst = value;
				nTC += mbPtr.nTimingCorrection;
				return;
			}
		}
	}

	if (mbPtr.bWritable)
	{
		*pDst = value;
		nTC += mbPtr.nTimingCorrection;
	}
	else
	{
		throw CExceptionHalt(addr, _T("Can't write this address."));
	}
}


void CMotherBoard::SetWord(const uint16_t addr, uint16_t value)
{
	int nTC = 0;
	SetWordT(addr, value, nTC);
}

void CMotherBoard::SetWordT(const uint16_t addr, uint16_t value, int &nTC)
{
	const int nBank = (addr >> 12) & 0x0f;
	BKMEMBank_t &mbPtr = m_MemoryMap[nBank];
	auto pDst = reinterpret_cast<uint16_t *>(mbPtr.addr + (addr & 07776));

	// Сперва проверим, на системные регистры
	if (nBank == 15)
	{
		const BK_DEV_MPI nBKFddType = GetFDDType();

		if (addr >= m_nBKPortsIOArea)
		{
			switch (nBKFddType)
			{
				case BK_DEV_MPI::SMK512:
					switch (GetAltProMode())
					{
						case ALTPRO_SMK_HLT11_MODE:
						case ALTPRO_SMK_HLT10_MODE:
							// case ALTPRO_SMK_OZU10_MODE:
							// в режиме Hlt11 и Hlt10 в этом диапазоне можно писать, а для старой версии, ещё и в режиме ОЗУ10
							goto swlb1;

						default:
							if (SetSystemRegister(addr, value, GSSR_NONE))
							{
								nTC += REG_TIMING_CORR_VALUE;
								return;
							}
					}

					// иначе, в этом диапазоне действуют обычные правила
					break;

				case BK_DEV_MPI::A16M:

					// в режиме Hlt11 в этом диапазоне можно писать
					if (GetAltProMode() == ALTPRO_A16M_HLT11_MODE)
					{
swlb1:
						SetSystemRegister(addr, value, GSSR_NONE);
						*pDst = value;
						nTC += mbPtr.nTimingCorrection;
						return;
					}

					[[fallthrough]];
				// иначе, как в обычном режиме, смотрим порты
				default:

					// в обычном режиме тут только регистры могут быть
					if (SetSystemRegister(addr, value, GSSR_NONE))
					{
						nTC += REG_TIMING_CORR_VALUE;
						return;
					}
					else
					{
						throw CExceptionHalt(addr, _T("Can't write this address."));
					}

					break;
			}
		}
		else
		{
			if (nBKFddType == BK_DEV_MPI::SMK512 && GetAltProMode() != ALTPRO_A16M_START_MODE)
			{
				*pDst = value;
				nTC += mbPtr.nTimingCorrection;
				return;
			}
		}
	}

	if (mbPtr.bWritable)
	{
		*pDst = value;
		nTC += mbPtr.nTimingCorrection;
	}
	else
	{
		throw CExceptionHalt(addr, _T("Can't write this address."));
	}
}


/*
GetByteIndirect,GetWordIndirect,SetByteIndirect,SetWordIndirect
этими функциями пользуются многие вспомогательные модули для того чтобы читать данные
напрямую из памяти, и как правило чтобы читать/писать что-то из/в регистров, а т.к.
внутренние системные регистры переехали в CPU, здесь делается перехват их адресов, чтобы
получить/записать значения из/в регистров в CPU
и да. тут не делаются проверки на невозможность, читать/писать в память, ибо эти функции
не CPU. и в экранную память вывод тоже не делается.
*/

uint8_t CMotherBoard::GetByteIndirect(uint16_t addr)
{
	int nBank = (addr >> 12) & 0x0f;
	uint8_t v;
	uint8_t m = m_MemoryMap[nBank].bReadable
	    ? *(m_MemoryMap[nBank].addr + (addr & 007777))
		: 0;

	if (GetSystemRegister(addr, &v, GSSR_INTERNAL | GSSR_BYTEOP))
	{
		return (GetAltProMode() == ALTPRO_A16M_START_MODE) ? (v | m) : v;
	}

	if ((0177700 <= addr) && (addr < 0177714))
	{
		uint16_t w = 0;/////m_cpu.GetSysRegsIndirect(addr);

		if (addr & 1)
		{
			w >>= 8;
		}

		if (GetAltProMode() == ALTPRO_A16M_START_MODE)
		{
			w |= m;
		}

		return LOBYTE(w);
	}

	return m;
}


uint16_t CMotherBoard::GetWordIndirect(uint16_t addr)
{
	int nBank = (addr >> 12) & 0x0f;
	uint16_t v;
	uint16_t m = m_MemoryMap[nBank].bReadable
	    ? *(uint16_t *)(m_MemoryMap[nBank].addr + (addr & 007776))
		: 0;

	if (GetSystemRegister(addr, &v, GSSR_INTERNAL))
	{
		return (GetAltProMode() == ALTPRO_A16M_START_MODE) ? (v | m) : v;
	}

	if ((0177700 <= addr) && (addr < 0177714))
	{
		uint16_t w = 0;/////m_cpu.GetSysRegsIndirect(addr);

		if (GetAltProMode() == ALTPRO_A16M_START_MODE)
		{
			w |= m;
		}

		return w;
	}

	return m;
}


void CMotherBoard::SetByteIndirect(const uint16_t addr, uint8_t value)
{
	const int nBank = (addr >> 12) & 0x0f;
	uint8_t *Dst = m_MemoryMap[nBank].bWritable
	    ? m_MemoryMap[nBank].addr + (addr & 007777)
		: nullptr;

	if (SetSystemRegister(addr, value, GSSR_INTERNAL | GSSR_BYTEOP))
	{
		if (Dst && (GetAltProMode() == ALTPRO_A16M_HLT11_MODE || (GetAltProMode() == ALTPRO_SMK_HLT10_MODE && GetFDDType() == BK_DEV_MPI::SMK512)))
		{
			*Dst = value;
		}

		return;
	}

	if ((0177700 <= addr) && (addr < 0177714))
	{
		/////m_cpu.SetSysRegs(addr, value);

		if (Dst && (GetAltProMode() == ALTPRO_A16M_HLT11_MODE || (GetAltProMode() == ALTPRO_SMK_HLT10_MODE && GetFDDType() == BK_DEV_MPI::SMK512)))
		{
			*Dst = value;
		}

		return;
	}

	if (Dst)
	{
		*Dst = value;
	}
}


void CMotherBoard::SetWordIndirect(const uint16_t addr, uint16_t value)
{
	const int nBank = (addr >> 12) & 0x0f;
	auto Dst = m_MemoryMap[nBank].bWritable
	    ? reinterpret_cast<uint16_t *>(m_MemoryMap[nBank].addr + (addr & 007776))
		: nullptr;

	if (SetSystemRegister(addr, value, GSSR_INTERNAL))
	{

		if (Dst && (GetAltProMode() == ALTPRO_A16M_HLT11_MODE || (GetAltProMode() == ALTPRO_SMK_HLT10_MODE && GetFDDType() == BK_DEV_MPI::SMK512)))
		{
			*Dst = value;
		}

		return;
	}

	if ((0177700 <= addr) && (addr < 0177714))
	{
		/////m_cpu.SetSysRegs(addr, value);

		if (Dst && (GetAltProMode() == ALTPRO_A16M_HLT11_MODE || (GetAltProMode() == ALTPRO_SMK_HLT10_MODE && GetFDDType() == BK_DEV_MPI::SMK512)))
		{
			*Dst = value;
		}

		return;
	}

	if (Dst)
	{
		*Dst = value;
	}
}


uint16_t CMotherBoard::GetRON(CCPU::REGISTER reg)
{
	return 0;/////m_cpu.GetRON(reg);
}

void CMotherBoard::SetRON(CCPU::REGISTER reg, uint16_t value)
{
	/////m_cpu.SetRON(reg, value);
}

// это ресет оборудования, выполняемый по команде RESET. вызывается из CPU
void CMotherBoard::ResetDevices()
{
	m_reg177660 = 0100; // команда RESET запрещает прерывания от клавиатуры. проверено.
	m_reg177714in = 0;
	m_reg177714out = 0;
	m_reg177716out_tap = 0200;
	m_reg177716in |= 0300;

	// если у нас есть сопр. по команде RESET он тоже должен инициализироваться (в правильной схеме подключения)
	if (m_pAYSnd)
	{
		m_pAYSnd->Reset();
	}

	// инициализация ОЗУ и CPU
	Reset(); // заставляет выполниться OnReset
}


bool CMotherBoard::InitBoard(uint16_t nNewStartAddr)
{
	if (nNewStartAddr == 0)
	{
		nNewStartAddr = m_nStartAddr;    // Используем адрес по умолчанию
	}
	else
	{
		m_nStartAddr = nNewStartAddr;    // иначе используем адрес из конфигурации
	}

	// инициализация регистров
	m_reg177716in = nNewStartAddr & 0177400;
	m_reg177662out = 047400;
	m_reg177664 = 0330; // начальное значение. на самом деле - оно случайное
///	m_pParent->GetScreen()->SetExtendedMode(!(m_reg177664 & 01000));
	m_reg177662in = 0;
	m_reg177716out_mem = 0;

	// инициализация карты памяти
	if (!InitMemoryModules())
	{
		return false;
	}

	if (m_pSpeaker)
	{
		m_pSpeaker->Reset();
	}

	if (m_pCovox)
	{
		m_pCovox->Reset();
	}

	/////m_cpu.InitCPU();
///	m_pParent->SendMessage(WM_RESET_KBD_MANAGER, 1); // почистим индикацию управляющих клавиш в статусбаре
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// на входе - маска, по которой сбрасываем биты адреса.
void CMotherBoard::ResetCold(uint16_t addrMask)
{
	m_reg177662out = 047400;    // регистр инициализируется
	//m_reg177664 = 0330;   ресет не должен затрагивать этот регистр
	m_reg177662in = 0;
	m_reg177716in = m_nStartAddr & ~addrMask; // формируем адрес запуска
	/////m_cpu.InitCPU(); // здесь делается чтение адреса запуска из регистра 177716in
	m_reg177716in = m_nStartAddr & 0177400; // и восстановим стандартное значение адреса запуска, без сброшенных битов
///	m_pParent->SendMessage(WM_RESET_KBD_MANAGER, 1); // почистим индикацию управляющих клавиш в статусбаре
}


void CMotherBoard::StopInterrupt()
{
	// нажали на кнопку стоп
	/////m_cpu.SetIRQ1();
}

void CMotherBoard::UnStopInterrupt()
{
	// отжали кнопку стоп
	////m_cpu.UnsetIRQ1();
}


void CMotherBoard::KeyboardInterrupt(uint16_t interrupt)
{
	m_reg177660 |= 0200;       // Установим состояние готовности в 177660

	if (!(m_reg177660 & 0100))	// если прерывания разрешены, делаем прерывание
	{
		/////m_cpu.InterruptVIRQ(interrupt); // делаем прерывание
	}
}

extern "C" volatile bool ctrlPressed;
extern "C" volatile bool altPressed;
extern "C" volatile bool shiftPressed;

int CMotherBoard::CalcStep()
{
	int code = 0;

	if (ctrlPressed)
	{
		code |= (1 << 1);
	}

	if (altPressed)
	{
		code |= (1 << 2);
	}

	if (shiftPressed)
	{
		code |= (1 << 0);
	}

	int step;

	switch (code)
	{
		default:
		case 0: // ничего не нажато
			step = 1000;
			break;

		case 1: // shift
			step = 1;
			break;

		case 2: // ctrl
			step = 10;
			break;

		case 3: // crtl+shift
			step = 100;
			break;

		case 4: // alt
			step = 1000000;
			break;

		case 5: // alt+shift
			step = 10000;
			break;

		case 6: // ctrl+alt
			step = 100000;
			break;

		case 7: // ctrl+alt+shift
			step = 0; // мегакомбо
			break;
	}

	return step;
}

void CMotherBoard::AccelerateCPU()
{
	if (m_nCPUFreq < m_nHighBound)
	{
		int n = CalcStep(); // если мегакомбо, для увеличения - увеличиваем на порядок

		if (n)
		{
			m_nCPUFreq += n;
		}
		else
		{
			m_nCPUFreq *= 10;
		}

		if (m_nCPUFreq > m_nHighBound)
		{
			m_nCPUFreq = m_nHighBound;
		}
	}
}

void CMotherBoard::SlowdownCPU()
{
	if (m_nCPUFreq > m_nLowBound)
	{
		int n = CalcStep(); // если мегакомбо, для уменьшения - уменьшаем на порядок

		if (n)
		{
			m_nCPUFreq -= n;
		}
		else
		{
			m_nCPUFreq /= 10;
		}

		if (m_nCPUFreq < m_nLowBound)
		{
			m_nCPUFreq = m_nLowBound;
		}
	}
}


void CMotherBoard::NormalizeCPU()
{
	if (g_Config.m_nCPUFrequency)
	{
		SetCPUFreq(g_Config.m_nCPUFrequency);
	}
	else
	{
		ResetToCPUBaseFreq();
	}
}

bool CMotherBoard::CanAccelerate()
{
	return (m_nCPUFreq < m_nHighBound);
}

bool CMotherBoard::CanSlowDown()
{
	return (m_nCPUFreq > m_nLowBound);
}

int CMotherBoard::GetLowBound()
{
	return m_nLowBound;
}

int CMotherBoard::GetHighBound()
{
	return m_nHighBound;
}

void CMotherBoard::SetCPUBaseFreq(int frq) // установка базовой частоты
{
	m_nCPUFreq = m_nBaseCPUFreq = frq;
}

void CMotherBoard::ResetToCPUBaseFreq()
{
	m_nCPUFreq = m_nBaseCPUFreq;
}

void CMotherBoard::SetCPUFreq(int frq) // установка текущей частоты
{
	if (frq < GetLowBound())
	{
		frq = GetLowBound();
	}
	else if (frq > GetHighBound())
	{
		frq = GetHighBound();
	}

	m_nCPUFreq = frq;
}

int CMotherBoard::GetCPUFreq() const // выдача текущей частоты
{
	return m_nCPUFreq;
}

int CMotherBoard::GetCPUSpeed() const // выдача текущей частоты для конфигурации
{
	return (m_nCPUFreq == m_nBaseCPUFreq) ? 0 : m_nCPUFreq;
}

CFDDController *CMotherBoard::GetFDD()
{
	return 0;//// &m_fdd;
}

void CMotherBoard::SetFDDType(BK_DEV_MPI model, bool bInit /*= true*/)
{
	DBGM_PRINT(("CMotherBoard::SetFDDType(BK_DEV_MPI model = %d, bool bInit = %d)", model, bInit));
	////m_fdd.SetFDDType(model);    // не забывать! bool bInit удалять нельзя, оно в других местах нужно.
}

BK_DEV_MPI CMotherBoard::GetFDDType()
{
	return BK_DEV_MPI::NONE;
}

uint16_t CMotherBoard::GetAltProMode()
{
	return LOWORD(m_ConfBKModel.nAltProMode);
}

void CMotherBoard::SetAltProMode(uint16_t w)
{
	m_ConfBKModel.nAltProMode = w;
	MemoryManager();
}

uint16_t CMotherBoard::GetAltProCode()
{
	return LOWORD(m_ConfBKModel.nAltProMemBank);
}

void CMotherBoard::SetAltProCode(uint16_t w)
{
	m_ConfBKModel.nAltProMemBank = w;
	MemoryManager();
}

uint16_t CMotherBoard::GetAltProExtCode()
{
	return LOWORD(m_ConfBKModel.nExtCodes);
}

void CMotherBoard::Set177716RegTap(uint16_t w)
{
	constexpr uint16_t mask = 0360;
	m_reg177716out_tap = (~mask & m_reg177716out_tap) | (mask & w);
	m_pSpeaker->SetData(m_reg177716out_tap);
	// пока не решится проблема ложных срабатываний, это лучше не использовать
	/*
	if (m_pParent->GetTapePtr()->IsWaveLoaded())
	{
	    if (m_reg177716out_tap & 0200)
	    {
	        m_pParent->GetTapePtr()->StopPlay();
	    }
	    else
	    {
	        m_pParent->GetTapePtr()->StartPlay();
	    }
	}
	// */
}

/****
uint8_t *CMotherBoard::GetMainMemory() const
{
	return const_cast<uint8_t *>(m_pMemory.data());
}****/

uint8_t *CMotherBoard::GetAddMemory() const
{
	return nullptr;
}

/*
Чтение из системных регистров
вход: addr - адрес регистра (177660, 177716 и т.п.)
      pDst - адрес в массиве, куда сохраняется новое значение
      bByteOperation - флаг операции true - байтовая, false - словная
*/
bool CMotherBoard::GetSystemRegister(uint16_t addr, void *pDst, UINT dwFlags)
{
	bool bRet = true;
	uint16_t v = 0;

	switch (addr & 0177776)
	{
		case 0177660:
			//TRACE1("Read 177660 in addr %06o\n", GetRON(CCPU::REGISTER::PC));
			v = m_reg177660;
			break;

		case 0177662:
			v = m_reg177662in;

			// если читаем из регистра данных клавиатуры,
			// сбросим бит готовности из регистра состояния клавиатуры
			//TRACE1("Read 177662 in addr %06o\n", GetRON(CCPU::REGISTER::PC));
			if (m_nKeyCleanEvent <= 0)
			{
				m_nKeyCleanEvent = 900; // бит 7 будем сбрасывать не сразу, а через некоторое время
			}

			//m_reg177660 &= ~0200;
			break;

		case 0177664:
			v = m_reg177664;
			break;

		case 0177714:
			v = m_reg177714in;
			break;

		case 0177716:
			v = m_reg177716in;

			// В БК 2й разряд SEL1 фиксирует любую запись в этот регистр, взводя триггер D9.1 на неограниченное время,
			// сбрасывается который любым чтением этого регистра.
			if (!(dwFlags & GSSR_INTERNAL))
			{
				m_reg177716in &= ~4;
			}

			break;

		default:
			bRet = false;
	}

	if (dwFlags & GSSR_BYTEOP)
	{
		*(reinterpret_cast<uint8_t *>(pDst)) = (addr & 1) ? HIBYTE(v) : LOBYTE(v);
	}
	else
	{
		*(reinterpret_cast<uint16_t *>(pDst)) = v;
	}

	return bRet;
}

/*
Запись данных в системные регистры БК0010
вход: addr - адрес регистра(177660, 177716 и т.п.)
      src - записываемое значение.
      bByteOperation - флаг операции true - байтовая, false - словная
*/
bool CMotherBoard::SetSystemRegister(uint16_t addr, uint16_t src, UINT dwFlags)
{
	switch (addr & 0177776)
	{
		case 0177660:
			/*
			177660
			Регистр состояния клавиатуры.
			(0100)бит 6 -- маскирование прерывание от клавиатуры,
			    "0" -- разрешено прерывание по вектору 060 либо 0274.
			    Прерывание вызывается при появлении "1" в бите 7. Начальное состояние: "0".
			    Доступен по чтению и по записи
			(0200)бит 7 -- готовность: "1" -- в регистре данных клавиатуры(177662) готов код нажатой клавиши.
			    Устанавливается при нажатии на клавишу, сбрасывается при чтении регистра данных клавиатуры.
			    Начальное состояние: "1".
			    Доступен только по чтению.
			другие биты: "0". Доступны только по чтению.
			*/
		{
			const uint16_t mask = dwFlags & GSSR_INTERNAL ? 0300 : 0100;

			if (dwFlags & GSSR_BYTEOP)
			{
				src &= 0377;

				if (addr & 1)
				{
					src <<= 8;
				}
			}

			// сбрасываем используемые биты и устанавливаем их значения из слова, которое записываем.
			// остальные биты - которые не используются - остаются нетронутыми.
			m_reg177660 = (m_reg177660 & ~mask) | (src & mask);
		}

		return true;

		case 0177662:
			/*
			177662
			Регистр данных клавиатуры.
			    биты 0-6: код клавиши. Доступ только по чтению.
			(040000)бит 14: разрешение прерывания по(внешнему) таймеру(50 Гц),
			    "0" -- прерывание разрешено,
			    "1" -- таймер отключён.
			    Доступен только по записи.
			    Бит 14 на БК0010 не документирован, и таймер работал только на некоторых экземплярах, можно считать, что его нет
			*/
			return false; // на бк10 запись запрещена. это был один из способов определения бк10 или бк11

		case 0177664:
			/*
			177664
			Регистр скроллинга. Доступен по записи и чтению.
			    биты 0-7: смещение скроллинга, строк. Начальное значение -- 0330.
			(01000)бит 9: сокращённый режим экрана, "0" -- сокращённый(1/4 экрана, старшие адреса),
			    "1" -- полный экран 256 строк.
			*/
		{
			constexpr uint16_t mask = 01377;

			if (dwFlags & GSSR_BYTEOP)
			{
				src &= 0377;
				/*
				оказалось, что в этот регистр не работает байтовая запись.
				чтение байта работает, но записывается всегда слово.
				а при байтовой операции, старший байт просто теряется.
				*/
			}

			m_reg177664 = (m_reg177664 & ~mask) | (src & mask);
		}

		return true;

		case 0177714:

			/*177714
			Регистр параллельного программируемого порта ввода вывода - два регистра, входной по чтению и выходной по записи.
			из выходного нельзя ничего прочитать т.к., читается оно из входного,
			во входной невозможно ничего записать, т.к. записывается оно в выходной.
			*/
			if (dwFlags & GSSR_BYTEOP)
			{
				src &= 0377; // работаем с младшим байтом

				if (addr & 1)
				{
					src <<= 8; // работаем со старшим байтом
				}
			}

			if (m_pAYSnd)
			{
				if (dwFlags & GSSR_BYTEOP)
				{
					m_pAYSnd->write_data(LOBYTE(src));
				}
				else
				{
					m_pAYSnd->write_address(src);
				}
			}

			if (m_pCovox)
			{
				m_pCovox->SetData(src);
			}

			if (m_pMenestrel)
			{
				m_pMenestrel->SetData(src);
			}

			m_reg177714out = src;

			if (g_Config.m_bICLBlock) // если включён блок нагрузок
			{
				// данные записанные в выходной порт передаются во входной
				m_reg177714in = src;
			}
	///		else if (g_Config.m_bMouseMars)
	///		{
	///			m_pParent->GetScreen()->SetMouseStrobe(src);
	///			m_reg177714in = m_pParent->GetScreen()->GetMouseStatus();
	///		}

			return true;

		case 0177716:

			/*
			177716
			Системный регистр. Внешний регистр 1(ВР1, SEL1) процессора ВМ1, регистр начального пуска.
			как и 177714 состоит из двух регистров, раздельных по чтению и по записи
			По чтению:
			(004)бит 2: признак записи в системный регистр (Внешняя схема). Устанавливается в "1" при
			    любой записи в регистр, сбрасывается в "0" после чтения из регистра.
			(010)бит 3: устанавливается при выполнении системного прерывания в HALT-mode.
			    Установка этого бита делается микропрограммным образом записью в 177716 бита 3.
			    На БК оно нигде не сохраняется и пропадает в никуда.
			    Бит 3 снимается командами START, STEP
			(020)бит 4: данные с ТЛГ-линии, "0" - логический 0, "1" - логическая 1
			(040)бит 5: данные с магнитофона, "0" - логический 0, "1" - логическая 1
			(100)бит 6: индикатор нажатия клавиши, установлен в "0" если нажата клавиша клавиатуры, "1" если нет нажатых клавиш.
			(200)бит 7: сигнал готовности с ТЛГ-линии, "0" - логический 0, "1" - логическая 1
			        Однако по заводской документации на БК0010 этот бит - константа "1", указывающая на отсутствие
			        в системе команд расширенной арифметики
			    биты 8-15: адрес начального пуска, 100000(БК-0010), младший байт при этом игнорируется
			    биты 0,1,3, не используются, "0".
			По записи:
			(020)бит 4: данные для передачи на ТЛГ-линию, "0" - логический 0, "1" - логическая 1
			(040)бит 5: данные на магнитофон(либо сигнал готовности на ТЛГ-линию - этого в тех документации нету.). Начальное состояние "0".
			(100)бит 6: данные на магнитофон и на пьезодинамик. Начальное состояние "0".
			(200)бит 7: включение двигателя магнитофона, "1" -- стоп, "0" -- пуск. Начальное состояние "1".
			    биты 0,1,3, не используются, "0".
			*/
			if (dwFlags & GSSR_BYTEOP)
			{
				src &= 0377; // работаем с младшим байтом

				if (addr & 1)
				{
					src <<= 8; // работаем со старшим байтом
				}
			}

			Set177716RegTap(src);

			// В БК 2й разряд SEL1 фиксирует любую запись в этот регистр, взводя триггер D9.1 на неограниченное время, сбрасывается который любым чтением этого регистра.
			if (!(dwFlags & GSSR_INTERNAL))
			{
				m_reg177716in |= 4;
			}

			return true;
	}

	return false;
}


void CMotherBoard::RunOver()
{
	// Run one command with go over command
	uint16_t pc = GetRON(CCPU::REGISTER::PC);
	uint16_t instr = GetWordIndirect(pc);
	uint16_t NextAddr = ADDRESS_NONE;

	if (m_pDebugger->IsInstructionOver(instr))
	{
		RunToAddr(pc + m_pDebugger->CalcInstructionLength(instr));
	}
	else if ((NextAddr = m_pDebugger->CalcNextAddr(pc)) != ADDRESS_NONE)
	{
		RunToAddr(NextAddr);
	}
	else
	{
		RunInto();
	}
}


void CMotherBoard::RunToAddr(uint16_t addr)
{
	// Run all commands to address
	UnbreakCPU(addr);
}

void CMotherBoard::RunInto()
{
	// Run one command with go into command
	UnbreakCPU(GO_INTO);
}


void CMotherBoard::RunOut()
{
	// Run all commands to function end
///	CDebugger::InitOutMode();
	UnbreakCPU(GO_OUT);
}


void CMotherBoard::BreakCPU()
{
	m_bBreaked = true;      // включаем отладочную приостановку
///	CDebugger::InitOutMode();
///	m_pParent->PostMessage(WM_CPU_DEBUGBREAK);
}

void CMotherBoard::UnbreakCPU(int nGoto)
{
	m_sTV.nGotoAddress = nGoto;
	m_bBreaked = false;     // отменяем отладочную приостановку
}

void CMotherBoard::RunCPU(bool bUnbreak)
{
	if (!m_bRunning)
	{
		if (bUnbreak)
		{
			UnbreakCPU(ADDRESS_NONE);
		}

		// Set running flag
		m_bRunning = true;
	}
}


void CMotherBoard::StopCPU(bool bUnbreak)
{
	if (m_bRunning) // если работает, выполняем, если уже остановлен - то не надо
	{
		m_bRunning = false;

		if (bUnbreak)
		{
			UnbreakCPU(ADDRESS_NONE); // отменяем отладочную приостановку, для случая, когда мы останавливаем
			// проц чтобы изменить конфигурацию, загрузить состояние или выйти из программы
		}
/***
		MSG msg;
		BOOL bRet;

		while (!m_mutRunLock.try_lock()) // ждём конца фрейма
		{
			// использовать lock_guard или просто lock нельзя, потому что это
			// остановит очередь сообщений и UI полностью зависнет в дедлоке
			// Хотя оно как-то умудряется всё равно остановить очередь сообщений,
			// поэтому берём на себя трансляцию сообщений, чтобы UI не зависало
			// этот костыль всё-таки нужен. Без него при загрузке сохранения получается дедлок UI.
			if (bRet = GetMessage(&msg, m_pParent->GetSafeHwnd(), 0, 0))
			{
				if (bRet != -1)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

		m_mutRunLock.unlock();****/
	}
}


/////////////////////////////////////////////////////////////////////////////
// перехват разных подпрограмм монитора, для их эмуляции
bool CMotherBoard::Interception()
{
	switch (GetRON(CCPU::REGISTER::PC) & 0177776)
	{
		case 0116256:
			return EmulateSaveTape();

		case 0116640:
			return EmulateLoadTape();
	}

	return false; // если ничего не выполнилось
}

int CMotherBoard::GetScreenPage() const
{
	return 5;
}

extern "C" {
#include "ROM10.h"
#include "Focal.h"
#include "FDDROM.h"
}

// выход: true - ПЗУ успешно прочитано
//       false - ПЗУ не прочитано или не задано
bool CMotherBoard::LoadRomModule(int iniRomNameIndex, int bank) {
	DBGM_PRINT(("CMotherBoard::LoadRomModule(%d, %d)", iniRomNameIndex, bank));
	switch(iniRomNameIndex) {
		case IDS_INI_BK10_RE2_017_MONITOR:
		    m_MemoryMap[8].addr = (uint8_t*) &ROM10[0x00000];
			m_MemoryMap[9].addr = (uint8_t*) &ROM10[0x01000];
		break;
		case IDS_INI_BK10_RE2_106_BASIC1:
		    m_MemoryMap[10].addr = (uint8_t*) &ROM10[0x02000];
			m_MemoryMap[11].addr = (uint8_t*) &ROM10[0x03000];
		break;
		case IDS_INI_BK10_RE2_107_BASIC2:
		    m_MemoryMap[12].addr = (uint8_t*) &ROM10[0x04000];
			m_MemoryMap[13].addr = (uint8_t*) &ROM10[0x05000];
		break;
		case IDS_INI_BK10_RE2_108_BASIC3:
		    m_MemoryMap[14].addr = (uint8_t*) &ROM10[0x06000];
			m_MemoryMap[15].addr = (uint8_t*) &ROM10[0x07000];
		break;
		case IDS_INI_BK10_RE2_084_FOCAL:
		    m_MemoryMap[10].addr = (uint8_t*) &ROM10FOCAL[0x00000];
			m_MemoryMap[11].addr = (uint8_t*) &ROM10FOCAL[0x01000];
		case IDS_INI_BK10_RE2_019_MSTD:
			m_MemoryMap[14].addr = (uint8_t*) &FDDROM[0x00000];
			m_MemoryMap[15].addr = (uint8_t*) &FDDROM[0x01000];
		break;
		default:
		    DBGM_PRINT(("CMotherBoard::LoadRomModule(%d, %d) not found", iniRomNameIndex, bank));
		break;
	}
    return true;
}

bool CMotherBoard::InitMemoryModules() { // TODO: redesign
	m_ConfBKModel.nROMPresent = 0;

	if (LoadRomModule(IDS_INI_BK10_RE2_017_MONITOR, BRD_10_MON10_BNK))
	{
		m_ConfBKModel.nROMPresent |= (3 << BRD_10_MON10_BNK);
	}

	if (LoadRomModule(IDS_INI_BK10_RE2_106_BASIC1, BRD_10_BASIC10_1_BNK))
	{
		m_ConfBKModel.nROMPresent |= (3 << BRD_10_BASIC10_1_BNK);
	}

	if (LoadRomModule(IDS_INI_BK10_RE2_107_BASIC2, BRD_10_BASIC10_2_BNK))
	{
		m_ConfBKModel.nROMPresent |= (3 << BRD_10_BASIC10_2_BNK);
	}

	if (LoadRomModule(IDS_INI_BK10_RE2_108_BASIC3, BRD_10_REGISTERS_BNK))
	{
		m_ConfBKModel.nROMPresent |= (3 << BRD_10_REGISTERS_BNK);
	}

	// и проинициализируем карту памяти
	MemoryManager();
	return true;
}

void CMotherBoard::MemoryManager() {
	for (int i = 0; i <= 017; ++i) {
		if (i < BRD_10_MON10_BNK) {
			m_MemoryMap[i].bReadable = true;
		} else {
			if (m_ConfBKModel.nROMPresent & (1 << i)) {
				m_MemoryMap[i].bReadable = true;
			} else {
				m_MemoryMap[i].bReadable = false;
			}
		}
		m_MemoryMap[i].bWritable = (i < BRD_10_MON10_BNK) ? true : false;
		m_MemoryMap[i].nBank = i;
		m_MemoryMap[i].nOffset = i << 12;
		m_MemoryMap[i].nTimingCorrection = (i < BRD_10_MON10_BNK) ? RAM_TIMING_CORR_VALUE_D : ROM_TIMING_CORR_VALUE;
	}
}


bool CMotherBoard::RestoreState(CMSFManager &msf, HBITMAP hScreenshot)
{
	if (RestorePreview(msf, hScreenshot))
	{
		if (RestoreConfig(msf))
		{
			if (RestoreRegisters(msf))
			{
				if (RestoreMemoryMap(msf)) // необходимо делать строго перед работой с памятью.
				{
					if (RestoreMemory(msf))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool CMotherBoard::RestoreConfig(CMSFManager &msf)
{ /***
	MSF_FRAMEDATA framedata{};

	if (msf.IsLoad())
	{
		msf.GetBlockConfig();

		if (msf.GetBlockFrameData(&framedata))
		{
			m_cpu.SetTimerSpeedInternal(framedata.nTimerSpeed, framedata.nTimerDiv);
			m_sTV.nVideoAddress = framedata.nVideoAddress & 0xffff; // видео адрес, младшие 6 бит - счётчик строк внутри строки
			m_sTV.bHgate = !!framedata.bHgate; // флаг отсчёта служебных видеоциклов в строке
			m_sTV.bVgate = !!framedata.bVgate; // флаг отсчёта служебных строк
			m_sTV.nVGateCounter = framedata.nVGateCounter; // дополнительный счётчик служебных строк
			m_sTV.nLineCounter = framedata.nLineCounter & 0377; // счётчик видео строк
			m_sTV.nCPUTicks = framedata.nCPUTicks;
			m_sTV.fMediaTicks = framedata.fMediaTicks;
			m_sTV.fMemoryTicks = framedata.fMemoryTicks;
			m_sTV.fFDDTicks = framedata.fFDDTicks;
			// вот это обязательно нужно сделать.
			// а то переинициализация фрейма делается опять не в то время, когда надо
			// с ещё старыми параметрами, а потом, с новыми уже не делается.
			m_nCPUFreq_prev = 0;
			FrameParam();
		}
		else
		{
			m_sTV.clear();
		}

		return true; // если конфиг не прочёлся, значит этого блока просто нет. ничего страшного
	}

	bool bRet = msf.SetBlockConfig();

	m_cpu.GetTimerSpeedInternal(framedata.nTimerSpeed, framedata.nTimerDiv);
	framedata.nVideoAddress = m_sTV.nVideoAddress; // видео адрес, младшие 6 бит - счётчик строк внутри строки
	framedata.bHgate = m_sTV.bHgate ? 1 : 0; // флаг отсчёта служебных видеоциклов в строке
	framedata.bVgate = m_sTV.bVgate ? 1 : 0; // флаг отсчёта служебных строк
	framedata.nVGateCounter = m_sTV.nVGateCounter; // дополнительный счётчик служебных строк
	framedata.nLineCounter = m_sTV.nLineCounter; // счётчик видео строк
	framedata.nCPUTicks = m_sTV.nCPUTicks;
	framedata.fMediaTicks = m_sTV.fMediaTicks;
	framedata.fMemoryTicks = m_sTV.fMemoryTicks;
	framedata.fFDDTicks = m_sTV.fFDDTicks;
	return  bRet && msf.SetBlockFrameData(&framedata); ***/ return true;
}


bool CMotherBoard::RestoreRegisters(CMSFManager &msf)
{/***
	MSF_CPU_REGISTERS cpu_reg{};
	MSF_PORT_REGS port_regs{};

	if (msf.IsLoad())
	{
		if (!msf.GetBlockCPURegisters(&cpu_reg))
		{
			return false;
		}

		SetRON(CCPU::REGISTER::R0, cpu_reg.r0);
		SetRON(CCPU::REGISTER::R1, cpu_reg.r1);
		SetRON(CCPU::REGISTER::R2, cpu_reg.r2);
		SetRON(CCPU::REGISTER::R3, cpu_reg.r3);
		SetRON(CCPU::REGISTER::R4, cpu_reg.r4);
		SetRON(CCPU::REGISTER::R5, cpu_reg.r5);
		SetRON(CCPU::REGISTER::SP, cpu_reg.sp);
		SetRON(CCPU::REGISTER::PC, cpu_reg.pc);
		SetPSW(cpu_reg.psw);

		if (msf.GetVersion() >= MSF_VERSION_MINIMAL)
		{
			if (!msf.GetBlockPortRegs(&port_regs))
			{
				return false;
			}

			m_reg177660 = port_regs.p0177660;
			m_reg177662in = port_regs.p0177662_in;
			m_reg177662out = port_regs.p0177662_out;
			m_reg177664 = port_regs.p0177664;
			m_reg177714in = port_regs.p0177714_in;
			m_reg177714out = port_regs.p0177714_out;
			m_reg177716in = port_regs.p0177716_in;
			m_reg177716out_tap = port_regs.p0177716_out_tap;
			m_reg177716out_mem = port_regs.p0177716_out_mem;
			m_cpu.SetSysRegsInternal(0177700, port_regs.p0177700);
			m_cpu.SetSysRegsInternal(0177702, port_regs.p0177702);
			m_cpu.SetSysRegsInternal(0177704, port_regs.p0177704);
			m_cpu.SetSysRegsInternal(0177706, port_regs.p0177706);
			m_cpu.SetSysRegsInternal(0177710, port_regs.p0177710);
			m_cpu.SetSysRegsInternal(0177712, port_regs.p0177712);
		}
		else
		{
			return false;
		}
	}
	else
	{
		cpu_reg.r0 = GetRON(CCPU::REGISTER::R0);
		cpu_reg.r1 = GetRON(CCPU::REGISTER::R1);
		cpu_reg.r2 = GetRON(CCPU::REGISTER::R2);
		cpu_reg.r3 = GetRON(CCPU::REGISTER::R3);
		cpu_reg.r4 = GetRON(CCPU::REGISTER::R4);
		cpu_reg.r5 = GetRON(CCPU::REGISTER::R5);
		cpu_reg.sp = GetRON(CCPU::REGISTER::SP);
		cpu_reg.pc = GetRON(CCPU::REGISTER::PC);
		cpu_reg.psw = GetPSW();

		if (!msf.SetBlockCPURegisters(&cpu_reg))
		{
			return false;
		}

		port_regs.p0177660 = m_reg177660;
		port_regs.p0177662_in = m_reg177662in;
		port_regs.p0177662_out = m_reg177662out;
		port_regs.p0177664 = m_reg177664;
		port_regs.p0177714_in = m_reg177714in;
		port_regs.p0177714_out = m_reg177714out;
		port_regs.p0177716_in = m_reg177716in;
		port_regs.p0177716_out_tap = m_reg177716out_tap;
		port_regs.p0177716_out_mem = m_reg177716out_mem;
		port_regs.p0177700 = m_cpu.GetSysRegsIndirect(0177700);
		port_regs.p0177702 = m_cpu.GetSysRegsIndirect(0177702);
		port_regs.p0177704 = m_cpu.GetSysRegsIndirect(0177704);
		port_regs.p0177706 = m_cpu.GetSysRegsIndirect(0177706);
		port_regs.p0177710 = m_cpu.GetSysRegsIndirect(0177710);
		port_regs.p0177712 = m_cpu.GetSysRegsIndirect(0177712);

		if (!msf.SetBlockPortRegs(&port_regs))
		{
			return false;
		}
	}
***/
	return true;
}


bool CMotherBoard::RestoreMemory(CMSFManager &msf)
{
///	if (msf.IsLoad())
///	{
///		return msf.GetBlockBaseMemory(GetMainMemory());
///	}
///	return msf.SetBlockBaseMemory(GetMainMemory());
}

bool CMotherBoard::RestoreMemoryMap(CMSFManager &msf)
{/***
	if (msf.IsLoad())
	{
		return msf.GetBlockMemMap(m_MemoryMap, &m_ConfBKModel);
	}

	return msf.SetBlockMemMap(m_MemoryMap, &m_ConfBKModel);**/
}


bool CMotherBoard::RestorePreview(CMSFManager &msf, HBITMAP hScreenshot)
{
	bool bRet = true;
/***
	if (msf.IsSave())
	{
		ASSERT(hScreenshot);

		if (hScreenshot)
		{
			auto hBm = (HBITMAP)CopyImage(hScreenshot, IMAGE_BITMAP, 256, 256, LR_CREATEDIBSECTION | LR_COPYDELETEORG);

			if (hBm)
			{
				bRet = msf.SetBlockPreview(hBm);
				DeleteObject(hBm);
			}

			DeleteObject(hScreenshot);
		}
	}
****/
	return bRet;
}

void CMotherBoard::StopTimerThread()
{
///	if (!m_mutLockTimerThread.try_lock())
	{
		m_bKillTimerEvent = true;
		cancel_repeating_timer(&m_TimerThread);
		StopCPU();
		MSG msg;
		BOOL bRet;

///		while (!m_mutLockTimerThread.try_lock()) // ждём завершения потока
		{
			// использовать lock_guard или просто lock нельзя, потому что это
			// остановит очередь сообщений и UI полностью зависнет в дедлоке
			// Хотя оно как-то умудряется всё равно остановить очередь сообщений,
			// поэтому берём на себя трансляцию сообщений, чтобы UI не зависало
			// этот костыль всё-таки нужен. Без него при загрузке сохранения получается дедлок UI.
///			if (bRet = GetMessage(&msg, m_pParent->GetSafeHwnd(), 0, 0))
			{
				if (bRet != -1)
				{
///					TranslateMessage(&msg);
///					DispatchMessage(&msg);
				}
			}
		}

		// поток остановился, и можно продолжать остальные действия по остановке
	}

///	m_mutLockTimerThread.unlock();
}

#define SYSTEM_TIMER_US (int64_t)20480

static bool system_timer_50_Hz_cb(repeating_timer_t *rt) {
	DBGM_PRINT(("system_timer_50_Hz_cb"))
    CMotherBoard *pb = (CMotherBoard*)rt->user_data;
    pb->TimerThreadFunc();
    return true;
}

bool CMotherBoard::StartTimerThread()
{
	DBGM_PRINT(("CMotherBoard::StartTimerThread()"))
	m_PreviousPC = ADDRESS_NONE;
	m_bKillTimerEvent = false;
	return add_repeating_timer_us (SYSTEM_TIMER_US, system_timer_50_Hz_cb, this, &m_TimerThread);
/**	m_TimerThread = std::thread(&CMotherBoard::TimerThreadFunc, this);
	m_bKillTimerEvent = false;

	if (m_TimerThread.joinable())
	{
		m_TimerThread.detach();
		return true;
	}

	g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	return false; **/
}


void CMotherBoard::FrameParam()
{
	if (m_nCPUFreq_prev != m_nCPUFreq) // если частота не менялась, пересчёта не делать
	{
		m_nCPUFreq_prev = m_nCPUFreq;
		auto fCPUfreq = double(m_nCPUFreq);
		m_sTV.fCpuTickTime = 1.0 / fCPUfreq; // длительность одного такта в секундах
		m_sTV.fMemory_Mod = fCPUfreq / 750000.0;      // коэффициент пересчёта ТЧ проца в количество обращений к видеопамяти за фрейм
		m_sTV.fFDD_Mod = fCPUfreq / 15625.0;  // 15625 = 5 оборотов в сек, в одном обороте 3125 слов,
		// норма 15625.0, больше - быстрее вращение, меньше - медленнее
		m_sTV.fMedia_Mod = fCPUfreq / double(g_Config.m_nSoundSampleRate); // коэффициент пересчёта тактов процессора в медиа такты
		// означает - через сколько тактов процессора надо вставлять 1 медиа такт, т.к. число не целое, то
		// получается приблизительно.
	}
}


void CMotherBoard::TimerThreadFunc()
{
	DBGM_PRINT(("CMotherBoard::TimerThreadFunc()"))
	if (m_bKillTimerEvent) return;
///	std::lock_guard<std::mutex> lk(m_mutLockTimerThread);
	uint16_t nPreviousPC = ADDRESS_NONE;    // предыдущее значение регистра РС
	// типы nPreviousPC и m_sTV.nGotoAddress не должны совпадать, иначе будет всегда срабатывать условие отладочного
	// останова, даже если нам этого не надо

///	do
	{
		if (m_bRunning) // если процессор работает, выполняем эмуляцию
		{
	///		m_mutRunLock.lock(); // блокируем участок, чтобы при остановке обязательно дождаться, пока фрейм не закончится

			if (m_bBreaked) // если процессор в отладочном останове
			{
				DrawDebugScreen();  // продолжаем обновлять экран
	///			Sleep(20);          // со стандартной частотой примерно 50Гц
			}
			else
			{
				// Выполняем набор инструкций
				// Если время пришло, выполняем текущую инструкцию
				if (--m_sTV.nCPUTicks <= 0)
				{
					try
					{
						if (Interception()) // если был перехват разных подпрограмм монитора, для их эмуляции
						{
							m_sTV.nCPUTicks = 8; // сколько-нибудь
						}
						else // если не был -
						{
							// Выполняем одну инструкцию, возвращаем время выполнения одной инструкции в тактах.
							/////m_sTV.nCPUTicks = m_cpu.TranslateInstruction();

							if (m_nKeyCleanEvent)
							{
								if ((m_nKeyCleanEvent -= m_sTV.nCPUTicks) <= 0)
								{
									m_nKeyCleanEvent = 0;
									m_reg177660 &= ~0200;
								}
							}
						}

						// Сохраняем текущее значение PC для отладки
						/////nPreviousPC = m_cpu.GetRON(CCPU::REGISTER::PC);
					}
					catch (CExceptionHalt &halt)
					{
						// BK Violation exception. Например запись в ПЗУ или чтение из несуществующей памяти
						if (g_Config.m_bAskForBreak)
						{
							BreakCPU();  // Останавливаем CPU для отладки
		///					CString strMessage;
		///					strMessage.Format(IDS_ERRMSG_ROM, halt.m_addr, nPreviousPC, halt.m_info);
		///					int answer = g_BKMsgBox.Show(strMessage, MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 | MB_ICONSTOP);

		///					switch (answer)
							{
		///						case IDIGNORE:  // если выбрали "игнорировать"
		///							g_Config.m_bAskForBreak = false; // то снимаем галку, и больше не вызываем диалог
		///							[[fallthrough]];
								// и выполним всё, что должно выполняться для "повтор"
		///						case IDRETRY: // если выбрали "повтор" - то просто продолжить выполнение и посмотреть, что будет дальше
									UnbreakCPU(ADDRESS_NONE); // обратно запускаем CPU
									//////m_cpu.ReplyError();  // Делаем прер. по вектору 4(halt) в следующем цикле
		///							break;
		return;

		///						case IDABORT: // если выбрали "прервать" - остаёмся в отладочном останове
		///						default:
		///							break;
							}
						}
						else
						{
							m_sTV.nCPUTicks = 64;
							/////m_cpu.ReplyError();  // Делаем прер. по вектору 4(halt) в следующем цикле
						}

						nPreviousPC = ADDRESS_NONE;
					}
					catch (...)
					{
						// Любое другое исключение. Неизвестное исключение или ошибка доступа к памяти WINDOWS.
						// Эта ошибка может быть неизвестной ошибкой BK или ошибкой эмулятора
	///					CString strMessage(MAKEINTRESOURCE(IDS_ERRMSG_INTERNAL));
	///					g_BKMsgBox.Show(strMessage, MB_OK | MB_ICONSTOP);
						BreakCPU();
	///					m_mutRunLock.unlock();
						StopCPU(false); // внутри этой функции мы пытаемся блокировать m_mutRunLock
	///					break; // Прекращаем выполнять инструкции - завершаем этот поток
	return;
					}
/***
					if (
					    m_pDebugger->GetDebugPCBreak(nPreviousPC) // Спросим отладчик на счёт условий остановки. Если ВОИСТИНУ останов
					    || (m_sTV.nGotoAddress == nPreviousPC)	// Если останов на адресе где стоит отладочный курсор
					    || (m_sTV.nGotoAddress == GO_INTO)		// Отладочный останов если только одиночный шаг
					    || (m_sTV.nGotoAddress == GO_OUT && CDebugger::IsInstructionOut(m_cpu.GetCurrentInstruction())
						    )  // Отладочный останов если команда выхода из п/п
					)
					{
						BreakCPU();
					}
***/
				}
				if (--m_sTV.fMemoryTicks <= 0.0)
				{
					do
					{
						Make_One_Screen_Cycle();  // тут выполняются циклы экрана
						m_sTV.fMemoryTicks += m_sTV.fMemory_Mod;
					}
					while (m_sTV.fMemoryTicks < 1.0);
				}

				m_pSpeaker->RCFilterLF(m_sTV.fCpuTickTime); // эмуляция конденсатора на выходе линейного входа.

				if (--m_sTV.fMediaTicks <= 0.0)
				{
					do
					{
						MediaTick();  // тут делается звучание всех устройств и обработка прочих устройств
						m_sTV.fMediaTicks += m_sTV.fMedia_Mod;
					}
					while (m_sTV.fMediaTicks < 1.0);
				}

				if (--m_sTV.fFDDTicks <= 0.0)
				{
					do
					{
					////	m_fdd.Periodic();     // Вращаем диск на одно слово на дорожке
						m_sTV.fFDDTicks += m_sTV.fFDD_Mod;
					}
					while (m_sTV.fFDDTicks < 1.0);
				}
			}

	///		m_mutRunLock.unlock();
		}
	///	else
		{
	///		Sleep(20);
		}
	}
///	while (!m_bKillTimerEvent);       // пока не придёт событие остановки
}

void CMotherBoard::SetMTC(int mtc)
{
	// переменные для медиа системы
	if (m_sTV.nMediaTicksPerFrame != mtc)
	{
		m_sTV.nMediaTicksPerFrame = mtc; // длина буфера в сэмплах

	///	if (m_sTV.pSoundBuffer)
		{
	///		m_sTV.pSoundBuffer.reset();
		}

	///	m_sTV.pSoundBuffer = std::make_unique<SAMPLE_INT[]>(static_cast<size_t>(m_sTV.nMediaTicksPerFrame) * BUFFER_CHANNELS);

	///	if (!m_sTV.pSoundBuffer)
		{
	///		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		}
	}
}

void CMotherBoard::MediaTick()
{
	// ASSERT(m_pSpeaker);

	// обработка медиатактов
	if (m_sTV.nMediaTickCount == 0)
	{
		// Получаем сохранённые звуковые данные, это чтобы слышать звучание с кассеты
	///	m_pParent->GetTapePtr()->PlayWaveGetBuffer(m_sTV.pSoundBuffer.get(), m_sTV.nMediaTicksPerFrame); // Берём данные с ленты -> m_pSoundBuffer
		// Посылаем данные с ленты в 177716 <- m_sTV.pSoundBuffer
	///	m_pSpeaker->ReceiveTapeBuffer(m_sTV.pSoundBuffer.get(), m_sTV.nMediaTicksPerFrame);
		m_sTV.nBufPos = 0;
	}

	// осуществляем приём данных с магнитофона, так он будет работать даже если спикер выключен
	if (m_pSpeaker->GetTapeSample())
	{
		m_reg177716in |= 040;
	}
	else
	{
		m_reg177716in &= ~040;
	}

	sOneSample sm{ 0.0, 0.0 };
///	m_pSound->SoundGen_ResetSample(m_sTV.pSoundBuffer[m_sTV.nBufPos], m_sTV.pSoundBuffer[static_cast<size_t>(m_sTV.nBufPos) + 1]); // задаём начальные уровни для микширования

	// Берём звуки со спикера BK
	if (m_pSpeaker->IsSoundEnabled())
	{
		m_pSpeaker->GetSample(&sm);
		m_pSound->SoundGen_MixSample(&sm); // микшируем сэмплы внутри m_pSound
	}

	// Берём данные с ковокса
	if (m_pCovox->IsSoundEnabled())
	{
		m_pCovox->GetSample(&sm);
		m_pSound->SoundGen_MixSample(&sm);
	}

	if (m_pMenestrel->IsSoundEnabled())
	{
		m_pMenestrel->GetSample(&sm);
		m_pSound->SoundGen_MixSample(&sm);
	}

	// Берём данные с ay8910
	if (m_pAYSnd->IsSoundEnabled())
	{
		m_pAYSnd->GetSample1(&sm);
		m_pSound->SoundGen_MixSample(&sm);

		if (g_Config.m_b2ndAY8910)
		{
			m_pAYSnd->GetSample2(&sm);
			m_pSound->SoundGen_MixSample(&sm);
		}
	}

	m_pSound->SoundGen_FeedDAC_Mixer(&sm); // получаем сюда смикшированное, а там - осуществляем звучание
///	m_sTV.pSoundBuffer[m_sTV.nBufPos++] = sm.s[OSL]; // это нужно в основном для осциллографа
///	m_sTV.pSoundBuffer[m_sTV.nBufPos++] = sm.s[OSR]; // и для экспорта вывода в WAV(TAP)

	if (++m_sTV.nMediaTickCount >= m_sTV.nMediaTicksPerFrame)
	{
		m_sTV.nMediaTickCount = 0;
		// копируем содержимое буфера осциллографу
	///	m_pParent->SendMessage(WM_OSC_FILLBUFFER, 0, reinterpret_cast<LPARAM>(m_sTV.pSoundBuffer.get()));
		// Отображаем копию буфера на осциллографе в фоне
	///	m_pParent->PostMessage(WM_OSC_DRAW);
		// Отправляем их на ленту <- m_sTV.pSoundBuffer
	///	m_pParent->GetTapePtr()->RecordWaveGetBuffer(m_sTV.pSoundBuffer.get(), m_sTV.nMediaTicksPerFrame);
	}
}


// это вариант точного алгоритма из VP1-037
void CMotherBoard::Make_One_Screen_Cycle()
{
	uint16_t dww = m_sTV.nVideoAddress & 076; // счётчик слов внутри строки
	uint16_t dwa = m_sTV.nVideoAddress & 037700; // адрес строки

	if (!(m_sTV.bVgate || m_sTV.bHgate))
	{
///		m_pParent->GetScreen()->SetExtendedMode(!(m_reg177664 & 01000));
///		DWORD_PTR nScrAddr = (static_cast<DWORD_PTR>(GetScreenPage())) << 14;
///		uint8_t *nScr = GetMainMemory() + nScrAddr + m_sTV.nVideoAddress;
///		m_pParent->GetScreen()->PrepareScreenLineWordRGB32(m_sTV.nLineCounter, dww, *reinterpret_cast<uint16_t *>(nScr));
	}

	dww += 2; // переходим к следующему слову

	if (m_sTV.bHgate) // считаем служебное поле в строке
	{
		if (dww == 040) // отсчитали 16. служебных слов
		{
			dww = 0;
			m_sTV.bHgate = false;
		}
	}
	else if (dww == 0100)
	{
		// если отсчитали 32. слова, нужно теперь считать служебные
		dww = 0;
		m_sTV.bHgate = true;
		int dwl = (m_sTV.nLineCounter + 1) & 0377; // Счётчик строк

		if (m_sTV.bVgate) // считаем служебные
		{
			if (--m_sTV.nVGateCounter <= 0) // отсчитали 64 служебные строки
			{
				dwl = 0;  // снова начнём считать информационные
				m_sTV.bVgate = false;
			}

			// когда считаем служебные строки, выбираем из них 353..350
			if ((m_sTV.nVGateCounter & 0374) == 050)    // if (050 <= m_nVGateCounter && m_nVGateCounter <= 053)
			{
				// если строки 351, 350
				if (!(m_sTV.nVGateCounter & 2))         // if (050 == m_nVGateCounter || m_nVGateCounter == 051)
				{
					// применяем новое смещение
					dwa = (m_reg177664 & 0377) << 6; // VA[13:6] <= RA[7:0];
				}

				// при этом на новую строку не переходим. в одной крутимся.
			}
			else
			{
				// переход на следующую строку
				dwa += 0100; // VA[13:6] <= VA[13:6] + 8'b00000001;
			}
		}
		else
		{
			if (dwl == 0)
			{
				// если отсчитали 256 информационных строк, нужно теперь считать служебные.
				m_sTV.bVgate = true;
				m_sTV.nVGateCounter = 64;

				if (!(m_reg177662out & 040000)) // если бит 14 установлен, таймер не работает.
				{
					Irq2Interrupt();

					if (m_pAYSnd)
					{
						// для лога дампа регистров AY
						m_pAYSnd->log_timerTick();
					}
				}

	///			m_pParent->SendMessage(WM_SCR_DRAW);
				// сделаем жёсткую непосредственную прорисовку кадра, при этом D2D в свёрнутом виде тормозит.
				//m_pParent->GetScreen()->DrawScreen(true);
			}

			// переход на следующую строку
			dwa += 0100; // VA[13:6] <= VA[13:6] + 8'b00000001;
		}

		m_sTV.nLineCounter = dwl;
	}

	m_sTV.nVideoAddress = (dwa | dww) & 037776;
}


// специально для отладки, рисуем экран старым методом.
// при этом новый метод не отключается и поэтому возможны разного рода казусы.
void CMotherBoard::DrawDebugScreen() const
{
	int nScrAddr = GetScreenPage() << 14;
///	m_pParent->PostMessage(WM_SCR_DEBUGDRAW, WPARAM(nScrAddr));
}

constexpr auto BK_NAMELENGTH = 16;   // Максимальная длина имени файла на БК - 16 байтов
constexpr auto BK_BMB10 = 0320;
constexpr auto BK_BMB10_ERRADDR = 0301;
constexpr auto BK_BMB10_CRCADDR = 0312;
constexpr auto BK_BMB10_ADDRESS = 2;
constexpr auto BK_BMB10_LENGTH = 4;
constexpr auto BK_BMB10_NAME = 6;
constexpr auto BK_BMB10_FOUND_ADDRESS = BK_BMB10_NAME + BK_NAMELENGTH;
constexpr auto BK_BMB10_FOUND_LENGTH = BK_BMB10_FOUND_ADDRESS + 2;
constexpr auto BK_BMB10_FOUND_NAME = BK_BMB10_FOUND_LENGTH + 2;

// для БК10.
bool CMotherBoard::EmulateLoadTape()
{
	/*
	 * Известные косяки: 1) для бейсиковских бин файлов удаляет расширение .bin, как исправить
	 * непонятно, потому что никак не узнать, что мы загружаем именно бейсиковский бин файл,
	 * а не какой-либо другой. То же самое и с любыми другими файлами, по задумке авторов,
	 * имеющими расширение .bin, оно как правило удаляется.
	 * 2) при загрузке файла с автозапуском, автозапуск делается из ячейки на слово выше, чем в реальности.
	 * Это может быть критично только для очень хитрых программ.
	 **/
	// если включена эмуляция и по этому адресу действительно ПЗУ монитора БК10
	if (g_Config.m_bEmulateLoadTape && ((GetWord(0116206) == 04767) && (GetWord(0116210) == 0426)))
	{ /***
		bool bFileSelect = false; // что делать после диалога выбора
		bool bCancelSelectFlag = false; // флаг для усложнения алгоритма
		bool bError = false;
		bool bIsDrop = false;
		CString strBinExt(MAKEINTRESOURCE(IDS_FILEEXT_BINARY));
		uint16_t fileAddr = 0;
		uint16_t abp = BK_BMB10;
		fs::path strFileName;
		// Внутренняя загрузка на БК
		uint8_t bkName[BK_NAMELENGTH] = { 0 };  // Максимальная длина имени файла на БК - BK_NAMELENGTH байтов
		uint8_t bkFoundName[BK_NAMELENGTH] = { 0 };

		if (!m_pParent->isBinFileNameSet())
		{
			// если загружаем не через драг-н-дроп, то действуем как обычно
			abp = GetRON(CCPU::REGISTER::R1);       // получим адрес блока параметров из R1 (BK emt 36)
			fileAddr = GetWord(abp + BK_BMB10_ADDRESS);    // второе слово - адрес загрузки/сохранения

			// Подбираем BK_NAMELENGTH байтовое имя файла из блока параметров
			for (uint16_t c = 0; c < BK_NAMELENGTH; ++c)
			{
				bkName[c] = GetByte(abp + BK_BMB10_NAME + c);
			}

			strFileName = Global::BKToUNICODE(bkName, BK_NAMELENGTH).Trim().GetString(); // тут надо перекодировать  имя файла из кои8 в unicode

			if (!strFileName.empty()) // если имя файла не пустое
			{
				strFileName += strBinExt.GetString(); // добавляем стандартное расширение для бин файлов,
				// чтобы не рушить логику следующих проверок
			}
		}
		else
		{
			// если загружаем через драг-н-дроп, то берём имя оттуда
			strFileName = m_pParent->GetStrBinFileName();
			bIsDrop = true;
		}

		if (strFileName.empty())
		{
			// Если имя пустое - то покажем диалог выбора файла.
			bFileSelect = false;
l_SelectFile:
			// Запомним текущую директорию
			fs::path strCurDir = fs::current_path();
			fs::current_path(g_Config.m_strBinPath);
			CString strFilterBin(MAKEINTRESOURCE(IDS_FILEFILTER_BIN));
			CFileDialog dlg(TRUE, nullptr, nullptr,
			                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
			                strFilterBin, m_pParent->GetScreen()->GetBackgroundWindow());
			// Зададим начальной директорией директорию с Bin файлами
			dlg.GetOFN().lpstrInitialDir = g_Config.m_strBinPath.c_str();

			if (dlg.DoModal() == IDCANCEL)
			{
				// Если нажали Отмену, установим ошибку во втором байте блока параметров
				SetByte(BK_BMB10_ERRADDR, 4);
				bError = true; // случилась ошибка
			}
			else
			{
				strFileName = dlg.GetPathName().GetString(); // вот выбранный файл
				g_Config.m_strBinPath = strFileName.parent_path();
				// имя файла надо бы как-то поместить в 0352..0372 иначе некоторые глюки наблюдаются
				CString strFound = dlg.GetFileTitle();
				Global::UNICODEtoBK(strFound, bkFoundName, BK_NAMELENGTH, true); // вот из этого массива будем потом помещать

				if (bFileSelect)
				{
					// тут надо проверить тот ли файл нам подсовывают.
					CString strFindEx = Global::BKToUNICODE(bkName, BK_NAMELENGTH); // с расширением
					fs::path p = strFindEx.GetString();
					CString strFind = p.stem().c_str(); // без расширения

					if (!bIsDrop) // только если не дроп. там не с чем сравнивать
					{
						if (strFind.CollateNoCase(strFound) != 0 && strFindEx.CollateNoCase(strFound) != 0)
						{
							int result = g_BKMsgBox.Show(IDS_BK_ERROR_WRONGFILE, MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);

							if (result == IDYES)
							{
								goto l_SelectFile;
							}
							else
							{
								SetByte(BK_BMB10_ERRADDR, 4);
								bError = true;
								bCancelSelectFlag = true; // не будем ничего делать, сразу выйдем.
							}
						}
					}
				}
			}

			// восстановим текущую директорию
			fs::current_path(strCurDir);
		}
		else    // Если имя не пустое
		{
			// If Saves Default flag is set loading from User directory
			// Else Load from Binary files directory
			fs::path strCurrentPath = g_Config.m_bSavesDefault ? g_Config.m_strSavesPath : g_Config.m_strBinPath;
			CString s = strFileName.c_str();
			Global::SetSafeName(s); // перекодируем небезопасные символы на безопасные
			strFileName = s.GetString();

			// сейчас узнаем, нужно ли нам добавлять расширение .bin, или наоборот, удалять
			if (CString(strFileName.extension().c_str()).CollateNoCase(strBinExt) == 0)
			{
				// у файла уже есть расширение бин
				if (!fs::exists(strCurrentPath / strFileName)) // если нету файла с расширением.
				{
					fs::path str = strFileName.stem(); // удаляем расширение

					if (fs::exists(strCurrentPath / str)) // если есть файл без расширения
					{
						strFileName = str; // оставим файл без расширения
					}
					else
					{
						// нет файла ни с расширением, ни без расширения
						SetByte(BK_BMB10_ERRADDR, 1);
						bError = true;
					}
				}
			}
			else
			{
				// у файла нету расширения бин
				if (!fs::exists(strCurrentPath / strFileName)) // если нету файла без расширения
				{
					fs::path str = strFileName;
					str += strBinExt.GetString(); // добавляем стандартное расширение для бин файлов.

					if (fs::exists(strCurrentPath / str))  // если есть файл с расширением
					{
						strFileName = str;
					}
					else
					{
						// нет файла ни с расширением, ни без расширения
						SetByte(BK_BMB10_ERRADDR, 1);
						bError = true;
					}
				}
			}

			CString str = strFileName.stem().c_str();
			Global::UNICODEtoBK(str, bkFoundName, BK_NAMELENGTH, true);
			strFileName = strCurrentPath / strFileName;
		}

		if (!bCancelSelectFlag)
		{
			CFile file;
			uint16_t readAddr = 0;
			uint16_t readSize = 0;
			uint16_t loadAddr = 01000;
			uint16_t loadLen = 0;
			uint16_t loadcrc = 0;

			// Загрузим файл, если ошибок не было
			if (!bError && file.Open(strFileName.c_str(), CFile::modeRead))
			{
				file.Read(&readAddr, sizeof(readAddr));   // Первое слово в файле - адрес загрузки
				file.Read(&readSize, sizeof(readSize));   // Второе слово в файле - длина
				// сплошь и рядом встречаются .bin файлы. у которых во втором слове указана длина
				// меньше, чем длина файла - 4. Это другой формат бин, у которого в начале указывается
				// адрес, длина, имя файла[BK_NAMELENGTH], массив[длина], КС - контрольная сумма в конце
				uint16_t filesz = (file.GetLength() < 65536) ? static_cast<uint16_t>(file.GetLength()) : 65535;
				bool bIsCRC = false;

				if (readSize == filesz - 4)
				{
					bIsCRC = false;
				}
				else if (readSize == filesz - 6)
				{
					bIsCRC = true;
				}
				else if (readSize == filesz - 22)
				{
					bIsCRC = true;
					file.Read(bkFoundName, BK_NAMELENGTH); // прочитаем оригинальное имя файла
				}
				else
				{
					// всё равно загрузим. Пусть не бин
					file.Seek(0, CFile::SeekPosition::begin);
					readAddr = 0;
					readSize = filesz;
				}

				SetWord(abp + BK_BMB10_FOUND_ADDRESS, readAddr);
				SetWord(abp + BK_BMB10_FOUND_LENGTH, readSize);

				if (bkFoundName[0])
				{
					// копируем прочитанное имя файла
					for (uint16_t i = 0; i < BK_NAMELENGTH; ++i)
					{
						SetByte(abp + BK_BMB10_FOUND_NAME + i, bkFoundName[i]);
					}
				}
				else
				{
					// копируем прочитанное имя файла
					for (uint16_t i = 0; i < BK_NAMELENGTH; ++i)
					{
						SetByte(abp + BK_BMB10_FOUND_NAME + i, bkName[i]);
					}
				}

				if (fileAddr == 0)
				{
					fileAddr = readAddr;
				}

//              if (fileAddr < 01000)
//              {
//                  // если файл с автозапуском, на всякий случай остановим скрипт
//                  // если он выполнялся
//                  m_pParent->GetScriptRunnerPtr()->StopScript();
//              }
				SetWord(0264, fileAddr); loadAddr = fileAddr;
				SetWord(0266, readSize); loadLen = readSize;
				DWORD cs = 0; // подсчитаем контрольную сумму

				// Загрузка по адресу fileAddr
				for (int i = 0; i < readSize; ++i)
				{
					uint8_t val;
					file.Read(&val, sizeof(val));
					SetByte(fileAddr++, val);
					cs += uint16_t(val);

					if (cs & 0xffff0000)
					{
						cs++;
						cs &= 0xffff;
					}
				}

				uint16_t crc;

				if (bIsCRC && file.Read(&crc, sizeof(crc)) == sizeof(uint16_t))
				{
					if (crc != LOWORD(cs))
					{
						SetByte(BK_BMB10_ERRADDR, 2);
						cs = crc;
					}
				}

				// а иначе, мы не знаем какая должна быть КС. поэтому считаем, что файл априори верный
				file.Close();
				// Заполняем системные ячейки, как это делает emt 36
				loadcrc = LOWORD(cs);
				SetWord(BK_BMB10_CRCADDR, loadcrc); // сохраним контрольную сумму
			}
			else
			{
				// При ошибке покажем сообщение
				CString strError;
				strError.Format(IDS_CANT_OPEN_FILE_S, strFileName.c_str());
				int result = g_BKMsgBox.Show(strError, MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON2);

				switch (result)
				{
					case IDNO:
						// если не хотим останавливаться, то пойдём на диалог, и поищем файл в другом месте.
						bError = false;
						SetByte(BK_BMB10_ERRADDR, 0);
						bFileSelect = true; // включим проверку на неподходящее имя.
						goto l_SelectFile;

					case IDYES:
						// если хотим остановиться - зададим останов.
						BreakCPU();
						[[fallthrough]];

					// если отмена - просто выходим с заданным кодом ошибки
					case IDCANCEL:
						SetByte(BK_BMB10_ERRADDR, 4);
						break;
				}
			}

			if (loadAddr < 0750)
			{
				// Помещаем в R1 последний адрес, куда производилось чтение, как в emt 36
				SetRON(CCPU::REGISTER::R0, 0);
				SetRON(CCPU::REGISTER::R1, loadAddr + loadLen);
				SetRON(CCPU::REGISTER::R2, 0);
				SetRON(CCPU::REGISTER::R3, 0177716);
				SetRON(CCPU::REGISTER::R5, 040);
				SetRON(CCPU::REGISTER::PC, 0117374); // выходим туда.
			}
			else
			{
				SetRON(CCPU::REGISTER::R0, loadcrc);
				SetRON(CCPU::REGISTER::R0, 0314);
				SetRON(CCPU::REGISTER::R3, 0177716);
				SetRON(CCPU::REGISTER::R4, 0);
				// Помещаем в R5 последний адрес, куда производилось чтение, как в emt 36
				SetRON(CCPU::REGISTER::R5, loadAddr + loadLen);
				SetRON(CCPU::REGISTER::PC, 0116710); // выходим туда.
			}
		}
		else
		{
			SetRON(CCPU::REGISTER::PC, 0116214); // выходим туда.
		}

		// Refresh keyboard
		m_pParent->SendMessage(WM_RESET_KBD_MANAGER, 0); // и почистим индикацию управляющих клавиш в статусбаре
TODO:		***/
		return true; // сэмулировали
	}

	return false; // не эмулируем
}

bool CMotherBoard::EmulateSaveTape()
{ /***
	// если включена эмуляция и по этому адресу действительно ПЗУ монитора БК10
	if (g_Config.m_bEmulateSaveTape && ((GetWord(0116170) == 04767) && (GetWord(0116172) == 062)))
	{
		// получим адрес блока параметров из R1 (BK emt 36)
		uint16_t abp = GetRON(CCPU::REGISTER::R1);
		uint16_t fileAddr = GetWord(abp + BK_BMB10_ADDRESS);  // второе слово - адрес загрузки/сохранения
		uint16_t fileSize = GetWord(abp + BK_BMB10_LENGTH);  // третье слово - длина файла (для загрузки может быть 0)

		if (fileSize)
		{
			bool bError = false;
			uint8_t bkName[BK_NAMELENGTH] {};  // Максимальная длина имени файла на БК - BK_NAMELENGTH байтов

			// Подбираем BK_NAMELENGTH байтовое имя файла из блока параметров
			for (uint16_t c = 0; c < BK_NAMELENGTH; ++c)
			{
				bkName[c] = GetByte(abp + BK_BMB10_NAME + c);
			}

			fs::path strFileName = Global::BKToUNICODE(bkName, BK_NAMELENGTH).Trim().GetString(); // тут надо перекодировать  имя файла из кои8 в unicode

			// Если имя пустое
			if (strFileName.empty())
			{
				// Покажем диалог сохранения
				CFileDialog dlg(FALSE, nullptr, nullptr,
				                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
				                nullptr, m_pParent->GetScreen()->GetBackgroundWindow());
				dlg.GetOFN().lpstrInitialDir = g_Config.m_strBinPath.c_str();

				if (dlg.DoModal() == IDOK)
				{
					// Получим имя
					strFileName = dlg.GetPathName().GetString();
					g_Config.m_strBinPath = strFileName.parent_path();
					CString s = strFileName.filename().c_str();
					Global::UNICODEtoBK(s, bkName, BK_NAMELENGTH, true);
				}
				else
				{
					// Если отмена - установим флаг ошибки
					SetByte(BK_BMB10_ERRADDR, 4);
					bError = true;
				}
			}
			else
			{
				CString strBinExt(MAKEINTRESOURCE(IDS_FILEEXT_BINARY));
				// Если имя не пустое
				CString s = strFileName.c_str();
				Global::SetSafeName(s); // перекодируем небезопасные символы на безопасные
				strFileName = s.GetString();
				strFileName = (g_Config.m_bSavesDefault ? g_Config.m_strSavesPath : g_Config.m_strBinPath) // подставляем соответствующий путь
				              / strFileName;
				strFileName += strBinExt; // добавляем стандартное расширение для бин файлов.
			}

			CFile file;

			// Save file array if no errors
			if (!bError && (file.Open(strFileName.c_str(), CFile::modeCreate | CFile::modeWrite)))
			{
				// Записываем заголовок бин файла
				file.Write(&fileAddr, sizeof(fileAddr)); // слово адреса
				file.Write(&fileSize, sizeof(fileSize)); // слово длины

				if (g_Config.m_bUseLongBinFormat)
				{
					file.Write(bkName, BK_NAMELENGTH); // имя файла
				}

				DWORD cs = 0; // подсчитаем контрольную сумму

				for (int i = 0; i < fileSize; ++i)
				{
					uint8_t val = GetByte(fileAddr++);
					file.Write(&val, sizeof(val));
					cs += uint16_t(val);

					if (cs & 0xffff0000)
					{
						cs++;
						cs &= 0xffff;
					}
				}

				if (g_Config.m_bUseLongBinFormat)
				{
					file.Write(&cs, sizeof(uint16_t)); // контрольная сумма
				}

				file.Close();
				SetWord(BK_BMB10_CRCADDR, LOWORD(cs)); // сохраним контрольную сумму на своё место
			}
		}

		SetRON(CCPU::REGISTER::PC, 0116402); // выходим туда.
		// Refresh keyboard
		m_pParent->SendMessage(WM_RESET_KBD_MANAGER, 0); // и почистим индикацию управляющих клавиш в статусбаре
		return true; // сэмулировали
	}
***/
	return false; // не эмулируем
}


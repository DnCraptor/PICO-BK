// CPU.cpp: implementation of the CCPU class.
// можно переделать под правильную реализацию логики


#include "pch.h"
#include "BKMessageBox.h"
#include "Board.h"
#include "CPU.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// CPU timing в тактах
constexpr auto TIMING_IDX_BASE      = 0 ;
constexpr auto TIMING_IDX_HALT      = 1 ;
constexpr auto TIMING_IDX_WAIT      = 2 ;
constexpr auto TIMING_IDX_RTI       = 3 ;
constexpr auto TIMING_IDX_RESET     = 4 ;
constexpr auto TIMING_IDX_BR_BASE   = 5 ;
constexpr auto TIMING_IDX_RTS       = 6 ;
constexpr auto TIMING_IDX_MARK      = 7 ;
constexpr auto TIMING_IDX_EMT       = 8 ;
constexpr auto TIMING_IDX_IOT       = 9 ;
constexpr auto TIMING_IDX_SOB       = 10;
constexpr auto TIMING_IDX_INT       = 11;
constexpr auto TIMING_IDX_BUS_ERROR = 12;


// Это для БК10
// однооперандные расчётные
//const int CCPU::timing_Misk_10[13] = { 12, 68, 12, 46, 1140, 16, 32, 56, 64, 104, 24, 64, 124 }; - это на мой взгляд более верные тайминги, но людям не понравилось
const int CCPU::timing_Misk_10[13] = { 12, 68, 12, 40, 1140, 16, 32, 56, 64, 104, 24, 40, 64 };
const int CCPU::timing_OneOps_TST_10[8]    = {12,  24, 24, 36, 28, 40, 36, 48};
const int CCPU::timing_OneOps_CLR_10[8]    = {12,  28, 28, 40, 32, 44, 40, 52};
const int CCPU::timing_OneOps_MTPS_10[8]   = {24,  32, 32, 44, 36, 48, 44, 56};
const int CCPU::timing_OneOps_XOR_10[8]    = {12,  32, 32, 44, 32, 44, 44, 56};
const int CCPU::timing_OneOps_JMP_10[8]    = { 8,  20, 24, 32, 24, 32, 32, 44};
const int CCPU::timing_OneOps_JSR_10[8]    = { 8,  32, 36, 40, 36, 40, 40, 52};
// двухоперандные расчётные
const int CCPU::timing_TwoOps_MOV_10[8][8] =
{
	12, 32, 36, 44, 36, 44, 44, 56,
	28, 40, 40, 48, 40, 48, 48, 60,
	28, 40, 40, 48, 40, 48, 48, 60,
	40, 52, 52, 60, 52, 60, 60, 72,
	28, 44, 44, 52, 44, 52, 52, 64,
	40, 56, 56, 64, 56, 64, 64, 76,
	40, 52, 52, 60, 52, 60, 60, 72,
	52, 64, 64, 72, 64, 72, 72, 84
};
const int CCPU::timing_TwoOps_CMP_10[8][8] =
{
	12, 32, 32, 44, 32, 44, 44, 56,
	28, 36, 36, 48, 36, 48, 48, 60,
	28, 36, 36, 48, 36, 48, 48, 60,
	40, 48, 48, 60, 48, 60, 60, 72,
	28, 40, 40, 52, 40, 52, 52, 64,
	40, 52, 52, 64, 52, 64, 64, 76,
	40, 48, 48, 60, 48, 60, 60, 72,
	52, 60, 60, 72, 60, 72, 72, 84
};
const int CCPU::timing_TwoOps_BIS_10[8][8] =
{
	12, 36, 36, 48, 36, 48, 48, 60,
	28, 40, 40, 52, 40, 52, 52, 64,
	28, 40, 40, 52, 40, 52, 52, 64,
	40, 52, 52, 64, 52, 64, 64, 76,
	28, 44, 44, 56, 44, 56, 56, 68,
	40, 56, 56, 68, 56, 68, 68, 80,
	40, 52, 52, 64, 52, 64, 64, 76,
	52, 64, 64, 76, 64, 76, 76, 88
};

// Это для БК11 4Мгц
//const int CCPU::timing_Misk_11[13] = { 16, 96, 16, 56, 1140, 16, 32, 56, 80, 128, 28, 72, 144 }; - это на мой взгляд более верные тайминги, но людям не понравилось
const int CCPU::timing_Misk_11[13] = { 16, 96, 16, 48, 1140, 16, 32, 56, 80, 128, 21, 48, 96 };
// однооперандные расчётные
const int CCPU::timing_OneOps_TST_11[8]    = { 16, 32, 32, 40, 32, 40, 40, 48 };
const int CCPU::timing_OneOps_CLR_11[8]    = { 16, 37, 37, 48, 37, 48, 48, 58 };
const int CCPU::timing_OneOps_MTPS_11[8]   = { 26, 37, 37, 48, 37, 48, 48, 58 };
const int CCPU::timing_OneOps_XOR_11[8]    = { 16, 37, 37, 48, 40, 48, 48, 58 };
const int CCPU::timing_OneOps_JMP_11[8]    = {  8, 24, 26, 37, 26, 37, 37, 48 };
const int CCPU::timing_OneOps_JSR_11[8]    = {  8, 32, 37, 48, 37, 48, 48, 58 };
// двухоперандные расчётные
const int CCPU::timing_TwoOps_MOV_11[8][8] =
{
	16, 32, 40, 48, 40, 48, 48, 64,
	32, 48, 48, 64, 48, 64, 64, 72,
	32, 48, 48, 64, 48, 64, 64, 72,
	40, 56, 64, 64, 64, 72, 64, 80,
	32, 48, 48, 64, 48, 64, 64, 72,
	40, 56, 64, 64, 64, 72, 64, 80,
	40, 56, 64, 64, 64, 72, 64, 80,
	48, 64, 72, 80, 72, 80, 80, 96
};
const int CCPU::timing_TwoOps_CMP_11[8][8] =
{
	16, 32, 32, 48, 32, 48, 48, 56,
	32, 48, 48, 56, 48, 56, 56, 64,
	32, 48, 48, 56, 48, 56, 56, 64,
	40, 56, 56, 64, 56, 64, 64, 80,
	32, 48, 48, 56, 48, 56, 56, 64,
	40, 56, 56, 64, 56, 64, 64, 80,
	40, 56, 56, 64, 56, 64, 64, 80,
	48, 64, 64, 80, 64, 80, 80, 80
};
const int CCPU::timing_TwoOps_BIS_11[8][8] =
{
	16, 42, 42, 53, 42, 53, 53, 64,
	32, 48, 48, 64, 53, 64, 64, 72,
	32, 48, 48, 64, 53, 64, 64, 72,
	40, 64, 64, 72, 64, 74, 72, 80,
	32, 48, 48, 64, 53, 64, 64, 72,
	40, 64, 64, 72, 64, 74, 72, 80,
	40, 64, 64, 72, 64, 74, 72, 80,
	48, 72, 72, 80, 74, 85, 80, 96
};


/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CCPU::CCPU()
	: m_pBoard(nullptr)
	, m_pExecuteMethodMap({ 0 })
	, timing_Misk(nullptr)
	, timing_OneOps_TST(nullptr)
	, timing_OneOps_CLR(nullptr)
	, timing_OneOps_MTPS(nullptr)
	, timing_OneOps_XOR(nullptr)
	, timing_OneOps_JMP(nullptr)
	, timing_OneOps_JSR(nullptr)
	, timing_TwoOps_MOV(nullptr)
	, timing_TwoOps_CMP(nullptr)
	, timing_TwoOps_BIS(nullptr)
	, m_bCBug(false)
	, m_b177702State(true)
	// инициализируем значения регистров по чтению
	, m_vSysRegs({ 0177740, 0177777, 0177440, 0000000, 0177777, 0177400 })
	// инициализируем маски регистров по записи. В нулевые биты ничего записать нельзя, в единичные - можно.
	, m_vSysRegsMask( {   7,       0,    0377, 0177777,       0,    0377 })
	, m_PSW(0340)
{
	memset(m_RON, 0, sizeof(m_RON));
	InitVars();
	PrepareCPU();
}

CCPU::~CCPU()
{
	DoneCPU();
}

// здесь делается переключение таймингов для БК10/БК11
void CCPU::AttachBoard(CMotherBoard *pBoard)
{
	m_pBoard = pBoard;

	switch (m_pBoard->GetBoardModel())
	{
		default:
		case BK_DEV_MPI::BK0010:
			timing_Misk        = timing_Misk_10;
			timing_OneOps_TST  = timing_OneOps_TST_10;
			timing_OneOps_CLR  = timing_OneOps_CLR_10;
			timing_OneOps_MTPS = timing_OneOps_MTPS_10;
			timing_OneOps_XOR  = timing_OneOps_XOR_10;
			timing_OneOps_JMP  = timing_OneOps_JMP_10;
			timing_OneOps_JSR  = timing_OneOps_JSR_10;
			timing_TwoOps_MOV  = &timing_TwoOps_MOV_10[0][0];
			timing_TwoOps_CMP  = &timing_TwoOps_CMP_10[0][0];
			timing_TwoOps_BIS  = &timing_TwoOps_BIS_10[0][0];
			break;

		case BK_DEV_MPI::BK0011:
		case BK_DEV_MPI::BK0011M:
			timing_Misk        = timing_Misk_11;
			timing_OneOps_TST  = timing_OneOps_TST_11;
			timing_OneOps_CLR  = timing_OneOps_CLR_11;
			timing_OneOps_MTPS = timing_OneOps_MTPS_11;
			timing_OneOps_XOR  = timing_OneOps_XOR_11;
			timing_OneOps_JMP  = timing_OneOps_JMP_11;
			timing_OneOps_JSR  = timing_OneOps_JSR_11;
			timing_TwoOps_MOV  = &timing_TwoOps_MOV_11[0][0];
			timing_TwoOps_CMP  = &timing_TwoOps_CMP_11[0][0];
			timing_TwoOps_BIS  = &timing_TwoOps_BIS_11[0][0];
			break;
	}
}
void CCPU::InitCPU()
{
	InitVars();
	ResetCPU();
	m_PSW = 0340;
	m_RON[static_cast<int>(REGISTER::PC)] = GetWord(0177716) & 0177400; // тут спецом читаем ячейку памяти, чтобы можно было использовать особенности старт режима A16M
}

void CCPU::ResetCPU()
{
	m_vSysRegs[static_cast<int>(PORTS::P_177700)] = 0177740;
	m_vSysRegs[static_cast<int>(PORTS::P_177702)] = 0177777;
	m_vSysRegs[static_cast<int>(PORTS::P_177704)] = 0177440;
	m_b177702State = true;
	m_bCBug = false;
	m_pBoard->ResetDevices();
}

void CCPU::InitVars()
{
	ResetTimer();
	m_nInternalTick = 0;
	m_bWaitMode = false;
	m_bStepMode = false;
	m_bTrace_RTT = false;
	m_bIRQ1rq = false;
	m_bIRQ1LL = false;
	m_bGetVector = false;
	m_bTwiceHangup = false;
	m_bIRQ1flg = false;
	m_bRPLYrq = false;
	m_bACLOrq = m_bDCLOrq = false;
	m_bRPL2rq = false;
	m_bVIRQErrrq = false;
	m_bIRQ2rq = false;
	m_bIRQ3rq = false;
	/////m_qVIRQ.clear();
}

void CCPU::ResetTimer()
{
	m_nCmdTicks = 0;
	// инициализируем ВЕ таймер, специально через функцию, чтобы внутренние переменные инициализировать.
	// регистр начального значения таймера при ресете не изменяется
	SetSysRegs(0177712, uint16_t(0)); // Управление
}


/*
таймер

177706 -- Регистр начального значения таймера. Доступен по чтению и записи.
177710 -- Реверсивный счётчик. Доступен по чтению, запись в регистр игнорируется.
177712 -- Программируемый таймер-- регистр управления.
любая запись в этот регистр вызывает перезапись в регистр счётчика константы из регистра начального значения таймера
(001)бит 0: STOP: "1" - остановка
При установке запрещает счёт
(002)бит 1: WRAPAROUND: "1" - Режим непрерывного счёта, отменяет действие битов EXPENABLE и ONESHOT
Запрещает фиксацию перехода счётчика через 0. Досчитав до нуля, таймер продолжает вычитание.
(004)бит 2: EXPENABLE: "1" - разрешение установки сигнала EXPIRY ("конец счета")
Установка включает индикацию, при очередном переходе таймера через 0 устанавливается в 1 бит EXPIRY, если ранее он был сброшен.
Нужно учитывать, что при первом (после включения ЭВМ или системного сброса) запуске таймера в данном режиме индикация
срабатывает только после ВТОРОГО перехода счётчика через 0, причём независимо от того, работал ли таймер до этого в
других режимах
(010)бит 3: ONESHOT: режим одновибратора
При установке запрещает повторный счёт, после первого досчёта до 0 сбрасывается бит RUN. Установка данного режима не отменяет
режим индикации (бит EXPENABLE)
(020)бит 4: RUN: запуск счётчика, запись "1"-- загружает счётчик из регистра 177706 и начинает отсчёт
В этом режиме при досчёте до нуля в счётчик заново заносится константа из регистра начального значения, следовательно
счёт всегда ведётся от константы до нуля (если только не запрещена фиксация перехода через 0 битом WRAPAROUND). При сбросе
в 0 в счётчик переписывается начальное значение из регистра начального значения
(040)бит 5: делитель на 16, "1"-- включён
Установка снижает скорость счёта в 16 раз (режим умножения времени на 16)
при одновременной установке скорость снижается, соответственно в 64 раза
(100)бит 6: делитель на 4, "1"-- включён
Установка снижает скорость счёта в 4 раза (режим умножения времени на 4)
(200)бит 7: EXPIRY: флаг окончания счета, устанавливается в "1" при достижении счётчиком нуля, сбрасывается только программно
Для установки бита должен быть установлен бит 2.
биты 8-15 не используются, "1".
*/
constexpr uint16_t TVE_SP  = 01  ;  // csr[0]
constexpr uint16_t TVE_CAP = 02  ;  // csr[1]
constexpr uint16_t TVE_MON = 04  ;  // csr[2]
constexpr uint16_t TVE_OS  = 010 ;  // csr[3]
constexpr uint16_t TVE_RUN = 020 ;  // csr[4]
constexpr uint16_t TVE_D16 = 040 ;  // csr[5]
constexpr uint16_t TVE_D4  = 0100;  // csr[6]
constexpr uint16_t TVE_FL  = 0200;  // csr[7]

void CCPU::Timerprocess()
{
	uint16_t &reg_177712 = m_vSysRegs[static_cast<int>(PORTS::P_177712)]; // Управление

	// если счётчик остановлен
	if (reg_177712 & TVE_SP)
	{
		return; // выход, счётчик не работает.
		// т.е. работает, но тактируется от внешнего входа, а не от частоты проца
	}

	// если счётчик запущен
	if (reg_177712 & TVE_RUN)
	{
		// обрабатываем
		if (++m_nTVE_Cnt >= m_nTVE_Divider)
		{
			m_nTVE_Cnt = 0;
			uint16_t &reg_177710 = m_vSysRegs[static_cast<int>(PORTS::P_177710)]; // Счётчик

			// уменьшаем текущее значение таймера на 1
			if (--reg_177710 == 0)
			{
				if (reg_177712 & TVE_CAP)    // если режим WRAPAROUND
				{
					// непрерывный счёт без перезагрузки счётчика
				}
				else
				{
					// если не режим WRAPAROUND, то значение счётчика перезагружаем
					reg_177710 = m_vSysRegs[static_cast<int>(PORTS::P_177706)]; // загрузим в счётчик начальное значение

					if (reg_177712 & TVE_MON)   // разрешение установки сигнала "конец счёта" ?
					{
						if (m_bTimerRq) // если запрос на прерывание ещё не обработан
						{
							reg_177712 |= TVE_FL;   // то, установим сигнал
						}
						else
						{
							m_bTimerRq = true; // иначе - устанавливаем запрос на прерывание
						}
					}

					if (reg_177712 & TVE_OS)    // установлен режим одновибратора?
					{
						reg_177712 &= ~TVE_RUN; // тогда сбросим бит 4
					}
				}
			}
		}
	}
}

uint8_t CCPU::GetByte(const uint16_t addr)
{
	if (0177700 <= addr && addr < 0177714)
	{
		uint16_t w;

		if (GetSysRegs(addr, w))
		{
			m_nROMTimingCorrection += REG_TIMING_CORR_VALUE;
			return ((addr & 1) ? (w >> 8) : w) & 0377;
		}
	}

	int nTC = 0;
	uint8_t b = m_pBoard->GetByteT(addr, nTC);
	m_nROMTimingCorrection += nTC;
	return b;
}

uint16_t CCPU::GetWord(const uint16_t addr)
{
	if (0177700 <= addr && addr < 0177714)
	{
		uint16_t w;

		if (GetSysRegs(addr, w))
		{
			m_nROMTimingCorrection += REG_TIMING_CORR_VALUE;
			return w;
		}
	}

	int nTC = 0;
	uint16_t w = m_pBoard->GetWordT(addr, nTC);
	m_nROMTimingCorrection += nTC;
	return w;
}

void CCPU::SetByte(const uint16_t addr, uint8_t value)
{
	if ((0177700 <= addr && addr < 0177714) && !(addr == 0177702 && !m_b177702State))
	{
		m_nROMTimingCorrection += REG_TIMING_CORR_VALUE;
		SetSysRegs(addr, value);
	}
	else
	{
		int nTC = 0;
		m_pBoard->SetByteT(addr, value, nTC);
		m_nROMTimingCorrection += nTC;
	}
}

void CCPU::SetWord(const uint16_t addr, uint16_t value)
{
	if ((0177700 <= addr && addr < 0177714) && !(addr == 0177702 && !m_b177702State))
	{
		m_nROMTimingCorrection += REG_TIMING_CORR_VALUE;
		SetSysRegs(addr, value);
	}
	else
	{
		int nTC = 0;
		m_pBoard->SetWordT(addr, value, nTC);
		m_nROMTimingCorrection += nTC;
	}
}

int CCPU::TranslateInstruction()
{
	m_nInternalTick = 0;
	m_nROMTimingCorrection = 0; // здесь накапливается коррекция тайминга за время выполнения инструкции

	// диспетчер прерываний. проверим, есть ли незамаскированные запросы на прерывания
	// если есть -  выполняем прерывание, инструкцию не выполняем
	// если нет - выполняем очередную инструкцию
	if (!InterruptDispatch())
	{
		// если была команда WAIT, не надо выполнять инструкцию, но чтобы не зацикливать эмулятор,
		// надо делать вид, что мы что-то делаем.
		if (m_bWaitMode)
		{
			m_nInternalTick = timing_Misk[TIMING_IDX_BASE];
		}
		else
		{
			m_instruction = GetWord(m_RON[static_cast<int>(REGISTER::PC)]); // берём следующую инструкцию.
			m_RON[static_cast<int>(REGISTER::PC)] += 2;
			m_datarg = m_ALU = 0;
			m_bByteOperation = !!(m_instruction & 0100000);
			m_Nbit = (m_bByteOperation ? 0200 : 0100000);
			int t = m_instruction;
			m_nRegDst = static_cast<REGISTER>(t & 7);
			t >>= 3;
			m_nMethDst = t & 7;
			t >>= 3;
			m_nRegSrc = static_cast<REGISTER>(t & 7);
			t >>= 3;
			m_nMethSrc = t & 7;
			m_Freg = m_PSW;

			if (m_bCBug)
			{
				m_bCBug = false;

				if (g_Config.m_bEmulateCBug)
				{
					m_Freg &= 0177776;
				}
			}

			// Find command implementation using the command map
			(this->*m_pExecuteMethodMap[m_instruction])();  // Call command implementation method
		}
	}

	m_nInternalTick -= m_nROMTimingCorrection;
	// здесь в принципе тоже неточность. таймер должен считать независимо, а не в промежутках между
	// выполнением команд, при чтении значений таймера тут будет возникать неточность.
	m_nCmdTicks += m_nInternalTick;

	while (m_nCmdTicks >= 128)
	{
		m_nCmdTicks -= 128;
		Timerprocess(); // Обрабатываем встроенный таймер каждый 128й такт
	}

	return m_nInternalTick;
}


void CCPU::ReplyError()
{
	if (m_bGetVector)
	{
		m_bGetVector = false;
		m_bVIRQErrrq = true;
	}
	else if (m_bTwiceHangup)
	{
		m_bTwiceHangup = false;
		m_bRPL2rq = true;
	}
	else
	{
		m_bRPLYrq = true;
	}
}

// диспетчер прерываний. проверим, есть ли незамаскированные запросы на прерывания
// если есть, выполним прерывание и выход true
// иначе выход false, и можно выполнять инструкцию.
bool CCPU::InterruptDispatch()
{
	uint32_t nVector = 0;  // младшее слово - адрес вектора
	// старшее слово - признаки:
	// бит 0 - векторное прерывание (чтобы 0 тоже можно было обрабатывать)
	// бит 1 - прерывание halt mode
	// бит 2 - PC надо уменьшить на 2 (только в halt mode)

	if (m_bVIRQErrrq)  // Ошибка передачи вектора.
	{
		m_bVIRQErrrq = false;
		m_bGetVector = false; // вектор больше не получаем, но ожидаем вероятность двойного зависания
		m_nInternalTick = timing_Misk[TIMING_IDX_BUS_ERROR];
		nVector = 0160012 | (1 << 17); // прерывание в пультовом режиме
	}
	else if (m_bRPL2rq) // Двойное зависание, priority 1
	{
		m_bRPL2rq = false;
		m_nInternalTick = timing_Misk[TIMING_IDX_BUS_ERROR];

		if (GetPSWBit(PSW_BIT::MASKI)) // Двойное зависание в HALT
		{
			// если этого не сделать, то мы насмерть зациклимся в HALT режиме
			// при отсутствии доступа к ячейкам 0177676 и 0177674, а так -
			// при любом выходе в HALT режим и невозможности в нём работать
			// вываливаемся обратно в USER с прерыванием по вектору 4
			nVector = INTERRUPT_4;
		}
		else // Двойное зависание в USER
		{
			nVector = 0160006 | (1 << 17); // прерывание в пультовом режиме
		}
	}
	else if (m_bRPLYrq) // тайм аут шины МПИ
	{
		m_bRPLYrq = false;
		m_bTwiceHangup = true;  // нужно отлавливать двойное зависание
		m_nInternalTick = timing_Misk[TIMING_IDX_BUS_ERROR];

		if (GetPSWBit(PSW_BIT::HALT)) // Зависание в HALT, priority 1
		{
			// !!! тут должно сохраняться PC-2 для ВМ1А и РС для ВМ1Г
			if (g_Config.m_bVM1G)
			{
				nVector = 0160002 | (1 << 17) | (1 << 18);  // прерывание в пультовом режиме
			}
			else
			{
				nVector = 0160002 | (1 << 17);  // прерывание в пультовом режиме, РС-2
			}
		}
		else
		{
			if (GetPSWBit(PSW_BIT::MASKI))   // Зависание в HALT, priority 1
			{
				nVector = 0160002 | (1 << 17) | (1 << 18); // прерывание в пультовом режиме
			}
			else    // Зависание в USER, priority 1
			{
				nVector = INTERRUPT_4; // прерывание в обычном режиме

				if (m_bIRQ1flg)
				{
					nVector |= (1 << 18); // вот это нужно, чтобы sub #2,(SP) rti - блокировка кнопки стоп работала
					m_bIRQ1flg = false;
				}
			}
		}
	}
	else if (GetPSWBit(PSW_BIT::T) && !m_bWaitMode)  // T-bit, priority 3
	{
		if (m_bTrace_RTT)
		{
			m_bTrace_RTT = false;
			// надо, чтобы следующая инструкция выполнилась без прерывания по T биту
		}
		else if (m_bStepMode)
		{
			m_bStepMode = false;
			// надо, чтобы следующая инструкция выполнилась без прерывания по T биту
		}
		else
		{
			nVector = INTERRUPT_14;
			m_nInternalTick = timing_Misk[TIMING_IDX_INT];
		}
	}
	else if (!GetPSWBit(PSW_BIT::MASKI)) // если прерывания не замаскированы
	{
		if (m_bACLOrq)  // ACLO, priority 4
		{
			m_bACLOrq = false;
			m_nInternalTick = timing_Misk[TIMING_IDX_INT];
			nVector = INTERRUPT_24;
		}
		else
		{
			if (m_bIRQ1rq && !GetPSWBit(PSW_BIT::HALT))
			{
				// обработка прерывания по нажатию кнопки СТОП, бит 11 в PSW маскирует это прерывание
				// Прер. irq1 и halt - почти одно и то же и должны обрабатываться одинаково, но IRQ1 можно замаскировать, а HALT - нет
				// одно хреново, так - не получится сэмулировать прерывание кнопкой СТОП выполнения на середине команды
				m_bIRQ1rq = false;
				m_bIRQ1flg = true; // обозначаем, что вызываем IRQ1 кнопкой СТОП
				m_nInternalTick = timing_Misk[TIMING_IDX_INT];
				nVector = 0160002 | (1 << 17); // прерывание в пультовом режиме, РС-2, эта функция выполняется перед чтением инструкции, поэтому PC указывает на ещё не выполненную инструкцию.
			}
			else if (!GetPSWBit(PSW_BIT::P)) // если бит приоритета не установлен
			{
				if (m_bIRQ2rq)  // прерывание от таймера, priority 6
				{
					m_bIRQ2rq = false;
					m_nInternalTick = timing_Misk[TIMING_IDX_INT];
					nVector = INTERRUPT_100;
				}
				else if (m_bIRQ3rq)  // прерывание от внешнего устройства, priority 6
				{
					m_bIRQ3rq = false;
					m_nInternalTick = timing_Misk[TIMING_IDX_INT];
					nVector = INTERRUPT_270;
				}
				else if (m_bTimerRq && g_Config.m_bVM1G) // прерывание от события ВЕ-таймера, только для ВМ1Г
				{
					m_bTimerRq = false;
					m_nInternalTick = timing_Misk[TIMING_IDX_INT];
					nVector = INTERRUPT_270;
				}
	/////			else if (!m_qVIRQ.empty())  // VIRQ, priority 7
				{
					// если в очереди есть что-то, достанем и обработаем
	/////				m_bGetVector = true;
	/////				nVector = m_qVIRQ.front() | (1 << 16);
	/////				m_qVIRQ.pop_front();
	/////				m_nInternalTick = timing_Misk[TIMING_IDX_INT];
				}
			}
		}
	}

	if (nVector)    // если запрос на прерывание есть
	{
		if (m_bWaitMode) // если мы были в режиме ожидания в команде wait
		{
			m_bWaitMode = false; // отменяем
			m_RON[static_cast<int>(REGISTER::PC)] += 2; // возвращаем РС как должно быть после wait.
			// игра с РС сделана потому, что возникли проблемы с пошаговой трассировкой
			// в дизассемблере
		}

		if (nVector & (1 << 17)) // выполняем прерывание
		{
//          TRACE2("System Trap to: %06o in addr: %06o\n", nVector & 0xffff, m_RON[static_cast<int>(REGISTER::PC)]);
			SystemInterrupt(nVector);
		}
		else
		{
//          TRACE2("User Trap to: %06o in addr: %06o\n", nVector & 0xffff, m_RON[static_cast<int>(REGISTER::PC)]);
			UserInterrupt(nVector);
		}

		return true; // очередную инструкцию не выполняем
	}

	return false;   // позволим выполнить очередную инструкцию
}


// выполнение пультового исключения и прерывания
//  для вектора установленный бит 18 означает, что PC надо увеличить на 2,
// как будто  была прочитана текущая инструкция
void CCPU::SystemInterrupt(uint32_t nVector)
{
	//uint16_t w;
	//m_pBoard->GetSystemRegister(0177716, &w, GSSR_NONE);
	m_pBoard->SetSystemRegister(0177716, 0210, GSSR_NONE); // выставляем признак HALT режима
	// при этом в регистр записи попадают данные регистра чтения и происходит некоторая фигня
	// поэтому будем делать проще, ничего читать не будем, но сразу запишем нужное число - бит 4 и биты звука - нули
	m_pBoard->SetWord(0177676, GetPSW()); // вот это-то и вызывает прерывание по вектору 4
	uint16_t pc = m_RON[static_cast<int>(REGISTER::PC)];

	if (nVector & (1 << 18))
	{
		pc += 2;
	}

	m_pBoard->SetWord(0177674, pc);
	uint16_t nVec = nVector & 0xffff;
	m_RON[static_cast<int>(REGISTER::PC)] = GetWord(nVec);
	// SetPSW((GetWord(nVec + 2) & 07777) | (1 << static_cast<int>(PSW_BIT::HALT))); // вот как я думаю должно быть
	SetPSW(GetWord(nVec + 2) & 0377); // вот как происходит на самом деле
	m_bTwiceHangup = false;
}

// выполнение пользовательского исключения и прерывания
void CCPU::UserInterrupt(uint32_t nVector)
{
	// сохраняем в стеке PC/PSW
	uint16_t &sp = m_RON[static_cast<int>(REGISTER::SP)];
	sp -= 2;
	SetWord(sp, GetPSW()); // теперь, если тут случится m_bRPLYrq, то по m_bTwiceHangup можно узнать что это двойное зависание
	sp -= 2;
	uint16_t pc = m_RON[static_cast<int>(REGISTER::PC)];

	if (nVector & (1 << 18))
	{
		pc += 2;
	}

	SetWord(sp, pc); // или тут
	m_bTwiceHangup = false;
	uint16_t nVec = nVector & 0xffff;
	m_RON[static_cast<int>(REGISTER::PC)] = GetWord(nVec); // теперь, если тут случится m_bRPLYrq, то по m_bGetVector можно узнать что это ошибка передачи вектора
	SetPSW(GetWord(nVector + 2) & 0377);
	m_bGetVector = false; // выставляется в false после операций с получением вектора
}


/*
Если приоритет текущей задачи ниже приоритета прерывания, то ЦП прервёт работу и обработает прерывание
иначе, ЦП сперва закончит вычисления, и только потом обработает прерывание

Таблица прерываний и исключений, значения векторов, стека сохранения PSW и PC и зависимость
от маски запрета, расположены в порядке приоритета

WCPU - признак выполнения команды WAIT

| Описание                                                   |Вектор    |PSW/PC    |Маска     | Вид
|                                                            |          |          |          |
-------------------------------------------------------------|----------|----------|----------|-------
| Начальный пуск после аппаратного сброса (читается только   |          |          |          |
| стартовый адрес в старшем байте регистра, PSW всегда       |177716    |          |          |
| устанавливается в 340)                                     |          |          |          |
-------------------------------------------------------------|----------|----------|----------|-------
| Зависание шины при приёме вектора прерываний из системной  |160012    |177674    |          | Внутр.
| магистрали (177676)<-(PSW), (177674)<-(PC),                |          |          |          |
| (PC)<-(160012), (PSW)<-(160014)                            |          |          |          |
-------------------------------------------------------------|----------|----------|----------|-------
| Двойное зависание шины (повторное при попытке обработать   |160006    |177674    |          | Внутр.
| первое исключение по зависанию) (177676)<-(PSW),           |          |          |          |
| (177674)<-(PC), (PC)<-(160006), (PSW)<-(160010)            |          |          |          |
-------------------------------------------------------------|----------|----------|----------|-------
| Зависание при передаче данных по системной магистрали      |          |          |          | Внутр.
| обработка зависит от битов 10 и 11 в PSW:                  |          |          |          |
| если PSW11=0, PSW10=0                                      |000004    | SP-4     | PSW11    |
| если хотя бы один бит PSW10 и PSW11 ненулевой              |160002    |177674    | PSW10    |
-------------------------------------------------------------|----------|----------|----------|-------
| Резервный код                                              |000010    | SP-4     |          | Внутр.
-------------------------------------------------------------|----------|----------|----------|-------
| Т-бит                                                      |000014    | SP-4     | WCPU     | Внутр.
| бит 4 в слове состояния процессора PSW. Если данный бит
| имеет единичное значение то в обычном режиме вызывает
| исключение по вектору 000014. В режиме ожидания по команде
| WAIT T-бит не вызывает немедленного исключения, процессор
| ожидает прерывание, любое незамаскированное прерывание ACLO,
| IRQ1, TVE, IRQ2, IRQ3, VIRQ вызывает выход из этого режима и
| переход по вектору, соответствующему активному прерыванию
-------------------------------------------------------------|----------|----------|----------|-------
| Сбой питания (прерывание ACLO)                             |000024    | SP-4     | PSW10    | Внеш.
-------------------------------------------------------------|----------|----------|----------|-------
| Нажатие на клавишу СТОП - (прерывание IRQ1)                |160002    |177674    | PSW10    | Внеш.
| (177716)<-000010!(177716), (177676)<-(PSW), (177674)<-(PC),|          |          | PSW11    |
| (PC)<-(160002), (PSW)<-(160004)                            |          |          |          |
| при этом происходит прерывание по вектору 4                |          |          |          |
| - запрос возникает при обнаружении низкого уровня на входе
| nIRQ1. Если уровень не будет снят после обработки запроса, то
| возникнет новый запрос, так как вход чувствителен к уровню,
| а не к фронту или срезу. Может быть замаскировано единичными
| битами PSW10 или PSW11 (запрещается, если хотя бы один бит
| установлен)

При возникновении прерывания по IRQ1 или команде HALT Написано следующее:
Микропрограммная обработка этого прерывания приводит к обращению по записи в РНП (177716),
что приводит к установке бита 2 БКшной схемой.

-------------------------------------------------------------|----------|----------|----------|-------
| Прерывание от ВЕ-таймера, есть только в К1801ВМ1Г , может  |0270      | SP-4     | PSW10    | Внеш.
| быть замаскировано обнулением бита MON в регистре управления                       PSW7
| таймером. Также прерывание может быть запрещено битами PSW7                        MON
| и PSW10 (запрещается, если хотя бы один бит установлен)
-------------------------------------------------------------|----------|----------|----------|-------
| Прерывание от внешнего устройства (IRQ2)                   |0100      | SP-4     | PSW10    | Внеш.
                                                                                     PSW7
-------------------------------------------------------------|----------|----------|----------|-------
| Прерывание от внешнего устройства (IRQ3)                   |0270      | SP-4     | PSW10    | Внеш.
                                                                                     PSW7
-------------------------------------------------------------|----------|----------|----------|-------
| Прерывание от вычислительного устройства (VIRQ)            |считывается|SP-4     | PSW10    | Внеш.
| (прерывание от клавиатуры 60 и 274 организованы            |из сист.  |          | PSW7     |
| через него и для других устройств тоже)                    |магистрали|          |          |
-------------------------------------------------------------|----------|----------|----------|-------
| прерывание по команде HALT                                 |0160002   | 177674   |          | Внутр.
-------------------------------------------------------------|----------|----------|----------|-------
| прерывание по команде BPT                                  |0014      | SP-4     |          | Внутр.
-------------------------------------------------------------|----------|----------|----------|-------
| прерывание по команде IOT                                  |0020      | SP-4     |          | Внутр.
-------------------------------------------------------------|----------|----------|----------|-------
| прерывание по команде EMT                                  |0030      | SP-4     |          | Внутр.
-------------------------------------------------------------|----------|----------|----------|-------
| прерывание по команде TRAP                                 |0034      | SP-4     |          | Внутр.

*/

void CCPU::ExecuteHALT()
{
	m_nInternalTick = timing_Misk[TIMING_IDX_HALT];
	SystemInterrupt(0160002);
}


void CCPU::ExecuteUNKNOWN()
{
	m_nInternalTick = timing_Misk[TIMING_IDX_INT];
	UserInterrupt(INTERRUPT_10);
}


void CCPU::ExecuteBPT()
{
	m_nInternalTick = timing_Misk[TIMING_IDX_EMT];
	UserInterrupt(INTERRUPT_14);
}


void CCPU::ExecuteIOT()
{
	m_nInternalTick = timing_Misk[TIMING_IDX_IOT];
	UserInterrupt(INTERRUPT_20);
}


void CCPU::ExecuteEMT()
{
	m_b177702State = true;
	m_nInternalTick = timing_Misk[TIMING_IDX_EMT];
	UserInterrupt(INTERRUPT_30);
}


void CCPU::ExecuteTRAP()
{
	m_nInternalTick = timing_Misk[TIMING_IDX_EMT];
	UserInterrupt(INTERRUPT_34);
}


void CCPU::InterruptVIRQ(uint16_t interrupt)
{
	/////for (auto &n : m_qVIRQ) // проверим, что там в очереди
	{
	/////	if (n == interrupt) // если такой вектор уже в очереди
		{
			return; // его не будем добавлять
		}
	}

	TRACE0("VIRQ irq\n");
	/////m_qVIRQ.push_back(interrupt); // помещаем адрес вектора прерываний в очередь
	// !!! при этом, если будем очень быстро помещать адреса в очередь, она
	// начнёт переполняться, потому что тут не так надо делать. надо игнорировать
	// все запросы на прерывание, пока не будет обработан текущий.
	// или не подавать так много запросов.
}

bool CCPU::GetSysRegs(uint16_t addr, uint16_t &val)
{
	// если мы что-то писали в регистр 177702, то он пропадает с шины и не выдаётся RPLY
	if (/*(0177700 <= addr && addr < 0177714) &&*/ !(addr == 0177702 && !m_b177702State))
	{
		val = m_vSysRegs[(addr >> 1) & 7];
		return true;
	}

	return false;
}

uint16_t CCPU::GetSysRegsIndirect(uint16_t addr)
{
	if (0177700 <= addr && addr < 0177714)
	{
		return m_vSysRegs[(addr >> 1) & 7];
	}

	return 0;
}

void CCPU::SetSysRegs(uint16_t addr, uint16_t value)
{
	if (0177700 <= addr && addr < 0177714)
	{
		int n = (addr >> 1) & 7;
		m_vSysRegs[n] = (m_vSysRegs[n] & ~m_vSysRegsMask[n]) | (value & m_vSysRegsMask[n]);

		// тут ещё и разные действия могут происходить, например
		switch (n)
		{
			case static_cast<int>(PORTS::P_177702):
				if (g_Config.m_bEmulate177702)
				{
					m_b177702State = false;
				}

				break;

			case static_cast<int>(PORTS::P_177712):
			{
				uint16_t &reg_177712 = m_vSysRegs[static_cast<int>(PORTS::P_177712)]; // Управление
				bool tve_zero = (m_vSysRegs[static_cast<int>(PORTS::P_177710)] == 0) && !(reg_177712 & TVE_CAP);

				if ((reg_177712 & TVE_MON) && tve_zero)
				{
					m_bTimerRq = true;
				}

				// инициализируем делители
				m_nTVE_Cnt = 0;

				switch (reg_177712 & (TVE_D4 | TVE_D16))
				{
					case 0:
						m_nTVE_Divider = 1;
						break;

					case TVE_D16: // делитель на 16
						m_nTVE_Divider = 16;
						break;

					case TVE_D4: // делитель на 4
						m_nTVE_Divider = 4;
						break;

					case (TVE_D4 | TVE_D16): // делитель на 64
						m_nTVE_Divider = 64;
						break;
				}

				// Если пишем в регистр управления таймером, то в счётчик сразу копируется начальное значение
				m_vSysRegs[static_cast<int>(PORTS::P_177710)] = m_vSysRegs[static_cast<int>(PORTS::P_177706)];
			}
			break;
		}
	}
}

void CCPU::SetSysRegs(uint16_t addr, uint8_t value)
{
	if (0177700 <= addr && addr < 0177714)
	{
		uint16_t v = value;

		if (addr & 1)
		{
			v <<= 8;
		}

		SetSysRegs(addr, v);
	}
}

void CCPU::SetSysRegsInternal(uint16_t addr, uint16_t value)
{
	if (0177700 <= addr && addr < 0177714)
	{
		m_vSysRegs[(addr >> 1) & 7] = value;
	}
}

void CCPU::SetSysRegsInternal(uint16_t addr, uint8_t value)
{
	if (0177700 <= addr && addr < 0177714)
	{
		int n = (addr >> 1) & 7;
		uint16_t v = value;

		if (addr & 1)
		{
			v <<= 8;
			m_vSysRegs[n] &= 0377;
			m_vSysRegs[n] |= v;
		}
		else
		{
			m_vSysRegs[n] &= 0177400;
			m_vSysRegs[n] |= v;
		}
	}
}

/*
получение аргумента источника
выход:  аргумент в m_datarg
*/
void CCPU::get_src_arg()
{
	get_src_addr();

	if (m_bByteOperation)
	{
		m_datarg = (m_nMethSrc) ? GetByte(m_nSrcAddr) : LOBYTE(m_RON[m_nSrcAddr]);
	}
	else
	{
		m_datarg = (m_nMethSrc) ? GetWord(m_nSrcAddr) : m_RON[m_nSrcAddr];
	}
}

/*
получение аргумента приёмника
выход:  аргумент в m_ALU
*/
void CCPU::get_dst_arg()
{
	get_dst_addr();

	if (m_bByteOperation)
	{
		m_ALU = (m_nMethDst) ? GetByte(m_nDstAddr) : LOBYTE(m_RON[m_nDstAddr]);
	}
	else
	{
		m_ALU = (m_nMethDst) ? GetWord(m_nDstAddr) : m_RON[m_nDstAddr];
	}
}

void CCPU::set_dst_arg()
{
	if (m_bByteOperation)
	{
		if (m_nMethDst)
		{
			SetByte(m_nDstAddr, LOBYTE(m_ALU));
		}
		else
		{
			m_RON[m_nDstAddr] &= 0177400;
			m_RON[m_nDstAddr] |= LOBYTE(m_ALU);
		}
	}
	else
	{
		if (m_nMethDst)
		{
			SetWord(m_nDstAddr, LOWORD(m_ALU));
		}
		else
		{
			m_RON[m_nDstAddr] = LOWORD(m_ALU);
		}
	}
}

/*
получение адреса источника.
выход:  m_nSrcAddr - адрес аргумента источника.
*/
void CCPU::get_src_addr()
{
	m_nSrcAddr = get_arg_addr(m_nMethSrc, m_nRegSrc);
}
/*
получение адреса приёмника.
выход:  m_nDstAddr - адрес аргумента приёмника.
*/
void CCPU::get_dst_addr()
{
	m_nDstAddr = get_arg_addr(m_nMethDst, m_nRegDst);
}


uint16_t CCPU::get_arg_addr(int meth, REGISTER reg)
{
	uint16_t addr;
	uint16_t index;
	auto nRg = static_cast<uint16_t>(reg);

	switch (meth)
	{
		case 0: // R0,      PC
			return nRg;

		case 1: // (R0),    (PC)
			return m_RON[nRg];

		case 2: // (R0)+,   #012345
			addr = m_RON[nRg];
			m_RON[nRg] += (m_bByteOperation && (reg < REGISTER::SP)) ? 1 : 2;
			return addr;

		case 3: // @(R0)+,  @#012345
			addr = GetWord(m_RON[nRg]);
			m_RON[nRg] += 2;
			return addr;

		case 4: // -(R0),   -(PC)
			m_RON[nRg] -= (m_bByteOperation && (reg < REGISTER::SP)) ? 1 : 2;
			return m_RON[nRg];

		case 5: // @-(R0),  @-(PC)
			m_RON[nRg] -= 2;
			return GetWord(m_RON[nRg]);

		case 6: // 345(R0), 345
			index = GetWord(m_RON[static_cast<int>(REGISTER::PC)]);
			m_RON[static_cast<int>(REGISTER::PC)] += 2;
			return index + m_RON[nRg];

		case 7: // @345(R0),@345
			index = GetWord(m_RON[static_cast<int>(REGISTER::PC)]);
			m_RON[static_cast<int>(REGISTER::PC)] += 2;
			return GetWord(index + m_RON[nRg]);
	}

	return 0;
}

void CCPU::ExecuteWAIT()
{
	m_bWaitMode = true;
	m_RON[static_cast<int>(REGISTER::PC)] -= 2; // возвращаем PC на команду wait.
	m_nInternalTick += timing_Misk[TIMING_IDX_WAIT];
}


void CCPU::ExecuteRESET()
{
	ResetCPU();
	ResetTimer();
	m_nInternalTick += timing_Misk[TIMING_IDX_RESET];
}


void CCPU::ExecuteRTI()
{
	uint16_t &sp = m_RON[static_cast<int>(REGISTER::SP)];
	m_RON[static_cast<int>(REGISTER::PC)] = GetWord(sp);
	sp += 2;
	SetPSW(GetWord(sp) & 0377); // рти и ртт восстанавливают только мл. байт, старший обнуляют
	sp += 2;
	m_nInternalTick += timing_Misk[TIMING_IDX_RTI];
}


/*
Отличие RTT от RTI состоит лишь в том, что прерывание по Т-разряду при отработке
RTT не наступает, а выполняется следующая команда и лишь после неё происходит прерывание
*/
void CCPU::ExecuteRTT()
{
	ExecuteRTI();

	// если установлен бит T
	if (GetPSWBit(PSW_BIT::T))
	{
		// надо сделать так, чтобы следующая команда выполнилась без прерывания
		m_bTrace_RTT = true;
	}
}

/*
1.
Утверждение, что команда START перед выполнением анализирует IRQ1 - ОШИБОЧНО.
Команды START и STEP выполняются АБСОЛЮТНО ОДИНАКОВО и при наличии IRQ1 и при отсутствии
(алгоритм см. здесь: http://zx.pk.ru/showthread.php?t=17277&postid=434967 ).
Разница лишь в том, что происходит ПОСЛЕ завершения выполнения этих команд
( в зависимости от битов загруженного PSW и активности IRQ1 ).
Если при этом установлен IRQ1, а биты 10 и 11 в загруженном командой значении PSW очищены,
то после команды START следующая команда (т.е. та команда, адрес которой команда START поместила в PC)
не выполняется, а вместо этого блок прерываний процессора 1801ВМ1 немедленно выполняет HALT_TrapTo_02.
После же команды STEP - работа блока прерываний блокируется на один шаг.
2.
Иногда для упрощения говорят, что если установлен IRQ1, а биты 10 и 11 в загруженном командой
значении PSW очищены, то после команды STEP сначала выполнится та команда, адрес которой
команда STEP поместила в PC и лишь затем произойдёт HALT_TrapTo_02 - но важно понимать,
что на самом деле такое утверждение ОШИБОЧНО. Если за тот шаг, на время которого команда
STEP заблокировала работу блока прерываний процессора, сигнал IRQ1 будет снят или
выполненной командой будет установлен бит 10 и/или бит 11 PSW, то никакого HALT_TrapTo_02
после завершения следующей за STEP команды НЕ ПРОИЗОЙДЁТ.
*/
// 000010,000011,000012,000013
void CCPU::ExecuteSTART()
{
	m_RON[static_cast<int>(REGISTER::PC)] = m_pBoard->GetWord(0177674);
	SetPSW(m_pBoard->GetWord(0177676));
	//uint16_t w;
	//m_pBoard->GetSystemRegister(0177716, &w, GSSR_NONE);
	m_pBoard->SetSystemRegister(0177716, 0200, GSSR_NONE); // сбрасываем признак HALT режима
	// при этом в регистр записи попадают данные регистра чтения и происходит некоторая фигня
	// поэтому будем делать проще, ничего читать не будем, но сразу запишем нужное число - без бита 4 и биты звука - нули
	m_nInternalTick += timing_Misk[TIMING_IDX_HALT];
}
// 000014,000015,000016,000017
void CCPU::ExecuteSTEP()
{
	ExecuteSTART();
	m_bStepMode = true;
}


void CCPU::ExecuteRTS()
{
	m_RON[static_cast<int>(REGISTER::PC)] = m_RON[static_cast<int>(m_nRegDst)];
	m_RON[static_cast<int>(m_nRegDst)] = GetWord(m_RON[static_cast<int>(REGISTER::SP)]);
	m_RON[static_cast<int>(REGISTER::SP)] += 2;
	m_nInternalTick += timing_Misk[TIMING_IDX_RTS];
}


// 000240-257
void CCPU::ExecuteCLS()
{
	m_PSW &= ~(m_instruction & 017);
	m_nInternalTick += timing_Misk[TIMING_IDX_BASE];
}
// 000260-277
void CCPU::ExecuteSET()
{
	m_PSW |= (m_instruction & 017);
	m_nInternalTick += timing_Misk[TIMING_IDX_BASE];
}


void CCPU::ExecuteJMP()
{
	if (m_nMethDst)
	{
		get_dst_addr();
		m_RON[static_cast<int>(REGISTER::PC)] = m_nDstAddr;
		m_nInternalTick += timing_OneOps_JMP[m_nMethDst];
	}
	else
	{
		UserInterrupt(INTERRUPT_4);
		m_nInternalTick = timing_Misk[TIMING_IDX_INT];
	}
}


void CCPU::ExecuteSWAB()
{
	get_dst_arg();
	m_ALU = SWAP_BYTE(m_ALU);
	uint8_t lb = LOBYTE(m_ALU);
	SetN(!!(lb & 0200));
	SetZ(lb == 0);
	SetV(false);
	SetC(false);

	if (m_nMethDst == 0)
	{
		m_bByteOperation = false; // значит будем работать со словом
		// баг процессора: если метод адресации 0, то внутренний регистр условий не обновляется
		m_bCBug = true;
	}

	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteCLR()
{
	get_dst_arg();      // clr работает как и все остальные однооперандные команды,
	m_ALU = 0;          // сперва читает содержимое, потом записывает туда 0,
	SetN(false);        // в отличие от mov который сразу просто записывает
	SetZ(true);
	SetV(false);
	SetC(false);
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteCOM()
{
	get_dst_arg();
	m_ALU = ~m_ALU;
	Set_NZ();
	SetV(false);
	SetC(true);
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteINC()
{
	get_dst_arg();
	uint32_t old = m_ALU++;
	Set_NZ();
	Set_V(old);
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteDEC()
{
	get_dst_arg();
	uint32_t old = m_ALU--;
	Set_NZ();
	Set_IV(old);
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteNEG()
{
	get_dst_arg();
#if defined _MSC_VER
#pragma warning(disable:4146)
	m_ALU = -m_ALU; // хотя, в VS прокатывает, оно использует neg eax
#pragma warning(default:4146)
#else
	// т.к. m_ALU - uint32_t, не все могут применять унарный минус к переменной
	m_ALU = ~m_ALU + 1;
#endif
	Set_NZ();

	if (m_bByteOperation) // число 0100000 не меняет знак, оно становится самим собой. поэтому считается, что V
	{
		SetV(LOBYTE(m_ALU) == 0200);
	}
	else
	{
		SetV(LOWORD(m_ALU) == 0100000);
	}

	SetC(!GetZ()); // очищается, если 0, иначе устанавливается
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteADC()
{
	get_dst_arg();
	uint32_t old = m_ALU;
	m_ALU += (GetC() ? 1 : 0);
	Set_NZ();
	Set_V(old);
	Set_C();
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteSBC()
{
	get_dst_arg();
	uint32_t old = m_ALU;
	m_ALU -= (GetC() ? 1 : 0);
	Set_NZ();
	Set_IV(old);
	Set_C();
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteTST()
{
	get_dst_arg();
	Set_NZ();
	SetV(false);
	SetC(false);
	m_nInternalTick += timing_OneOps_TST[m_nMethDst];
}


void CCPU::ExecuteROR()
{
	get_dst_arg(); // загружаем данные в АЛУ
	bool bC = !!(m_ALU & 1); // в будущее С помещаем младший бит
	m_ALU >>= 1;

	if (GetC())
	{
		m_ALU |= m_Nbit;    // текущее С помещаем в старший разряд
	}

	Set_NZ();
	SetV(GetN() ^ bC);
	SetC(bC);
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteROL()
{
	get_dst_arg(); // загружаем данные в АЛУ
	bool bC = !!(m_ALU & m_Nbit); // в будущее С помещаем старший бит
	m_ALU <<= 1;

	if (GetC())
	{
		m_ALU |= 1;    // в младший бит помещаем текущее С
	}

	Set_NZ();
	SetV(GetN() ^ bC);
	SetC(bC);
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteASR()
{
	get_dst_arg();
	bool bN = !!(m_ALU & m_Nbit);
	bool bC = !!(m_ALU & 1); // в будущее С помещаем младший бит
	m_ALU >>= 1;

	if (bN)
	{
		m_ALU |= m_Nbit;    // в старший бит помещаем текущее N, т.е. знак остаётся как был
	}

	Set_NZ();
	SetV(GetN() ^ bC);
	SetC(bC);
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteASL()
{
	get_dst_arg();
	bool bC = !!(m_ALU & m_Nbit); // в будущее С помещаем старший бит
	m_ALU <<= 1; // сдвиг
	Set_NZ();
	SetV(GetN() ^ bC);
	SetC(bC);
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteSXT()
{
	get_dst_addr();

	if (GetN())
	{
		m_ALU = ~(0);
		SetZ(false);
	}
	else
	{
		m_ALU = 0;
		SetZ(true);
	}

	SetV(false);
	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteMTPS()
{
	get_dst_arg();
	m_PSW &= 0177420; // MTPS не изменяет биты старшего байта и бит T
	m_PSW |= (m_ALU & 0357);  // этой командой можно изменить только приоритет и признаки
	m_nInternalTick += timing_OneOps_MTPS[m_nMethDst];
}


void CCPU::ExecuteMFPS()
{
	get_dst_addr();
	// работаем с байтом, тем более что у команды бит 15 всё равно ==1
	m_ALU = (int)(char)LOBYTE(GetPSW()); // распространяем знак
	Set_NZ();
	SetV(false);

	// при записи в регистр - портит старший байт
	if (m_nMethDst == 0)
	{
		m_bByteOperation = false; // значит будем работать со словом
		// баг процессора: если метод адресации 0, то внутренний регистр условий не обновляется
		m_bCBug = true;
	}

	set_dst_arg();
	m_nInternalTick += timing_OneOps_CLR[m_nMethDst];
}


void CCPU::ExecuteBR()
{
	m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBNE()
{
	if (!GetZ_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBEQ()
{
	if (GetZ_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBGE()
{
	if (GetN_br() == GetV_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBLT()
{
	if (GetN_br() != GetV_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBGT()
{
	if (!(GetZ_br() || (GetN_br() != GetV_br())))
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBLE()
{
	if (GetZ_br() || (GetN_br() != GetV_br()))
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBPL()
{
	if (!GetN_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBMI()
{
	if (GetN_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBHI()
{
	if (!(GetZ_br() || GetC_br()))
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBLOS()
{
	if (GetZ_br() || GetC_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBVC()
{
	if (!GetV_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBVS()
{
	if (GetV_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBHIS()
{
	if (!GetC_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteBLO()
{
	if (GetC_br())
	{
		m_RON[static_cast<int>(REGISTER::PC)] += ((short)(char)LOBYTE(m_instruction)) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_BR_BASE];
}


void CCPU::ExecuteXOR()
{
	get_dst_arg();
	m_ALU ^= m_RON[static_cast<int>(m_nRegSrc)];
	Set_NZ();
	SetV(false);
	set_dst_arg();
	m_nInternalTick += timing_OneOps_XOR[m_nMethDst];
}


void CCPU::ExecuteSOB()
{
	if (--m_RON[static_cast<int>(m_nRegSrc)])
	{
		m_RON[static_cast<int>(REGISTER::PC)] -= (m_instruction & 077) * 2;
	}

	m_nInternalTick += timing_Misk[TIMING_IDX_SOB];
}


void CCPU::ExecuteMOV()
{
	get_src_arg();
	get_dst_addr();
	// эта команда портит старший байт регистра, если пишем в мл.байт регистра, то старший байт заполняется знаками
	m_ALU = (m_bByteOperation) ? (int)(char)LOBYTE(m_datarg) : m_datarg;
	Set_NZ();
	SetV(false);

	// если мы что-то пишем в регистр, то с ним надо работать как со словом, испортить старший байт
	if (m_bByteOperation && (m_nMethDst == 0))
	{
		m_bByteOperation = false;
		m_bCBug = true; // баг процессора: если метод адресации 0, то внутренний регистр условий не обновляется
	}

	set_dst_arg();
	m_nInternalTick += timing_TwoOps_MOV[m_nMethSrc * 8 + m_nMethDst];
}


void CCPU::ExecuteCMP()
{
	get_src_arg();
	get_dst_arg();
	uint32_t dwNd = m_ALU;
	m_ALU = m_datarg - m_ALU;
	Set_NZ();
	/*
	V = (Ns ^ Nd) & ~(Nd ^ Nr)
	если операнды были разных знаков, и знак приёмника был тот же, что и знак результата
	(разности операндов), иначе чистится

	a xor b == false если a == b, ==true если a != b
	булевы переменные можно тупо сравнивать, а вот отлавливать бит приходится xor'ом
	*/
	SetV(!!(((m_datarg ^ dwNd) & ~(dwNd ^ m_ALU)) & m_Nbit));
	Set_C();
	m_nInternalTick += timing_TwoOps_CMP[m_nMethSrc * 8 + m_nMethDst];
}


void CCPU::ExecuteBIT()
{
	get_src_arg();
	get_dst_arg();
	m_ALU &= m_datarg;
	Set_NZ();
	SetV(false);
	m_nInternalTick += timing_TwoOps_CMP[m_nMethSrc * 8 + m_nMethDst];
}


void CCPU::ExecuteBIC()
{
	get_src_arg();
	get_dst_arg();
	m_ALU &= ~m_datarg;
	Set_NZ();
	SetV(false);
	set_dst_arg();
	m_nInternalTick += timing_TwoOps_BIS[m_nMethSrc * 8 + m_nMethDst];
}


void CCPU::ExecuteBIS()
{
	get_src_arg();
	get_dst_arg();
	m_ALU |= m_datarg;
	Set_NZ();
	SetV(false);
	set_dst_arg();
	m_nInternalTick += timing_TwoOps_BIS[m_nMethSrc * 8 + m_nMethDst];
}


void CCPU::ExecuteADD()
{
	get_src_arg();
	get_dst_arg();
	uint32_t dwNd = m_ALU;
	m_ALU += m_datarg;
	Set_NZ();
	/*
	    V = ~(Ns ^ Nd) & (Nd ^ Nr)
	    V устанавливается, если оба операнда были одного знака, а результат - противоположного,
	    иначе - чистится
	*/
	SetV(!!((~(m_datarg ^ dwNd) & (dwNd ^ m_ALU)) & m_Nbit));
	Set_C();
	set_dst_arg();
	m_nInternalTick += timing_TwoOps_BIS[m_nMethSrc * 8 + m_nMethDst];
}


void CCPU::ExecuteSUB()
{
	m_bByteOperation = false; // несмотря на m_bByteOperation это словная операция
	m_Nbit = 0100000;
	get_src_arg();
	get_dst_arg();
	uint32_t dwNd = m_ALU;
	m_ALU -= m_datarg;
	Set_NZ();
	/*
	    V = (Ns ^ Nd) & ~(Ns ^ Nr)
	    V устанавливается, если операнды были различных знаков, а знак источника был таким же,
	    как и знак результата, иначе чистится
	*/
	SetV(!!((m_datarg ^ dwNd) & ~(m_datarg ^ m_ALU) & m_Nbit));
	Set_C();
	set_dst_arg();
	m_nInternalTick += timing_TwoOps_BIS[m_nMethSrc * 8 + m_nMethDst];
}

void CCPU::ExecuteJSR()
{
	if (m_nMethDst)
	{
		get_dst_addr();
		m_RON[static_cast<int>(REGISTER::SP)] -= 2;
		SetWord(m_RON[static_cast<int>(REGISTER::SP)], m_RON[static_cast<int>(m_nRegSrc)]);
		m_RON[static_cast<int>(m_nRegSrc)] = m_RON[static_cast<int>(REGISTER::PC)];
		m_RON[static_cast<int>(REGISTER::PC)] = m_nDstAddr;
		m_nInternalTick += timing_OneOps_JSR[m_nMethDst];
	}
	else
	{
		UserInterrupt(INTERRUPT_4);
		m_nInternalTick = timing_Misk[TIMING_IDX_INT];
	}
}


void CCPU::ExecuteMARK()
{
	m_RON[static_cast<int>(REGISTER::SP)] = m_RON[static_cast<int>(REGISTER::PC)] + (m_instruction & 077) * 2;
	m_RON[static_cast<int>(REGISTER::PC)] = m_RON[static_cast<int>(REGISTER::R5)];
	m_RON[static_cast<int>(REGISTER::R5)] = GetWord(m_RON[static_cast<int>(REGISTER::SP)]);
	m_RON[static_cast<int>(REGISTER::SP)] += 2;
	m_nInternalTick += timing_Misk[TIMING_IDX_MARK];
}


/////////////////////////////////////////////////////////////////////////////////
// EIS инструкции
void CCPU::ExecuteMUL()
{
	// здесь у нас источник находится в поле DST
	get_dst_arg(); // m_ALU - аргумент источника
	int multipler = short(m_ALU & 0xffff);
	auto nReg = m_nRegSrc;   // регистры приёмники
	auto nReg1 = static_cast<REGISTER>(static_cast<int>(m_nRegSrc) | 1);
	int nRes = static_cast<short>(m_RON[static_cast<int>(nReg)]) * multipler; // умножаем
	// сохраняем результат
	m_RON[static_cast<int>(nReg)] = (nRes >> 16) & 0xffff;
	m_RON[static_cast<int>(nReg1)] = nRes & 0xffff;
	// устанавливаем признаки
	SetZ(nRes == 0);
	SetV(false);

	if (nRes < 0)
	{
		SetN(true);
		nRes = -nRes;
	}
	else
	{
		SetN(false);
	}

	SetC(nRes >= 0100000); // если результат больше 15 бит длиной, то С
	m_nInternalTick += 96 + timing_OneOps_TST[m_nMethDst];
}

void CCPU::ExecuteDIV()
{
	get_dst_arg(); // m_ALU - аргумент источника
	int divider = short(m_ALU & 0xffff);

	if (divider)
	{
		auto nReg = m_nRegSrc;   // регистры приёмники
		auto nReg1 = static_cast<REGISTER>(static_cast<int>(m_nRegSrc) | 1);
		int dividend = (m_RON[static_cast<int>(nReg)] << 16) | m_RON[static_cast<int>(nReg1)]; // делимое
		int nRes = dividend / divider; // целая часть
		int nFrac = dividend % divider; // остаток

		if (uint32_t(dividend) == 0x80000000 || (abs(nRes) >= 65536))
		{
			// если делимое == -0 или результат не влазит в 16 бит
			// то деления не происходит, и устанавливается флаг V
			SetN(false);
			SetZ(false);
			SetV(true);
			SetC(false);
		}
		else
		{
			SetN(nRes < 0);
			SetZ(nRes == 0);
			SetC(false);
			SetV(false);
			m_RON[static_cast<int>(nReg)] = nRes & 0xffff;
			m_RON[static_cast<int>(nReg1)] = nFrac & 0xffff;
		}
	}
	else
	{
		// при делении на 0 - устанавливается флаг С, остальные сбрасываются
		// а ещё в другом источнике - ещё и V устанавливается.
		SetN(false);
		SetZ(false);
		SetV(true);
		SetC(true);
	}

// пока что-то не так с алгоритмом
	m_nInternalTick += 112 + timing_OneOps_TST[m_nMethDst];
}

void CCPU::ExecuteASH()
{
	get_dst_arg(); // m_ALU - аргумент источника
	int src = m_ALU & 077;// получим количество сдвигов
	m_ALU = m_RON[static_cast<int>(m_nRegSrc)]; // аргумент приёмника
	int n = 0; // количество сдвигов
	bool C = false;
	bool V = false;

	if (src & 040)
	{
		n = 040 - (src & 037); // эквивалент смены знака
		uint32_t n15bit = m_ALU & m_Nbit;

		for (int i = 0; i < n; ++i) // sar m_ALU,n
		{
			C = !!(m_ALU & 1); // сохраняем информацию о бите переноса
			m_ALU >>= 1;
			m_ALU |= n15bit;
		}
	}
	else
	{
		n = src;

		for (int i = 0; i < n; ++i) // sal m_ALU,n
		{
			C = !!(m_ALU & m_Nbit); // сохраняем информацию о бите переноса
			uint32_t old = m_ALU;
			m_ALU <<= 1;
			V = V || !!((old ^ m_ALU) & m_Nbit);
		}
	}

	m_RON[static_cast<int>(m_nRegSrc)] = m_ALU & 0xffff; // сохраняем результат

	// используется странная логика, когда арифметическим переполнением считается
	// возникновение смены знака в процессе сдвига.
	// на настоящих PDP-11 не так, там арифметическим переполнением считается смена знака
	// в сравнении с начальным и конечным результатом
	// т.е. если число 0125252 сдвигать на 3 или 5 разрядов влево,
	// на PDP-11 перенос не возникнет, а на наших 1801ВМ2,3 - возникнет, потому что в процессе
	// сдвига знак менялся столько раз, сколько было сдвигов - 3 или 5.
	if (n)
	{
		SetV(V); // если знак менялся. не важно с какого на какой, неважно когда
		SetC(C);
	}

	Set_NZ();
	m_nInternalTick += 24 + 42 * n + timing_OneOps_TST[m_nMethDst];
}

void CCPU::ExecuteASHC()
{
	get_dst_arg(); // m_ALU - аргумент источника
	int src = m_ALU & 077;// получим количество сдвигов
	auto nReg = m_nRegSrc;   // регистры приёмники
	auto nReg1 = static_cast<REGISTER>(static_cast<int>(m_nRegSrc) | 1);
	m_ALU = (uint32_t(m_RON[static_cast<int>(nReg)]) << 16) | m_RON[static_cast<int>(nReg1)]; // аргумент приёмника
	int n = 0; // количество сдвигов
	bool C = false;
	bool V = false;

	if (src & 040)
	{
		n = 040 - (src & 037); // эквивалент смены знака
		uint32_t n31bit = m_ALU & 0x80000000;

		for (int i = 0; i < n; ++i) // sar m_ALU,n
		{
			C = !!(m_ALU & 1); // сохраняем информацию о бите переноса
			m_ALU >>= 1;
			m_ALU |= n31bit;
		}
	}
	else
	{
		n = src;

		for (int i = 0; i < n; ++i) // sal m_ALU,n
		{
			C = !!(m_ALU & 0x80000000); // сохраняем информацию о бите переноса
			uint32_t old = m_ALU;
			m_ALU <<= 1;
			V = V || !!((old ^ m_ALU) & 0x80000000);
		}
	}

	// сохраняем результат
	m_RON[static_cast<int>(nReg)] = (m_ALU >> 16) & 0xffff;
	m_RON[static_cast<int>(nReg1)] = m_ALU & 0xffff;
	SetN((int)m_ALU < 0);
	SetZ(m_ALU == 0);

	if (n)
	{
		SetV(V); // если знак менялся. не важно с какого на какой
		SetC(C);
	}

	m_nInternalTick += 32 + 42 * n + timing_OneOps_TST[m_nMethDst];
}
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
// выделение экспоненты из числа
int CCPU::GetExponent(uint32_t F)
{
	return (F >> 23) & 0377;
}
// выделение мантиссы из числа
uint32_t CCPU::GetMantiss(uint32_t F)
{
	F &= 0x007fffff; // выделяем мантиссу
	return F | 0x00800000; // добавляем подразумеваемый бит
}
// нормализация вправо
__int64 CCPU::NormRight(__int64 D, int &res_exp)
{
	while (D >= 0x010000000000)
	{
		D /= 2; // арифметический сдвиг вправо на 1
		res_exp++;
	}

	return D;
}
// нормализация влево
__int64 CCPU::NormLeft(__int64 D, int &res_exp)
{
	while (!(D & 0x008000000000))
	{
		res_exp--;
		D <<= 1;
	}

	return D;
}

bool CCPU::CheckRes(__int64 D, bool res_sign, int &res_exp, int &res)
{
	// округляем до 32 бит
	D += 0100000;

	if (D & 0xff0000000000)
	{
		res_exp++;
		D &= 0x00ffffffffff;
	}

	// получаем 24битный результат
	res = (D >> 16) & 0xffffffff;

	if (res_exp <= 0)
	{
		// underflow
		FISUnderflow();
		return false;
	}

	if (res_exp >= 0400)
	{
		// overflow
		FISOverflow();
		return false;
	}

	if (res_sign)
	{
		res |= 0x80000000; // ставим бит знака
	}

	res &= ~0x00800000; // убираем подразумеваемый бит
	res |= res_exp << 23; // помещаем на место экспоненту
	return true;
}

void CCPU::FISEx(int res)
{
	// сохраняем результат
	m_fisTmpReg -= 2; SetWord(m_fisTmpReg, uint16_t(res & 0xffff)); // сперва младшую часть
	m_fisTmpReg -= 2; SetWord(m_fisTmpReg, uint16_t((res >> 16) & 0xffff)); // потом старшую
	m_RON[static_cast<int>(m_nRegDst)] = m_fisTmpReg;

	if (m_nRegDst == REGISTER::PC) // если это был PC
	{
		m_RON[static_cast<int>(m_nRegDst)] += 4; // то его не надо отодвигать назад
	}

	SetN(res < 0);
	SetZ(!GetExponent(res));
	SetV(false);
	SetC(false);
	SetPSWBit(PSW_BIT::P5, false);
	SetPSWBit(PSW_BIT::P6, false);
}

void CCPU::FISOverflow(uint8_t flg)
{
	flg |= 2; // ставим флаг V
	SetN(!!(flg & 010));
	SetZ(false);
	SetV(!!(flg & 2));
	SetC(!!(flg & 1));
	SetPSWBit(PSW_BIT::P5, false);
	SetPSWBit(PSW_BIT::P6, false);

	// регистры, кроме PC не должны менять своё значение
	if (m_nRegDst == REGISTER::PC)
	{
		m_RON[static_cast<int>(m_nRegDst)] = m_fisTmpReg;
	}

	UserInterrupt(0244);
	m_nInternalTick = timing_Misk[TIMING_IDX_INT];
}

void CCPU::FISUnderflow(uint8_t flg)
{
	FISOverflow(flg | 010); // ставим флаг N
}

void CCPU::FISDivideBy0(uint8_t flg)
{
	FISUnderflow(flg | 1); // ставим флаг C
}

void CCPU::FISAddSub(int A, int B)
{
	int res = 0; // здесь получается результат
	bool res_sign = false; // знак результата
	int B_exp = GetExponent(B);
	int A_exp = GetExponent(A);

	if (B_exp)
	{
		if (A_exp)
		{
			int t = B;
			B = GetMantiss(B);

			if (t < 0)
			{
				B = -B;
			}

			t = A;
			A = GetMantiss(A);

			if (t < 0)
			{
				A = -A;
			}

			// двигаем всё это влево на 16 бит, чтобы удобней было
			__int64 A64 = __int64(A) << 16;
			__int64 B64 = __int64(B) << 16;
			int res_exp = B_exp;

			if (B_exp != A_exp)
			{
				int n = 0;

				if (B_exp > A_exp)
				{
					n = B_exp - A_exp;
				}
				else
				{
					res_exp = A_exp;
					n = A_exp - B_exp;
					__int64 t = A64;
					A64 = B64;
					B64 = t;
				}

				if (n > 24)
				{
					A64 = 0;
				}
				else
				{
					for (int i = 0; i < n; i++)
					{
						A64 /= 2; // арифметический сдвиг вправо на 1 бит
					}
				}
			}

			// просто складываем
			__int64 res64 = B64 + A64;

			if (res64 < 0)
			{
				res64 = -res64;
				res_sign = !res_sign;
			}

			if (res64)
			{
				res64 = NormRight(res64, res_exp);
				res64 = NormLeft(res64, res_exp);

				if (!CheckRes(res64, res_sign, res_exp, res))
				{
					m_nInternalTick = 220; // значение взято от балды, т.к. я не нашёл информации о быстродействии
					return;
				}
			}
		}
		else
		{
			// если A == 0 то результат просто B
			res = B;
		}
	}
	else
	{
		// если B == 0 то результат просто A
		res = A;
	}

	// FPUEX
	FISEx(res);
	m_nInternalTick = 250; // значение взято от балды, т.к. я не нашёл информации о быстродействии
}

void CCPU::ExecuteFADD()
{
	m_fisTmpReg = m_RON[static_cast<int>(m_nRegDst)];
	// m_nRegDst - номер регистра, в котором хранится адрес блока параметров.
	uint16_t B_hi = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t B_lo = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t A_hi = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t A_lo = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	// сформируем 32 разрядные float
	int B = (uint32_t(B_hi) << 16) | B_lo;
	int A = (uint32_t(A_hi) << 16) | A_lo;
	FISAddSub(A, B);
}

void CCPU::ExecuteFSUB()
{
	m_fisTmpReg = m_RON[static_cast<int>(m_nRegDst)];
	// m_nRegDst - номер регистра, в котором хранится адрес блока параметров.
	uint16_t B_hi = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t B_lo = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t A_hi = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t A_lo = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	// сформируем 32 разрядные float
	int B = (uint32_t(B_hi) << 16) | B_lo;
	int A = (uint32_t(A_hi) << 16) | A_lo;
	FISAddSub(A, B + 0x80000000);
}

void CCPU::ExecuteFMUL()
{
	m_fisTmpReg = m_RON[static_cast<int>(m_nRegDst)];
	// m_nRegDst - номер регистра, в котором хранится адрес блока параметров.
	uint16_t B_hi = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t B_lo = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t A_hi = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t A_lo = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	// сформируем 32 разрядные float
	int B = (uint32_t(B_hi) << 16) | B_lo;
	int A = (uint32_t(A_hi) << 16) | A_lo;
	int res = 0;
	bool res_sign = (B < 0); // знак результата

	if (A < 0)
	{
		res_sign = !res_sign;
	}

	int B_exp = GetExponent(B);
	int A_exp = GetExponent(A);
	B = GetMantiss(B);
	A = GetMantiss(A);

	if (B_exp && A_exp)
	{
		int res_exp = B_exp + A_exp - 0200;
		__int64 res64 = __int64(A) * __int64(B);
		res64 >>= 8;

		if (res64)
		{
			res64 = NormLeft(res64, res_exp);

			if (!CheckRes(res64, res_sign, res_exp, res))
			{
				m_nInternalTick = 300; // значение взято от балды, т.к. я не нашёл информации о быстродействии
				return;
			}
		}
	}

	FISEx(res);
	m_nInternalTick = 350; // значение взято от балды, т.к. я не нашёл информации о быстродействии
}

void CCPU::ExecuteFDIV()
{
	m_fisTmpReg = m_RON[static_cast<int>(m_nRegDst)];
	// m_nRegDst - номер регистра, в котором хранится адрес блока параметров.
	uint16_t B_hi = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t B_lo = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t A_hi = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	uint16_t A_lo = GetWord(m_fisTmpReg); m_fisTmpReg += 2;
	// сформируем 32 разрядные float
	int B = (uint32_t(B_hi) << 16) | B_lo;
	int A = (uint32_t(A_hi) << 16) | A_lo;
	int res = 0;
	bool res_sign = (B < 0); // знак результата

	if (A < 0)
	{
		res_sign = !res_sign;
	}

	int B_exp = GetExponent(B);
	int A_exp = GetExponent(A);
	B = GetMantiss(B);
	A = GetMantiss(A);

	if (!B_exp)
	{
		FISDivideBy0();
		return;
	}

	if (A_exp)
	{
		int res_exp = A_exp - B_exp + 0177;
		__int64 res64 = (__int64(A) << 32) / B;
		res64 <<= 9; // не знаю, что за магия и почему надо именно так сдвигать.
		// но работает.
//      // / это вот реализация такого алгоритма деления
//      bool c;
//      int t = 0100;
//      goto l_5;
//
//      do
//      {
//          A <<= 1;
// l_5:     A -= B;
//          if (A < 0)
//          {
//              goto l_7;
//          }
// l_8:     c = !!(t & 0x80000000);
//          t <<= 1;
//          t |= 1;
//      } while (!c);
//      goto l_9;
//
//      do
//      {
//          A <<= 1;
//          A += B;
//          if (A >= 0)
//          {
//              goto l_8;
//          }
// l_7:     c = !!(t & 0x80000000);
//          t <<= 1;
//      } while (!c);
//
// l_9:
//      __int64 res64 = __int64(t) << 16;
//      // / это вот такой алгоритм деления

		if (res64)
		{
			res64 = NormRight(res64, res_exp);
			res64 = NormLeft(res64, res_exp);

			if (!CheckRes(res64, res_sign, res_exp, res))
			{
				m_nInternalTick = 300; // значение взято от балды, т.к. я не нашёл информации о быстродействии
				return;
			}
		}
	}

	FISEx(res);
	m_nInternalTick = 350; // значение взято от балды, т.к. я не нашёл информации о быстродействии
}

/////////////////////////////////////////////////////////////////////////////////


void CCPU::PrepareCPU()
{
	{
		// Сначала заполняем таблицу ссылками на метод ExecuteUNKNOWN, выполняющий TRAP 10
		RegisterMethodRef(0000000, 0177777, &CCPU::ExecuteUNKNOWN);
		RegisterMethodRef(0000000, 0000000, &CCPU::ExecuteHALT);
		RegisterMethodRef(0000001, 0000001, &CCPU::ExecuteWAIT);
		RegisterMethodRef(0000002, 0000002, &CCPU::ExecuteRTI);
		RegisterMethodRef(0000003, 0000003, &CCPU::ExecuteBPT);
		RegisterMethodRef(0000004, 0000004, &CCPU::ExecuteIOT);
		RegisterMethodRef(0000005, 0000005, &CCPU::ExecuteRESET);
		RegisterMethodRef(0000006, 0000006, &CCPU::ExecuteRTT);
		RegisterMethodRef(0000010, 0000013, &CCPU::ExecuteSTART);
		RegisterMethodRef(0000014, 0000017, &CCPU::ExecuteSTEP);
		RegisterMethodRef(0000100, 0000177, &CCPU::ExecuteJMP);
		RegisterMethodRef(0000200, 0000207, &CCPU::ExecuteRTS);    // RTS / RETURN
		RegisterMethodRef(0000240, 0000257, &CCPU::ExecuteCLS);
		RegisterMethodRef(0000260, 0000277, &CCPU::ExecuteSET);
		RegisterMethodRef(0000300, 0000377, &CCPU::ExecuteSWAB);
		RegisterMethodRef(0000400, 0000777, &CCPU::ExecuteBR);
		RegisterMethodRef(0001000, 0001377, &CCPU::ExecuteBNE);
		RegisterMethodRef(0001400, 0001777, &CCPU::ExecuteBEQ);
		RegisterMethodRef(0002000, 0002377, &CCPU::ExecuteBGE);
		RegisterMethodRef(0002400, 0002777, &CCPU::ExecuteBLT);
		RegisterMethodRef(0003000, 0003377, &CCPU::ExecuteBGT);
		RegisterMethodRef(0003400, 0003777, &CCPU::ExecuteBLE);
		RegisterMethodRef(0004000, 0004777, &CCPU::ExecuteJSR);    // JSR / CALL
		RegisterMethodRef(0005000, 0005077, &CCPU::ExecuteCLR);
		RegisterMethodRef(0005100, 0005177, &CCPU::ExecuteCOM);
		RegisterMethodRef(0005200, 0005277, &CCPU::ExecuteINC);
		RegisterMethodRef(0005300, 0005377, &CCPU::ExecuteDEC);
		RegisterMethodRef(0005400, 0005477, &CCPU::ExecuteNEG);
		RegisterMethodRef(0005500, 0005577, &CCPU::ExecuteADC);
		RegisterMethodRef(0005600, 0005677, &CCPU::ExecuteSBC);
		RegisterMethodRef(0005700, 0005777, &CCPU::ExecuteTST);
		RegisterMethodRef(0006000, 0006077, &CCPU::ExecuteROR);
		RegisterMethodRef(0006100, 0006177, &CCPU::ExecuteROL);
		RegisterMethodRef(0006200, 0006277, &CCPU::ExecuteASR);
		RegisterMethodRef(0006300, 0006377, &CCPU::ExecuteASL);
		RegisterMethodRef(0006400, 0006477, &CCPU::ExecuteMARK);
		RegisterMethodRef(0006700, 0006777, &CCPU::ExecuteSXT);
		RegisterMethodRef(0010000, 0017777, &CCPU::ExecuteMOV);
		RegisterMethodRef(0020000, 0027777, &CCPU::ExecuteCMP);
		RegisterMethodRef(0030000, 0037777, &CCPU::ExecuteBIT);
		RegisterMethodRef(0040000, 0047777, &CCPU::ExecuteBIC);
		RegisterMethodRef(0050000, 0057777, &CCPU::ExecuteBIS);
		RegisterMethodRef(0060000, 0067777, &CCPU::ExecuteADD);

		/////////////////////////////////////////////////////////////////////////////////
		if (g_Config.m_bEmulateEIS || g_Config.m_bVM1G)
		{
			RegisterMethodRef(0070000, 0070777, &CCPU::ExecuteMUL);
		}

		if (g_Config.m_bEmulateEIS)
		{
			RegisterMethodRef(0071000, 0071777, &CCPU::ExecuteDIV);
			RegisterMethodRef(0072000, 0072777, &CCPU::ExecuteASH);
			RegisterMethodRef(0073000, 0073777, &CCPU::ExecuteASHC);
		}

		/////////////////////////////////////////////////////////////////////////////////
		RegisterMethodRef(0074000, 0074777, &CCPU::ExecuteXOR);

		if (g_Config.m_bEmulateFIS)
		{
			RegisterMethodRef(0075000, 0075007, &CCPU::ExecuteFADD);
			RegisterMethodRef(0075010, 0075017, &CCPU::ExecuteFSUB);
			RegisterMethodRef(0075020, 0075027, &CCPU::ExecuteFMUL);
			RegisterMethodRef(0075030, 0075037, &CCPU::ExecuteFDIV);
		}

		RegisterMethodRef(0077000, 0077777, &CCPU::ExecuteSOB);
		RegisterMethodRef(0100000, 0100377, &CCPU::ExecuteBPL);
		RegisterMethodRef(0100400, 0100777, &CCPU::ExecuteBMI);
		RegisterMethodRef(0101000, 0101377, &CCPU::ExecuteBHI);
		RegisterMethodRef(0101400, 0101777, &CCPU::ExecuteBLOS);
		RegisterMethodRef(0102000, 0102377, &CCPU::ExecuteBVC);
		RegisterMethodRef(0102400, 0102777, &CCPU::ExecuteBVS);
		RegisterMethodRef(0103000, 0103377, &CCPU::ExecuteBHIS);    // BCC
		RegisterMethodRef(0103400, 0103777, &CCPU::ExecuteBLO);     // BCS
		RegisterMethodRef(0104000, 0104377, &CCPU::ExecuteEMT);
		RegisterMethodRef(0104400, 0104777, &CCPU::ExecuteTRAP);
		RegisterMethodRef(0105000, 0105077, &CCPU::ExecuteCLR);    // CLRB
		RegisterMethodRef(0105100, 0105177, &CCPU::ExecuteCOM);    // COMB
		RegisterMethodRef(0105200, 0105277, &CCPU::ExecuteINC);    // INCB
		RegisterMethodRef(0105300, 0105377, &CCPU::ExecuteDEC);    // DECB
		RegisterMethodRef(0105400, 0105477, &CCPU::ExecuteNEG);    // NEGB
		RegisterMethodRef(0105500, 0105577, &CCPU::ExecuteADC);    // ADCB
		RegisterMethodRef(0105600, 0105677, &CCPU::ExecuteSBC);    // SBCB
		RegisterMethodRef(0105700, 0105777, &CCPU::ExecuteTST);    // TSTB
		RegisterMethodRef(0106000, 0106077, &CCPU::ExecuteROR);    // RORB
		RegisterMethodRef(0106100, 0106177, &CCPU::ExecuteROL);    // ROLB
		RegisterMethodRef(0106200, 0106277, &CCPU::ExecuteASR);    // ASRB
		RegisterMethodRef(0106300, 0106377, &CCPU::ExecuteASL);    // ASLB
		RegisterMethodRef(0106400, 0106477, &CCPU::ExecuteMTPS);
		RegisterMethodRef(0106700, 0106777, &CCPU::ExecuteMFPS);
		RegisterMethodRef(0110000, 0117777, &CCPU::ExecuteMOV);    // MOVB
		RegisterMethodRef(0120000, 0127777, &CCPU::ExecuteCMP);    // CMPB
		RegisterMethodRef(0130000, 0137777, &CCPU::ExecuteBIT);    // BITB
		RegisterMethodRef(0140000, 0147777, &CCPU::ExecuteBIC);    // BICB
		RegisterMethodRef(0150000, 0157777, &CCPU::ExecuteBIS);    // BISB
		RegisterMethodRef(0160000, 0167777, &CCPU::ExecuteSUB);
	}
}

void CCPU::DoneCPU()
{
}

void CCPU::RegisterMethodRef(uint16_t start, uint16_t end, ExecuteMethodRef methodref)
{
	for (int opcode = start; opcode <= end; ++opcode)
	{
		m_pExecuteMethodMap[opcode] = methodref;
	}
}

// CPU.h: interface for the CCPU class.
//


#pragma once

#include "Config.h"
#include <deque>

#define SWAP_BYTE(A) ( (((A) & 0x00FF) << 8) | (((A) >> 8) & 0x00FF) )

/*формат регистра PSW: PPPTNZVC
PPP - приоритет
если приоритет установленный на данный момент для ЦП ниже, чем установленный приоритет
ЗАПРОСА НА ПРЕРЫВАНИЕ, то ЦП прервёт работу и обработает прерывание.
Если же приоритет текущей задачи равен или выше приоритета запроса, то ЦП сперва закончит вычисления,
а затем обработает прерывание
У приоритета реально используется только бит 7. остальные биты ни на что не влияют.
T - флаг трассировки
N - отрицательность
Z - ноль
V - арифметический перенос, т.е. при операции со знаковыми числами, происходит перенос в знаковый разряд,
который самый старший
C - перенос, который происходит за пределы разрядности операндов
*/

// PSW bits
enum class PSW_BIT : int
{
	C = 0,
	V = 1,
	Z = 2,
	N = 3,
	T = 4,
	P5 = 5,
	P6 = 6,
	P = 7,
	MASKI = 10, // 10й бит, маскирует IRQ1, IRQ2, IRQ3, VIRQ
	HALT = 11   // 11й бит - маскирует только IRQ1
};

/*
биты 8,9 - аппаратный номер процессора на шине. только по чтению.

При установленном в PSW бите 10 запрещены все прерывания, включая IRQ1.
Кроме того, при установленном бите 10 прерывание зависания вместо Trap_to_4
вызывает прерывание особого вида - HALT_Trap.

При установленном бите 11 запрещено только прерывание IRQ1, но данный
режим реализован немного кривовато, так как сбрасывается командами RTI и RTT
(которые всегда пишут 0 в старший байт PSW), а значит имеет смысл только при
дополнительно запрещённых обычных прерываниях и не использовании программных прерываний.
*/
class CMotherBoard;

#pragma inline_recursion(on)

class CCPU
{
		static const int timing_Misk_10[13];
		static const int timing_OneOps_TST_10[8];
		static const int timing_OneOps_CLR_10[8];
		static const int timing_OneOps_MTPS_10[8];
		static const int timing_OneOps_XOR_10[8];
		static const int timing_OneOps_JMP_10[8];
		static const int timing_OneOps_JSR_10[8];
		static const int timing_TwoOps_MOV_10[8][8];
		static const int timing_TwoOps_CMP_10[8][8];
		static const int timing_TwoOps_BIS_10[8][8];

		static const int timing_Misk_11[13];
		static const int timing_OneOps_TST_11[8];
		static const int timing_OneOps_CLR_11[8];
		static const int timing_OneOps_MTPS_11[8];
		static const int timing_OneOps_XOR_11[8];
		static const int timing_OneOps_JMP_11[8];
		static const int timing_OneOps_JSR_11[8];
		static const int timing_TwoOps_MOV_11[8][8];
		static const int timing_TwoOps_CMP_11[8][8];
		static const int timing_TwoOps_BIS_11[8][8];

		const int *timing_Misk;
		const int *timing_OneOps_TST;
		const int *timing_OneOps_CLR;
		const int *timing_OneOps_MTPS;
		const int *timing_OneOps_XOR;
		const int *timing_OneOps_JMP;
		const int *timing_OneOps_JSR;
		const int *timing_TwoOps_MOV;
		const int *timing_TwoOps_CMP;
		const int *timing_TwoOps_BIS;


		CMotherBoard   *m_pBoard;
		int             m_nInternalTick; // количество тактов, выполняемой инструкции
		int             m_nROMTimingCorrection; // коррекция таймингов для быстрой памяти
		int             m_nCmdTicks;     // счётчик тактов для обработки встроенного таймера
		enum class PORTS : int
		{
			P_177700,
			P_177702,
			P_177704,
			P_177706,
			P_177710,
			P_177712,
			P_NUMBER
		};
	public:
		enum class REGISTER : int
		{
			R0 = 0,
			R1,
			R2,
			R3,
			R4,
			R5,
			SP,
			PC,
			PSW
		};
	protected: // Statics
		using ExecuteMethodRef = void (CCPU::*)();
	//	std::unique_ptr<ExecuteMethodRef[]> m_pExecuteMethodMap;
		void            RegisterMethodRef(uint16_t start, uint16_t end, ExecuteMethodRef methodref);

		uint16_t        m_RON[static_cast<int>(REGISTER::PSW)];   // PSW не входит в массив
		uint16_t        m_PSW;          // PSW отдельно
		uint16_t        m_Freg;         // копия флагов состояния NZVC (для простоты будет копия PSW целиком, прост использоваться будут только флаги)
		bool            m_bCBug;        // флаг, когда применять баг бита С
		bool            m_b177702State; // состояние регистра
		std::vector<uint16_t> m_vSysRegs;   // массив внутренних системных регистров процессора 1777700..1777712
		std::vector<uint16_t> m_vSysRegsMask; // массив масок битов, которые доступны по записи

		uint16_t        m_instruction;  // текущая инструкция
		uint16_t        m_nSrcAddr;     // адрес источника
		uint16_t        m_nDstAddr;     // адрес приёмника
		uint32_t        m_datarg;       // аргумент источника
		uint32_t        m_ALU;          // приёмник
		uint32_t        m_Nbit;         // знаковый разряд для операции. 200 для байта, 100000 для слова
		bool            m_bByteOperation;   // флаг байтовой операции
		int             m_nMethSrc;
		int             m_nMethDst;
		REGISTER        m_nRegSrc;
		REGISTER        m_nRegDst;


		bool            m_bTrace_RTT;
		bool            m_bWaitMode;    // WAIT
		bool            m_bStepMode;
		bool            m_bIRQ1rq;      // импульс ~\_ для срабатывания IRQ1
		bool            m_bIRQ1LL;      // кнопка СТОП
		bool            m_bIRQ1flg;     // чтобы различать, по команде HALT или IRQ1 было прерывание
		bool            m_bGetVector;   // отлов ошибки передачи вектора.
		bool            m_bTwiceHangup; // отлов двойного зависания - это когда при прерывании по вектору 4 при записи в стек случается m_bRPLYrq
		bool            m_bRPLYrq;      // Прерывание по тайм-ауту внешней шины МПИ - отсутствие ответа RPLY
		bool            m_bACLOrq;      // Прерывание по аварии питания аналогового
		bool            m_bDCLOrq;      // Прерывание по аварии питания цифрового
		bool            m_bRPL2rq;      // Прерывание по двойному зависанию
		bool            m_bVIRQErrrq;   // Прерывание по ошибке передачи вектора
		bool            m_bIRQ2rq;      // Прерывание от таймера 50Гц
		bool            m_bIRQ3rq;      // Прерывание по вектору 270
		bool            m_bTimerRq;     // Запрос на прерывание от ВЕ-таймера
		std::deque<uint16_t> m_qVIRQ;   // очередь векторов прерываний VIRQ, это излишество, но всё же, пусть будет

		int             m_nTVE_Cnt;     // скорость счётчика
		int             m_nTVE_Divider; // делитель скорости

		// функции чтения/записи слова/байта с перехватом адресов внутренних системных регистров,
		// дальше CPU они не идут, поэтому нужны функции получения их значений.
		uint8_t         GetByte(const uint16_t addr);
		uint16_t        GetWord(const uint16_t addr);
		void            SetByte(const uint16_t addr, uint8_t value);
		void            SetWord(const uint16_t addr, uint16_t value);

		void            get_src_arg();
		void            get_dst_arg();
		void            set_dst_arg();
		void            get_src_addr();
		void            get_dst_addr();
		uint16_t        get_arg_addr(int meth, REGISTER reg);

		inline void     Set_NZ()
		{
			// установка N
			SetN(!!(m_ALU & m_Nbit));

			// установка Z
			if (m_bByteOperation)
			{
				SetZ(!LOBYTE(m_ALU));
			}
			else
			{
				SetZ(!LOWORD(m_ALU));
			}
		}
		inline void     Set_V(uint32_t old)
		{
			// установка V - это смена значения знакового бита. с 0 на 1 (для сложений)
			SetV(!!(~old & m_ALU & m_Nbit));
		}
		inline void     Set_IV(uint32_t old)
		{
			// установка V - это смена значения знакового бита. с 1 на 0 (для вычитаний)
			SetV(!!(old & ~m_ALU & m_Nbit));
		}
		inline void     Set_C()
		{
			// установка C
			SetC(!!(m_ALU & ((m_bByteOperation) ? 0xffffff00 : 0xffff0000)));
		}

		void            SetC(bool bFlag);
		bool            GetC() const;
		void            SetV(bool bFlag);
		bool            GetV() const;
		void            SetN(bool bFlag);
		bool            GetN() const;
		void            SetZ(bool bFlag);
		bool            GetZ() const;

		// специальные функции для проверки условий команд ветвления

		bool            GetC_br() const;
		bool            GetV_br() const;
		bool            GetZ_br() const;
		bool            GetN_br() const;

		void            ExecuteUNKNOWN();
		// No fields
		void            ExecuteHALT();
		void            ExecuteWAIT();
		void            ExecuteRTI();
		void            ExecuteBPT();
		void            ExecuteIOT();
		void            ExecuteRESET();
		void            ExecuteRTT();
		void            ExecuteSTART();
		void            ExecuteSTEP();
		void            ExecuteCLS();
		void            ExecuteSET();

		// One fields
		void            ExecuteRTS();

		// Two fields
		void            ExecuteJMP();
		void            ExecuteSWAB();
		void            ExecuteCLR();
		void            ExecuteCOM();
		void            ExecuteINC();
		void            ExecuteDEC();
		void            ExecuteNEG();
		void            ExecuteADC();
		void            ExecuteSBC();
		void            ExecuteTST();
		void            ExecuteROR();
		void            ExecuteROL();
		void            ExecuteASR();
		void            ExecuteASL();
		void            ExecuteMARK();
		void            ExecuteSXT();
		void            ExecuteMTPS();
		void            ExecuteMFPS();

		// Branches & interrupts
		void            ExecuteBR();
		void            ExecuteBNE();
		void            ExecuteBEQ();
		void            ExecuteBGE();
		void            ExecuteBLT();
		void            ExecuteBGT();
		void            ExecuteBLE();
		void            ExecuteBPL();
		void            ExecuteBMI();
		void            ExecuteBHI();
		void            ExecuteBLOS();
		void            ExecuteBVC();
		void            ExecuteBVS();
		void            ExecuteBHIS();
		void            ExecuteBLO();

		void            ExecuteEMT();
		void            ExecuteTRAP();

		// Three fields
		void            ExecuteJSR();
		void            ExecuteXOR();
		void            ExecuteSOB();

		// Four fields
		void            ExecuteMOV();
		void            ExecuteCMP();
		void            ExecuteBIT();
		void            ExecuteBIC();
		void            ExecuteBIS();

		void            ExecuteADD();
		void            ExecuteSUB();

		// EIS проходит тест 791402, FIS проходит тест 791403
		// EIS
		void            ExecuteMUL();
		void            ExecuteDIV();
		void            ExecuteASH();
		void            ExecuteASHC();
		//End EIS
		//FIS
		int             GetExponent(uint32_t F);
		uint32_t        GetMantiss(uint32_t F);
		__int64         NormRight(__int64 D, int &res_exp);
		__int64         NormLeft(__int64 D, int &res_exp);
		bool            CheckRes(__int64 D, bool res_sign, int &res_exp, int &res);
		uint16_t        m_fisTmpReg;    // временный регистр для операций FIS
		void            FISEx(int res);
		void            FISOverflow(uint8_t flg = 0);
		void            FISUnderflow(uint8_t flg = 0);
		void            FISDivideBy0(uint8_t flg = 0);

		void            FISAddSub(int A, int B);

		void            ExecuteFADD();
		void            ExecuteFSUB();
		void            ExecuteFMUL();
		void            ExecuteFDIV();
		//End FIS

		bool            InterruptDispatch();
		void            SystemInterrupt(uint32_t nVector);
		void            UserInterrupt(uint32_t nVector);

		void            Timerprocess(); // внутренний таймер
		void            ResetTimer();   // инициализация таймера

		void            PrepareCPU();   // Инициализация таблицы декодирования команд
		void            DoneCPU();      // удаление таблицы декодирования команд и освобождение памяти
		// приём/передача внутренних системных регистров 177700-177712 от/к внешних(им) устройств(ам) (в MB функции)
		bool            GetSysRegs(uint16_t addr, uint16_t &val);

	public:
		CCPU();
		virtual ~CCPU();
		void            InitVars();     // инициализация переменных

		void            InitCPU();      // начальная инициализация внутренних системных регистров, PSW, PC при подаче питания
		void            ResetCPU();     // функция выполняется по команде RESET или по внешнему сигналу INIT

		inline void     GetTimerSpeedInternal(int &tve_cnt, int &tve_div)
		{
			tve_cnt = m_nTVE_Cnt, tve_div = m_nTVE_Divider;
		}
		inline void     SetTimerSpeedInternal(int tve_cnt, int tve_div)
		{
			m_nTVE_Cnt = tve_cnt, m_nTVE_Divider = tve_div;
		}

		void            AttachBoard(CMotherBoard *pBoard);

		int             TranslateInstruction();

		inline void     SetIRQ1()
		{
			// обработка нажатия на кнопку СТОП, или каким либо другим способом установка сигнала в IRQ1
			if (!m_bIRQ1LL)
			{
				m_bIRQ1LL = true;
				m_bIRQ1rq = true;
			}
		}
		inline void     UnsetIRQ1()
		{
			m_bIRQ1LL = false;
		}

		inline void     TickIRQ2()
		{
			m_bIRQ2rq = true;
		}
		inline void     TickIRQ3()
		{
			m_bIRQ3rq = true;
		}
		inline void     ACPowerFail()
		{
			m_bACLOrq = true;
		}
		inline void     DCPowerFail()
		{
			m_bDCLOrq = true;
		}
		void            ReplyError();
		void            InterruptVIRQ(uint16_t interrupt);

		// приём/передача регистров общего назначения и PSW от/в внешних(им) устройств (в MB функции)
		inline bool     GetFREGBit(PSW_BIT pos) const
		{
			return !!(m_Freg & (1 << static_cast<int>(pos)));
		}

		inline bool     GetPSWBit(PSW_BIT pos) const
		{
			return !!(m_PSW & (1 << static_cast<int>(pos)));
		}
		inline void     SetPSWBit(PSW_BIT pos, bool val)
		{
			if (val)
			{
				m_PSW |= (1 << static_cast<int>(pos));
			}
			else
			{
				m_PSW &= ~(1 << static_cast<int>(pos));
			}
		}
		uint16_t        GetPSW() const
		{
			return m_PSW;
		}
		void            SetPSW(uint16_t value)
		{
			// биты 8 и 9 только по чтению, и их значение всегда остаётся неизменным.
			m_PSW = (m_PSW & 01400) | (value & ~01400);
		}

		inline void     SetRON(REGISTER reg, uint16_t value)
		{
			if (reg == REGISTER::PSW)
			{
				SetPSW(value);
			}
			else
			{
				m_RON[static_cast<int>(reg)] = value;
			}
		}
		inline uint16_t GetRON(REGISTER reg) const
		{
			if (reg == REGISTER::PSW)
			{
				return GetPSW();
			}

			return m_RON[static_cast<int>(reg)];
		}

		// приём/передача внутренних системных регистров 177700-177712 от/к внешних(им) устройств(ам) (в MB функции)
		uint16_t        GetSysRegsIndirect(uint16_t addr);
		void            SetSysRegs(uint16_t addr, uint16_t value);
		void            SetSysRegs(uint16_t addr, uint8_t value);
		// простое сохранение значений регистров, используется во внутренних целях
		void            SetSysRegsInternal(uint16_t addr, uint16_t value);
		void            SetSysRegsInternal(uint16_t addr, uint8_t value);

		inline uint16_t GetCurrentInstruction()
		{
			return m_instruction;
		}
};


inline void CCPU::SetC(bool bFlag)
{
	SetPSWBit(PSW_BIT::C, bFlag);
}


inline bool CCPU::GetC() const
{
	return GetPSWBit(PSW_BIT::C);
}


inline void CCPU::SetV(bool bFlag)
{
	SetPSWBit(PSW_BIT::V, bFlag);
}


inline bool CCPU::GetV() const
{
	return GetPSWBit(PSW_BIT::V);
}


inline void CCPU::SetZ(bool bFlag)
{
	SetPSWBit(PSW_BIT::Z, bFlag);
}


inline bool CCPU::GetZ() const
{
	return GetPSWBit(PSW_BIT::Z);
}


inline void CCPU::SetN(bool bFlag)
{
	SetPSWBit(PSW_BIT::N, bFlag);
}


inline bool CCPU::GetN() const
{
	return GetPSWBit(PSW_BIT::N);
}

inline bool CCPU::GetC_br() const
{
	return GetFREGBit(PSW_BIT::C);
}

inline bool CCPU::GetV_br() const
{
	return GetFREGBit(PSW_BIT::V);
}

inline bool CCPU::GetZ_br() const
{
	return GetFREGBit(PSW_BIT::Z);
}

inline bool CCPU::GetN_br() const
{
	return GetFREGBit(PSW_BIT::N);
}
#pragma inline_recursion(off)


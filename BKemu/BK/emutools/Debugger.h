// Debugger.h: interface for the CDebugger class.
//


#pragma once

#include "DisasmView.h"
#include "BreakPoint.h"
#include "CPU.h"

#define COLORED_TAG "<C>"
#define COLORED_TAG_LENGTH 3

enum : int
{
	SYS_PORT_177660 = 0,
	SYS_PORT_177662_IN,
	SYS_PORT_177662_OUT,
	SYS_PORT_177664,
	SYS_PORT_177700,
	SYS_PORT_177702,
	SYS_PORT_177704,
	SYS_PORT_177706,
	SYS_PORT_177710,
	SYS_PORT_177712,
	SYS_PORT_177714_IN,
	SYS_PORT_177714_OUT,
	SYS_PORT_177716_IN,
	SYS_PORT_177716_OUT_TAP,
	SYS_PORT_177716_OUT_MEM
};


// массив цветов для подсветки синтаксиса
enum : int
{
	HLCOLOR_DEFAULT = 0,
	HLCOLOR_ADDRESS,
	HLCOLOR_MNEMONIC,
	HLCOLOR_REGISTER,
	HLCOLOR_NUMBER,
	HLCOLOR_SYMBOL,
	HLCOLOR_MNEMONICFPU,
	HLCOLOR_REGISTERFPU,
	HLCOLOR_NUM_COLS    // количество цветов
};
extern const COLORREF g_crDebugColorHighLighting[HLCOLOR_NUM_COLS];
constexpr int MNEMONIX_NUM = 113;

class CMotherBoard;

class CDebugger
{
		using CalcInstrLenRef     = int (CDebugger::*)() const; // в качестве аргумента - значение в m_wInstr, результат - возвращаемое значение
		using CalcNextAddrRef     = uint16_t (CDebugger::*)(); // в качестве аргумента - значение в m_wInstr, m_wPC - адрес за опкодом, результат - следующий адрес, или ADDRESS_NONE, если рассчитать невозможно
		using DisassembleInstrRef = int (CDebugger::*)(uint16_t *); // в качестве аргумента - значение в m_wInstr и m_wPC, результат - в m_strInstr и m_strArg, возвращает кол-во доп.слов инструкции

		struct InstrFuncRefs
		{
			const CString      *pMnemonic;
			CalcInstrLenRef     InstrLenRef;
			CalcNextAddrRef     NextAddrRef;
			DisassembleInstrRef DisasmInstrRef;
		};

		InstrFuncRefs* *m_pInstrRefsMap;
		void RegisterMethodRef(uint16_t start, uint16_t end, const CString *mnemonic, CalcInstrLenRef ilenmref, CalcNextAddrRef nxamref, DisassembleInstrRef dsimref);

		static int          m_outLevel;
		static const CString m_strRegNames[8];
		static const CString m_strRegNamesFPU[8];
		static const CString m_strAddrFormat[8];
		static const CString m_strAddrFormat_PC[8];
		static const CString m_strArgFormat_Addr;
		static const CString m_strArgFormat_Number;
		static const CString m_strArgFormat_Comma;

		static const CString m_strMnemonix[MNEMONIX_NUM];

		CMotherBoard       *m_pBoard;
		CDisasmView        *m_pDisasmDlg;
		CBreakPointList     m_breakpointList;

		bool                m_bPrevCmdC;    // флаг, как дизассемблировать BCC/BHIS BCS/BLO
		bool                m_bPrevCmdCp;   // после команды CMP - сравнение, иначе - битС

		// для того, чтобы не передавать функциям кучу лишней информации, заведём глобальные переменные

		uint16_t            m_wPC;          // текущий адрес текущей инструкции
		uint16_t            m_wInstr;       // сама инструкция
		bool                m_bCBug;        // флаг бага флага С
		uint16_t            m_wFreg;        // регистр флагов состояний NZVC с учётом бага
		CString             m_strInstr;     // мнемоника
		CString             m_strArg;       // аргументы, если есть

		//флаги дизассемблирования FPU
		bool                m_bFD, m_bFL;


		inline bool         GetFREGBit(PSW_BIT pos)
		{
			return !!(m_wFreg & (1 << static_cast<int>(pos)));
		}

		int                 ConvertArgToString(int arg, uint16_t pc, CString &strSrc, uint16_t &code) const;
		int                 ConvertArgToStringFPU(int arg, uint16_t pc, CString &strSrc, uint16_t &code) const;


		uint16_t            GetArgD(int pos);
		uint16_t            GetArgAddrD(int meth, CCPU::REGISTER reg) const;
		int                 CalcArgLength(int pos) const;

		inline int          CalcLenOneWord() const;
		inline int          CalcLenTwoFields() const;
		inline int          CalcLenFourFields() const;
		inline int          CalcLenFIS() const;

		uint16_t            CalcNextAddrRegular4();
		uint16_t            CalcNextAddrRegular2();
		uint16_t            CalcNextAddrRegular();
		uint16_t            CalcNextAddrUNKNOWN();
		uint16_t            CalcNextAddrRTI();
		uint16_t            CalcNextAddrRTS();
		uint16_t            CalcNextAddrJMP();
		uint16_t            CalcNextAddrMARK();
		uint16_t            CalcNextAddrSOB();
		uint16_t            CalcNextAddrBR();
		uint16_t            CalcNextAddrBNE();
		uint16_t            CalcNextAddrBEQ();
		uint16_t            CalcNextAddrBGE();
		uint16_t            CalcNextAddrBLT();
		uint16_t            CalcNextAddrBGT();
		uint16_t            CalcNextAddrBLE();
		uint16_t            CalcNextAddrBPL();
		uint16_t            CalcNextAddrBMI();
		uint16_t            CalcNextAddrBHI();
		uint16_t            CalcNextAddrBLOS();
		uint16_t            CalcNextAddrBVC();
		uint16_t            CalcNextAddrBVS();
		uint16_t            CalcNextAddrBCC();
		uint16_t            CalcNextAddrBCS();
		uint16_t            CalcNextAddrSWAB_MFPS();
		uint16_t            CalcNextAddrMOVB();

		int                 DisassembleNoArgs(uint16_t *codes);
		int                 DisassembleUnknown(uint16_t *codes);
		int                 DisassembleCLS(uint16_t *codes);
		int                 DisassembleSET(uint16_t *codes);
		int                 DisassembleRTS(uint16_t *codes);
		int                 DisassembleTwoField(uint16_t *codes);
		int                 DisassembleFourField(uint16_t *codes);
		int                 DisassembleMARK(uint16_t *codes);
		int                 DisassembleEMT(uint16_t *codes);
		int                 DisassembleBR(uint16_t *codes);
		int                 DisassembleBCC(uint16_t *codes);
		int                 DisassembleBCS(uint16_t *codes);
		int                 DisassembleJSR(uint16_t *codes);
		int                 DisassembleEISExt(uint16_t *codes);
		int                 DisassembleXOR(uint16_t *codes);
		int                 DisassembleFIS(uint16_t *codes);
		int                 DisassembleSOB(uint16_t *codes);
		int                 DisassembleCMP(uint16_t *codes);
		int                 DisassembleSETF(uint16_t *codes);
		int                 DisassembleSETD(uint16_t *codes);
		int                 DisassembleSETL(uint16_t *codes);
		int                 DisassembleSETI(uint16_t *codes);
		int                 DisassembleTwoFieldFPUD(uint16_t *codes);
		int                 DisassembleACDFieldFPUD(uint16_t *codes);
		int                 DisassembleACDFieldLDX(uint16_t *codes);
		int                 DisassembleACDFieldLDEXP(uint16_t *codes);
		int                 DisassembleACDFieldLDCJX(uint16_t *codes);
		int                 DisassembleACDFieldLDCXY(uint16_t *codes);
		int                 DisassembleACSFieldFPUD(uint16_t *codes);
		int                 DisassembleACSFieldSTX(uint16_t *codes);
		int                 DisassembleACSFieldSTEXP(uint16_t *codes);
		int                 DisassembleACSFieldSTCXJ(uint16_t *codes);
		int                 DisassembleACSFieldSTCXY(uint16_t *codes);

//////////////////////////////////////////////////////////////////////////
//
// Тут будет всё для ассемблирования команды.
//
//////////////////////////////////////////////////////////////////////////

		enum class CPUCmdGroup
		{
			NOOPS,
			CBRANCH,
			EIS,
			TRAP,
			SOB,
			MARK,
			TWOOPREG,
			FIS,
			PUSH,
			ONEOPS,
			TWOOPS,
			FPUCLASS1,
			FPUCLASS2,
			FPUCLASS2_1,
		};

		enum class OpcodeType
		{
			REGULAR,
			MMG,
			EIS,
			FIS,
			FPU
		};

		struct CPUCommandStruct
		{
			CString     strName;    // Имя мнемоники
			uint16_t    nOpcode;    // генерируемый опкод
			CPUCmdGroup nGroup;     // группа, к которой принадлежит команда
			OpcodeType  nType;      // тип инструкции, чтобы вык/выкл нужные группы инструкций
		};
		struct Registers
		{
			CString name;
			int nNum;
		};

		static const std::vector<CPUCommandStruct> m_pCPUCommands;
		static const std::vector<Registers> m_pCPURegs;
		static const std::vector<Registers> m_pFPURegs;

		bool                m_bOperandType;

		bool                SkipWhitespace(TCHAR **pch);
		bool                ReadToken(CString &str, TCHAR **pch, bool bNoSkip);
		bool                ReadOctalNumber(int &num, TCHAR **pch, bool bNoSkip);
		bool                isEoln(TCHAR **pch);
		bool                NeedChar(TCHAR ch, TCHAR **pch, bool bNoSkip);

		bool                assembleBR(int &pc, uint16_t *buf, int &len, TCHAR **pch);
		bool                assembleTRAP(uint16_t *buf, int &len, TCHAR **pch);
		bool                assembleMARK(uint16_t *buf, int &len, TCHAR **pch);
		bool                assemble2ROP(int &pc, uint16_t *buf, int &len, TCHAR **pch);
		bool                assemble1OP(int &pc, uint16_t *buf, int &len, TCHAR **pch);
		bool                assemble1OPFPU(int &pc, uint16_t *buf, int &len, TCHAR **pch);
		bool                assemble2OP(int &pc, uint16_t *buf, int &len, TCHAR **pch);
		bool                assemble1OPR(int &pc, uint16_t *buf, int &len, TCHAR **pch);
		bool                assemble1OPRFPU(int &pc, uint16_t *buf, int &len, TCHAR **pch);
		bool                assemble2OPR(int &pc, uint16_t *buf, int &len, TCHAR **pch);
		bool                assemble2OPRFPU(int &pc, uint16_t *buf, int &len, TCHAR **pch, bool bFPU);
		bool                assembleSOB(int &pc, uint16_t *buf, int &len, TCHAR **pch);
		bool                ReadRegName(uint16_t *buf, TCHAR **pch, bool bNoSkip);
		bool                ReadFPURegName(uint16_t *buf, TCHAR **pch, bool bShort, bool bNoSkip);
		void                SetAddrReg(uint16_t *buf, uint16_t val);
		bool                Operand_analyse(int &pc, uint16_t *buf, int &len, TCHAR **pch, bool bFPU);

	public:
		CDebugger();
		virtual ~CDebugger();
		void                InitMaps(bool bReinit = false);
		bool                GetDebugPCBreak(uint16_t addr) const;

		uint16_t            CalcNextAddr(uint16_t pc);

		int                 CalcInstructionLength(uint16_t instr);
		static void         InitOutMode()
		{
			m_outLevel = 0;
		}
		static bool         IsInstructionOut(const uint16_t instruction);
		bool                IsInstructionOver(const uint16_t instruction) const;

		// Breakpoint managment methods
		bool                IsBpeakpointExists(const CBreakPoint &breakpoint) const;
		bool                IsBpeakpointAtAddress(const uint16_t addr) const;
		bool                SetSimpleBreakpoint(uint16_t addr);
		bool                SetSimpleBreakpoint();
		bool                RemoveBreakpoint(const uint16_t addr);
		bool                RemoveBreakpoint();
		void                ClearBreakpointList();

		void                AttachBoard(CMotherBoard *pBoard);
		inline CMotherBoard *GetBoard()
		{
			return m_pBoard;
		}
		inline void         AttachWnd(CDisasmView *pDlg)
		{
			m_pDisasmDlg = pDlg;
		}
		void                SetCurrentAddress(uint16_t address) const;

		int                 DebugInstruction(uint16_t pc, CString &strInstr, uint16_t *codes);
		bool                CheckDebuggedLine(uint16_t wLineAddr) const;

		int                 AssembleCPUInstruction(int pc, uint16_t *buf, CString *pStr);

		uint16_t            GetCursorAddress();
		uint16_t            GetBottomAddress();

		uint16_t            GetRegister(const CCPU::REGISTER reg) const;
		uint16_t            GetPortValue(const int addr) const;
		uint16_t            GetDebugMemDumpWord(const uint16_t addr) const;
		uint8_t             GetDebugMemDumpByte(const uint16_t addr) const;
		uint16_t            GetAltProData(const int reg) const;
		uint16_t            GetFDDData(const int reg) const;
		uint16_t            GetDebugHDDRegs(const int nDrive, const int num, const bool bReadMode) const;

		bool                SetDebugRegs(const CCPU::REGISTER nAddress, const uint16_t nValue);
		bool                SetDebugPorts(const uint16_t nAddress, const uint16_t nValue);
		bool                SetDebugMemDump(const uint16_t nAddress, const uint16_t nValue);
		bool                SetDebugMemDump(const uint16_t nAddress, const uint8_t nValue);
		bool                SetDebugAltProData(const uint16_t nAddress, const uint16_t nValue);
};

/*
Перерисовка окна дизассемблера.
всё начинается с функции CMotherBoard::BreakCPU()
    в ней посылается сообщение WM_CPU_DEBUGBREAK,
    в результате срабатывает функция CMainFrame::OnCpuBreak
        там берётся текущий PC и передаётся функции CDebugger::SetCurrentAddress
            этот адрес делается начальным и перерисовывается список дизассемблера

Просто, примитивно и неудобно.
особенно, работа с курсором.
 */
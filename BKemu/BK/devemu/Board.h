// Board.h: interface for the CMotherBoard class.
//

#pragma once

//#include <afxtempl.h>

#include "Device.h"
#include "CPU.h"
#include "FDDController.h"
#include "Debugger.h"
#include "Screen.h"
#include "BkSound.h"
#include "Speaker.h"
#include "Covox.h"
#include "AYSnd.h"
#include "Menestrel.h"
#include "MSFManager.h"
#include "ExceptionHalt.h"
#include "MainFrm.h"
#include "Config.h"

constexpr auto BRD_10_MON10_BNK = 8;
constexpr auto BRD_10_BASIC10_1_BNK = 10;
constexpr auto BRD_10_BASIC10_2_BNK = 12;
constexpr auto BRD_10_REGISTERS_BNK = 14;

#include <thread>

class CMainFrame;

class CMotherBoard : public CDevice
{
		friend class CCPU;
		friend class CFDDController;

	public:
		// регистры, публичные, т.к. лень на каждый регистр писать функцию, хотя и можно.
		volatile uint16_t   m_reg177660;
		volatile uint16_t   m_reg177662in;
		volatile uint16_t   m_reg177662out;
		volatile uint16_t   m_reg177664;
		volatile uint16_t   m_reg177714in;
		volatile uint16_t   m_reg177714out;
		volatile uint16_t   m_reg177716in;
		volatile uint16_t   m_reg177716out_tap;
		volatile uint16_t   m_reg177716out_mem;

		enum                { ADDRESS_NONE = -1, GO_INTO = -2, GO_OUT = -3 };

	protected:
		struct ThreadVars_t
		{
			int     nGotoAddress;       // адрес, на котором будет сделан отладочный останов

			int     nCPUTicks;          // текущий счётчик тактов процессора
			double  fCpuTickTime;

			double  fMediaTicks;        // счётчик медиа тактов, происходящих за одну инструкцию
			double  fMedia_Mod;
			double  fMemoryTicks;
			double  fMemory_Mod;
			double  fFDDTicks;
			double  fFDD_Mod;

			// переменные для медиатактов
			int     nMediaTicksPerFrame; // количество медиатактов во фрейме (длина звукового буфера в сэмплах).
			int     nMediaTickCount;    // счётчик медиа тактов (их должно быть не более g_Config.m_nSoundSampleRate/CPU_FRAMES_PER_SECOND в текущем фрейме)
			int     nBufPos;            // позиция в звуковом буфере
			std::unique_ptr<SAMPLE_INT[]> pSoundBuffer;   // звуковой буфер

			// переменные для эмуляции луча ЭЛТ
			uint16_t nVideoAddress;     // видео адрес, младшие 6 бит - счётчик строк внутри строки
			bool    bHgate;             // флаг отсчёта служебных видеоциклов в строке
			bool    bVgate;             // флаг отсчёта служебных строк
			int     nVGateCounter;      // дополнительный счётчик служебных строк
			int     nLineCounter;       // счётчик видео строк

			void init()
			{
				nGotoAddress = ADDRESS_NONE;
				clear();
				fCpuTickTime = 0.0;
				fMedia_Mod = 0.0;
				fMemory_Mod = 0.0;
				fFDD_Mod = 0.0;
				nMediaTicksPerFrame = 0;
				nMediaTickCount = 0;
				nBufPos = 0;
				pSoundBuffer = nullptr;
			}
			void clear()
			{
				nCPUTicks = 0;
				fMediaTicks = 0.0;
				fMemoryTicks = 0.0;
				fFDDTicks = 0.0;
				nVideoAddress = 0; // видео адрес, младшие 6 бит - счётчик строк внутри строки
				bHgate = false;
				bVgate = true;
				nVGateCounter = 64;
				nLineCounter = 0; // счётчик видео строк
			}
		};

		BK_DEV_MPI          m_BoardModel;       // тип модели БК 10 или 11 или 11М
		uint16_t            m_nBKPortsIOArea;   // адрес, выше которого нет памяти, и все адреса считаются регистрами и портами
		uint16_t            m_nStartAddr;       // адрес запуска материнской платы

		int                 m_nLowBound;        // нижняя граница, меньше которой не может быть частота
		int                 m_nHighBound;       // верхняя граница, больше которой не может быть частота

		int                 m_nCPUFreq;         // текущая частота работы.
		int                 m_nCPUFreq_prev;    // текущая частота работы.
		int                 m_nBaseCPUFreq;     // базовая частота работы, по которой вычисляется множитель

		ThreadVars_t        m_sTV;              // блок переменных, используемых в потоке

		CFDDController      m_fdd;              // контроллер 1801ВП1-128.
		CCPU                m_cpu;              // Процессор 1801ВМ1
		mutable std::vector<uint8_t> m_pMemory; // Основная память БК + дополнительная для контроллеров
		BKMEMBank_t         m_MemoryMap[16];    // карта памяти 64кбайтного адресного пространства, там помещается 16 банков по 4 кб.
		ConfBKModel_t       m_ConfBKModel;
		std::vector<WindowParam_t> m_vWindows;  // вектор окон для дампера.

		CBkSound           *m_pSound;           // указатель на модуль звуковой подсистемы
		CSpeaker           *m_pSpeaker;         // указатель на объект пищалка
		CCovox             *m_pCovox;           // указатель на объект ковокс
		CMenestrel         *m_pMenestrel;       // указатель на объект Менестрель
		CAYSnd             *m_pAYSnd;           // указатель на объект мульти сопроцессор Ay8910-3
		CDebugger          *m_pDebugger;        // указатель на отладчик

		CMainFrame         *m_pParent;          // указатель на родительский фрейм, куда всякие сообщения посылаются
		volatile bool       m_bBreaked;         // флаг состояния - процессор приостановлен для отладки
		volatile bool       m_bRunning;         // флаг состояния - процессор работает/стоит

		int                 m_nKeyCleanEvent;   // счётчик для события сброса бита 7 в 177660

		void                MediaTick();

		// поток с точным таймером
		void                TimerThreadFunc();  // собственно функция
		std::thread         m_TimerThread;
		std::mutex          m_mutLockTimerThread; // флаг работы потока.
		std::mutex          m_mutRunLock;       // блокировка работы фрейма.
		volatile bool       m_bKillTimerEvent;  // флаг для остановки потока.

		void                Make_One_Screen_Cycle();

		// Инициализация памяти
		virtual bool        InitMemoryModules();
		virtual void        InitMemoryValues(int nMemSize);
		virtual void        MemoryManager();
		bool                LoadRomModule(int iniRomNameIndex, int bank);   // загрузка нужного модуля ПЗУ по заданному адресу

		// Методы для загрузки и сохранения состояния эмулятора
		virtual bool        RestoreRegisters(CMSFManager &msf);
		virtual bool        RestoreMemory(CMSFManager &msf);
		virtual bool        RestoreMemoryMap(CMSFManager &msf);
		virtual bool        RestorePreview(CMSFManager &msf, HBITMAP hScreenshot);
		virtual bool        RestoreConfig(CMSFManager &msf);


		// Методы, эмулирующие поведение регистров
		virtual bool        SetSystemRegister(uint16_t addr, uint16_t src, UINT dwFlags = GSSR_NONE);
		virtual bool        GetSystemRegister(uint16_t addr, void *pDst, UINT dwFlags = GSSR_NONE);

		virtual bool        Interception(); // Вызывается после каждой команды, для перехвата функций

		bool                EmulateLoadTape();
		bool                EmulateSaveTape();

		virtual int         GetScreenPage() const; // получение номера страницы экрана
		int                 CalcStep();

		virtual bool        FillWndVectorPtr(int nMemSize);

	public:
		CMotherBoard(BK_DEV_MPI model = BK_DEV_MPI::BK0010);
		virtual ~CMotherBoard() override;

		virtual MSF_CONF    GetConfiguration();
		BK_DEV_MPI          GetBoardModel();

		auto                GetWndVectorPtr()
		{
			return &m_vWindows;
		}
		void                AttachWindow(CMainFrame *pParent);
		void                AttachSound(CBkSound *pSnd);
		void                AttachSpeaker(CSpeaker *pDevice);
		void                AttachCovox(CCovox *pDevice);
		void                AttachMenestrel(CMenestrel *pDevice);
		void                AttachAY8910(CAYSnd *pDevice);
		void                AttachDebugger(CDebugger *pDevice);

		void                SetMTC(int mtc);

		void                StopTimerThread();
		bool                StartTimerThread();

		virtual bool        RestoreState(CMSFManager &msf, HBITMAP hScreenshot);

		// Виртуальные методы, вызываемые после команды reset
		virtual void        OnReset() override;

		// Методы Set/Get byte/word
		virtual uint8_t     GetByteT(const uint16_t addr, int &nTC);
		virtual uint16_t    GetWordT(const uint16_t addr, int &nTC);
		virtual void        SetByteT(const uint16_t addr, uint8_t value, int &nTC);
		virtual void        SetWordT(const uint16_t addr, uint16_t value, int &nTC);

		virtual uint8_t     GetByte(const uint16_t addr);
		virtual uint16_t    GetWord(const uint16_t addr);
		virtual void        GetByte(const uint16_t addr, uint8_t *pValue) override;
		virtual void        GetWord(const uint16_t addr, uint16_t *pValue) override;
		virtual void        SetByte(const uint16_t addr, uint8_t value) override;
		virtual void        SetWord(const uint16_t addr, uint16_t value) override;

		virtual uint8_t     GetByteIndirect(const uint16_t addr);
		virtual uint16_t    GetWordIndirect(const uint16_t addr);
		virtual void        SetByteIndirect(const uint16_t addr, uint8_t value);
		virtual void        SetWordIndirect(const uint16_t addr, uint16_t value);

		uint16_t            GetRON(CCPU::REGISTER reg);
		void                SetRON(CCPU::REGISTER reg, uint16_t value);
		inline uint16_t     GetPSW()
		{
			return m_cpu.GetPSW();
		}
		inline void         SetPSW(uint16_t value)
		{
			m_cpu.SetPSW(value);
		}

		inline bool         GetPSWBit(PSW_BIT pos)
		{
			return m_cpu.GetPSWBit(pos);
		}
		inline void         SetPSWBit(PSW_BIT pos, bool val)
		{
			m_cpu.SetPSWBit(pos, val);
		}

		inline void         Irq2Interrupt()
		{
			m_cpu.TickIRQ2();
		}
		inline void         Irq3Interrupt()
		{
			m_cpu.TickIRQ3();
		}
		virtual void        StopInterrupt();
		virtual void        UnStopInterrupt();
		void                KeyboardInterrupt(uint16_t interrupt);

		void                FrameParam();

		void                ResetDevices();
		bool                InitBoard(uint16_t nNewStartAddr); // Эта функция используется единственный раз при начальной инициализации модели.
		void                ResetCold(uint16_t addrMask);
		void                RunInto();
		void                RunOver();
		void                RunOut();
		void                RunToAddr(uint16_t addr);
		void                RunCPU(bool bUnbreak = true); // запуск. по умолчанию сбрасывается отладочный приостанов
		void                StopCPU(bool bUnbreak = true); // остановка. по умолчанию сбрасывается отладочный приостанов
		inline bool         IsCPURun()
		{
			return m_bRunning;
		}

		void                BreakCPU();
		void                UnbreakCPU(int nGoto);
		inline bool         IsCPUBreaked()
		{
			return m_bBreaked;
		}

		void                AccelerateCPU();
		void                SlowdownCPU();
		void                NormalizeCPU();
		bool                CanAccelerate();
		bool                CanSlowDown();
		int                 GetLowBound();
		int                 GetHighBound();
		void                SetCPUBaseFreq(int frq); // установка базовой частоты
		void                ResetToCPUBaseFreq(); // установка базовой частоты
		void                SetCPUFreq(int frq); // установка текущей частоты
		int                 GetCPUFreq() const; // выдача текущей частоты
		int                 GetCPUSpeed() const;   // выдача текущей частоты для конфигурации

		// Передача данных экрану
		virtual void        ChangePalette() {}

		// Приём/передача данных fdd контроллеру
		CFDDController     *GetFDD();
		virtual void        SetFDDType(BK_DEV_MPI model, bool bInit = true);
		virtual BK_DEV_MPI  GetFDDType();
		uint16_t            GetAltProMode();
		void                SetAltProMode(uint16_t w);
		uint16_t            GetAltProCode();
		void                SetAltProCode(uint16_t w);
		uint16_t            GetAltProExtCode();
		// прочие функции
		virtual void        Set177716RegMem(uint16_t w) {}
		virtual void        Set177716RegTap(uint16_t w);
		virtual void        SetMemPages(int pg0, int pg1) {}
		virtual void        RestoreMemPages() {}

		// функции для карты памяти
		virtual uint8_t    *GetMainMemory() const;
		virtual uint8_t    *GetAddMemory() const;

		void                DrawDebugScreen() const;
};

/*
Организация массива памяти (делается один большой массив)
32кб: ОЗУ
000000--банк 0 ---страница 0 --- 0
010000--банк 1
020000--банк 2
030000--банк 3
040000--банк 4 ---страница 1 --- 16384
050000--банк 5
060000--банк 6
070000--банк 7
32кб: ПЗУ
100000--банк 8 ---страница 2 --- 32768 - монитор MONIT10.ROM
110000--банк 9
120000--банк 10------------------------- бейсик Basic10_1.rom
130000--банк 11
140000--банк 12---страница 3 --- 49152 - бейсик Basic10_2.rom
150000--банк 13
160000--банк 14------------------------- бейсик и регистры
170000--банк 15
*/

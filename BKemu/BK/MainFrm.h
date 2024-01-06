
// MainFrm.h : интерфейс класса CMainFrame
//

#pragma once
#include "BKView.h"
#include "RegDumpViewCPU.h"
#include "RegDumpViewFDD.h"
#include "MemDumpView.h"
#include "DisasmView.h"
#include "TapeCtrlView.h"
#include "OscillatorView.h"
#include "BKVKBDView.h"

#include "DropTarget.h"
#include "ScriptRunner.h"
#include "BkSound.h"
#include "Speaker.h"
#include "Covox.h"
#include "Menestrel.h"
#include "AYSnd.h"
#include "Tape.h"
#include "Config.h"

class CMotherBoard;
class CScreen;
class CBKMEMDlg;

constexpr DWORD CLI_KEY_B = (1 << 0);   // ключ /B - задать имя bin файла
constexpr DWORD CLI_KEY_M = (1 << 1);   // ключ /M - задать имя msf файла
constexpr DWORD CLI_KEY_T = (1 << 2);   // ключ /T - задать имя tap/wav файла
constexpr DWORD CLI_KEY_S = (1 << 3);   // ключ /S - задать имя script файла
constexpr DWORD CLI_KEY_C = (1 << 4);   // ключ /C - задать имя загружаемой конфигурации
constexpr DWORD CLI_KEY_P = (1 << 5);   // ключ /P - задать номера подключаемых страниц в конф. БК11(М)
constexpr DWORD CLI_KEY_L = (1 << 6);   // ключ /L - задать адрес загрузки bin файла
constexpr DWORD CLI_KEY_A = (1 << 7);   // ключ /A - задать адрес запуска bin файла
constexpr DWORD CLI_KEY_D = (1 << 8);   /* ключ /D - задать имя bin файла, который будет загружен с адреса,
                                        заданного ключом /L (если не задано, то по адресу из бин заголовка),
                                        и запустить его с адреса, заданного ключом /A (если не задано, то
                                        либо по адресу ключа /L, либо по адресу из бин заголовка) */
constexpr DWORD CLI_KEY_R = (1 << 9);   // ключ /R без параметров, запустить дамп после загрузки, иначе - не запускать
constexpr DWORD CLI_KEY_F = (1 << 10);  // ключ /F без параметров, загружать дамп не в формате бин.
constexpr DWORD CLI_KEY_P_PAGE0 = (1 << 16); // флаг задания параметра для окна 0 в ключе /P
constexpr DWORD CLI_KEY_P_PAGE1 = (1 << 17); // флаг задания параметра для окна 1 в ключе /P

struct CLI_PARAMETERS
{
	DWORD       nStatus;    // флаги наличия значений параметров
	fs::path    strBinFileName;     // /B
	fs::path    strMemFileName;     // /M
	fs::path    strTapeFileName;    // /T
	fs::path    strScriptFileName;  // /S
	CString     strPage0, strPage1;     // номера страниц, для подключения на БК11(М) получаемые через командную строку
	uint16_t    nLoadAddr;          // /L
	uint16_t    nStartAddr;         // /A
	CLI_PARAMETERS()
		: nStatus(0)
		, nLoadAddr(0)
		, nStartAddr(0)
	{};
	void clear()
	{
		strBinFileName.clear();
		strMemFileName.clear();
		strTapeFileName.clear();
		strScriptFileName.clear();
		strPage0.Empty();
		strPage1.Empty();
		nLoadAddr = 0;
		nStartAddr = 0;
		nStatus = 0;
	}
	void clearBinFName()
	{
		strBinFileName.clear();
		nStatus &= ~CLI_KEY_B;
	}
	void clearScriptFName()
	{
		strScriptFileName.clear();
		nStatus &= ~CLI_KEY_S;
	}
};



class CMainFrame ///: public CFrameWndEx
{
	///	DECLARE_DYNCREATE(CMainFrame)

		using initRegParam = struct
		{
			CString strSection;
			CString strDescription;
			CString strArguments;
			DWORD iconID;
			UINT nExtStrID;
			bool bShell;
		};

		// встроенные члены панели элементов управления
	///	CMFCMenuBar         m_wndMenuBar;
	///	CMFCToolBar         m_wndToolBar;
	///	CMFCToolBar         m_wndToolBarSound;
	///	CMFCToolBar         m_wndToolBarDebug;
	///	CMFCToolBar         m_wndToolBarVCapt;

//      CMFCToolBarImages   m_UserImages;
	///	CMFCStatusBar       m_wndStatusBar;

		CRegDumpViewCPU     m_paneRegistryDumpViewCPU;  // панель дампа регистров CPU
		CRegDumpViewFDD     m_paneRegistryDumpViewFDD;  // панель дампа регистров FDD
		CMemDumpView        m_arPaneMemoryDumpView[NUMBER_VIEWS_MEM_DUMP]; // панель дампа памяти
		CDisasmView         m_paneDisassembleView;      // панель отладчика
		CTapeCtrlView       m_paneTapeCtrlView;         // панель управления записью
		COscillatorlView    m_paneOscillatorView;       // панель осциллографа
		CBKVKBDView         m_paneBKVKBDView;           // панель виртуальной клавиатуры

//      UINT                m_nInterAppToolGlobalMessage;
		CBKMEMDlg          *m_pBKMemView;           // объект отображения карты памяти
		bool                m_bBKMemViewOpen;       // флаг, что m_pBKMemView достоверен
		CRect               m_rectMemMap;           // координаты окна карты памяти
		// Эмулятор
		std::unique_ptr<CMotherBoard> m_pBoard;     // объект материнской платы БК
		std::unique_ptr<CBkSound> m_pSound;         // модуль звуковой подсистемы
		CDebugger          *m_pDebugger;
		CScreen            *m_pScreen;              // модуль подсистемы вывода на экран

		CSpeaker            m_speaker;              // объект пищалка
		CCovox              m_covox;                // объект ковокс
		CMenestrel          m_menestrel;            // объект Менестрель
		CAYSnd              m_aySnd;                // объект мульти сопроцессор Ay8910-3
		CTape               m_tape;                 // объект обработчика кассет

		CScriptRunner       m_Script;               // объект обработчика скриптов
		CDropTarget         m_dropTarget;           // объект поддержки драг-н-дропа (вроде даже работает)
		// счётчики
		bool                m_bBeginPeriod;
		DWORD               m_nStartTick;
		int                 m_nRegsDumpCounter;     // счётчик. через сколько 20мс интервалов обновлять на экране дамп регистров
		bool                m_bLongResetPress;      // специально для А16М, вводим длинный ресет

		// разные параметры, получаемые через ДнД или командную строку
		CLI_PARAMETERS      m_cli;

		// размеры экрана
		enum ScreenSizeNumber : int
		{
			SCREENSIZE_CUSTOM = 0,
			SCREENSIZE_256X192,
			SCREENSIZE_324X243,
			SCREENSIZE_432X324,
			SCREENSIZE_512X384,
			SCREENSIZE_576X432,
			SCREENSIZE_768X576,
			SCREENSIZE_1024X768,
			SCREENSIZE_1280X960,
			SCREENSIZE_1536X1152,
			SCREENSIZE_1792X1344,
			SCREENSIZE_2048X1536,
			SCREENSIZE_NUMBER
		};
		ScreenSizeNumber    m_nScrSizeNum;
		int                 m_nScreen_X, m_nScreen_CustomX;
		static int          m_aScreenSizes[SCREENSIZE_NUMBER];
		static CString      m_strMenuScrSizes[SCREENSIZE_NUMBER];

		bool                m_bFoundFFMPEG;

	protected:
		// функции инициализации эмулятора
		void                InitKbdStatus();
		void                InitEmulator();
		void                InitRegistry();
		void                InitScrViewPort(int w);

		void                RegisterDefaultIcon(CString &strName, initRegParam *regParam);
		void                RegisterShellCommand(CString &strName, initRegParam *regParam);

		void                ParseCommandLineParameters(CString strCommands);
		bool                SetCmdLParameters(TCHAR command, const CString &strParam);
		void                SetupConfiguration(CONF_BKMODEL nConf = CONF_BKMODEL::BK_0010_01);
		bool                ConfigurationConstructor(CONF_BKMODEL nConf, bool bStart = true);
		bool                ConfigurationConstructor_LoadConf(CONF_BKMODEL nConf);
		void                AttachObjects();

		void                UpdateToolbarDriveIcons();
		void                UpdateToolbarSize();
		void                ResetToolbar(UINT uiToolBarId);

		bool                LoadMemoryState(const fs::path &strPath);
		bool                SaveMemoryState(const fs::path &strPath);

		void                SetDebugCtrlsState();

		bool                MakeScreenShot();

		bool                StartPlayTape(const fs::path &strPath);
		void                ClearProcessingFiles();
		bool                ProcessFile(bool bCreate = false);  // bCreate - признак, откуда вызывается функция
		// работа с иконками загрузки/выгрузки образов на панели инструментов
		void                LoadFileImage(UINT nBtnID, FDD_DRIVE eDrive);
		void                ChangeImageIcon(UINT nBtnID, FDD_DRIVE eDrive);

		inline void         StartTimer()
		{
	///		SetTimer(BKTIMER_UI_REFRESH, 20, nullptr);  // запустить таймер для OnMainLoop
	///		SetTimer(BKTIMER_UI_TIME, 1000, nullptr);   // запустить таймер для OnMainLoopTimer
		}
		inline void         StopTimer()
		{
	///		KillTimer(BKTIMER_UI_REFRESH);      // остановить таймер для OnMainLoop
	///		KillTimer(BKTIMER_UI_TIME);         // остановить таймер для OnMainLoopTimer
		}
		void                OnMainLoop();       // функция вызова по таймеру
		void                OnMainLoopTime();   // функция вызова по таймеру
		void                StopAll();          // остановить всё - процессор, BKTIMER_UI_REFRESH
		void                StartAll();         // запустить всё - процессор, BKTIMER_UI_REFRESH
		bool                CheckDebugMemmap();
		CString             MakeUniqueName();
		void                StoreIniParams();
		void                SetAppLook(UINT id);
	public:
		enum class UIMENUS : UINT
		{
			IMGCOLL_MENU,
			MAIN_MENU,
			SOUND_MENU,
			DEBUG_MENU,
			VCAPT_MENU,
			UIMENUS_SIZE
		};

	protected:
		UINT                GetTBID(UIMENUS);
		void				RunDoc(const fs::path &doc);
		void				UpdateStatusFreqPane();
		void				UpdateStatusFreq();
// Переопределение
	public:
	///	virtual BOOL        LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd *pParentWnd = nullptr, CCreateContext *pContext = nullptr) override;
	///	virtual BOOL        OnShowPopupMenu(CMFCPopupMenu *pMenuPopup) override;
	///	virtual BOOL        PreTranslateMessage(MSG *pMsg) override;

// Реализация
	public:
		CMainFrame();
		virtual ~CMainFrame(); /// override;
#ifdef _DEBUG
		virtual void        AssertValid() const override;
		virtual void        Dump(CDumpContext &dc) const override;
#endif
		void                SetFocusToBK();
		void                SetFocusToDebug();

		void                SetScreen(CScreen *scr, CDebugger *dbg)
		{
			m_pScreen = scr;   // задание объекта экранной подсистемы. Он создаётся в другом месте
			m_pDebugger = dbg;
		}
		// выдача указателей на объекты всем желающим
		inline CBkSound     *GetBKSoundPtr()
		{
			return m_pSound.get();
		}
		inline CSpeaker     *GetSpeakerPtr()
		{
			return &m_speaker;
		}
		inline CCovox       *GetCovoxPtr()
		{
			return &m_covox;
		}
		inline CMenestrel   *GetMenestrelPtr()
		{
			return &m_menestrel;
		}
		inline CAYSnd       *GetAYSndPtr()
		{
			return &m_aySnd;
		}
		inline CTape        *GetTapePtr()
		{
			return &m_tape;
		}
		inline CScriptRunner *GetScriptRunnerPtr()
		{
			return &m_Script;
		}
		inline COscillatorlView *GetOscillatorViewPtr()
		{
			return &m_paneOscillatorView;
		}

		inline CBKVKBDView *GetBKVKBDViewPtr()
		{
			return &m_paneBKVKBDView;
		}

		inline CMotherBoard *GetBoard()
		{
			return m_pBoard.get();
		}
		inline CScreen *GetScreen()
		{
			return m_pScreen;
		}

		inline bool isBinFileNameSet()
		{
			return !!(m_cli.nStatus & CLI_KEY_B);
		}
		fs::path GetStrBinFileName()
		{
			fs::path str = m_cli.strBinFileName;
			m_cli.clearBinFName();
			return str;
		}
// Созданные функции схемы сообщений
	protected:
		bool CreateDockingWindows();
		void SetDockingWindowIcons(bool bHiColorIcons);
		afx_msg LRESULT OnMemMapClose(WPARAM, LPARAM);      // событие передаваемое из объекта карты памяти, говорящее, что оно закрывается, и не надо больше его вызывать
		afx_msg LRESULT OnMemDumpUpdate(WPARAM, LPARAM);
		afx_msg LRESULT OnDropFile(WPARAM, LPARAM);
		afx_msg LRESULT OnToolbarCreateNew(WPARAM, LPARAM);
		afx_msg LRESULT OnToolbarReset(WPARAM, LPARAM);
		afx_msg LRESULT OnScreenSizeChanged(WPARAM, LPARAM);
		afx_msg LRESULT OnResetKbdManager(WPARAM wp, LPARAM);
		afx_msg LRESULT OnCpuBreak(WPARAM, LPARAM);
		afx_msg LRESULT OnOutKeyboardStatus(WPARAM, LPARAM);
		afx_msg LRESULT OnStartPlatform(WPARAM, LPARAM);
		afx_msg LRESULT OnOscilloscopeSetBuffer(WPARAM, LPARAM);
		afx_msg LRESULT OnOscilloscopeFillBuffer(WPARAM, LPARAM);
		afx_msg LRESULT OnDebugDrawScreen(WPARAM, LPARAM);
		afx_msg LRESULT OnDrawBKScreen(WPARAM, LPARAM);
		afx_msg LRESULT OnDrawOscilloscope(WPARAM, LPARAM);
		afx_msg LRESULT OnSetCPUFreq(WPARAM, LPARAM);

		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnViewCustomize();
		afx_msg void OnApplicationLook(UINT id);
		afx_msg void OnUpdateApplicationLook(CCmdUI *pCmdUI);
		afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
		afx_msg void OnLVolumeSlider(NMHDR *pNMHDR, LRESULT *pResult);

		afx_msg void OnClose();
	///	afx_msg BOOL OnCopyData(CWnd *pWnd, COPYDATASTRUCT *pCopyDataStruct);
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		afx_msg void OnSetFocus(CWnd *pOldWnd);

// меню Файл
		afx_msg void OnFileLoadstate();
		afx_msg void OnFileSavestate();
		afx_msg void OnFileLoadtape();
		afx_msg void OnUpdateFileLoadtape(CCmdUI *pCmdUI);
		afx_msg void OnFileScreenshot();
// меню Конфигурация
		afx_msg void OnCpuRunbk0010();
		afx_msg void OnUpdateCpuRunbk0010(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk001001();
		afx_msg void OnUpdateCpuRunbk001001(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk001001Focal();
		afx_msg void OnUpdateCpuRunbk001001Focal(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk00100132k();
		afx_msg void OnUpdateCpuRunbk00100132k(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk001001Fdd();
		afx_msg void OnUpdateCpuRunbk001001Fdd(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk001001Fdd16k();
		afx_msg void OnUpdateCpuRunbk001001Fdd16k(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk001001FddSmk512();
		afx_msg void OnUpdateCpuRunbk001001FddSmk512(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk001001FddSamara();
		afx_msg void OnUpdateCpuRunbk001001FddSamara(CCmdUI *pCmdUI);

		afx_msg void OnCpuRunbk0011();
		afx_msg void OnUpdateCpuRunbk0011(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk0011Fdd();
		afx_msg void OnUpdateCpuRunbk0011Fdd(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk0011FddA16m();
		afx_msg void OnUpdateCpuRunbk0011FddA16m(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk0011FddSmk512();
		afx_msg void OnUpdateCpuRunbk0011FddSmk512(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk0011FddSamara();
		afx_msg void OnUpdateCpuRunbk0011FddSamara(CCmdUI *pCmdUI);

		afx_msg void OnCpuRunbk0011m();
		afx_msg void OnUpdateCpuRunbk0011m(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk0011mFDD();
		afx_msg void OnUpdateCpuRunbk0011mFDD(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk0011mFddA16m();
		afx_msg void OnUpdateCpuRunbk0011mFddA16m(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk0011mFddSmk512();
		afx_msg void OnUpdateCpuRunbk0011mFddSmk512(CCmdUI *pCmdUI);
		afx_msg void OnCpuRunbk0011mFddSamara();
		afx_msg void OnUpdateCpuRunbk0011mFddSamara(CCmdUI *pCmdUI);
// меню Управление
		afx_msg void OnCpuResetCpu();
		afx_msg void OnCpuSuResetCpu();
		afx_msg void OnCpuLongReset();

		afx_msg void OnCpuAccelerate();
		afx_msg void OnUpdateCpuAccelerate(CCmdUI *pCmdUI);
		afx_msg void OnCpuSlowdown();
		afx_msg void OnUpdateCpuSlowdown(CCmdUI *pCmdUI);
		afx_msg void OnCpuNormalspeed();
		afx_msg void OnCpuMaxspeed();
		afx_msg void OnUpdateCpuMaxspeed(CCmdUI *pCmdUI);

		afx_msg void OnCmdToggleV100();
		afx_msg void OnCmdToggleV270();

// меню Опции
		afx_msg void OnOptionsEnableSpeaker();
		afx_msg void OnUpdateOptionsEnableSpeaker(CCmdUI *pCmdUI);
		afx_msg void OnOptionsEnableCovox();
		afx_msg void OnUpdateOptionsEnableCovox(CCmdUI *pCmdUI);
		afx_msg void OnOptionsStereoCovox();
		afx_msg void OnUpdateOptionsStereoCovox(CCmdUI *pCmdUI);
		afx_msg void OnOptionsEnableMenestrel();
		afx_msg void OnUpdateOptionsEnableMenestrel(CCmdUI *pCmdUI);
		afx_msg void OnOptionsEnableAy8910();
		afx_msg void OnUpdateOptionsEnableAy8910(CCmdUI *pCmdUI);
		afx_msg void OnOptionsSpeakerFilter();
		afx_msg void OnUpdateOptionsSpeakerFilter(CCmdUI *pCmdUI);
		afx_msg void OnOptionsCovoxFilter();
		afx_msg void OnUpdateOptionsCovoxFilter(CCmdUI *pCmdUI);
		afx_msg void OnOptionsMenestrelFilter();
		afx_msg void OnUpdateOptionsMenestrelFilter(CCmdUI *pCmdUI);
		afx_msg void OnOptionsAy8910Filter();
		afx_msg void OnUpdateOptionsAy8910Filter(CCmdUI *pCmdUI);
		afx_msg void OnOptionsSpeakerDcoffset();
		afx_msg void OnUpdateOptionsSpeakerDcoffset(CCmdUI *pCmdUI);
		afx_msg void OnOptionsCovoxDcoffset();
		afx_msg void OnUpdateOptionsCovoxDcoffset(CCmdUI *pCmdUI);
		afx_msg void OnOptionsMenestrelDcoffset();
		afx_msg void OnUpdateOptionsMenestrelDcoffset(CCmdUI *pCmdUI);
		afx_msg void OnOptionsAy8910Dcoffset();
		afx_msg void OnUpdateOptionsAy8910Dcoffset(CCmdUI *pCmdUI);
		afx_msg void OnOptionsLogAy8910();
		afx_msg void OnUpdateOptionsLogAy8910(CCmdUI *pCmdUI);
		afx_msg void OnOptionsEmulateBkkeyboard();
		afx_msg void OnUpdateOptionsEmulateBkkeyboard(CCmdUI *pCmdUI);
		afx_msg void OnOptionsEnableJoystick();
		afx_msg void OnUpdateOptionsEnableJoystick(CCmdUI *pCmdUI);
		afx_msg void OnOptionsEmulateFddio();
		afx_msg void OnUpdateOptionsEmulateFddio(CCmdUI *pCmdUI);
		afx_msg void OnOptionsUseSavesdirectory();
		afx_msg void OnUpdateOptionsUseSavesdirectory(CCmdUI *pCmdUI);
		afx_msg void OnOptionsEmulateTapeLoading();
		afx_msg void OnUpdateOptionsEmulateTapeLoading(CCmdUI *pCmdUI);
		afx_msg void OnOptionsEmulateTapeSaving();
		afx_msg void OnUpdateOptionsEmulateTapeSaving(CCmdUI *pCmdUI);
		afx_msg void OnOptionsTapemanager();
		afx_msg void OnAppSettings();
		afx_msg void OnPaletteEdit();
		afx_msg void OnOptionsJoyedit();
		afx_msg void OnSettAyvolpan();
		afx_msg void OnOptionsNativeruslatswitch();
		afx_msg void OnUpdateOptionsNativeruslatswitch(CCmdUI *pCmdUI);
// меню Отладка
		afx_msg void OnDebugBreak();
		afx_msg void OnUpdateDebugBreak(CCmdUI *pCmdUI);
		afx_msg void OnDebugStepinto();
		afx_msg void OnUpdateDebugStepinto(CCmdUI *pCmdUI);
		afx_msg void OnDebugStepover();
		afx_msg void OnUpdateDebugStepover(CCmdUI *pCmdUI);
		afx_msg void OnDebugStepout();
		afx_msg void OnUpdateDebugStepout(CCmdUI *pCmdUI);
		afx_msg void OnDebugRuntocursor();
		afx_msg void OnUpdateDebugRuntocursor(CCmdUI *pCmdUI);
		afx_msg void OnDebugBreakpoint();
		afx_msg void OnDebugMemmap();
		afx_msg void OnDebugDumpregsInterval(UINT id);
		afx_msg void OnUpdateDebugDumpregsInterval(CCmdUI *pCmdUI);
		afx_msg void OnDebugDialogAskForBreak();
		afx_msg void OnUpdateDebugDialogAskForBreak(CCmdUI *pCmdUI);
		afx_msg void OnDebugPauseCpuAfterStart();
		afx_msg void OnUpdateDebugPauseCpuAfterStart(CCmdUI *pCmdUI);
		afx_msg void OnDebugEnableIclblock();
		afx_msg void OnUpdateDebugEnableIclblock(CCmdUI *pCmdUI);
// меню Вид
		afx_msg void OnOptionsShowPerformanceOnStatusbar();
		afx_msg void OnUpdateOptionsShowPerformanceOnStatusbar(CCmdUI *pCmdUI);
		afx_msg void OnVkbdtypeKeys(UINT id);
		afx_msg void OnUpdateVkbdtypeKeys(CCmdUI *pCmdUI);
		afx_msg void OnViewSmoothing();
		afx_msg void OnUpdateViewSmoothing(CCmdUI *pCmdUI);
		afx_msg void OnViewFullscreenmode();
		afx_msg void OnViewColormode();
		afx_msg void OnUpdateViewColormode(CCmdUI *pCmdUI);
		afx_msg void OnViewAdaptivebwmode();
		afx_msg void OnUpdateViewAdaptivebwmode(CCmdUI *pCmdUI);
		afx_msg void OnViewLuminoforemode();
		afx_msg void OnUpdateViewLuminoforemode(CCmdUI *pCmdUI);
		afx_msg void OnSetScreenAspect(UINT id);
		afx_msg void OnUpdateSetScreenAspect(CCmdUI *pCmdUI);
		afx_msg void OnSetScreenSize(UINT id);
		afx_msg void OnUpdateSetScreenSize(CCmdUI *pCmdUI);
// меню Инструменты
		afx_msg void OnToolLaunch(UINT id);
// меню справка
		afx_msg void OnQuickStartGuide();
		afx_msg void OnHelpDoc();
		afx_msg void OnHelpDMBK0010();
		afx_msg void OnHelpDMBK0011();
		afx_msg void OnHelpDMBK0011M();
		afx_msg void OnHelpProgBK();
		afx_msg void OnHelpZaltsman();
		afx_msg void OnHelpPCBKKbdLayout();
// тулбар для работы с дискетами и их меню
		afx_msg void OnFileLoadDrive(UINT id);
		afx_msg void OnUpdateFileLoadDrive(CCmdUI *pCmdUI);
		afx_msg void OnFileUnmount(UINT id);
		afx_msg void OnFileOpenInBKDE(UINT id);
// Захват видео
		afx_msg void OnVideoCaptureStart();
		afx_msg void OnUpdateVideoCaptureStart(CCmdUI *pCmdUI);
		afx_msg void OnVideoCaptureStop();
		afx_msg void OnUpdateVideoCaptureStop(CCmdUI *pCmdUI);
		DECLARE_MESSAGE_MAP()
};

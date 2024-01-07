
// MainFrm.cpp : реализация класса CMainFrame
// этот файл стал неприлично большого размера. надо что-то делать.

#include "pch.h"
#include "BK.h"
///#include "ToolManager.h"
#include "MainFrm.h"
#include "Board.h"
#include "Board_10.h"
///#include "Board_MSTD.h"
///#include "Board_EXT32.h"
///#include "Board_10_FDD.h"
///#include "Board_11.h"
///#include "Board_11_FDD.h"
///#include "Board_11M.h"
///#include "Board_11M_FDD.h"

///#include "Screen.h"
///#include "MSFManager.h"
///#include "LoadImgDlg.h"
///#include "LoadMemoryDlg.h"
///#include "LoadTapeDlg.h"
///#include "TapeManagerDlg.h"
///#include "BKSettingsDlg.h"
///#include "BKPaletteDlg.h"
///#include "JoyEditDlg.h"
///#include "BKAYVolPan.h"

///#include "BKMEMDlg.h"
///#include "BKMessageBox.h"
///#include "SliderButton.h"
///#include "MSF.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame
/***
IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

constexpr int  iMaxUserToolbars = 10;
constexpr UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
constexpr UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, &CMainFrame::OnToolbarReset)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)

	ON_WM_SETTINGCHANGE()
	ON_WM_PALETTECHANGED()
	ON_WM_COPYDATA()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_WM_SETFOCUS()

	ON_MESSAGE(WM_RESET_KBD_MANAGER, &CMainFrame::OnResetKbdManager)
	ON_MESSAGE(WM_CPU_DEBUGBREAK, &CMainFrame::OnCpuBreak)
	ON_MESSAGE(WM_MEMMAP_CLOSE, &CMainFrame::OnMemMapClose)
	ON_MESSAGE(WM_MEMDUMP_NEED_UPDATE, &CMainFrame::OnMemDumpUpdate)
	ON_MESSAGE(WM_DROP_TARGET, &CMainFrame::OnDropFile)
	ON_MESSAGE(WM_SCREENSIZE_CHANGED, &CMainFrame::OnScreenSizeChanged)
	ON_MESSAGE(WM_OUTKBDSTATUS, &CMainFrame::OnOutKeyboardStatus)
	ON_MESSAGE(WM_START_PLATFORM, &CMainFrame::OnStartPlatform)

	ON_MESSAGE(WM_SCR_DEBUGDRAW, &CMainFrame::OnDebugDrawScreen)
	ON_MESSAGE(WM_SCR_DRAW, &CMainFrame::OnDrawBKScreen)
	ON_MESSAGE(WM_OSC_DRAW, &CMainFrame::OnDrawOscilloscope)
	ON_MESSAGE(WM_OSC_SETBUFFER, &CMainFrame::OnOscilloscopeSetBuffer)
	ON_MESSAGE(WM_OSC_FILLBUFFER, &CMainFrame::OnOscilloscopeFillBuffer)
	ON_MESSAGE(WM_SETCPUFREQ, &CMainFrame::OnSetCPUFreq)
	////////////// Команды ////////////////////
	ON_NOTIFY(NM_CUSTOMDRAW, ID_OPTIONS_SOUND_VOLUME, &CMainFrame::OnLVolumeSlider) // Momental Change volume
	// файл
	ON_COMMAND(ID_FILE_LOADSTATE, &CMainFrame::OnFileLoadstate)
	ON_COMMAND(ID_FILE_SAVESTATE, &CMainFrame::OnFileSavestate)
	ON_COMMAND(ID_FILE_LOADTAPE, &CMainFrame::OnFileLoadtape)
	ON_COMMAND(ID_FILE_SCREENSHOT, &CMainFrame::OnFileScreenshot)
	ON_COMMAND_RANGE(ID_FILE_LOADDRIVE_A, ID_FILE_LOADDRIVE_D, &CMainFrame::OnFileLoadDrive)
	ON_COMMAND_RANGE(ID_FILE_UMOUNT_A, ID_FILE_UMOUNT_D, &CMainFrame::OnFileUnmount)
	ON_COMMAND_RANGE(ID_FILE_OPENIN_A, ID_FILE_OPENIN_D, &CMainFrame::OnFileOpenInBKDE)
	// Конфигурация
	ON_COMMAND(ID_CPU_RUNBK0010, &CMainFrame::OnCpuRunbk0010)
	ON_COMMAND(ID_CPU_RUNBK001001, &CMainFrame::OnCpuRunbk001001)
	ON_COMMAND(ID_CPU_RUNBK001001_FOCAL, &CMainFrame::OnCpuRunbk001001Focal)
	ON_COMMAND(ID_CPU_RUNBK001001_32K, &CMainFrame::OnCpuRunbk00100132k)
	ON_COMMAND(ID_CPU_RUNBK001001_FDD, &CMainFrame::OnCpuRunbk001001Fdd)
	ON_COMMAND(ID_CPU_RUNBK001001_FDD16K, &CMainFrame::OnCpuRunbk001001Fdd16k)
	ON_COMMAND(ID_CPU_RUNBK001001_FDD_SMK512, &CMainFrame::OnCpuRunbk001001FddSmk512)
	ON_COMMAND(ID_CPU_RUNBK001001_FDD_SAMARA, &CMainFrame::OnCpuRunbk001001FddSamara)
	ON_COMMAND(ID_CPU_RUNBK0011M, &CMainFrame::OnCpuRunbk0011m)
	ON_COMMAND(ID_CPU_RUNBK0011M_FDD, &CMainFrame::OnCpuRunbk0011mFDD)
	ON_COMMAND(ID_CPU_RUNBK0011M_FDD_A16M, &CMainFrame::OnCpuRunbk0011mFddA16m)
	ON_COMMAND(ID_CPU_RUNBK0011M_FDD_SMK512, &CMainFrame::OnCpuRunbk0011mFddSmk512)
	ON_COMMAND(ID_CPU_RUNBK0011M_FDD_SAMARA, &CMainFrame::OnCpuRunbk0011mFddSamara)
	ON_COMMAND(ID_CPU_RUNBK0011, &CMainFrame::OnCpuRunbk0011)
	ON_COMMAND(ID_CPU_RUNBK0011_FDD, &CMainFrame::OnCpuRunbk0011Fdd)
	ON_COMMAND(ID_CPU_RUNBK0011_FDD_A16M, &CMainFrame::OnCpuRunbk0011FddA16m)
	ON_COMMAND(ID_CPU_RUNBK0011_FDD_SMK512, &CMainFrame::OnCpuRunbk0011FddSmk512)
	ON_COMMAND(ID_CPU_RUNBK0011_FDD_SAMARA, &CMainFrame::OnCpuRunbk0011FddSamara)
	// Управление
	ON_COMMAND(ID_CPU_RESETCPU, &CMainFrame::OnCpuResetCpu)
	ON_COMMAND(ID_CPU_SURESETCPU, &CMainFrame::OnCpuSuResetCpu)
	ON_COMMAND(ID_CPU_LONGRESET, &CMainFrame::OnCpuLongReset)
	ON_COMMAND(ID_CPU_ACCELERATE, &CMainFrame::OnCpuAccelerate)
	ON_COMMAND(ID_CPU_SLOWDOWN, &CMainFrame::OnCpuSlowdown)
	ON_COMMAND(ID_CPU_NORMALSPEED, &CMainFrame::OnCpuNormalspeed)
	ON_COMMAND(ID_CPU_MAXSPEED, &CMainFrame::OnCpuMaxspeed)
	ON_COMMAND(ID_CMD_TOGGLE_V100, &CMainFrame::OnCmdToggleV100)
	ON_COMMAND(ID_CMD_TOGGLE_V270, &CMainFrame::OnCmdToggleV270)
	// Опции
	ON_COMMAND(ID_OPTIONS_ENABLE_SPEAKER, &CMainFrame::OnOptionsEnableSpeaker)
	ON_COMMAND(ID_OPTIONS_ENABLE_COVOX, &CMainFrame::OnOptionsEnableCovox)
	ON_COMMAND(ID_OPTIONS_STEREO_COVOX, &CMainFrame::OnOptionsStereoCovox)
	ON_COMMAND(ID_OPTIONS_ENABLE_MENESTREL, &CMainFrame::OnOptionsEnableMenestrel)
	ON_COMMAND(ID_OPTIONS_ENABLE_AY8910, &CMainFrame::OnOptionsEnableAy8910)
	ON_COMMAND(ID_OPTIONS_LOG_AY8910, &CMainFrame::OnOptionsLogAy8910)
	ON_COMMAND(ID_OPTIONS_SPEAKER_FILTER, &CMainFrame::OnOptionsSpeakerFilter)
	ON_COMMAND(ID_OPTIONS_COVOX_FILTER, &CMainFrame::OnOptionsCovoxFilter)
	ON_COMMAND(ID_OPTIONS_MENESTREL_FILTER, &CMainFrame::OnOptionsMenestrelFilter)
	ON_COMMAND(ID_OPTIONS_AY8910_FILTER, &CMainFrame::OnOptionsAy8910Filter)
	ON_COMMAND(ID_OPTIONS_SPEAKER_DCOFFSET, &CMainFrame::OnOptionsSpeakerDcoffset)
	ON_COMMAND(ID_OPTIONS_COVOX_DCOFFSET, &CMainFrame::OnOptionsCovoxDcoffset)
	ON_COMMAND(ID_OPTIONS_MENESTREL_DCOFFSET, &CMainFrame::OnOptionsMenestrelDcoffset)
	ON_COMMAND(ID_OPTIONS_AY8910_DCOFFSET, &CMainFrame::OnOptionsAy8910Dcoffset)
	ON_COMMAND(ID_OPTIONS_EMULATE_BKKEYBOARD, &CMainFrame::OnOptionsEmulateBkkeyboard)
	ON_COMMAND(ID_OPTIONS_ENABLE_JOYSTICK, &CMainFrame::OnOptionsEnableJoystick)
	ON_COMMAND(ID_OPTIONS_USE_SAVESDIRECTORY, &CMainFrame::OnOptionsUseSavesdirectory)
	ON_COMMAND(ID_OPTIONS_EMULATE_TAPE_LOADING, &CMainFrame::OnOptionsEmulateTapeLoading)
	ON_COMMAND(ID_OPTIONS_EMULATE_TAPE_SAVING, &CMainFrame::OnOptionsEmulateTapeSaving)
	ON_COMMAND(ID_OPTIONS_SHOWPERFORMANCEONSTATUSBAR, &CMainFrame::OnOptionsShowPerformanceOnStatusbar)
	ON_COMMAND(ID_OPTIONS_EMULATE_FDDIO, &CMainFrame::OnOptionsEmulateFddio)
	ON_COMMAND(ID_OPTIONS_TAPEMANAGER, &CMainFrame::OnOptionsTapemanager)
	ON_COMMAND(ID_APP_SETTINGS, &CMainFrame::OnAppSettings)
	ON_COMMAND(ID_OPTIONS_PALETTE, &CMainFrame::OnPaletteEdit)
	ON_COMMAND(ID_OPTIONS_JOYEDIT, &CMainFrame::OnOptionsJoyedit)
	ON_COMMAND(ID_OPTIONS_AYVOLPAN, &CMainFrame::OnSettAyvolpan)
	ON_COMMAND(ID_OPTIONS_NATIVERUSLATSWITCH, &CMainFrame::OnOptionsNativeruslatswitch)
	// Отладка
	ON_COMMAND(ID_DEBUG_STARTBREAK, &CMainFrame::OnDebugBreak)
	ON_COMMAND(ID_DEBUG_STEPINTO, &CMainFrame::OnDebugStepinto)
	ON_COMMAND(ID_DEBUG_STEPOVER, &CMainFrame::OnDebugStepover)
	ON_COMMAND(ID_DEBUG_STEPOUT, &CMainFrame::OnDebugStepout)
	ON_COMMAND(ID_DEBUG_RUNTOCURSOR, &CMainFrame::OnDebugRuntocursor)
	ON_COMMAND(ID_DEBUG_BREAKPOINT, &CMainFrame::OnDebugBreakpoint)
	ON_COMMAND(ID_DEBUG_DIALOG_ASK_FOR_BREAK, &CMainFrame::OnDebugDialogAskForBreak)
	ON_COMMAND(ID_DEBUG_PAUSE_CPU_AFTER_START, &CMainFrame::OnDebugPauseCpuAfterStart)
	ON_COMMAND_RANGE(ID_DEBUG_DUMPREGS_INTERVAL_0, ID_DEBUG_DUMPREGS_INTERVAL_50, &CMainFrame::OnDebugDumpregsInterval)
	ON_COMMAND(ID_DEBUG_MEMMAP, &CMainFrame::OnDebugMemmap)
	ON_COMMAND(ID_DEBUG_ENABLE_ICLBLOCK, &CMainFrame::OnDebugEnableIclblock)
	// Вид
	ON_COMMAND_RANGE(ID_VKBDTYPE_KEYS, ID_VKBDTYPE_MEMBRANE, &CMainFrame::OnVkbdtypeKeys)
	ON_COMMAND(ID_VIEW_SMOOTHING, &CMainFrame::OnViewSmoothing)
	ON_COMMAND(ID_VIEW_COLORMODE, &CMainFrame::OnViewColormode)
	ON_COMMAND(ID_VIEW_FULLSCREENMODE, &CMainFrame::OnViewFullscreenmode)
	ON_COMMAND(ID_VIEW_ADAPTIVEBWMODE, &CMainFrame::OnViewAdaptivebwmode)
	ON_COMMAND(ID_VIEW_LUMINOFOREMODE, &CMainFrame::OnViewLuminoforemode)
	ON_COMMAND_RANGE(ID_VIEW_ASPECT1X2, ID_VIEW_ASPECT5X4, &CMainFrame::OnSetScreenAspect)
	ON_COMMAND_RANGE(ID_VIEW_SCREENSIZE_CUSTOM, ID_VIEW_SCREENSIZE_2048X1536, &CMainFrame::OnSetScreenSize)
	// Инструменты
	ON_COMMAND_RANGE(ID_TOOL_MENU_0, ID_TOOL_MENU_9, &CMainFrame::OnToolLaunch)
	// Справка
	ON_COMMAND(ID_HELP_QUICKSTARTGUIDE, &CMainFrame::OnQuickStartGuide)
	ON_COMMAND(ID_HELP_DOC, &CMainFrame::OnHelpDoc)
	ON_COMMAND(ID_HELP_DMBK0010, &CMainFrame::OnHelpDMBK0010)
	ON_COMMAND(ID_HELP_DMBK0011, &CMainFrame::OnHelpDMBK0011)
	ON_COMMAND(ID_HELP_DMBK0011M, &CMainFrame::OnHelpDMBK0011M)
	ON_COMMAND(ID_HELP_PROGBK, &CMainFrame::OnHelpProgBK)
	ON_COMMAND(ID_HELP_ZALTSMANBOOK, &CMainFrame::OnHelpZaltsman)
	ON_COMMAND(ID_HELP_EMUKBDLAYOUT, &CMainFrame::OnHelpPCBKKbdLayout)
	// Захват видео
	ON_COMMAND(ID_VCAPT_START, &CMainFrame::OnVideoCaptureStart)
	ON_COMMAND(ID_VCAPT_STOP, &CMainFrame::OnVideoCaptureStop)
	////////////// Интерфейс меню ////////////////////
	// файл
	ON_UPDATE_COMMAND_UI(ID_FILE_LOADTAPE, &CMainFrame::OnUpdateFileLoadtape)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_LOADDRIVE_A, ID_FILE_LOADDRIVE_D, &CMainFrame::OnUpdateFileLoadDrive)
	// Конфигурация
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0010, &CMainFrame::OnUpdateCpuRunbk0010)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK001001, &CMainFrame::OnUpdateCpuRunbk001001)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK001001_FOCAL, &CMainFrame::OnUpdateCpuRunbk001001Focal)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK001001_32K, &CMainFrame::OnUpdateCpuRunbk00100132k)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK001001_FDD, &CMainFrame::OnUpdateCpuRunbk001001Fdd)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK001001_FDD16K, &CMainFrame::OnUpdateCpuRunbk001001Fdd16k)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK001001_FDD_SMK512, &CMainFrame::OnUpdateCpuRunbk001001FddSmk512)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK001001_FDD_SAMARA, &CMainFrame::OnUpdateCpuRunbk001001FddSamara)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0011M, &CMainFrame::OnUpdateCpuRunbk0011m)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0011M_FDD, &CMainFrame::OnUpdateCpuRunbk0011mFDD)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0011M_FDD_A16M, &CMainFrame::OnUpdateCpuRunbk0011mFddA16m)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0011M_FDD_SMK512, &CMainFrame::OnUpdateCpuRunbk0011mFddSmk512)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0011M_FDD_SAMARA, &CMainFrame::OnUpdateCpuRunbk0011mFddSamara)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0011, &CMainFrame::OnUpdateCpuRunbk0011)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0011_FDD, &CMainFrame::OnUpdateCpuRunbk0011Fdd)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0011_FDD_A16M, &CMainFrame::OnUpdateCpuRunbk0011FddA16m)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0011_FDD_SMK512, &CMainFrame::OnUpdateCpuRunbk0011FddSmk512)
	ON_UPDATE_COMMAND_UI(ID_CPU_RUNBK0011_FDD_SAMARA, &CMainFrame::OnUpdateCpuRunbk0011FddSamara)
	// Управление
	ON_UPDATE_COMMAND_UI(ID_CPU_ACCELERATE, &CMainFrame::OnUpdateCpuAccelerate)
	ON_UPDATE_COMMAND_UI(ID_CPU_SLOWDOWN, &CMainFrame::OnUpdateCpuSlowdown)
	ON_UPDATE_COMMAND_UI(ID_CPU_MAXSPEED, &CMainFrame::OnUpdateCpuMaxspeed)
	// Опции
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_ENABLE_SPEAKER, &CMainFrame::OnUpdateOptionsEnableSpeaker)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_ENABLE_COVOX, &CMainFrame::OnUpdateOptionsEnableCovox)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_STEREO_COVOX, &CMainFrame::OnUpdateOptionsStereoCovox)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_ENABLE_MENESTREL, &CMainFrame::OnUpdateOptionsEnableMenestrel)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_ENABLE_AY8910, &CMainFrame::OnUpdateOptionsEnableAy8910)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_LOG_AY8910, &CMainFrame::OnUpdateOptionsLogAy8910)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_SPEAKER_FILTER, &CMainFrame::OnUpdateOptionsSpeakerFilter)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_COVOX_FILTER, &CMainFrame::OnUpdateOptionsCovoxFilter)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_MENESTREL_FILTER, &CMainFrame::OnUpdateOptionsMenestrelFilter)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_AY8910_FILTER, &CMainFrame::OnUpdateOptionsAy8910Filter)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_SPEAKER_DCOFFSET, &CMainFrame::OnUpdateOptionsSpeakerDcoffset)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_COVOX_DCOFFSET, &CMainFrame::OnUpdateOptionsCovoxDcoffset)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_MENESTREL_DCOFFSET, &CMainFrame::OnUpdateOptionsMenestrelDcoffset)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_AY8910_DCOFFSET, &CMainFrame::OnUpdateOptionsAy8910Dcoffset)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_EMULATE_BKKEYBOARD, &CMainFrame::OnUpdateOptionsEmulateBkkeyboard)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_ENABLE_JOYSTICK, &CMainFrame::OnUpdateOptionsEnableJoystick)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_USE_SAVESDIRECTORY, &CMainFrame::OnUpdateOptionsUseSavesdirectory)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_EMULATE_TAPE_LOADING, &CMainFrame::OnUpdateOptionsEmulateTapeLoading)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_EMULATE_TAPE_SAVING, &CMainFrame::OnUpdateOptionsEmulateTapeSaving)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_SHOWPERFORMANCEONSTATUSBAR, &CMainFrame::OnUpdateOptionsShowPerformanceOnStatusbar)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_EMULATE_FDDIO, &CMainFrame::OnUpdateOptionsEmulateFddio)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_NATIVERUSLATSWITCH, &CMainFrame::OnUpdateOptionsNativeruslatswitch)
	// Отладка
	ON_UPDATE_COMMAND_UI(ID_DEBUG_STARTBREAK, &CMainFrame::OnUpdateDebugBreak)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_STEPINTO, &CMainFrame::OnUpdateDebugStepinto)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_STEPOVER, &CMainFrame::OnUpdateDebugStepover)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_STEPOUT, &CMainFrame::OnUpdateDebugStepout)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_RUNTOCURSOR, &CMainFrame::OnUpdateDebugRuntocursor)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_DIALOG_ASK_FOR_BREAK, &CMainFrame::OnUpdateDebugDialogAskForBreak)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_PAUSE_CPU_AFTER_START, &CMainFrame::OnUpdateDebugPauseCpuAfterStart)
	ON_UPDATE_COMMAND_UI_RANGE(ID_DEBUG_DUMPREGS_INTERVAL_0, ID_DEBUG_DUMPREGS_INTERVAL_50, &CMainFrame::OnUpdateDebugDumpregsInterval)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_ENABLE_ICLBLOCK, &CMainFrame::OnUpdateDebugEnableIclblock)
	// Вид
	ON_UPDATE_COMMAND_UI_RANGE(ID_VKBDTYPE_KEYS, ID_VKBDTYPE_MEMBRANE, &CMainFrame::OnUpdateVkbdtypeKeys)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SMOOTHING, &CMainFrame::OnUpdateViewSmoothing)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COLORMODE, &CMainFrame::OnUpdateViewColormode)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ADAPTIVEBWMODE, &CMainFrame::OnUpdateViewAdaptivebwmode)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LUMINOFOREMODE, &CMainFrame::OnUpdateViewLuminoforemode)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_ASPECT1X2, ID_VIEW_ASPECTCUSTOM, &CMainFrame::OnUpdateSetScreenAspect)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_SCREENSIZE_CUSTOM, ID_VIEW_SCREENSIZE_2048X1536, &CMainFrame::OnUpdateSetScreenSize)
	// Захват видео
	ON_UPDATE_COMMAND_UI(ID_VCAPT_START, &CMainFrame::OnUpdateVideoCaptureStart)
	ON_UPDATE_COMMAND_UI(ID_VCAPT_STOP, &CMainFrame::OnUpdateVideoCaptureStop)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // индикатор строки состояния
	ID_SEPARATOR,           // тут выводится ЛАТ/РУС
	ID_SEPARATOR,           // тут выводится ЗАГЛ/СТР
	ID_SEPARATOR,           // тут выводится АР2
	ID_SEPARATOR,           // тут выводится СУ
	ID_SEPARATOR,           // тут выводится разрешение экрана
	ID_SEPARATOR,           // тут выводится частота процессора
	ID_SEPARATOR,           // тут выводится всякая общая инфа
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};
**/
// создание/уничтожение CMainFrame

CMainFrame::CMainFrame()
	: m_pBKMemView(nullptr)
	, m_pBoard(nullptr)
	, m_pSound(nullptr)
	, m_pDebugger(nullptr)
	, m_pScreen(nullptr)
	, m_rectMemMap(CRect(0, 0, 0, 0))
	, m_nRegsDumpCounter(0)
	, m_bLongResetPress(false)
	, m_bBeginPeriod(false)
	, m_nScreen_X(512)
	, m_nScreen_CustomX(512)
	, m_nScrSizeNum(SCREENSIZE_CUSTOM)
	, m_bBKMemViewOpen(false)
	, m_bFoundFFMPEG(false)
{
///	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2008);
///	m_bBeginPeriod = (timeBeginPeriod(1) == TIMERR_NOERROR);
	m_nStartTick = GetTickCount();
//  m_nInterAppToolGlobalMessage = ::RegisterGlobalMessage();
	// Инициализация звуковой подсистемы
	m_pSound = std::make_unique<CBkSound>();

	if (!m_pSound)
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}

CMainFrame::~CMainFrame()
{
	if (m_bBeginPeriod)
	{
	///	timeEndPeriod(1);
	}

///	m_wndToolBar.CleanUpImages();
///	m_wndToolBarSound.CleanUpImages();
///	m_wndToolBarDebug.CleanUpImages();
///	m_wndToolBarVCapt.CleanUpImages();
///	GetCmdMgr()->CleanUp();
}


struct ButtonIDs
{
	UINT nBigHiCol;
	UINT nBigLoCol;
	UINT nStdHiCol;
	UINT nStdLoCol;
};

UINT CMainFrame::GetTBID(UIMENUS m)
{
	static const ButtonIDs aUIBtn[static_cast<UINT>(UIMENUS::UIMENUS_SIZE)] =
	{
		{ IDR_MENU_IMAGES_256_2,    IDR_MENU_IMAGES_2,  IDR_MENU_IMAGES_256,  IDR_MENU_IMAGES,  },
		{ IDR_MAINFRAME_256_2,      IDR_MAINFRAME_2,    IDR_MAINFRAME_256,    IDR_MAINFRAME,    },
		{ IDR_SOUNDTOOLBAR_256_2,   IDR_SOUNDTOOLBAR_2, IDR_SOUNDTOOLBAR_256, IDR_SOUNDTOOLBAR, },
		{ IDR_DEBUGTOOLBAR_256_2,   IDR_DEBUGTOOLBAR_2, IDR_DEBUGTOOLBAR_256, IDR_DEBUGTOOLBAR, },
		{ IDR_VCAPTTOOLBAR_256_2,   IDR_VCAPTTOOLBAR_2, IDR_VCAPTTOOLBAR_256, IDR_VCAPTTOOLBAR  }
	};
	auto n = static_cast<UINT>(m);
	return theApp.m_bHiColorIcons ? (g_Config.m_bBigButtons ? aUIBtn[n].nBigHiCol : aUIBtn[n].nStdHiCol) : (g_Config.m_bBigButtons ? aUIBtn[n].nBigLoCol : aUIBtn[n].nStdLoCol);
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{ /***
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
	{
		ASSERT(false);
		return -1;
	}

	g_Config.m_bSysCapsStatus = !!(GetKeyState(VK_CAPITAL) & 1); // сохраним состояние клавиши Капслок

	CBasePane::m_bMultiThreaded = TRUE;
	// подменим заголовок окна, добавим к нему версию
	m_strTitle = theApp.m_strProgramTitleVersion;
	BOOL bNameValid;

	if (!m_wndMenuBar.Create(this))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFMENUBARERROR, MB_OK);
		TRACE0("Не удалось создать строку меню.\n");
		return -1;      // не удалось создать
	}

	// включить быструю (Alt+перетаскивание) настройку панелей инструментов
	CMFCToolBar::EnableQuickCustomization();
//  if (CMFCToolBar::GetUserImages() == nullptr)
//  {
//      // загрузить изображения пользовательских панелей инструментов
//      if (m_UserImages.Load(_T(".\\UserImages.bmp")))
//      {
//          CMFCToolBar::SetUserImages(&m_UserImages);
//      }
//  }
	/*
	IDR_MENU_IMAGES_256 - ресурс тулбара с картинками, который добавляется в коллекцию картинок, для того
	чтобы можно было по ID картинок данного тулбара менять картинки на кнопках видимых тулбаров.
	сам CMFCToolBar на основе данного тулбара не создаётся, У каждой картинки должен быть свой, уникальный ID
	* /
	CMFCToolBar::AddToolBarForImageCollection(GetTBID(UIMENUS::IMGCOLL_MENU));
	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);
	::FillTools(&m_wndMenuBar);
	// предотвращение фокусировки строки меню на активации
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
	        !m_wndToolBar.LoadToolBar(GetTBID(UIMENUS::MAIN_MENU)))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFSTDTOOLBARERROR, MB_OK);
		TRACE0("Не удалось создать стандартную панель инструментов.\n");
		return -1;      // не удалось создать
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	if (!m_wndToolBarSound.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
	                                CRect(1, 1, 1, 1), IDW_SOUND_TOOLBAR) ||
	        !m_wndToolBarSound.LoadToolBar(GetTBID(UIMENUS::SOUND_MENU)))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFSNDTOOLBARERROR, MB_OK);
		TRACE0("Не удалось создать звуковую панель инструментов.\n");
		return -1;      // не удалось создать
	}

	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_SOUND);
	ASSERT(bNameValid);
	m_wndToolBarSound.SetWindowText(strToolBarName);
	m_wndToolBarSound.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	if (!m_wndToolBarDebug.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
	                                CRect(1, 1, 1, 1), IDW_DEBUG_TOOLBAR) ||
	        !m_wndToolBarDebug.LoadToolBar(GetTBID(UIMENUS::DEBUG_MENU)))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFDBGTOOLBARERROR, MB_OK);
		TRACE0("Не удалось создать панель инструментов отладки.\n");
		return -1;      // не удалось создать
	}

	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_DEBUG);
	ASSERT(bNameValid);
	m_wndToolBarDebug.SetWindowText(strToolBarName);
	m_wndToolBarDebug.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	if (!m_wndToolBarVCapt.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
	                                CRect(1, 1, 1, 1), IDW_VCAPT_TOOLBAR) ||
	        !m_wndToolBarVCapt.LoadToolBar(GetTBID(UIMENUS::VCAPT_MENU)))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFVCAPTOOLBARERROR, MB_OK);
		TRACE0("Не удалось создать панель инструментов захвата видео.\n");
		return -1;      // не удалось создать
	}

	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_VCAPT);
	ASSERT(bNameValid);
	m_wndToolBarVCapt.SetWindowText(strToolBarName);
	m_wndToolBarVCapt.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	// Разрешить операции с пользовательскими панелями инструментов:
	InitUserToolbars(nullptr, uiFirstUserToolBarId, uiLastUserToolBarId);
	// Настройки всех тулбаров: высота
	UpdateToolbarSize();

	if (!m_wndStatusBar.Create(this))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFSTATUSBARERROR, MB_OK);
		TRACE0("Не удалось создать строку состояния.\n");
		return -1;      // не удалось создать
	}

	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_wndStatusBar.SetPaneWidth(static_cast<int>(STATUS_FIELD::KBD_XLAT), ::MulDiv(25, nPixelW, DEFAULT_DPIX));
	m_wndStatusBar.SetPaneWidth(static_cast<int>(STATUS_FIELD::KBD_CAPS), ::MulDiv(35, nPixelW, DEFAULT_DPIX));
	m_wndStatusBar.SetPaneWidth(static_cast<int>(STATUS_FIELD::KBD_AR2), ::MulDiv(25, nPixelW, DEFAULT_DPIX));
	m_wndStatusBar.SetPaneWidth(static_cast<int>(STATUS_FIELD::KBD_SU), ::MulDiv(25, nPixelW, DEFAULT_DPIX));
	m_wndStatusBar.SetPaneWidth(static_cast<int>(STATUS_FIELD::SCR_REZ), ::MulDiv(92, nPixelW, DEFAULT_DPIX));
	m_wndStatusBar.SetPaneWidth(static_cast<int>(STATUS_FIELD::CPU_FRQ), ::MulDiv(100, nPixelW, DEFAULT_DPIX));
	m_wndStatusBar.SetPaneWidth(static_cast<int>(STATUS_FIELD::INFO), ::MulDiv(136, nPixelW, DEFAULT_DPIX));
	m_wndStatusBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBarSound.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBarDebug.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBarVCapt.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);
	DockPane(&m_wndToolBarVCapt);
	DockPaneLeftOf(&m_wndToolBarDebug, &m_wndToolBarVCapt);
	DockPaneLeftOf(&m_wndToolBarSound, &m_wndToolBarDebug);
	DockPaneLeftOf(&m_wndToolBar, &m_wndToolBarSound);
	// включить режим работы закрепляемых окон стилей Visual Studio 2005
	CDockingManager::SetDockingMode(DT_SMART);
	// включить режим работы автоматического скрытия закрепляемых окон стилей Visual Studio 2005
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// создать закрепляемые окна
	if (!CreateDockingWindows())
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFDOCKINGBARERROR, MB_OK);
		TRACE0("Не удалось создать закрепляемые окна.\n");
		return -1;
	}

	m_paneDisassembleView.EnableDocking(CBRS_ALIGN_ANY);

	for (auto &pane : m_arPaneMemoryDumpView)
	{
		pane.EnableDocking(CBRS_ALIGN_ANY);
	}

	m_paneBKVKBDView.EnableDocking(CBRS_ALIGN_ANY);
	m_paneOscillatorView.EnableDocking(CBRS_ALIGN_ANY);
	m_paneRegistryDumpViewCPU.EnableDocking(CBRS_ALIGN_ANY);
	m_paneRegistryDumpViewFDD.EnableDocking(CBRS_ALIGN_ANY);
	m_paneTapeCtrlView.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_paneDisassembleView);

	for (auto &pane : m_arPaneMemoryDumpView)
	{
		DockPane(&pane);
	}

	DockPane(&m_paneBKVKBDView);
	DockPane(&m_paneOscillatorView);
	DockPane(&m_paneRegistryDumpViewCPU);
	DockPane(&m_paneRegistryDumpViewFDD);
	// для этого панели m_DisassembleView и m_memoryDumpView обязательно должны быть видимые.
	// иначе, m_memoryDumpView не может придокиться к m_DisassembleView и остаётся висеть в воздухе, так что даже перемещать её нельзя.
	m_arPaneMemoryDumpView[0].DockToWindow(&m_paneDisassembleView, CBRS_ALIGN_BOTTOM);
	m_paneBKVKBDView.AttachToTabWnd(&m_paneOscillatorView, DM_SHOW, FALSE, nullptr);
	m_paneTapeCtrlView.DockToWindow(&m_paneRegistryDumpViewCPU, CBRS_ALIGN_BOTTOM);

	for (int i = 1; i < NUMBER_VIEWS_MEM_DUMP; ++i)
	{
		m_arPaneMemoryDumpView[i].AttachToTabWnd(&m_arPaneMemoryDumpView[i - 1], DM_UNKNOWN, FALSE, nullptr);
		m_arPaneMemoryDumpView[i].ShowPane(FALSE, FALSE, FALSE);
	}

	m_paneRegistryDumpViewFDD.AttachToTabWnd(&m_arPaneMemoryDumpView[NUMBER_VIEWS_MEM_DUMP - 1], DM_SHOW, FALSE, nullptr);
	// теперь присоединяем отладчик к панелям
	m_paneRegistryDumpViewCPU.AttachDebugger(m_pDebugger);
	m_paneRegistryDumpViewFDD.AttachDebugger(m_pDebugger);

	for (auto &pane : m_arPaneMemoryDumpView)
	{
		pane.AttachDebugger(m_pDebugger);
	}

	m_paneDisassembleView.AttachDebugger(m_pDebugger);
	m_dropTarget.Register(this);
	// Включить функцию замены меню панелей инструментов и закрепляемых окон
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);
	// установите наглядный диспетчер и стиль на основе постоянного значения
	SetAppLook(theApp.m_nAppLook);
	InitRegistry(); // регистрируем типы и расширения, чтоб по клику на файлах прога запускалась
	m_bFoundFFMPEG = Global::CheckFFMPEG(); ***/
	return 0;
}


LRESULT CMainFrame::OnToolbarReset(WPARAM wp, LPARAM)
{
	// Настройки всех тулбаров: высота
	UpdateToolbarSize();
	// Сброс тулбаров через меню Настройки тулбаров
	ResetToolbar(UINT(wp));
	return S_OK;
}

void CMainFrame::ResetToolbar(UINT uiToolBarId)
{ /***
	if (uiToolBarId == GetTBID(UIMENUS::SOUND_MENU))
	{
		CSliderButton btnSlider(ID_OPTIONS_SOUND_VOLUME, -1, TBS_TRANSPARENTBKGND);
		btnSlider.SetRange(0, 0xffff);
//      if (g_Config.m_bBigButtons) {
//          // Поменяем высоту контрола Слайдера на лету
//          // CRect rectok, wind;
//      }
		m_wndToolBarSound.ReplaceButton(ID_OPTIONS_SOUND_VOLUME, btnSlider);
		btnSlider.SetValue(g_Config.m_nSoundVolume);
	}
	else if (uiToolBarId == GetTBID(UIMENUS::MAIN_MENU))
	{
		// Шестипалов просит добавить прерывание по 270 вектору. Поэтому делаем драпдауну
		CMenu menuIRQS;     // Прерывание по вектору 100
		VERIFY(menuIRQS.LoadMenu(IDR_IRQS));
		CMenu *pSubMenu = menuIRQS.GetSubMenu(0);
		CMFCToolBarMenuButton btnBackIRQS(ID_CMD_TOGGLE_V100, pSubMenu->GetSafeHmenu(), -1);
		m_wndToolBar.ReplaceButton(ID_CMD_TOGGLE_V100, btnBackIRQS);
		// создаём кнопки с выпадающими меню. 4 привода - 4 разных кнопки с 4мя разными-одинаковыми меню
		CMenu menuA;
		VERIFY(menuA.LoadMenu(IDR_LOADDRIVE_MENU_A));
		pSubMenu = menuA.GetSubMenu(0);
		CMFCToolBarMenuButton btnBackA(ID_FILE_LOADDRIVE_A, pSubMenu->GetSafeHmenu(), -1);
		m_wndToolBar.ReplaceButton(ID_FILE_LOADDRIVE_A, btnBackA);
		CMenu menuB;
		VERIFY(menuB.LoadMenu(IDR_LOADDRIVE_MENU_B));
		pSubMenu = menuB.GetSubMenu(0);
		CMFCToolBarMenuButton btnBackB(ID_FILE_LOADDRIVE_B, pSubMenu->GetSafeHmenu(), -1);
		m_wndToolBar.ReplaceButton(ID_FILE_LOADDRIVE_B, btnBackB);
		CMenu menuC;
		VERIFY(menuC.LoadMenu(IDR_LOADDRIVE_MENU_C));
		pSubMenu = menuC.GetSubMenu(0);
		CMFCToolBarMenuButton btnBackC(ID_FILE_LOADDRIVE_C, pSubMenu->GetSafeHmenu(), -1);
		m_wndToolBar.ReplaceButton(ID_FILE_LOADDRIVE_C, btnBackC);
		CMenu menuD;
		VERIFY(menuD.LoadMenu(IDR_LOADDRIVE_MENU_D));
		pSubMenu = menuD.GetSubMenu(0);
		CMFCToolBarMenuButton btnBackD(ID_FILE_LOADDRIVE_D, pSubMenu->GetSafeHmenu(), -1);
		m_wndToolBar.ReplaceButton(ID_FILE_LOADDRIVE_D, btnBackD);
		// И тут-же обновим иконки дисков
		UpdateToolbarDriveIcons();
	} **/
}

// наглядно отобразим, что и в каком дисководе находится
void CMainFrame::UpdateToolbarDriveIcons()
{
	ChangeImageIcon(ID_FILE_LOADDRIVE_A, FDD_DRIVE::A);
	ChangeImageIcon(ID_FILE_LOADDRIVE_B, FDD_DRIVE::B);
	ChangeImageIcon(ID_FILE_LOADDRIVE_C, FDD_DRIVE::C);
	ChangeImageIcon(ID_FILE_LOADDRIVE_D, FDD_DRIVE::D);
}

// Эта же функция может быть вызвана при изменении размеров иконок кнопкой
void CMainFrame::UpdateToolbarSize()
{ /***
	int nSzBtnX = 20;
	int nSzBtnY = 20;
	int nSzImgX = 16;
	int nSzImgY = 15;

	if (g_Config.m_bBigButtons)
	{
		nSzBtnX = 28;
		nSzBtnY = 28;
		nSzImgX = 24;
		nSzImgY = 23;
	}

	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	int nPixelH = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);
	CSize szBtn = CSize(::MulDiv(nSzBtnX, nPixelW, DEFAULT_DPIX), ::MulDiv(nSzBtnY, nPixelH, DEFAULT_DPIY));
	CSize szImg = CSize(nSzImgX, nSzImgY);
	m_wndToolBar.SetSizes(szBtn, szImg);
	m_wndToolBarSound.SetSizes(szBtn, szImg);
	m_wndToolBarDebug.SetSizes(szBtn, szImg);
	m_wndToolBarVCapt.SetSizes(szBtn, szImg); ***/
}


void CMainFrame::OnLVolumeSlider(NMHDR *pNMHDR, LRESULT *pResult)
{ /***
	auto pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);

	if (pNMCD->dwDrawStage == CDDS_PREPAINT)
	{
		auto pSlider = DYNAMIC_DOWNCAST(CSliderButton, m_wndToolBarSound.GetButton(m_wndToolBarSound.CommandToIndex(ID_OPTIONS_SOUND_VOLUME)));

		if (pSlider)
		{
			g_Config.m_nSoundVolume = pSlider->GetValue();
		}

		if (m_pSound)
		{
			m_pSound->SoundGen_SetVolume(g_Config.m_nSoundVolume);
		}
	}
***/
	*pResult = S_OK;
}

bool CMainFrame::CreateDockingWindows()
{ /***
	// Создать окно дампа регистров
	CString strWndName;
	BOOL bNameValid = strWndName.LoadString(IDS_WND_REGDUMP_CPU);
	ASSERT(bNameValid);

	if (!m_paneRegistryDumpViewCPU.Create(strWndName, this, TRUE, MAKEINTRESOURCE(IDD_REGDUMP_CPU_DLG), WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI, ID_VIEW_REGDUMP_CPU))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFDOCKREGDUMPVIEWERROR, MB_OK);
		TRACE1("Не удалось создать окно \"%s\"\n", strWndName);
		return false; // не удалось создать
	}

	bNameValid = strWndName.LoadString(IDS_WND_REGDUMP_FDD);
	ASSERT(bNameValid);

	if (!m_paneRegistryDumpViewFDD.Create(strWndName, this, TRUE, MAKEINTRESOURCE(IDD_REGDUMP_FDD_DLG), WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI, ID_VIEW_REGDUMP_FDD))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFDOCKREGDUMPVIEWERROR, MB_OK);
		TRACE1("Не удалось создать окно \"%s\"\n", strWndName);
		return false; // не удалось создать
	}

	// Создать окно управления лентами
	bNameValid = strWndName.LoadString(IDS_WND_TAPECTRL);
	ASSERT(bNameValid);

	if (!m_paneTapeCtrlView.Create(strWndName, this, TRUE, MAKEINTRESOURCE(IDD_TAPE_CONTROL), WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI, ID_VIEW_TAPECTRLWND))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFDOCKTAPECTRLVIEWERROR, MB_OK);
		TRACE1("Не удалось создать окно \"%s\"\n", strWndName);
		return false; // не удалось создать
	}

	// Создать окно отладка
	bNameValid = strWndName.LoadString(IDS_WND_DEBUG);
	ASSERT(bNameValid);

	if (!m_paneDisassembleView.Create(strWndName, this, TRUE, MAKEINTRESOURCE(IDD_BKDISASM_DLG), WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI, ID_VIEW_DEBUGWND))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFDOCKDEBUGVIEWERROR, MB_OK);
		TRACE1("Не удалось создать окно \"%s\"\n", strWndName);
		return false; // не удалось создать
	}

	bNameValid = strWndName.LoadString(IDS_WND_MEMDUMP);
	ASSERT(bNameValid);
	// массив IDов окошек дампов памяти, их надо в ресурсах прописывать
	static UINT arViewMemDumpWndIDs[NUMBER_VIEWS_MEM_DUMP] =
	{
		ID_VIEW_MEMDUMP_0, ID_VIEW_MEMDUMP_1, ID_VIEW_MEMDUMP_2, ID_VIEW_MEMDUMP_3
	};

	for (int i = 0; i < NUMBER_VIEWS_MEM_DUMP; ++i)
	{
		CString s;
		s.Format(_T(" #%d"), i);
		UINT nStyle = i ? 0 : WS_VISIBLE;

		if (!m_arPaneMemoryDumpView[i].Create(strWndName + s, this, TRUE, MAKEINTRESOURCE(IDD_MEMDUMP_DLG), nStyle | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI, arViewMemDumpWndIDs[i]))
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_MFDOCKMEMDUMPVIEWERROR, MB_OK);
			TRACE1("Не удалось создать окно \"%s\"\n", strWndName);
			return false; // не удалось создать
		}
	}

	// Создать окно осциллографа
	bNameValid = strWndName.LoadString(IDS_WND_OSCILLATOR);
	ASSERT(bNameValid);

	if (!m_paneOscillatorView.Create(strWndName, this, CRect(0, 0, 400, 200), TRUE, ID_VIEW_OSCILLATOR, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFDOCKOSCVIEWERROR, MB_OK);
		TRACE1("Не удалось создать окно \"%s\"\n", strWndName);
		return false; // не удалось создать
	}

	// Создать окно виртуальной клавиатуры
	bNameValid = strWndName.LoadString(IDS_WND_VKBD);

	if (!m_paneBKVKBDView.Create(strWndName, this, CRect(0, 0, 700, 200), TRUE, ID_VIEW_VKBD, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_HIDE_INPLACE | CBRS_BOTTOM | CBRS_FLOAT_MULTI))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MFDOCKTAPECTRLVIEWERROR, MB_OK);
		TRACE1("Не удалось создать окно \"%s\"\n", strWndName);
		return false; // не удалось создать
	}

	SetDockingWindowIcons(theApp.m_bHiColorIcons); **/
	return true;
}

void CMainFrame::SetDockingWindowIcons(bool bHiColorIcons)
{ /***
	auto hIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_REGDUMPCPU_WND_HC : IDI_REGDUMPCPU_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_paneRegistryDumpViewCPU.SetIcon(hIcon, FALSE);
	hIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_REGDUMPFDD_WND_HC : IDI_REGDUMPFDD_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_paneRegistryDumpViewFDD.SetIcon(hIcon, FALSE);
	hIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_MEMDUMP_WND_HC : IDI_MEMDUMP_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);

	for (auto &pane : m_arPaneMemoryDumpView)
	{
		pane.SetIcon(hIcon, FALSE);
	}

	hIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_DEBUG_WND_HC : IDI_DEBUG_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_paneDisassembleView.SetIcon(hIcon, FALSE);
	hIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_TAPE_WND_HC : IDI_TAPE_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_paneTapeCtrlView.SetIcon(hIcon, FALSE);
	hIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_OSC_WND_HC : IDI_OSC_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_paneOscillatorView.SetIcon(hIcon, FALSE);
	hIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_VKBD_WND_HC : IDI_VKBD_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_paneBKVKBDView.SetIcon(hIcon, FALSE); **/
}

// диагностика CMainFrame

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext &dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif // _DEBUG


// обработчики сообщений CMainFrame

void CMainFrame::OnViewCustomize()
{ /***
	auto pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* сканировать меню* /);

	if (pDlgCust)
	{
		pDlgCust->EnableUserDefinedToolbars();
		pDlgCust->Create();
	} **/
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp, LPARAM lp)
{ /***
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp, lp);

	if (lres)
	{
		auto pUserToolbar = reinterpret_cast<CMFCToolBar *>(lres);
		ASSERT_VALID(pUserToolbar);
		CString strCustomize;
		BOOL bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
		ASSERT(bNameValid);
		pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	}

	return lres; **/ return 0;
}


void CMainFrame::SetAppLook(UINT id)
{ /***
	switch (id)
	{
		case ID_VIEW_APPLOOK_WIN_2000:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
			break;

		case ID_VIEW_APPLOOK_OFF_XP:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
			break;

		case ID_VIEW_APPLOOK_WIN_XP:
			CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
			break;

		case ID_VIEW_APPLOOK_OFF_2003:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
			break;

		case ID_VIEW_APPLOOK_VS_2005:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
			break;

		case ID_VIEW_APPLOOK_VS_2008:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
			break;

		case ID_VIEW_APPLOOK_WINDOWS_7:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
			break;

		default:
			switch (id)
			{
				case ID_VIEW_APPLOOK_OFF_2007_BLUE:
					CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
					break;

				case ID_VIEW_APPLOOK_OFF_2007_BLACK:
					CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
					break;

				case ID_VIEW_APPLOOK_OFF_2007_SILVER:
					CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
					break;

				case ID_VIEW_APPLOOK_OFF_2007_AQUA:
					CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
					break;
			}

			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
	}

	CDockingManager::SetDockingMode(DT_SMART);
	// сохраним цвет
	g_Config.m_clrText = CMFCVisualManager::GetInstance()->GetToolbarButtonTextColor(
		m_wndMenuBar.GetButton(0), CMFCVisualManager::AFX_BUTTON_STATE::ButtonsIsRegular
	); **/
}


void CMainFrame::OnApplicationLook(UINT id)
{
	theApp.m_nAppLook = id;
	SetRedraw(FALSE);
	SetAppLook(id);
	CDockingManager *pDockManager = GetDockingManager();

	if (pDockManager != nullptr)
	{
		ASSERT_VALID(pDockManager);
		pDockManager->AdjustPaneFrames();
	}

	CTabbedPane::ResetTabs();
	CTabbedPane::m_StyleTabWnd = CMFCTabCtrl::STYLE_3D;
	RecalcLayout();
	SetRedraw(TRUE);
	RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);
	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CFrameWndEx::OnSettingChange(uFlags, lpszSection);
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd *pParentWnd, CCreateContext *pContext)
{
	//GetDockingManager()->DisableRestoreDockState(TRUE); // эта штука запрещает брать из реестра состояние панелек. но наблюдаются глюки с посторонними разделителями.

	// базовый класс не работает
	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}

	// включить кнопку настройки для всех пользовательских панелей инструментов
	CString strCustomize;
	BOOL bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; ++i)
	{
		CMFCToolBar *pUserToolbar = GetUserToolBarByIndex(i);

		if (pUserToolbar != nullptr)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}

	return TRUE;
}

LRESULT CMainFrame::OnStartPlatform(WPARAM wParam, LPARAM lParam)
{
	// вызывается после создания CBKView и во время его первой инициализации
	// в функции CBKView::OnInitialUpdate()
	CString strLine = ::GetCommandLine();
	ParseCommandLineParameters(strLine);
	m_pScreen->SetAspectRatio(g_Config.m_dAspectRatio);
	m_nScreen_X = g_Config.m_nScreenW;
	InitScrViewPort(m_nScreen_X);

	if (ProcessFile(true)) // вот здесь только создаётся конфигурация бкашки
	{
		if (theApp.m_bNewConfig)
		{
			OnAppSettings();
		}
		UpdateStatusFreq();
		return S_OK;
	}

	CString str;
	str.Format(IDS_BK_ERROR_MFBKBOARDERROR, g_Config.GetBKConfName());
	g_BKMsgBox.Show(str, MB_OK);
	// сбросим модель на дефолтную
	g_Config.SetBKModel(CONF_BKMODEL::BK_0010_01);
	DestroyWindow(); // не создалось - ничего не можем сделать. выходим.
	return S_FALSE;
}

void CMainFrame::InitKbdStatus()
{
	auto vw = DYNAMIC_DOWNCAST(CBKView, GetActiveView());

	if (vw)
	{
		vw->CapsLockOn(); // там включается индикация капслока
		
	}
	// Но не всегда. поэтому продублируем
	bool bCaps = !!(GetKeyState(VK_CAPITAL) & 1);
	m_paneBKVKBDView.SetKeyboardStatus(STATUS_FIELD::KBD_CAPS, bCaps);	// начальное состояние ЗАГЛ
	m_paneBKVKBDView.SetKeyboardStatus(STATUS_FIELD::KBD_XLAT, false);	// начальное состояние ЛАТ
	m_paneBKVKBDView.SetKeyboardStatus(STATUS_FIELD::KBD_AR2, false);	// начальное состояние АР2 не нажат
	m_paneBKVKBDView.SetKeyboardStatus(STATUS_FIELD::KBD_SU, false);	// начальное состояние СУ не нажат
}

LRESULT CMainFrame::OnDrawBKScreen(WPARAM, LPARAM)
{
	auto view = DYNAMIC_DOWNCAST(CBKView, GetActiveView());

	if (view && view->IsWindowVisible())
	{
		view->Invalidate(FALSE);
	}

	return S_OK;
}

LRESULT CMainFrame::OnDebugDrawScreen(WPARAM wParam, LPARAM)
{
	// чтобы видеть как происходит движение луча в процессе прорисовки экрана
	// эти строки нужно закомментировать.
	// а иногда наоборот, очень неудобно когда экран по частям перерисовывается.
	// тогда эти строки надо раскомментировать
	// !!!TODO: может сделать чекбокс, с выбором способа отладочной перерисовки
	m_pScreen->SetRegister(m_pBoard->m_reg177664);
	m_pScreen->PrepareScreenRGB32(m_pBoard->GetMainMemory() + DWORD_PTR(wParam));
	auto view = GetActiveView();

	if (view)
	{
		view->Invalidate(FALSE);
	}

	return S_OK;
}

LRESULT CMainFrame::OnDrawOscilloscope(WPARAM, LPARAM)
{
	if (m_paneOscillatorView.IsWindowVisible())
	{
		m_paneOscillatorView.Invalidate(FALSE);
	}

	return S_OK;
}

LRESULT CMainFrame::OnSetCPUFreq(WPARAM, LPARAM)
{
	UpdateStatusFreq();
	return S_OK;
}

// вход: wParam - длина звукового буфера
LRESULT CMainFrame::OnOscilloscopeSetBuffer(WPARAM wParam, LPARAM)
{
	m_paneOscillatorView.SetBuffer(static_cast<int>(wParam));
	return S_OK;
}
// вход: lParam - указатель на звуковой буфер
LRESULT CMainFrame::OnOscilloscopeFillBuffer(WPARAM, LPARAM lParam)
{
	m_paneOscillatorView.FillBuffer(reinterpret_cast<SAMPLE_INT *>(lParam));
	return S_OK;
}

void CMainFrame::InitRegistry()
{
	static initRegParam regParam[5] =
	{
		// Register BinFile
		{ _T("BKEmulator.BinFile"), _T("binary file"), _T("/b \"%1\""), IDR_MAINFRAME, IDS_FILEEXT_BINARY, true },
		// Register MemFile
		{ _T("BKEmulator.MemFile"), _T("memory state"), _T("/m \"%1\""), IDI_MEMORY, IDS_FILEEXT_MEMORYSTATE, true },
		// Register TapeFle
		{ _T("BKEmulator.TapeFile"), _T("tape file"), _T("/t \"%1\""), IDI_TAPE, IDS_FILEEXT_TAPE, true },
		// Register ScriptFile
		{ _T("BKEmulator.ScriptFile"), _T("script file"), _T("/s \"%1\""), IDI_SCRIPT, IDS_FILEEXT_SCRIPT, true },
		// Register ROMFile
		{ _T("BKEmulator.RomFile"), _T("ROM file"), _T(""), IDI_ROM, IDS_FILEEXT_ROM, false }
	};

	for (auto &i : regParam)
	{
		RegisterDefaultIcon(theApp.m_strProgramName, &i);

		if (i.bShell)
		{
			RegisterShellCommand(theApp.m_strProgramName, &i);
		}
	}
}

void CMainFrame::RegisterDefaultIcon(CString &strName, initRegParam *regParam)
{
	CString strTitle(MAKEINTRESOURCE(IDS_EMUL_TITLE));
	CString strIcon = Global::IntToString(regParam->iconID * -1, 10);
	CString strExt(MAKEINTRESOURCE(regParam->nExtStrID));
	HKEY hKey;
	LSTATUS hr = ::RegCreateKey(HKEY_CLASSES_ROOT, strExt, &hKey);
	hr = ::RegSetValue(hKey, nullptr, REG_SZ, regParam->strSection, 0);
	hr = ::RegCloseKey(hKey);
	hr = ::RegCreateKey(HKEY_CLASSES_ROOT, regParam->strSection, &hKey);
	hr = ::RegSetValue(hKey, nullptr, REG_SZ, strTitle + _T(" ") + regParam->strDescription, 0);
	hr = ::RegCloseKey(hKey);
	hr = ::RegCreateKey(HKEY_CLASSES_ROOT, regParam->strSection + _T("\\DefaultIcon"), &hKey);
	fs::path p = g_Config.GetConfCurrPath() / strName.GetString();
	CString s = _T("\"") + CString(p.c_str()) + _T("\",") + strIcon;
	hr = ::RegSetValue(hKey, nullptr, REG_SZ, s, 0);
	hr = ::RegCloseKey(hKey);
}

void CMainFrame::RegisterShellCommand(CString &strName, initRegParam *regParam)
{
	HKEY hKey;
	LSTATUS hr = ::RegCreateKey(HKEY_CLASSES_ROOT, regParam->strSection + _T("\\Shell"), &hKey);
	hr = ::RegCloseKey(hKey);
	hr = ::RegCreateKey(HKEY_CLASSES_ROOT, regParam->strSection + _T("\\Shell\\Open"), &hKey);
	hr = ::RegCloseKey(hKey);
	hr = ::RegCreateKey(HKEY_CLASSES_ROOT, regParam->strSection + _T("\\Shell\\Open\\Command"), &hKey);
	fs::path p = g_Config.GetConfCurrPath() / strName.GetString();
	CString s = _T("\"") + CString(p.c_str()) + _T("\",") + regParam->strArguments;
	hr = ::RegSetValue(hKey, nullptr, REG_SZ, s, 0);
	hr = ::RegCloseKey(hKey);
}

BOOL CMainFrame::PreTranslateMessage(MSG *pMsg)
{
	/* обработка сообщения, которым обмениваются между собой копии приложения
	*  основная прога обрабатывает запрос от копии */
	HWND hwnd = GetSafeHwnd();

	// если это наше сообщение, и это запрос от копии, и окно наше (а то из-за брудкаста все дочерние окна отзываются и сообщение 11 раз обрабатывается)
	if (pMsg->hwnd == hwnd && pMsg->message == theApp.m_nInterAppGlobalMsg && pMsg->wParam == QUESTION_PRIME_HWND)
	{
		// на запрос от копии, посылаем ей HWND, куда слать сообщение
		TRACE("MainWnd -> copy thread\n");
		// если другая копия проги спрашивает эту копию, надо ей ответить, и послать
		// хэндл окна этой копии
		auto copy_thID = static_cast<DWORD>(pMsg->lParam); // это ид потока, откуда пришёл запрос

		// поскольку мы разослали сообщение брудкастом, то и сами себе его получили, поэтому
		// если copy_thID == theApp.m_nThreadID, то получили сообщение сами от себя, нафиг оно нам надо?
		if (copy_thID != theApp.m_nThreadID)    // копия проги не должна самой себе посылать ответ
		{
			::PostThreadMessage(copy_thID, theApp.m_nInterAppGlobalMsg, ANSWER_PRIME_HWND, reinterpret_cast<LPARAM>(hwnd));
		}

		return TRUE;
	}

//  else if (pMsg->message == m_nInterAppToolGlobalMessage)
//  {
//      // проверка вот этой идеи, хотя пока не нужно это.
//      // однако, если прибить BKDE из диспетчера задач, то никаких сообщений не посылается.
//      switch (pMsg->wParam)
//      {
// //       case TOOL_BKDE_INITIATED:
// //           g_BKMsgBox.Show(_T("Получено сообщение о запуске BKDE"), MB_OK);
// //           break;
//      case TOOL_BKDE_TERMINATED:
//          g_BKMsgBox.Show(_T("Получено сообщение из ExitInstance BKDE"), MB_OK);
//          break;
// //       case TOOL_BKDE_CLOSED:
// //           g_BKMsgBox.Show(_T("Получено сообщение при выходе из InitInstance BKDE"), MB_OK);
// //           break;
//      }
//  }

	if (pMsg->message == WM_SYSKEYDOWN && pMsg->wParam == VK_MENU)
	{
		// Если нажали любой Alt
		return FALSE;   // будем обрабатывать сами
	}

	if (pMsg->message == WM_SYSKEYUP)
	{
		// Если отжали любой Alt или кнопку F10
		if (pMsg->wParam == VK_MENU || pMsg->wParam == VK_F10)
		{
			return FALSE;   // будем обрабатывать сами
		}
	}

	return CFrameWndEx::PreTranslateMessage(pMsg);
}


BOOL CMainFrame::OnCopyData(CWnd *pWnd, COPYDATASTRUCT *pCopyDataStruct)
{
	if (pCopyDataStruct->dwData == 2)
	{
		TRACE("MainWnd get copy cmdline\n");
		CString strLine(reinterpret_cast<LPCTSTR>(pCopyDataStruct->lpData), pCopyDataStruct->cbData);
		strLine.ReleaseBuffer();
		ParseCommandLineParameters(strLine);
		bool bRes = ProcessFile(false);
		SetForegroundWindow();
		return bRes;
	}

	return CFrameWnd::OnCopyData(pWnd, pCopyDataStruct);
}

//#include <shellapi.h>
void CMainFrame::ParseCommandLineParameters(CString strCommands)
{
//  // вот такая вот ещё есть функция. требует #include <shellapi.h>
//  // парсит строку в массив строк.
//  // недостатки: удаляет кавычки внутри аргументов. В результате нельзя задать параметр, внутри которого есть пробелы
//  // ключ и его значение обязательно нужно разделять пробелами.
//  int nArgc = 0;
//  LPWSTR* pStrArgv = CommandLineToArgvW(strCommands, &nArgc);
//  for (int i = 0; i < nArgc; ++i)
//  {
//      CString arg = CString(pStrArgv[i]);
//  }
//  LocalFree(pStrArgv);
	ClearProcessingFiles();
	int nFindPos = strCommands.Find(_T('/'));

	if (nFindPos == -1)
	{
		return; // ключей нету, разбирать нечего, просто выходим.
	}

	// если ключи есть, оставляем только их.
	strCommands = strCommands.Right(strCommands.GetLength() - max(0, nFindPos));

	while (int nStrLen = strCommands.GetLength()) // nStrLen - текущая длина оставшейся неразобранной строки
	{
		nFindPos = strCommands.Find(_T('/'), 1); // ищем, есть ли ещё ключи

		if (nFindPos == -1) // если нету
		{
			nFindPos = nStrLen; // то всё что есть - параметр данного ключа
		}

		CString strParam = strCommands.Left(nFindPos); // выделяем параметр в отдельную строку
		strCommands = strCommands.Right(nStrLen - nFindPos); // и отсекаем их от обрабатываемой строки.
		strParam.Trim(); // уберём пробелы, если есть
		TCHAR command = toupper(strParam[1]); // получим значение ключа
		strParam = strParam.Right(strParam.GetLength() - 2); // выделим аргументы
		strParam.Trim(); // снова уберём пробелы. теперь они точно есть
		//if (command != _T('S'))  это если будем передавать аргументы скрипта, кавычки надо будет удалять там.
		{
			strParam.Trim(_T('\"')); // а ещё кавычки, если есть
		}

		if (!SetCmdLParameters(command, strParam))
		{
			// если ппараметр не опознан, проверим, может это вызов справки
			switch (command)
			{
				case _T('?'):
				case _T('H'):
				{
					CString strTitle(MAKEINTRESOURCE(IDS_EMUL_TITLE));
					CString strPrompt(MAKEINTRESOURCE(IDS_COMMAND_PROMPT_INFO));
					g_BKMsgBox.Show(strTitle + strPrompt, MB_OK);
					return;
				}

					// если да - покажем
			}

			// если нет - просто игнорируем
		}
	}
}

bool CMainFrame::SetCmdLParameters(TCHAR command, const CString &strParam)
{
	switch (command)
	{
		case _T('B'): // Binary file
			if (!(m_cli.nStatus & CLI_KEY_D))
			{
				fs::path sp = strParam.GetString();
				// если задан ключ /D, ключ /B игнорируем
				m_cli.strBinFileName = sp.filename();

				if (sp.has_filename())
				{
					m_cli.nStatus |= CLI_KEY_B;

					if (sp.has_parent_path())
					{
						g_Config.m_strBinPath = sp.parent_path();
					}
				}
			}

			break;

		case _T('D'):
		{
			m_cli.nStatus &= ~CLI_KEY_B; // если был задан ключ /B, то его снимаем.
			fs::path sp = strParam.GetString();
			m_cli.strBinFileName = sp.filename();

			if (sp.has_filename())
			{
				m_cli.nStatus |= CLI_KEY_D;

				if (sp.has_parent_path())
				{
					g_Config.m_strBinPath = sp.parent_path();
				}
			}
		}
		break;

		case _T('M'): // Memory state file
		{
			fs::path sp = strParam.GetString();
			m_cli.strMemFileName = sp.filename();

			if (sp.has_filename())
			{
				m_cli.nStatus |= CLI_KEY_M;

				if (sp.has_parent_path())
				{
					g_Config.m_strMemPath = sp.parent_path();
				}
			}
		}
		break;

		case _T('T'): // BK tape file
		{
			fs::path sp = strParam.GetString();
			m_cli.strTapeFileName = sp.filename();

			if (sp.has_filename())
			{
				m_cli.nStatus |= CLI_KEY_T;

				if (sp.has_parent_path())
				{
					g_Config.m_strTapePath = sp.parent_path();
				}
			}
		}
		break;

		case _T('S'): // Initial Script file
		{
			fs::path sp = strParam.GetString();
			m_cli.strScriptFileName = sp.filename();

			if (sp.has_filename())
			{
				m_cli.nStatus |= CLI_KEY_S;

				if (sp.has_parent_path())
				{
					g_Config.m_strScriptsPath = sp.parent_path();
				}
			}

			// потенциально, за скриптом можно передавать список агрументов, которые будут передаваться скрипту
			// name.bkscript arg1,arg2,"string arg3",arg4
			// но тут сильно всё усложняется тогда и пока не ясно как ошибки обрабатывать.
			// запятая - разделитель аргументов, как быть, если запятая - символ в составе аргумента?
			// и как быть, если слеш -тоже символ в составе аргумента?
			// и обработка кавычек тоже фиг знает как делается. Т.е. как деляется - ясно, нужно писать большой сложный
			// парсер строки, а хочется, чтобы кто-то за меня это сделал.
		}
		break;

		case _T('C'):
			g_Config.SetBKBoardType(strParam);
			m_cli.nStatus |= CLI_KEY_C;
			break;

		case _T('P'):
			// /P nn:nn
		{
			m_cli.nStatus |= CLI_KEY_P;
			CString str = strParam;
			str.Trim();
			int nDlmPos = str.Find(_T(':')); // ищем, есть ли разделитель

			if (nDlmPos != -1) // если есть
			{
				if (nDlmPos > 0) // и не самый первый
				{
					m_cli.strPage0 = str.Left(nDlmPos).Trim();
					m_cli.nStatus |= CLI_KEY_P_PAGE0;
				}

				str = str.Right(str.GetLength() - nDlmPos - 1);

				if (str.GetLength() > 0)
				{
					m_cli.strPage1 = str.Trim();
					m_cli.nStatus |= CLI_KEY_P_PAGE1;
				}
			}
			else //если нету, то значит только первое окно
			{
				if (str.GetLength() > 0)
				{
					m_cli.strPage0 = str;
					m_cli.nStatus |= CLI_KEY_P_PAGE0;
				}
			}
		}
		break;

		case _T('L'):   // задать адрес загрузки файла дампа
			if (!strParam.IsEmpty())
			{
				m_cli.nLoadAddr = Global::OctStringToWord(strParam);
				m_cli.nStatus |= CLI_KEY_L;
			}

			break;

		case _T('A'):   // задать адрес запуска файла дампа, если он отличается от адреса загрузки
			if (!strParam.IsEmpty())
			{
				m_cli.nStartAddr = Global::OctStringToWord(strParam);
				m_cli.nStatus |= CLI_KEY_A;
			}

			break;

		case _T('R'):
			m_cli.nStatus |= CLI_KEY_R;
			break;

		case _T('F'):
			m_cli.nStatus |= CLI_KEY_F;
			break;

		default:
			return false;
	}

	return true;
}

void CMainFrame::ClearProcessingFiles()
{
	m_cli.clear();
}

bool CMainFrame::StartPlayTape(const fs::path &strPath)
{
	CString strExt = strPath.extension().c_str();
	TAPE_FILE_INFO tfi{};
	memset(&tfi, 255, sizeof(TAPE_FILE_INFO));

	if (!strExt.CollateNoCase(CString(MAKEINTRESOURCE(IDS_FILEEXT_WAVE))))
	{
		m_tape.LoadWaveFile(strPath);
	}
	else if (!strExt.CollateNoCase(CString(MAKEINTRESOURCE(IDS_FILEEXT_TAPE))))
	{
		m_tape.LoadMSFFile(strPath);
	}
	else if (!strExt.CollateNoCase(CString(MAKEINTRESOURCE(IDS_FILEEXT_BINARY))))
	{
		m_tape.LoadBinFile(strPath, &tfi);
	}
	else
	{
		return false;
	}

	m_tape.GetWaveFile(&tfi); // вычисляем и заполняем внутренние переменные
	m_tape.SetWaveLoaded(true);
	m_paneTapeCtrlView.StartPlayTape();
	return true;
}



void CMainFrame::StoreIniParams()
{
	g_Config.m_nCPUFrequency = (g_Config.m_nCPUTempFreq < 0) ? m_pBoard->GetCPUSpeed() : g_Config.m_nCPUTempFreq;

	for (int i = 0; i < NUMBER_VIEWS_MEM_DUMP; ++i)
	{
		g_Config.m_arDumper[i].nAddr = m_arPaneMemoryDumpView[i].GetAddress();
		g_Config.m_arDumper[i].nPageListPos = m_arPaneMemoryDumpView[i].GetDumpPageListPos();
		g_Config.m_arDumper[i].nAddrListPos = m_arPaneMemoryDumpView[i].GetDumpAddrListPos();
	}

	CPoint s = m_pScreen->GetScreenViewport();
	g_Config.m_nScreenW = s.x;
	g_Config.m_dAspectRatio = m_pScreen->getAspectRatio();
}

void CMainFrame::OnClose()
{
	// тут надо занести в конфиг разные переменные и параметры опций, которые надо сохранить
	StoreIniParams();
	StopAll();
	CheckDebugMemmap(); // если карта памяти была открыта - закроем

	CFrameWndEx::OnClose();
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
		case BKTIMER_UI_REFRESH:
			OnMainLoop();
			break;

		case BKTIMER_UI_TIME:
			OnMainLoopTime();
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// по идее, вызывается каждые 20 миллисекунд, обновляет информацию в
// пользовательском интерфейсе
void CMainFrame::OnMainLoop()
{
	if (m_pBoard)   // работаем только если есть матплата
	{
		// карту памяти тоже тут обновляем
		if (m_bBKMemViewOpen)
		{
			m_pBKMemView->DrawTab();
		}

		// Парсим скрипт, если он есть
		m_Script.RunScript();
		m_pBoard->FrameParam();  // применяем новые параметры

		// если проц остановлен, то информацию не обновлять, т.к. она принудительно обновляется при пошаговой отладке.
		// а если работает - то обновлять
		if (!m_pBoard->IsCPUBreaked())
		{
			// это имеет смысл только в информативных целях, иначе сильно проседает производительность
			// на слабых конфигурациях. причём эта штука должна быть именно здесь.
			if (g_Config.m_nRegistersDumpInterval && !m_pScreen->IsFullScreenMode())
			{
				if (--m_nRegsDumpCounter <= 0)
				{
					m_nRegsDumpCounter = g_Config.m_nRegistersDumpInterval;
					SetDebugCtrlsState();
				}
			}
		}
	}
}

void CMainFrame::OnMainLoopTime()
{
	DWORD nTmpTick = GetTickCount() - m_nStartTick;
	nTmpTick /= 1000;
	// таймер аптайма. поточнее, но из-за того, что OnMainLoopTime() вызывается не точно через 1000мс,
	// и сюда попадаем иногда больше чем через секунду, то при выводе иногда счётчик перескакивает
	// через секунду. Но тем не менее, тут показывается реальное время работы, а не счётчик попаданий,
	// как раньше.
	DWORD seconds = nTmpTick % 60;
	nTmpTick /= 60;
	DWORD minutes = nTmpTick % 60;
	DWORD hours = nTmpTick / 60;

// вот эта штука возвращает фокус программе в полоэкранном режиме, после нажатий на разного рода
// управляющие клавиши, которые не обрабатываются эмулятором и приводят к потере фокуса эмулятором
//  if (m_pScreen->IsFullScreenMode()) // если мы в полноэкранном режиме
//  {
// #ifdef TARGET_WINXP
//      // для D3D9 таких проблем нету
//      SetFocusToBK(); // будем принудительно передавать фокус Экрану
// #else
//
//      // если в полноэкранном режиме и рендер не D3D, то только тогда
//      if (g_Config.m_nScreenRenderType != CONF_SCREEN_RENDER_D3D)
//      {
//          SetFocusToBK(); // будем принудительно передавать фокус Экрану
//      }
//
//      // Тут возникла проблема с полноэкранным режимом D3D11.
//      // если мы передаём фокус CBKView - вываливаемся из режима. Потому что
//      // в полноэкранном режиме фокус должен быть у CScreen, как только он будет потерян
//      // тут же выпадаем из полноэкранного режима
// #endif
//  }

	if (g_Config.m_bShowPerformance) // если в оконном и включена опция показа информации
	{
		// Показываем производительность в строке состояния
		CString strPerformance;
		strPerformance.Format(_T("FPS: %i, Uptime: %02i:%02i:%02i"), m_pScreen->GetFPS(), hours, minutes, seconds);
		m_wndStatusBar.SetPaneText(static_cast<int>(STATUS_FIELD::INFO), strPerformance.GetString());
	}
}

LRESULT CMainFrame::OnDropFile(WPARAM wParam, LPARAM lParam)
{
	auto pStrPath = reinterpret_cast<CString *>(lParam);

	if (pStrPath)
	{
		ClearProcessingFiles();
		fs::path ps = pStrPath->GetString();
		CString strFileExt = ps.extension().c_str();
		TCHAR command = 0;

		if (!strFileExt.CollateNoCase(CString(MAKEINTRESOURCE(IDS_FILEEXT_BINARY))))  // Binary file
		{
			command = _T('B');
		}
		else if (!strFileExt.CollateNoCase(CString(MAKEINTRESOURCE(IDS_FILEEXT_MEMORYSTATE))))  // Memory state file
		{
			command = _T('M');
		}
		else if (!strFileExt.CollateNoCase(CString(MAKEINTRESOURCE(IDS_FILEEXT_TAPE)))
		         || !strFileExt.CollateNoCase(CString(MAKEINTRESOURCE(IDS_FILEEXT_WAVE))))  // BK tape file
		{
			command = _T('T');
		}
		else if (!strFileExt.CollateNoCase(CString(MAKEINTRESOURCE(IDS_FILEEXT_SCRIPT))))  // Initial Script file
		{
			command = _T('S');
		}
		else
		{
			ASSERT(false);
			return S_FALSE;
		}

		SetCmdLParameters(command, *pStrPath);
		ProcessFile(false); // здесь если надо создаётся новая конфигурация бкашки, а если нет - остаётся текущая
		return S_OK;
	}

	return S_FALSE;
}

LRESULT CMainFrame::OnOutKeyboardStatus(WPARAM wParam, LPARAM lParam)
{
	auto pane = static_cast<int>(wParam);
	auto pStr = reinterpret_cast<CString *>(lParam);
	m_wndStatusBar.SetPaneText(pane, *pStr);
	return S_OK;
}

void CMainFrame::SetupConfiguration(CONF_BKMODEL nConf)
{
	m_Script.StopScript();

	if (ConfigurationConstructor(nConf))
	{
		SetFocusToBK();
	}
	else
	{
		// надо вообще закрыть прогу. нечего тут делать.
		CString str;
		str.Format(IDS_BK_ERROR_MFBKBOARDERROR, g_Config.GetBKConfName());
		g_BKMsgBox.Show(str, MB_OK);
		// сбросим модель на дефолтную
		g_Config.SetBKModel(CONF_BKMODEL::BK_0010_01);
		DestroyWindow(); // не создалось - ничего не можем сделать. выходим.
	}
}

// Вход: bCreate = true - вызывается из OnCreate, нужно создавать новую конфигурацию
//              = false - вызывается из OnDrop или OnCopy - от второй копии проги, нужно использовать существующую конфигурацию
bool CMainFrame::ProcessFile(bool bCreate)
{
	bool bRes = false;

	// Инициализация конфигурации БК
	if (m_cli.nStatus & CLI_KEY_M) // если задан .msf файл, то пробуем загрузить его
	{
		m_Script.StopScript();
		// Если не можем загрузить msf файл
		fs::path str = g_Config.m_strMemPath / m_cli.strMemFileName;

		if (!(bRes = LoadMemoryState(str)))
		{
			bRes = ConfigurationConstructor(CONF_BKMODEL::BK_0010_01);    // Создаём конфигурацию по умолчанию
		}
	}
	else if (m_cli.nStatus & CLI_KEY_D) // если задана опция грузить дамп.
	{
		/*
		    1. ключ /C - задать конфигурацию
		    2. ключ /P - задать страницы
		    3. ключ /L - задать адрес загрузки
		    4. ключ /A - задать адрес запуска
		    5. ключ /R - запустить по заданному адресу
		    6. ключ /F - загружать дамп не в формате бин
		*/
		if (bCreate)
		{
			bRes = ConfigurationConstructor(g_Config.GetBKModel(), false);  // Создаём заданную конфигурацию, и не запускаем на выполнение
		}
		else
		{
			// иначе - останавливаем текущую конфигурацию. Загрузку туда делать будем
			StopAll();
			bRes = true;
		}

		if (bRes)
		{
			m_cli.clearScriptFName(); //скрипт надо игнорировать.
			m_Script.StopScript();
			// теперь надо загрузить дамп
			uint8_t[] *buf;
			uint16_t nAddr, nLen;
			bool bStrict = !(m_cli.nStatus & CLI_KEY_F); //ключ /F == (bStrict == false)

			if (Global::LoadBinFile(buf, nAddr, nLen, g_Config.m_strBinPath / m_cli.strBinFileName, bStrict))
			{
				if (m_cli.nStatus & CLI_KEY_L)
				{
					nAddr = m_cli.nLoadAddr;
				}

				int nPg0 = (m_cli.nStatus & CLI_KEY_P_PAGE0) ? Global::OctStringToWord(m_cli.strPage0) : -1;
				int nPg1 = (m_cli.nStatus & CLI_KEY_P_PAGE1) ? Global::OctStringToWord(m_cli.strPage1) : -1;
				m_pBoard->SetMemPages(nPg0, nPg1);

				// не дадим загружать за пределами адресного пространства
				if (int(nAddr) + int(nLen) >= 0200000)
				{
					nLen = 0200000 - nAddr;
				}

				// заносим в Память, если там ПЗУ - оно портится.
				// Это в принципе можно поправить. Не знаю, нужно ли.
				uint16_t nLoadAddr = nAddr;

				for (int i = 0; i < nLen; ++i)
				{
					m_pBoard->SetByteIndirect(nLoadAddr++, buf[i]);
				}

				m_pBoard->RestoreMemPages();

				if (m_cli.nStatus & CLI_KEY_R)
				{
					uint16_t nStartAddr = (m_cli.nStatus & CLI_KEY_A) ? m_cli.nStartAddr : nAddr;
					m_pBoard->SetRON(CCPU::REGISTER::PC, nStartAddr);
				}
			}

			StartAll();

			// если не установлен флаг остановки после создания
			if (g_Config.m_bPauseCPUAfterStart)
			{
				m_pBoard->BreakCPU();
			}
		}
	}
	else if (m_cli.nStatus & CLI_KEY_B) // Если задан bin файл
	{
		fs::path tmp = g_Config.m_strBinPath; // сохраняем свой путь к бин файлу
		CONF_BKMODEL model = CONF_BKMODEL::BK_0010_01;

		if (bCreate && (m_cli.nStatus & CLI_KEY_C)) // если вызвали из оболочки виндовс по ассоциации, или из ком.строки
		{
			model = g_Config.GetBKModel(); // берём модель, которую задали ключом  /C
		}

		// иначе - вызвали из ДрагнДропа, и модель надо использовать текущую
		// корректируем модель, чтобы была без КНГМД.
		bool b11m = false;

		if ((b11m = g_Config.isBK11M()) || g_Config.isBK11())
		{
			if (b11m)
			{
				model = CONF_BKMODEL::BK_0011M;

				if (!(m_cli.nStatus & CLI_KEY_S))
				{
					m_cli.strScriptFileName = _T("autorun\\bk11m_load.bkscript");
					m_cli.nStatus |= CLI_KEY_S;
					m_cli.nStatus &= ~CLI_KEY_B; //ключ бин надо убрать, а то алгоритм эмуляции загрузки не в ту ветвь условий лезет. Но всё равно работает как надо.
				}
			}
			else
			{
				model = CONF_BKMODEL::BK_0011;

				if (!(m_cli.nStatus & CLI_KEY_S))
				{
					m_cli.strScriptFileName = _T("autorun\\bk11_load.bkscript");
					m_cli.nStatus |= CLI_KEY_S;
					m_cli.nStatus &= ~CLI_KEY_B;
				}
			}

			CString str;

			if (m_cli.nStatus & CLI_KEY_P_PAGE0)
			{
				str = m_cli.strPage0 + _T(";0C");
			}

			m_Script.SetArgument(str);
			str.Empty();

			if (m_cli.nStatus & CLI_KEY_P_PAGE1)
			{
				str = m_cli.strPage1 + _T(";1C");
			}

			m_Script.SetArgument(str);
		}
		else
		{
			model = CONF_BKMODEL::BK_0010_01; // скорректируем для гарантии

			if (!(m_cli.nStatus & CLI_KEY_S))
			{
				m_cli.strScriptFileName = _T("autorun\\monitor_load.bkscript");
				m_cli.nStatus |= CLI_KEY_S;
			}
		}

		bRes = ConfigurationConstructor(model);     // Создаём нужную конфигурацию

		if (bRes)
		{
			CString str = m_cli.strBinFileName.stem().c_str();
			m_Script.SetArgument(str);
			m_Script.SetScript(g_Config.m_strScriptsPath, m_cli.strScriptFileName, m_paneBKVKBDView.GetXLatStatus());
		}

		g_Config.m_strBinPath = tmp; // восстанавливаем свой путь к бин файлу
	}
	else
	{
		if (bCreate) // если вызвали из onCreate - то создаём заданную модель
		{
			m_Script.StopScript();
			bRes = ConfigurationConstructor(g_Config.GetBKModel());  // Создаём заданную конфигурацию
		}
		else
		{
			// иначе вообще ничего не создаём, т.е. дропнули что-то, что не требует пересоздания конфигурации
			bRes = true;
		}

		if (bRes) // если конфигурация была создана успешно, или текущая.
		{
			// Если задан файл ленты, запускаем его играться
			if (m_cli.nStatus & CLI_KEY_T)
			{
				StartPlayTape(g_Config.m_strTapePath / m_cli.strTapeFileName);
			}

			// Если задан файл скрипта, запускаем его выполняться
			if (m_cli.nStatus & CLI_KEY_S)
			{
				m_Script.SetScript(g_Config.m_strScriptsPath, m_cli.strScriptFileName, m_paneBKVKBDView.GetXLatStatus());
			}
		}
	}

	SetFocusToBK();
	return bRes;
}

void CMainFrame::InitEmulator()
{
	CString str = CString(MAKEINTRESOURCE(g_mstrConfigBKModelParameters[static_cast<int>(g_Config.GetBKModel())].nIDBKModelName));
	UpdateFrameTitleForDocument(str);

	switch (g_Config.m_nVKBDType)
	{
		default:
		case 0:
			m_paneBKVKBDView.SetKeyboardView(IDB_BITMAP_SOFT);
			break;

		case 1:
			m_paneBKVKBDView.SetKeyboardView(IDB_BITMAP_PLEN);
			break;
	}

	auto pSlider = DYNAMIC_DOWNCAST(CSliderButton, m_wndToolBarSound.GetButton(m_wndToolBarSound.CommandToIndex(ID_OPTIONS_SOUND_VOLUME)));

	if (pSlider)
	{
		pSlider->SetValue(g_Config.m_nSoundVolume);
	}

	m_pSound->SoundGen_SetVolume(g_Config.m_nSoundVolume); // продублируем для надёжности
	// инициализация текущих настроек
	m_speaker.EnableSound(g_Config.m_bSpeaker);
	m_speaker.SetFilter(g_Config.m_bSpeakerFilter);
	m_speaker.SetDCOffsetCorrect(g_Config.m_bSpeakerDCOffset);
	m_covox.EnableSound(g_Config.m_bCovox);
	m_covox.SetFilter(g_Config.m_bCovoxFilter);
	m_covox.SetDCOffsetCorrect(g_Config.m_bCovoxDCOffset);
	m_covox.SetStereo(g_Config.m_bStereoCovox);
	m_menestrel.EnableSound(g_Config.m_bMenestrel);
	m_menestrel.SetFilter(g_Config.m_bMenestrelFilter);
	m_menestrel.SetDCOffsetCorrect(g_Config.m_bMenestrelDCOffset);
	m_menestrel.SetStereo(true); // всегда стерео
	m_aySnd.EnableSound(g_Config.m_bAY8910);
	m_aySnd.SetFilter(g_Config.m_bAY8910Filter);
	m_aySnd.SetDCOffsetCorrect(g_Config.m_bAY8910DCOffset);
	m_aySnd.SetStereo(true); // всегда стерео
	m_tape.SetWaveParam(g_Config.m_nSoundSampleRate, BUFFER_CHANNELS);
	// параметры экрана
	m_pBoard->ChangePalette();
	m_pScreen->SetSmoothing(g_Config.m_bSmoothing);
	m_pScreen->SetColorMode(g_Config.m_bColorMode);
	m_pScreen->SetAdaptMode(g_Config.m_bAdaptBWMode);
	m_pScreen->SetLuminoforeEmuMode(g_Config.m_bLuminoforeEmulMode);
	g_Config.m_bFullscreenMode ? m_pScreen->SetFullScreenMode() : m_pScreen->SetWindowMode();

	// обновим адрес дампа памяти
	for (int i = 0; i < NUMBER_VIEWS_MEM_DUMP; ++i)
	{
		m_arPaneMemoryDumpView[i].SetDumpWindows(m_pBoard->GetWndVectorPtr(), g_Config.m_arDumper[i].nPageListPos, g_Config.m_arDumper[i].nAddrListPos);
		m_arPaneMemoryDumpView[i].SetAddress(g_Config.m_arDumper[i].nAddr);
	}

	// обновим адрес дизассемблера
	m_paneDisassembleView.SetAddr(g_Config.m_nDisasmAddr);
	// Настройка панели управления записью
	m_paneTapeCtrlView.InitParams(&m_tape);
	// наглядно отобразим, что и в каком дисководе находится
	UpdateToolbarDriveIcons();
	UpdateData(FALSE);
}

// выносим это в отдельную процедуру, т.к. есть вероятность неудачного прочтения MSF,
// при этом надо восстановить всё как было раньше
void CMainFrame::AttachObjects()
{
	auto vw = DYNAMIC_DOWNCAST(CBKView, GetActiveView());

	if (vw)
	{
		vw->ReCreateSCR(); // пересоздаём экран с новыми параметрами.
		m_nScreen_X = g_Config.m_nScreenW;
		m_pScreen->SetScreenViewport(m_nScreen_X);
		InitScrViewPort(m_nScreen_X);
	}

	int nMtc = m_pSound->ReInit(g_Config.m_nSoundSampleRate); // пересоздаём звук с новыми параметрами, на выходе - длина медиафрейма в сэмплах
	m_speaker.ReInit();     // ещё надо переинициализирвоать устройства, там
	m_speaker.ConfigureTapeBuffer(nMtc);// переопределяем буферы в зависимости от текущей частоты дискретизации
	m_covox.ReInit();       // есть вещи, зависящие от частоты дискретизации,
	m_menestrel.ReInit();   //
	m_aySnd.ReInit();       // которая теперь величина переменная. Но требует перезапуска конфигурации.
	m_paneOscillatorView.ReCreateOSC(); // пересоздаём осциллограф с новыми параметрами
	// при необходимости откорректируем размер приёмного буфера.
	m_paneOscillatorView.SetBuffer(nMtc); //SendMessage(WM_OSC_SETBUFFER, WPARAM(nMtc));

	if (m_pBoard)
	{
		m_pBoard->AttachWindow(this);  // цепляем к MotherBoard этот класс
		// порядок имеет значение. сперва нужно делать обязательно AttachWindow(this)
		// и только затем m_pBoard->SetMTC(). И эта функция обязательна, там звуковой буфер вычисляется
		// и выделяется
		m_pBoard->SetMTC(nMtc); // и здесь ещё. тройная работа получается.
		// Присоединяем к новосозданному чипу устройства
		m_pBoard->AttachSound(m_pSound.get());
		m_pBoard->AttachSpeaker(&m_speaker);
		m_pBoard->AttachMenestrel(&m_menestrel);
		m_pBoard->AttachCovox(&m_covox);
		m_pBoard->AttachAY8910(&m_aySnd);
		// если в ини файле задана частота, то применим её, вместо частоты по умолчанию.
		m_pBoard->NormalizeCPU();
		// Цепляем к новому чипу отладчик, т.е. наоборот, к отладчику чип
		m_pDebugger->AttachBoard(GetBoard());
		m_paneRegistryDumpViewCPU.SetFreqParam();
		// Цепляем обработчик скриптов
		m_Script.AttachBoard(GetBoard());
	}
}

bool CMainFrame::ConfigurationConstructor(CONF_BKMODEL nConf, bool bStart)
{
	bool bReopenMemMap = false;

	if (m_pBoard)
	{
		// Если конфигурация уже существует, удалим её
		StopAll();  // сперва всё остановим
		bReopenMemMap = CheckDebugMemmap(); // флаг переоткрытия карты памяти
		// перед сохранением настройки флагов заберём из диалога
		StoreIniParams();
		g_Config.SaveConfig();

		if (m_pBoard)   // удалим конфигурацию
		{
			m_pBoard.reset();
		}
	}

	g_Config.SetBKModel(nConf);

	// создадим новую конфигурацию
	switch (g_Config.m_BKBoardModel)
	{
		case MSF_CONF::BK10:
			m_pBoard = std::make_unique<CMotherBoard_10>();
			break;

		case MSF_CONF::BK1001:
			m_pBoard = std::make_unique<CMotherBoard>();
			break;

		case MSF_CONF::BK1001_MSTD:
			m_pBoard = std::make_unique<CMotherBoard_MSTD>();
			break;

		case MSF_CONF::BK1001_EXT32:
			m_pBoard = std::make_unique<CMotherBoard_EXT32>();
			break;

		case MSF_CONF::BK1001_FDD:
			m_pBoard = std::make_unique<CMotherBoard_10_FDD>();
			break;

		case MSF_CONF::BK11:
			m_pBoard = std::make_unique<CMotherBoard_11>();
			break;

		case MSF_CONF::BK11_FDD:
			m_pBoard = std::make_unique<CMotherBoard_11_FDD>();
			break;

		case MSF_CONF::BK11M:
			m_pBoard = std::make_unique<CMotherBoard_11M>();
			break;

		case MSF_CONF::BK11M_FDD:
			m_pBoard = std::make_unique<CMotherBoard_11M_FDD>();
			break;

		default:
			ASSERT(false);
			return false;
	}

	if (m_pBoard)
	{
		m_pBoard->SetFDDType(g_Config.m_BKFDDModel);
		g_Config.LoadConfig(false); // Читаем из ини файла параметры
		AttachObjects();    // пересоздадим и присоединим необходимые устройства.

		if (m_pBoard->InitBoard(g_Config.m_nCPURunAddr))
		{
			InitEmulator();         // переинициализируем модель

			if (bReopenMemMap)      // если надо
			{
				OnDebugMemmap();    // заново откроем карту памяти
			}

			if (bStart)
			{
				// Запускаем CPU
				StartAll();

				// если не установлен флаг остановки после создания
				if (g_Config.m_bPauseCPUAfterStart)
				{
					m_pBoard->BreakCPU();
				}
			}

			SetFocusToBK();
			return true;
		}

		// если ресет не удался - значит не удалось проинициализировать
		// память - значит не удалось загрузить какие-то дампы прошивок -
		// значит дальше работать невозможно.
		if (m_pBoard)
		{
			m_pBoard.reset();
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	return false;
}

// упрощённый вариант функции для загрузки конфигурации
bool CMainFrame::ConfigurationConstructor_LoadConf(CONF_BKMODEL nConf)
{
	g_Config.SetBKModel(nConf);

	// создадим новую конфигурацию
	switch (g_Config.m_BKBoardModel)
	{
		case MSF_CONF::BK10:
			m_pBoard = std::make_unique<CMotherBoard_10>();
			break;

		case MSF_CONF::BK1001:
			m_pBoard = std::make_unique<CMotherBoard>();
			break;

		case MSF_CONF::BK1001_MSTD:
			m_pBoard = std::make_unique<CMotherBoard_MSTD>();
			break;

		case MSF_CONF::BK1001_EXT32:
			m_pBoard = std::make_unique<CMotherBoard_EXT32>();
			break;

		case MSF_CONF::BK1001_FDD:
			m_pBoard = std::make_unique<CMotherBoard_10_FDD>();
			break;

		case MSF_CONF::BK11:
			m_pBoard = std::make_unique<CMotherBoard_11>();
			break;

		case MSF_CONF::BK11_FDD:
			m_pBoard = std::make_unique<CMotherBoard_11_FDD>();
			break;

		case MSF_CONF::BK11M:
			m_pBoard = std::make_unique<CMotherBoard_11M>();
			break;

		case MSF_CONF::BK11M_FDD:
			m_pBoard = std::make_unique<CMotherBoard_11M_FDD>();
			break;

		default:
			ASSERT(false);
			return false;
	}

	if (m_pBoard)
	{
		m_pBoard->SetFDDType(g_Config.m_BKFDDModel);
		// присоединим устройства, чтобы хоть что-то было для выполнения ResetHot
		AttachObjects();

		if (m_pBoard->InitBoard(g_Config.m_nCPURunAddr))
		{
			SetFocusToBK();
			return true;
		}

		// если ресет не удался - значит не удалось проинициализировать
		// память - значит не удалось загрузить какие-то дампы прошивок -
		// значит дальше работать невозможно.
		if (m_pBoard)
		{
			m_pBoard.reset();
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	return false;
}

LRESULT CMainFrame::OnResetKbdManager(WPARAM wp, LPARAM)
{
	if (wp)
	{
		InitKbdStatus();
	}

	auto vw = DYNAMIC_DOWNCAST(CBKView, GetActiveView());

	if (vw)
	{
		vw->ClearKPRS();
	}

	m_pBoard->m_reg177660 &= ~0200;  // ещё удалим флаг готовности нового кода
	m_pBoard->m_reg177716in |= 0100;  // ещё сбросим флаг нажатия клавиши
	SetFocusToBK();
	return S_OK;
}

bool CMainFrame::LoadMemoryState(const fs::path &strPath)
{
	CMSFManager msf;
	bool bRet = false;

	if (msf.OpenFile(strPath.c_str(), true))
	{
		if (msf.GetType() == MSF_STATE_ID && msf.GetVersion() >= MSF_VERSION_MINIMAL)
		{
			StopAll();
			bool bReopenMemMap = CheckDebugMemmap(); // флаг переоткрытия карты памяти

			// временно выгрузим все образы дискет и винчестеров.
			if (m_pBoard->GetFDDType() != BK_DEV_MPI::NONE)
			{
				m_pBoard->GetFDD()->DetachDrives();
			}

			// Сохраняем старую конфигурацию
			std::unique_ptr<CMotherBoard> pOldBoard = std::move(m_pBoard);
			CONF_BKMODEL nOldConf = g_Config.GetBKModel();
			m_pBoard = nullptr;

			if (ConfigurationConstructor_LoadConf(msf.GetConfiguration()))
			{
				if (m_pBoard->RestoreState(msf, nullptr))
				{
					if (pOldBoard)
					{
						pOldBoard.reset();
					}

					bRet = true;
				}
				else
				{
					// не удалось восстановить состояние, надо вернуть старую конфигурацию.
					g_BKMsgBox.Show(IDS_ERRMSF_WRONG, MB_OK);

					if (m_pBoard)
					{
						m_pBoard.reset();
					}

					m_pBoard = std::move(pOldBoard);
					g_Config.SetBKModel(nOldConf);
					g_Config.LoadConfig();      // восстановим из ини файла параметры
				}

				// приаттачим все образы дискет и винчестеров.
				if (m_pBoard->GetFDDType() != BK_DEV_MPI::NONE)
				{
					m_pBoard->GetFDD()->AttachDrives();
				}

				AttachObjects();            // переприсоединим устройства, уже с такой, какой надо конфигурацией
				InitEmulator();             // переинициализируем модель
				m_nScreen_X = g_Config.m_nScreenW;
				InitScrViewPort(m_nScreen_X);
			}
			else
			{
				// Неподдерживаемая конфигурация, или ошибка при создании
				g_BKMsgBox.Show(IDS_ERRMSF_WRONG, MB_OK);
				m_pBoard = std::move(pOldBoard);
				g_Config.SetBKModel(nOldConf);
				AttachObjects();
			}

			if (bReopenMemMap && !m_bBKMemViewOpen)
			{
				OnDebugMemmap();
			}

			StartAll();

			// если не установлен флаг остановки после создания
			if (g_Config.m_bPauseCPUAfterStart)
			{
				m_pBoard->BreakCPU();
			}
		}
		else
		{
			g_BKMsgBox.Show(IDS_ERRMSF_OLD, MB_OK);
		}
	}

	return bRet;
}


bool CMainFrame::SaveMemoryState(const fs::path &strPath)
{
	if (m_pBoard)
	{
		CMSFManager msf;
		msf.SetConfiguration(g_Config.GetBKModel());

		if (msf.OpenFile(strPath.c_str(), false))
		{
			StopTimer();
			m_pBoard->StopCPU(false);
			StoreIniParams();
			m_pBoard->RestoreState(msf, m_pScreen->GetScreenshot());
			m_pBoard->RunCPU(false);
			StartTimer();
		}
		else
		{
			return false;
		}
	}

	return true;
}

/* оставим код на память.
bool CMainFrame::MakeScreenShot()
{
    bool bRet = false;
    // это не совсем честно. на самом деле BK_SCREEN_WIDTH х BK_SCREEN_HEIGHT - это viewport экрана
    // а оригинальный размер формируемой текстуры экрана - 512х256
    // просто такой скриншот выглядит не очень красиво, слишком мелкий и сплющенный по высоте.
    HBITMAP hBitmap = (HBITMAP)CopyImage(m_pScreen->GetScreenshot(), IMAGE_BITMAP, BK_SCREEN_WIDTH, BK_SCREEN_HEIGHT, 0);

    CString strName;
    strName.Format(_T("screenshot_%d.bmp"), g_Config.m_nScreenshotNumber++);
    CString szFileName = g_Config.m_strScreenShotsPath + strName;

    CFile File;
    BITMAPINFO *pbmi;
    BITMAPFILEHEADER bfh;
    BITMAP bmp;

    // Получение параметров рисунка
    if (!GetObject(hBitmap, sizeof(bmp), &bmp))
        return false;

    // Количество битов под пиксель
    uint16_t wClrBits = (uint16_t)(bmp.bmPlanes*bmp.bmBitsPixel);
    if (wClrBits == 1);
    else if (wClrBits <= 4)
        wClrBits = 4;
    else if (wClrBits <= 8)
        wClrBits = 8;
    else if (wClrBits <= 16)
        wClrBits = 16;
    else if (wClrBits <= 24)
        wClrBits = 24;
    else if (wClrBits <= 32)
        wClrBits = 32;

    // Выделение памяти для BITMAPINFO
    if (wClrBits != 24)
    {
        pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1<<wClrBits));
    }
    else
    {
        pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));
    }
    // Заполнение BITMAPINFO
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biHeight = bmp.bmHeight;
    pbmi->bmiHeader.biWidth = bmp.bmWidth;
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
    if (wClrBits < 24)
        pbmi->bmiHeader.biClrUsed = (1<<wClrBits);
    pbmi->bmiHeader.biCompression = BI_RGB;

    // ..количество памяти, необходимое для таблицы цветов
    pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * wClrBits + 31) & ~31) / 8 * pbmi->bmiHeader.biHeight;
    pbmi->bmiHeader.biClrImportant = 0;

    // Получение памяти под таблицу цветов и массив индексов
    LPBYTE lpData = new uint8_t[pbmi->bmiHeader.biSizeImage];
    if (lpData)
    {
        // Копирование таблицы цветов в выделенную область памяти
        HDC hDC = GetDC()->GetSafeHdc();
        HDC hTempDC = CreateCompatibleDC(hDC);

        SelectObject(hTempDC, hBitmap);
        if (GetDIBits(hTempDC, hBitmap, 0, (uint16_t)bmp.bmHeight, lpData, pbmi, DIB_RGB_COLORS))
        {
            // Создание файла
            _tmkdir(g_Config.m_strScreenShotsPath);
            if (File.Open(szFileName, CFile::modeCreate | CFile::shareExclusive | CFile::modeWrite))
            {
                // Заполняем структуру информации о файле
                bfh.bfType = 0x4d42;
                // Смещение данных рисунка от начала файла
                bfh.bfOffBits = (DWORD)(sizeof(BITMAPFILEHEADER) + pbmi->bmiHeader.biSize +
                    pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD));
                bfh.bfSize = bfh.bfOffBits + pbmi->bmiHeader.biSizeImage;
                bfh.bfReserved1 = 0;
                bfh.bfReserved2 = 0;

                // Запись данных в файл, ошибки записи проигнорируем. ну и их нафиг, скриншот же.
                // ..BITMAPFILEHEADER
                File.Write(&bfh, sizeof(bfh));
                // ..BITMAPINFOHEADER и RGBQUAD
                File.Write(&pbmi->bmiHeader, sizeof(BITMAPINFOHEADER) + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD));
                // ..массив цветов и индексов
                File.Write(lpData, pbmi->bmiHeader.biSizeImage);
                // Запись закончена
                File.Close();
                bRet = true;
            }
        }
        DeleteDC(hTempDC);
        delete lpData;
    }
    DeleteObject(hBitmap);
    return bRet;
}
*/

bool CMainFrame::MakeScreenShot()
{
	bool bRet = false;
	HBITMAP hBitmap;

	if (g_Config.m_bOrigScreenshotSize)
	{
		hBitmap = m_pScreen->GetScreenshot();
	}
	else
	{
		// это не совсем честно. на самом деле svw.x и svw.y - это viewport экрана
		// а оригинальный размер формируемой текстуры экрана - 512х256
		// просто такой скриншот выглядит не очень красиво, слишком мелкий и сплющенный по высоте.
		CPoint svw = m_pScreen->GetScreenViewport();
		hBitmap = (HBITMAP)CopyImage(m_pScreen->GetScreenshot(), IMAGE_BITMAP, svw.x, svw.y, LR_COPYDELETEORG);
	}

	if (hBitmap)
	{
		CString strName = _T("screenshot_") +
		                  (g_Config.m_nDateInsteadOfScreenshotNumber ? MakeUniqueName() :
		                   Global::IntToString(g_Config.m_nScreenshotNumber++)) + _T(".png");
		fs::path szFileName = g_Config.m_strScreenShotsPath / strName.GetString();

		if (OpenClipboard())
		{
			HANDLE hRet = SetClipboardData(CF_BITMAP, hBitmap);
#ifdef _DEBUG

			if (!hRet) // нужно для отладки, когда что-то идёт не так
			{
				Global::GetLastErrorOut((LPTSTR)_T("SetClipboardData"));
			}

#endif
			CloseClipboard();
		}

		fs::create_directory(g_Config.m_strScreenShotsPath);
		CImage image; // оказывается, в MFC есть вот такая херота.
		image.Attach(hBitmap, CImage::DIBOR_DEFAULT);

		if (SUCCEEDED(image.Save(szFileName.c_str(), Gdiplus::ImageFormatPNG))) // и как всё оказывается просто.
		{
			bRet = true;
		}

		DeleteObject(hBitmap);
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	return bRet;
}

bool CMainFrame::CheckDebugMemmap()
{
	bool bRet = m_bBKMemViewOpen;

	if (bRet)   // если была открыта карта памяти
	{
		m_bBKMemViewOpen = false;
		m_pBKMemView->GetWindowRect(m_rectMemMap); // а пока закроем
		m_pBKMemView->DestroyWindow();
	}

	return bRet;    // скажем, что после её надо будет переоткрыть
}

void CMainFrame::SetDebugCtrlsState()
{
	if (m_pBoard)
	{
		m_paneRegistryDumpViewCPU.DisplayRegDump();
		m_paneRegistryDumpViewFDD.DisplayRegDump();

		// тормозит, если много строк на экране.
		for (auto &pane : m_arPaneMemoryDumpView)
		{
			pane.DisplayMemDump();
		}
	}
}

void CMainFrame::ChangeImageIcon(UINT nBtnID, FDD_DRIVE eDrive)
{
	if (m_pBoard)
	{
		int iImage = (m_pBoard->GetFDD()->IsAttached(eDrive)) ?
		             GetCmdMgr()->GetCmdImage(ID_FILE_LOADEDDRIVE) :
		             GetCmdMgr()->GetCmdImage(ID_FILE_EMPTYDRIVE);
		int nIndex = m_wndToolBar.CommandToIndex(nBtnID);
		m_wndToolBar.GetButton(nIndex)->SetImage(iImage);
		m_wndToolBar.InvalidateButton(nIndex); // Перерисовываем только нужную кнопку
//      m_wndToolBar.Invalidate(FALSE); // перерисовываем весь тулбар, а то какие-то непонятные явления в Вин8 случаются:
		// не желает перерисовываться кнопка.
	}
}

void CMainFrame::LoadFileImage(UINT nBtnID, FDD_DRIVE eDrive)
{
	CString strFilterIMG(MAKEINTRESOURCE(IDS_FILEFILTER_BKIMG));
	CLoadImgDlg dlg(true, nullptr, nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
	                strFilterIMG, m_pScreen->GetBackgroundWindow());
	dlg.GetOFN().lpstrInitialDir = g_Config.m_strIMGPath.c_str(); // диалог всегда будем начинать с домашней директории образов.

	if (dlg.DoModal() == IDOK)
	{
		fs::path str = dlg.GetPathName().GetString();
		// тут надо примонтировать str в заданный привод nDrive
		g_Config.SetDriveImgName(eDrive, str);

		if (m_pBoard)
		{
			m_pBoard->GetFDD()->AttachImage(eDrive, str);
		}
	}

	// nBtnID - ид кнопки, нужно заменить картинку, на картинку со значком.
	ChangeImageIcon(nBtnID, eDrive);
	SetFocusToBK();
}

void CMainFrame::StopAll()
{
	StopTimer();

	if (m_pBoard)
	{
		m_pBoard->StopCPU(); // остановка CPU - там прекращается обработка команд и поток работает вхолостую
		m_pBoard->StopTimerThread(); // остановка и завершение потока.
	}
}

void CMainFrame::StartAll()
{
	if (m_pBoard)
	{
		m_pBoard->StartTimerThread();
		m_pBoard->RunCPU();
	}

	StartTimer();
}

void CMainFrame::SetFocusToBK()
{
	auto vw = GetActiveView();

	if (vw)
	{
		vw->SetFocus();
	}
	else
	{
		TRACE("Nope Active Views. Focus set to main Window!\n");
		AfxGetMainWnd()->SetFocus();
	}
}


void CMainFrame::SetFocusToDebug()
{
	m_paneDisassembleView.SetFocus();
}

LRESULT CMainFrame::OnCpuBreak(WPARAM wParam, LPARAM lParam)
{
	if (m_paneDisassembleView.IsWindowVisible())
	{
		if (m_pBoard)
		{
			uint16_t pc = m_pBoard->GetRON(CCPU::REGISTER::PC);
			// прорисовываем окно дизассемблера
			m_pDebugger->SetCurrentAddress(pc);
		}
	}

	SetDebugCtrlsState();
	return S_OK;
}

void CMainFrame::OnFileLoadstate()
{
	CString strFilterMSF(MAKEINTRESOURCE(IDS_FILEFILTER_MSF));
	CLoadMemoryDlg dlg(true, nullptr, nullptr,
	                   OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
	                   strFilterMSF, m_pScreen->GetBackgroundWindow());
	dlg.GetOFN().lpstrInitialDir = g_Config.m_strMemPath.c_str();

	if (dlg.DoModal() == IDOK)
	{
		fs::path str = dlg.GetPathName().GetString();
		LoadMemoryState(str);
	}

	SetFocusToBK();
}

void CMainFrame::OnFileSavestate()
{
	m_pBoard->BreakCPU(); // ставим на паузу
	CString strFilterMSF(MAKEINTRESOURCE(IDS_FILEFILTER_MSF));
	CFileDialog dlg(false, _T("msf"), nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
	                strFilterMSF, m_pScreen->GetBackgroundWindow());
	dlg.GetOFN().lpstrInitialDir = g_Config.m_strMemPath.c_str();

	if (dlg.DoModal() == IDOK)
	{
		fs::path str = dlg.GetPathName().GetString();

		if (SaveMemoryState(str))
		{
			// тут должен быть какой-то код, но не придумалось
		}

		if (str.has_parent_path())
		{
			g_Config.m_strMemPath = str.parent_path();
		}
	}

	m_pBoard->UnbreakCPU(CMotherBoard::ADDRESS_NONE); // продолжаем выполнение
	SetFocusToBK();
}

void CMainFrame::OnFileLoadtape()
{
	CString strFilterTape(MAKEINTRESOURCE(IDS_FILEFILTER_TAPE_LOAD));
	CString strTapeExt(MAKEINTRESOURCE(IDS_FILEEXT_TAPE));
	CLoadTapeDlg dlg(true, strTapeExt, nullptr,
	                 OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR,
	                 strFilterTape, m_pScreen->GetBackgroundWindow());
	dlg.GetOFN().lpstrInitialDir = g_Config.m_strTapePath.c_str();

	if (dlg.DoModal() == IDOK)
	{
		fs::path str = dlg.GetPathName().GetString();
		StartPlayTape(str);
	}

	SetFocusToBK();
}

void CMainFrame::OnUpdateFileLoadtape(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!g_Config.m_bEmulateLoadTape);
}


void CMainFrame::OnFileScreenshot()
{
	MakeScreenShot();
}


void CMainFrame::OnCpuLongReset()
{
	m_bLongResetPress = true;
	OnCpuResetCpu();
}

void CMainFrame::OnCpuSuResetCpu()
{
	bool bSU = m_paneBKVKBDView.GetSUStatus();
	m_paneBKVKBDView.SetSUStatus(true);
	OnCpuResetCpu();
	m_paneBKVKBDView.SetSUStatus(bSU);
}

void CMainFrame::OnCpuResetCpu()
{
	if (m_pBoard && m_pBoard->IsCPURun()) // защита от множественного вызова функции.
	{
		m_Script.StopScript(); //выполнение скрипта прекратим. А то фигня какая-то получается.
		m_pBoard->StopCPU();
		BK_DEV_MPI fdd_model = m_pBoard->GetFDDType();

		// если контроллер А16М и сделали длинный ресет или контроллер СМК512 - делаем перезапуск
		// с адреса, который задаётся контроллером.
		if (
		    ((fdd_model == BK_DEV_MPI::A16M) && m_bLongResetPress) ||
		    (fdd_model == BK_DEV_MPI::SMK512)
		)
		{
			m_pBoard->SetAltProMode(ALTPRO_A16M_START_MODE);
			m_pBoard->SetAltProCode(0);
			m_pBoard->ResetCold(0);
		}
		else
		{
			// если у нас БК11, то можно реализовать СУ/ресет - перезапуск по 100000
			if ((m_pBoard->GetBoardModel() != BK_DEV_MPI::BK0010) && m_paneBKVKBDView.GetSUStatus())
			{
				m_pBoard->ResetCold(040000);
			}
			else
			{
				m_pBoard->ResetCold(0);
			}

			// СУ/ресет не реализовывается на контроллере СМК512 потому, что всегда подменяется
			// содержимое 177716 по чтению наложением ПЗУ.
			// А на А16М при коротком ресете и на обычных КНГМД СУ/ресет работает
		}

		m_bLongResetPress = false;
		// Запускаем CPU
		m_pBoard->RunCPU();

		// если установлен флаг остановки после создания, приостановим выполнение
		if (g_Config.m_bPauseCPUAfterStart)
		{
			m_pBoard->BreakCPU();
		}
	}
}

void CMainFrame::OnCpuRunbk0010()
{
	SetupConfiguration(CONF_BKMODEL::BK_0010);
}

void CMainFrame::OnCpuRunbk001001()
{
	SetupConfiguration(CONF_BKMODEL::BK_0010_01);
}

void CMainFrame::OnCpuRunbk001001Focal()
{
	SetupConfiguration(CONF_BKMODEL::BK_0010_01_MSTD);
}

void CMainFrame::OnCpuRunbk00100132k()
{
	SetupConfiguration(CONF_BKMODEL::BK_0010_01_EXT32RAM);
}

void CMainFrame::OnCpuRunbk001001Fdd()
{
	SetupConfiguration(CONF_BKMODEL::BK_0010_01_FDD);
}

void CMainFrame::OnCpuRunbk001001Fdd16k()
{
	SetupConfiguration(CONF_BKMODEL::BK_0010_01_A16M);
}

void CMainFrame::OnCpuRunbk001001FddSmk512()
{
	SetupConfiguration(CONF_BKMODEL::BK_0010_01_SMK512);
}

void CMainFrame::OnCpuRunbk001001FddSamara()
{
	SetupConfiguration(CONF_BKMODEL::BK_0010_01_SAMARA);
}

void CMainFrame::OnCpuRunbk0011()
{
	SetupConfiguration(CONF_BKMODEL::BK_0011);
}

void CMainFrame::OnCpuRunbk0011Fdd()
{
	SetupConfiguration(CONF_BKMODEL::BK_0011_FDD);
}

void CMainFrame::OnCpuRunbk0011FddA16m()
{
	SetupConfiguration(CONF_BKMODEL::BK_0011_A16M);
}

void CMainFrame::OnCpuRunbk0011FddSmk512()
{
	SetupConfiguration(CONF_BKMODEL::BK_0011_SMK512);
}

void CMainFrame::OnCpuRunbk0011FddSamara()
{
	SetupConfiguration(CONF_BKMODEL::BK_0011_SAMARA);
}

void CMainFrame::OnCpuRunbk0011m()
{
	SetupConfiguration(CONF_BKMODEL::BK_0011M);
}

void CMainFrame::OnCpuRunbk0011mFDD()
{
	SetupConfiguration(CONF_BKMODEL::BK_0011M_FDD);
}

void CMainFrame::OnCpuRunbk0011mFddA16m()
{
	SetupConfiguration(CONF_BKMODEL::BK_0011M_A16M);
}

void CMainFrame::OnCpuRunbk0011mFddSmk512()
{
	SetupConfiguration(CONF_BKMODEL::BK_0011M_SMK512);
}

void CMainFrame::OnCpuRunbk0011mFddSamara()
{
	SetupConfiguration(CONF_BKMODEL::BK_0011M_SAMARA);
}

void CMainFrame::OnUpdateCpuRunbk0010(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0010);
}

void CMainFrame::OnUpdateCpuRunbk001001(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0010_01);
}

void CMainFrame::OnUpdateCpuRunbk001001Focal(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0010_01_MSTD);
}

void CMainFrame::OnUpdateCpuRunbk00100132k(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0010_01_EXT32RAM);
}

void CMainFrame::OnUpdateCpuRunbk001001Fdd(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0010_01_FDD);
}

void CMainFrame::OnUpdateCpuRunbk001001Fdd16k(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0010_01_A16M);
}

void CMainFrame::OnUpdateCpuRunbk001001FddSmk512(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0010_01_SMK512);
}

void CMainFrame::OnUpdateCpuRunbk001001FddSamara(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0010_01_SAMARA);
}

void CMainFrame::OnUpdateCpuRunbk0011(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0011);
}

void CMainFrame::OnUpdateCpuRunbk0011Fdd(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0011_FDD);
}


void CMainFrame::OnUpdateCpuRunbk0011FddA16m(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0011_A16M);
}

void CMainFrame::OnUpdateCpuRunbk0011FddSmk512(CCmdUI *pCmdUI)
{
	// на БК11 СМК не работает, т.к. использует п/п ПЗУ БК11М
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0011_SMK512);
}

void CMainFrame::OnUpdateCpuRunbk0011FddSamara(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0011_SAMARA);
}

void CMainFrame::OnUpdateCpuRunbk0011m(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0011M);
}

void CMainFrame::OnUpdateCpuRunbk0011mFDD(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0011M_FDD);
}

void CMainFrame::OnUpdateCpuRunbk0011mFddA16m(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0011M_A16M);
}

void CMainFrame::OnUpdateCpuRunbk0011mFddSmk512(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0011M_SMK512);
}

void CMainFrame::OnUpdateCpuRunbk0011mFddSamara(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(g_Config.m_BKConfigModelNumber == CONF_BKMODEL::BK_0011M_SAMARA);
}


void CMainFrame::OnCpuAccelerate()
{
	if (m_pBoard)
	{
		m_pBoard->AccelerateCPU();
		UpdateStatusFreqPane();
	}
}

void CMainFrame::OnUpdateCpuAccelerate(CCmdUI *pCmdUI)
{
	bool bOn = (m_pBoard) ? m_pBoard->CanAccelerate() : false;
	pCmdUI->Enable(bOn);
}

void CMainFrame::OnCpuSlowdown()
{
	if (m_pBoard)
	{
		m_pBoard->SlowdownCPU();
		UpdateStatusFreqPane();
	}
}

void CMainFrame::OnUpdateCpuSlowdown(CCmdUI *pCmdUI)
{
	bool bOn = (m_pBoard) ? m_pBoard->CanSlowDown() : false;
	pCmdUI->Enable(bOn);
}

void CMainFrame::OnCpuNormalspeed()
{
	if (m_pBoard)
	{
		m_pBoard->NormalizeCPU();
		g_Config.m_nCPUTempFreq = -1;   // и выключаем режим макс. частоты
		UpdateStatusFreqPane();
	}
}

void CMainFrame::UpdateStatusFreqPane()
{
	m_paneRegistryDumpViewCPU.UpdateFreq();
	UpdateStatusFreq();
}


void CMainFrame::UpdateStatusFreq()
{
	CString str;
	str.Format(_T("CPU: %d Hz"), m_pBoard->GetCPUFreq());
	m_wndStatusBar.SetPaneText(static_cast<int>(STATUS_FIELD::CPU_FRQ), str.GetString());
}


void CMainFrame::OnCpuMaxspeed()
{
	if (m_pBoard)
	{
		if (g_Config.m_nCPUTempFreq < 0)    // если режим был выключен
		{
			g_Config.m_nCPUTempFreq = m_pBoard->GetCPUFreq(); // то текущую частоту сохраним
			m_pBoard->SetCPUFreq(m_pBoard->GetHighBound()); // и установим максимальную
		}
		else    // если режим был включен
		{
			m_pBoard->SetCPUFreq(g_Config.m_nCPUTempFreq); // возвращаем предыдущее значение
			g_Config.m_nCPUTempFreq = -1;   // и выключаем режим
		}

		UpdateStatusFreqPane();
	}
}

void CMainFrame::OnUpdateCpuMaxspeed(CCmdUI *pCmdUI)
{
	bool bChk = (g_Config.m_nCPUTempFreq >= 0);
	pCmdUI->SetCheck(bChk);
}


void CMainFrame::OnOptionsEnableSpeaker()
{
	g_Config.m_bSpeaker = !m_speaker.IsSoundEnabled();
	m_speaker.EnableSound(g_Config.m_bSpeaker);
}

void CMainFrame::OnUpdateOptionsEnableSpeaker(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_speaker.IsSoundEnabled());
}

void CMainFrame::OnOptionsEnableCovox()
{
	g_Config.m_bCovox = !m_covox.IsSoundEnabled();
	m_covox.EnableSound(g_Config.m_bCovox);

	if (g_Config.m_bCovox)
	{
		// выключим AY
		g_Config.m_bAY8910 = false;
		m_aySnd.EnableSound(false);
		// выключим менестрель
		g_Config.m_bMenestrel = false;
		m_menestrel.EnableSound(false);
	}
}

void CMainFrame::OnUpdateOptionsEnableCovox(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_covox.IsSoundEnabled());
}

void CMainFrame::OnOptionsStereoCovox()
{
	g_Config.m_bStereoCovox = !m_covox.IsStereo();
	m_covox.SetStereo(g_Config.m_bStereoCovox);
}

void CMainFrame::OnUpdateOptionsStereoCovox(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_covox.IsStereo());
}

void CMainFrame::OnOptionsEnableMenestrel()
{
	g_Config.m_bMenestrel = !m_menestrel.IsSoundEnabled();
	m_menestrel.EnableSound(g_Config.m_bMenestrel);

	if (g_Config.m_bMenestrel)
	{
		// выключим ковокс
		g_Config.m_bCovox = false;
		m_covox.EnableSound(false);
		// выключим AY
		g_Config.m_bAY8910 = false;
		m_aySnd.EnableSound(false);
	}
}

void CMainFrame::OnUpdateOptionsEnableMenestrel(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_menestrel.IsSoundEnabled());
}

void CMainFrame::OnOptionsEnableAy8910()
{
	g_Config.m_bAY8910 = !m_aySnd.IsSoundEnabled();
	m_aySnd.EnableSound(g_Config.m_bAY8910);

	if (g_Config.m_bAY8910)
	{
		// выключим ковокс
		g_Config.m_bCovox = false;
		m_covox.EnableSound(false);
		// выключим менестрель
		g_Config.m_bMenestrel = false;
		m_menestrel.EnableSound(false);
	}
}

void CMainFrame::OnUpdateOptionsEnableAy8910(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_aySnd.IsSoundEnabled());
}

void CMainFrame::OnOptionsSpeakerFilter()
{
	g_Config.m_bSpeakerFilter = !m_speaker.IsFilter();
	m_speaker.SetFilter(g_Config.m_bSpeakerFilter);
}

void CMainFrame::OnUpdateOptionsSpeakerFilter(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_speaker.IsFilter());
}

void CMainFrame::OnOptionsCovoxFilter()
{
	g_Config.m_bCovoxFilter = !m_covox.IsFilter();
	m_covox.SetFilter(g_Config.m_bCovoxFilter);
}

void CMainFrame::OnUpdateOptionsCovoxFilter(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_covox.IsFilter());
}

void CMainFrame::OnOptionsMenestrelFilter()
{
	g_Config.m_bMenestrelFilter = !m_menestrel.IsFilter();
	m_menestrel.SetFilter(g_Config.m_bMenestrelFilter);
}

void CMainFrame::OnUpdateOptionsMenestrelFilter(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_menestrel.IsFilter());
}

void CMainFrame::OnOptionsAy8910Filter()
{
	g_Config.m_bAY8910Filter = !m_aySnd.IsFilter();
	m_aySnd.SetFilter(g_Config.m_bAY8910Filter);
}

void CMainFrame::OnUpdateOptionsAy8910Filter(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_aySnd.IsFilter());
}


void CMainFrame::OnOptionsSpeakerDcoffset()
{
	g_Config.m_bSpeakerDCOffset = !m_speaker.IsDCOffsetCorrect();
	m_speaker.SetDCOffsetCorrect(g_Config.m_bSpeakerDCOffset);
}


void CMainFrame::OnUpdateOptionsSpeakerDcoffset(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_speaker.IsDCOffsetCorrect());
}


void CMainFrame::OnOptionsCovoxDcoffset()
{
	g_Config.m_bCovoxDCOffset = !m_covox.IsDCOffsetCorrect();
	m_covox.SetDCOffsetCorrect(g_Config.m_bCovoxDCOffset);
}


void CMainFrame::OnUpdateOptionsCovoxDcoffset(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_covox.IsDCOffsetCorrect());
}


void CMainFrame::OnOptionsMenestrelDcoffset()
{
	g_Config.m_bMenestrelDCOffset = !m_menestrel.IsDCOffsetCorrect();
	m_menestrel.SetDCOffsetCorrect(g_Config.m_bMenestrelDCOffset);
}


void CMainFrame::OnUpdateOptionsMenestrelDcoffset(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_menestrel.IsDCOffsetCorrect());
}


void CMainFrame::OnOptionsAy8910Dcoffset()
{
	g_Config.m_bAY8910DCOffset = !m_aySnd.IsDCOffsetCorrect();
	m_aySnd.SetDCOffsetCorrect(g_Config.m_bAY8910DCOffset);
}


void CMainFrame::OnUpdateOptionsAy8910Dcoffset(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_aySnd.IsDCOffsetCorrect());
}

void CMainFrame::OnOptionsEmulateBkkeyboard()
{
	g_Config.m_bBKKeyboard = !g_Config.m_bBKKeyboard;
}

void CMainFrame::OnUpdateOptionsEmulateBkkeyboard(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bBKKeyboard);
}


void CMainFrame::OnOptionsEnableJoystick()
{
	g_Config.m_bJoystick = !g_Config.m_bJoystick;

	if (g_Config.m_bJoystick) // если включаем джойстик
	{
		g_Config.m_bICLBlock = false; // блок нагрузок выключаем
	}
}

void CMainFrame::OnUpdateOptionsEnableJoystick(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bJoystick);
}

void CMainFrame::OnDebugEnableIclblock()
{
	g_Config.m_bICLBlock = !g_Config.m_bICLBlock;

	if (g_Config.m_bICLBlock) // если включаем блок нагрузок
	{
		g_Config.m_bJoystick = false; // джойстик выключаем
	}
}

void CMainFrame::OnUpdateDebugEnableIclblock(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bICLBlock);
}

void CMainFrame::OnOptionsEmulateFddio()
{
	g_Config.m_bEmulateFDDIO = !g_Config.m_bEmulateFDDIO;
}

void CMainFrame::OnUpdateOptionsEmulateFddio(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bEmulateFDDIO);
}

void CMainFrame::OnOptionsUseSavesdirectory()
{
	g_Config.m_bSavesDefault = !g_Config.m_bSavesDefault;
}

void CMainFrame::OnUpdateOptionsUseSavesdirectory(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bSavesDefault);
	pCmdUI->Enable(g_Config.m_bEmulateSaveTape);
}

void CMainFrame::OnOptionsEmulateTapeLoading()
{
	g_Config.m_bEmulateLoadTape = !g_Config.m_bEmulateLoadTape;
}

void CMainFrame::OnUpdateOptionsEmulateTapeLoading(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bEmulateLoadTape);
}

void CMainFrame::OnOptionsEmulateTapeSaving()
{
	g_Config.m_bEmulateSaveTape = !g_Config.m_bEmulateSaveTape;
}

void CMainFrame::OnUpdateOptionsEmulateTapeSaving(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bEmulateSaveTape);
}

void CMainFrame::OnOptionsTapemanager()
{
	auto pdlg = std::make_unique<CTapeManagerDlg>(m_pScreen->GetBackgroundWindow());

	if (pdlg)
	{
		pdlg->DoModal();
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	SetFocusToBK();
}

void CMainFrame::OnAppSettings()
{
	auto pSettingsDlg = std::make_unique<CSettingsDlg>();

	if (pSettingsDlg)
	{
		if (m_pBoard)
		{
			m_pBoard->StopCPU(); // остановка CPU, чтобы ничего не попортить
		}

		// возвращаются три своих кастомных значения
		INT_PTR res = pSettingsDlg->DoModal();

		if (res != NO_CHANGES)
		{
			// обновим данные конфигов и параметров и пользовательский интерфейс
			// в соответствии с новыми данными
			InitEmulator();
			// в этой функции не всё обязательно обновлять в этом месте
			// но чтобы не дублировать функции, будем вызывать её. Всё равно ничего страшного
			// не случится.
			// ещё нужно обновить значение частоты, а то оно старым перебивается.
			m_pBoard->NormalizeCPU();
		}

		if (res == CHANGES_NEEDREBOOT) // если нужен перезапуск
		{
			// перезапускаем конфигурацию.
			CONF_BKMODEL n = g_Config.GetBKModel();
			m_Script.StopScript();
			ConfigurationConstructor(n); // конфиг сохраняем там.
		}
		else
		{
			// если не нужен перезапуск или вообще отмена - просто возобновляем работу
			if (m_pBoard)
			{
				m_pBoard->RunCPU();
			}
		}

		SetFocusToBK();
	}
}

void CMainFrame::OnPaletteEdit()
{
	auto pPaletteDlg = std::make_unique <CBKPaletteDlg>();

	if (pPaletteDlg)
	{
		INT_PTR res = pPaletteDlg->DoModal();

		if (res == IDOK)
		{
			m_pScreen->InitColorTables();
		}
	}
}

void CMainFrame::OnOptionsJoyedit()
{
	auto pJoyEditDlg = std::make_unique <CJoyEditDlg>();

	if (pJoyEditDlg)
	{
		INT_PTR res = pJoyEditDlg->DoModal();
	}
}

void CMainFrame::OnSettAyvolpan()
{
	auto pAYVolPanDlg = std::make_unique <CBKAYVolPan>();

	if (pAYVolPanDlg)
	{
		INT_PTR res = pAYVolPanDlg->DoModal();
	}
}


void CMainFrame::OnDebugBreak()
{
	if (m_pBoard)
	{
		if (m_pBoard->IsCPUBreaked())
		{
			m_pBoard->UnbreakCPU(CMotherBoard::ADDRESS_NONE);
			SetFocusToBK();
		}
		else
		{
			m_pBoard->BreakCPU();
			SetFocusToDebug();
		}
	}
}

void CMainFrame::OnUpdateDebugBreak(CCmdUI *pCmdUI)
{
	if (m_pBoard)
	{
		int nIndex = m_wndToolBarDebug.CommandToIndex(ID_DEBUG_STARTBREAK);
		CMFCToolBarButton *pBtn = m_wndToolBarDebug.GetButton(nIndex);
		CString strMenu;
		int iImage;

		if (m_pBoard->IsCPUBreaked())
		{
			strMenu = CString(MAKEINTRESOURCE(IDS_MENU_DEBUG_CONTINUE));
			iImage = GetCmdMgr()->GetCmdImage(ID_DEBUG_START);
		}
		else
		{
			strMenu = CString(MAKEINTRESOURCE(IDS_MENU_DEBUG_BREAK));
			iImage = GetCmdMgr()->GetCmdImage(ID_DEBUG_BREAK);
		}

		pBtn->SetImage(iImage);
		pCmdUI->SetText(strMenu);
		m_wndToolBarDebug.UpdateData();
		m_wndToolBarDebug.RedrawWindow();
	}
}

void CMainFrame::OnDebugStepinto()
{
	if (m_pBoard)
	{
		m_pBoard->RunInto();
	}
}

void CMainFrame::OnUpdateDebugStepinto(CCmdUI *pCmdUI)
{
	pCmdUI->Enable((m_pBoard) ? m_pBoard->IsCPUBreaked() : FALSE);
}

void CMainFrame::OnDebugStepover()
{
	if (m_pBoard)
	{
		m_pBoard->RunOver();
	}
}

void CMainFrame::OnUpdateDebugStepover(CCmdUI *pCmdUI)
{
	pCmdUI->Enable((m_pBoard) ? m_pBoard->IsCPUBreaked() : FALSE);
}

void CMainFrame::OnDebugStepout()
{
	if (m_pBoard)
	{
		m_pBoard->RunOut();
	}
}

void CMainFrame::OnUpdateDebugStepout(CCmdUI *pCmdUI)
{
	pCmdUI->Enable((m_pBoard) ? m_pBoard->IsCPUBreaked() : FALSE);
}

void CMainFrame::OnDebugRuntocursor()
{
	if (m_pBoard)
	{
		m_pBoard->RunToAddr(m_pDebugger->GetCursorAddress());
	}
}

void CMainFrame::OnUpdateDebugRuntocursor(CCmdUI *pCmdUI)
{
	pCmdUI->Enable((m_pBoard) ? m_pBoard->IsCPUBreaked() : FALSE);
}

void CMainFrame::OnDebugBreakpoint()
{
	if (!m_pDebugger->SetSimpleBreakpoint())
	{
		m_pDebugger->RemoveBreakpoint();
	}

	m_paneDisassembleView.Invalidate(FALSE);
}

void CMainFrame::OnDebugMemmap()
{
	if (m_pBoard)
	{
		/*
		TODO доделать:
		1. синхронизацию. Нельзя удалять, пока работает DrawCurrentTab и нельзя его запускать когда объект удаляется.
		Если перед удалением объекта останавливать поток TimerThreadFunc, то конфликтов не будет.
		Удаление никогда не будет работать параллельно с DrawCurrentTab
		2. подумать насчёт переделки в DockingTab. хоть размеры и не способствуют, может будет проще управлять
		этой штукой, если она будет DockingTab с запрещённым докингом.
		*/
		m_bBKMemViewOpen = false;

		if (m_pBKMemView) // если раньше окно уже было открыто, а мы снова пробуем открыть
		{
			m_pBKMemView->DestroyWindow(); // старое окно удалим
		}

		// и пойдём создавать новое окно
		auto pBKMemVw = new CBKMEMDlg(m_pBoard->GetBoardModel(), m_pBoard->GetFDDType(),
		                              m_pBoard->GetMainMemory(), m_pBoard->GetAddMemory(), this); // обязательно создавать динамически.

		if (pBKMemVw)
		{
			if (pBKMemVw->Create(IDD_BKMEM_MAP_DLG, this))
			{
				if (m_rectMemMap != CRect(0, 0, 0, 0))
				{
					pBKMemVw->MoveWindow(m_rectMemMap, FALSE);
				}

				pBKMemVw->ShowWindow(SW_SHOW);
				m_pBKMemView = pBKMemVw;
				m_bBKMemViewOpen = true;
			}
		}
		else
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		}
	}
}

// эта функция нужна только для того, чтобы m_pBKMemView присвоить nullptr
// когда мы закрываем карту памяти кнопкой крестик в правом верхнем углу
LRESULT CMainFrame::OnMemMapClose(WPARAM, LPARAM)
{
	m_bBKMemViewOpen = false; // при закрытии крестиком ещё и эту переменную надо разблокировать
	m_pBKMemView = nullptr;
	return S_OK;
}

LRESULT CMainFrame::OnMemDumpUpdate(WPARAM, LPARAM)
{
	for (auto &pane : m_arPaneMemoryDumpView)
	{
		pane.DisplayMemDump();
	}

	return S_OK;
}


void CMainFrame::OnDebugDumpregsInterval(UINT id)
{
	switch (id)
	{
		default:
		case ID_DEBUG_DUMPREGS_INTERVAL_0:
			g_Config.m_nRegistersDumpInterval = 0;
			break;

		case ID_DEBUG_DUMPREGS_INTERVAL_1:
			g_Config.m_nRegistersDumpInterval = 1;
			break;

		case ID_DEBUG_DUMPREGS_INTERVAL_2:
			g_Config.m_nRegistersDumpInterval = 2;
			break;

		case ID_DEBUG_DUMPREGS_INTERVAL_3:
			g_Config.m_nRegistersDumpInterval = 3;
			break;

		case ID_DEBUG_DUMPREGS_INTERVAL_4:
			g_Config.m_nRegistersDumpInterval = 4;
			break;

		case ID_DEBUG_DUMPREGS_INTERVAL_5:
			g_Config.m_nRegistersDumpInterval = 5;
			break;

		case ID_DEBUG_DUMPREGS_INTERVAL_10:
			g_Config.m_nRegistersDumpInterval = 10;
			break;

		case ID_DEBUG_DUMPREGS_INTERVAL_15:
			g_Config.m_nRegistersDumpInterval = 15;
			break;

		case ID_DEBUG_DUMPREGS_INTERVAL_20:
			g_Config.m_nRegistersDumpInterval = 20;
			break;

		case ID_DEBUG_DUMPREGS_INTERVAL_25:
			g_Config.m_nRegistersDumpInterval = 25;
			break;

		case ID_DEBUG_DUMPREGS_INTERVAL_50:
			g_Config.m_nRegistersDumpInterval = 50;
			break;
	}
}

void CMainFrame::OnUpdateDebugDumpregsInterval(CCmdUI *pCmdUI)
{
	switch (g_Config.m_nRegistersDumpInterval)
	{
		default:
			g_Config.m_nRegistersDumpInterval = 0;
			[[fallthrough]];
		// тут break не нужен! Но и стоять это должно строго перед case 0:
		case 0:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_0);
			break;

		case 1:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_1);
			break;

		case 2:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_2);
			break;

		case 3:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_3);
			break;

		case 4:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_4);
			break;

		case 5:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_5);
			break;

		case 10:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_10);
			break;

		case 15:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_15);
			break;

		case 20:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_20);
			break;

		case 25:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_25);
			break;

		case 50:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_DEBUG_DUMPREGS_INTERVAL_50);
			break;
	}
}

void CMainFrame::OnDebugDialogAskForBreak()
{
	if (m_pBoard)
	{
		g_Config.m_bAskForBreak = !g_Config.m_bAskForBreak;
	}
}

void CMainFrame::OnUpdateDebugDialogAskForBreak(CCmdUI *pCmdUI)
{
	if (m_pBoard)
	{
		pCmdUI->SetCheck(g_Config.m_bAskForBreak);
	}
}

void CMainFrame::OnDebugPauseCpuAfterStart()
{
	g_Config.m_bPauseCPUAfterStart = !g_Config.m_bPauseCPUAfterStart;
}

void CMainFrame::OnUpdateDebugPauseCpuAfterStart(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bPauseCPUAfterStart);
}


void CMainFrame::OnOptionsShowPerformanceOnStatusbar()
{
	g_Config.m_bShowPerformance = !g_Config.m_bShowPerformance;

	if (!g_Config.m_bShowPerformance)
	{
		m_wndStatusBar.SetPaneText(static_cast<int>(STATUS_FIELD::INFO), _T(""));
	}
}

void CMainFrame::OnUpdateOptionsShowPerformanceOnStatusbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bShowPerformance);
}


void CMainFrame::OnOptionsNativeruslatswitch()
{
	g_Config.m_bNativeRusLatSwitch = !g_Config.m_bNativeRusLatSwitch;
}


void CMainFrame::OnUpdateOptionsNativeruslatswitch(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bNativeRusLatSwitch);
}

void CMainFrame::OnVkbdtypeKeys(UINT id)
{
	switch (id)
	{
		default:
		case ID_VKBDTYPE_KEYS:
			g_Config.m_nVKBDType = 0;
			m_paneBKVKBDView.SetKeyboardView(IDB_BITMAP_SOFT);
			break;

		case ID_VKBDTYPE_MEMBRANE:
			g_Config.m_nVKBDType = 1;
			m_paneBKVKBDView.SetKeyboardView(IDB_BITMAP_PLEN);
			break;
	}
}

void CMainFrame::OnUpdateVkbdtypeKeys(CCmdUI *pCmdUI)
{
	switch (g_Config.m_nVKBDType)
	{
		default:
			g_Config.m_nVKBDType = 0; // тут break не нужен! Но и стоять это должно строго перед case 0:

		case 0:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VKBDTYPE_KEYS);
			break;

		case 1:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VKBDTYPE_MEMBRANE);
			break;
	}
}

void CMainFrame::OnViewFullscreenmode()
{
	if (m_pBoard)
	{
		m_pBoard->StopCPU(false);

		if (m_pScreen->IsFullScreenMode())
		{
			m_pScreen->SetWindowMode();
		}
		else
		{
			m_pScreen->SetFullScreenMode();
		}

		// вышеприведённые функции в процессе работы меняют флаг g_Config.m_bFullscreenMode
		g_Config.m_bFullscreenMode = m_pScreen->IsFullScreenMode(); // поэтому его надо восстановить
		m_pBoard->RunCPU(false);
		theApp.GetMainWnd()->ShowWindow(SW_RESTORE); // после выхода из полноэкранного режима окно надо полностью перерисовать
		// а то после вызова диалогов в полноэкранном режиме, при выходе из полноэкранного режима не перерисовывается меню и тулбар
	}
}

void CMainFrame::OnViewSmoothing()
{
	g_Config.m_bSmoothing = !m_pScreen->IsSmoothing();
	m_pScreen->SetSmoothing(g_Config.m_bSmoothing);
}

void CMainFrame::OnUpdateViewSmoothing(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pScreen->IsSmoothing());
}

void CMainFrame::OnViewColormode()
{
	g_Config.m_bColorMode = !m_pScreen->IsColorMode();
	m_pScreen->SetColorMode(g_Config.m_bColorMode);
}

void CMainFrame::OnUpdateViewColormode(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pScreen->IsColorMode());
}

void CMainFrame::OnViewAdaptivebwmode()
{
	g_Config.m_bAdaptBWMode = !m_pScreen->IsAdaptMode();
	m_pScreen->SetAdaptMode(g_Config.m_bAdaptBWMode);
}

void CMainFrame::OnUpdateViewAdaptivebwmode(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pScreen->IsColorMode());
	pCmdUI->SetCheck(m_pScreen->IsAdaptMode());
}

void CMainFrame::OnViewLuminoforemode()
{
	g_Config.m_bLuminoforeEmulMode = !m_pScreen->GetLuminoforeEmuMode();
	m_pScreen->SetLuminoforeEmuMode(g_Config.m_bLuminoforeEmulMode);
}

void CMainFrame::OnUpdateViewLuminoforemode(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pScreen->GetLuminoforeEmuMode());
}

void CMainFrame::OnToolLaunch(UINT id)
{
	LaunchTool(id - ID_TOOL_MENU_0);
}

void CMainFrame::OnFileLoadDrive(UINT id)
{
	FDD_DRIVE nDrive = FDD_DRIVE::NONE;

	switch (id)
	{
		case ID_FILE_LOADDRIVE_A:
			nDrive = FDD_DRIVE::A;
			break;

		case ID_FILE_LOADDRIVE_B:
			nDrive = FDD_DRIVE::B;
			break;

		case ID_FILE_LOADDRIVE_C:
			nDrive = FDD_DRIVE::C;
			break;

		case ID_FILE_LOADDRIVE_D:
			nDrive = FDD_DRIVE::D;
			break;

		default:
			return;
	}

	LoadFileImage(id, nDrive);
}

void CMainFrame::OnUpdateFileLoadDrive(CCmdUI *pCmdUI)
{
	FDD_DRIVE nDrive = FDD_DRIVE::NONE;

	switch (pCmdUI->m_nID)
	{
		case ID_FILE_LOADDRIVE_A:
			nDrive = FDD_DRIVE::A;
			break;

		case ID_FILE_LOADDRIVE_B:
			nDrive = FDD_DRIVE::B;
			break;

		case ID_FILE_LOADDRIVE_C:
			nDrive = FDD_DRIVE::C;
			break;

		case ID_FILE_LOADDRIVE_D:
			nDrive = FDD_DRIVE::D;
			break;

		default:
			return;
	}

	pCmdUI->Enable(m_pBoard ? m_pBoard->GetFDD()->GetDriveState(nDrive) : FALSE);
}

void CMainFrame::OnFileUnmount(UINT id)
{
	FDD_DRIVE nDrive = FDD_DRIVE::NONE;
	UINT nIconID = 0;

	switch (id)
	{
		case ID_FILE_UMOUNT_A:
			nDrive = FDD_DRIVE::A;
			nIconID = ID_FILE_LOADDRIVE_A;
			break;

		case ID_FILE_UMOUNT_B:
			nDrive = FDD_DRIVE::B;
			nIconID = ID_FILE_LOADDRIVE_B;
			break;

		case ID_FILE_UMOUNT_C:
			nDrive = FDD_DRIVE::C;
			nIconID = ID_FILE_LOADDRIVE_C;
			break;

		case ID_FILE_UMOUNT_D:
			nDrive = FDD_DRIVE::D;
			nIconID = ID_FILE_LOADDRIVE_D;
			break;

		default:
			return;
	}

	if (m_pBoard)
	{
		m_pBoard->GetFDD()->DetachImage(nDrive);
		ChangeImageIcon(nIconID, nDrive);
	}

	g_Config.SetDriveImgName(nDrive, {g_strEmptyUnit.GetString()});
}

void CMainFrame::OnFileOpenInBKDE(UINT id)
{
	FDD_DRIVE nDrive = FDD_DRIVE::NONE;
	UINT nIconID = 0;

	switch (id)
	{
		case ID_FILE_OPENIN_A:
			nDrive = FDD_DRIVE::A;
			nIconID = ID_FILE_LOADDRIVE_A;
			break;

		case ID_FILE_OPENIN_B:
			nDrive = FDD_DRIVE::B;
			nIconID = ID_FILE_LOADDRIVE_B;
			break;

		case ID_FILE_OPENIN_C:
			nDrive = FDD_DRIVE::C;
			nIconID = ID_FILE_LOADDRIVE_C;
			break;

		case ID_FILE_OPENIN_D:
			nDrive = FDD_DRIVE::D;
			nIconID = ID_FILE_LOADDRIVE_D;
			break;

		default:
			return;
	}

	if (m_pBoard && g_nToolBKDEMenuIdx != -1)
	{
		fs::path strImageName = g_Config.GetDriveImgName(nDrive);

		if (!Global::isEmptyUnit(strImageName))
		{
			StopAll(); // всё остановим
			// отмонтируем нужный нам образ
			m_pBoard->GetFDD()->DetachImage(nDrive);
			ChangeImageIcon(nIconID, nDrive);
			//вот тут действие по запуску проги
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));
			CString t = CString(g_pToolsMenuData[g_nToolBKDEMenuIdx]).Trim(_T('\"')).Trim();
			t += _T(" ") + CString(strImageName.c_str());

			if (CreateProcess(nullptr,                // No module name (use command line)
			                  t.GetBuffer(),          // Command line
			                  nullptr,                // Process handle not inheritable
			                  nullptr,                // Thread handle not inheritable
			                  FALSE,                  // Set handle inheritance to FALSE
			                  NORMAL_PRIORITY_CLASS,  // No creation flags
			                  nullptr,                // Use parent's environment block
			                  nullptr,                // Use parent's starting directory
			                  &si,                    // Pointer to STARTUPINFO structure
			                  &pi))                   // Pointer to PROCESS_INFORMATION structure
			{
				// Wait until child process exits.
				WaitForSingleObject(pi.hProcess, INFINITE);
				// Close process and thread handles.
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}

			// обратно примонтируем образ
			m_pBoard->GetFDD()->AttachImage(nDrive, strImageName);
			ChangeImageIcon(nIconID, nDrive);
			StartAll(); // всё запускаем
		}
	}
}

/*
 Как оказалось, единственно возможный способ заменить текст в выпадающем меню -
 перехватить событие, и подменять строку при вызове меню.
*/
BOOL CMainFrame::OnShowPopupMenu(CMFCPopupMenu *pMenuPopup)
{
	if (pMenuPopup)
	{
		auto pParentButton = pMenuPopup->GetParentButton();

		if (pParentButton)
		{
			if (pParentButton->m_strText == _T("Установить размер экрана"))
			{
				// тут динамически формируем меню видов размеров экрана
				double d = m_pScreen->getAspectRatio();
				int n = pMenuPopup->GetMenuItemCount();

				if (n > 0)
				{
					for (int i = 0; i < n - 1; ++i)
					{
						auto item = pMenuPopup->GetMenuItem(i);
						CString str;
						int x = m_aScreenSizes[i + 1];
						int y = int(double(x) / d);
						str.Format(m_strMenuScrSizes[i], x, y);
						item->m_strText = str;
					}
				}
			}
			else
			{
				// тут формируем выпадающие меню кнопок приводов
				FDD_DRIVE eDrive = FDD_DRIVE::NONE;

				switch (pParentButton->m_nID)
				{
					case ID_FILE_LOADDRIVE_A:
						eDrive = FDD_DRIVE::A;
						break;

					case ID_FILE_LOADDRIVE_B:
						eDrive = FDD_DRIVE::B;
						break;

					case ID_FILE_LOADDRIVE_C:
						eDrive = FDD_DRIVE::C;
						break;

					case ID_FILE_LOADDRIVE_D:
						eDrive = FDD_DRIVE::D;
						break;
				}

				if (m_pBoard && eDrive != FDD_DRIVE::NONE)
				{
					// заменяемое значение пункта меню
					CString strn = m_pBoard->GetFDD()->IsAttached(eDrive) ?
					               CString(g_Config.GetShortDriveImgName(eDrive).c_str()) : g_strEmptyUnit;
					auto item = pMenuPopup->GetMenuItem(pMenuPopup->GetMenuItemCount() - 1);
					item->m_strText = strn;

					if (g_nToolBKDEMenuIdx == -1 || !m_pBoard->GetFDD()->IsAttached(eDrive))
					{
						// хер знает как запретить пункт меню, но можно скрыть
						auto item = pMenuPopup->GetMenuItem(1);
						item->SetVisible(FALSE);
					}
				}
			}

			return CFrameWndEx::OnShowPopupMenu(pMenuPopup);
		}
	}

	return TRUE;
}

void CMainFrame::OnSetScreenAspect(UINT id)
{
	switch (id)
	{
		case ID_VIEW_ASPECT1X2:
			m_pScreen->SetAspectRatio(1.0 / 2.0);
			break;

		case ID_VIEW_ASPECT3X4:
			m_pScreen->SetAspectRatio(3.0 / 4.0);
			break;

		case ID_VIEW_ASPECT1X1:
			m_pScreen->SetAspectRatio(1.0);
			break;

		case ID_VIEW_ASPECT4X3:
			m_pScreen->SetAspectRatio(4.0 / 3.0);
			break;

		case ID_VIEW_ASPECT2X1:
			m_pScreen->SetAspectRatio(2.0);
			break;

		case ID_VIEW_ASPECT5X4:
			m_pScreen->SetAspectRatio(5.0 / 4.0);
			break;
	}

	InitScrViewPort(m_nScreen_X);
}

void CMainFrame::OnUpdateSetScreenAspect(CCmdUI *pCmdUI)
{
	double d = m_pScreen->getAspectRatio();

	if (d == 0.5)
	{
		pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_ASPECT1X2);
	}
	else if (d == 3.0 / 4.0)
	{
		pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_ASPECT3X4);
	}
	else if (d == 1.0)
	{
		pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_ASPECT1X1);
	}
	else if (d == 4.0 / 3.0)
	{
		pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_ASPECT4X3);
	}
	else if (d == 2.0)
	{
		pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_ASPECT2X1);
	}
	else if (d == 5.0 / 4.0)
	{
		pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_ASPECT5X4);
	}
	else
	{
		pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_ASPECTCUSTOM);
	}
}


CString CMainFrame::m_strMenuScrSizes[SCREENSIZE_NUMBER] =
{
	_T("%d x %d (x0.5)"),
	_T("%d x %d"),
	_T("%d x %d"),
	_T("%d х %d (x1)"),
	_T("%d x %d"),
	_T("%d x %d (x1.5)"),
	_T("%d x %d (x2)"),
	_T("%d x %d (x2.5)"),
	_T("%d x %d (x3)"),
	_T("%d x %d (x3.5)"),
	_T("%d x %d (x4)"),
	_T("Свой"),
};

int CMainFrame::m_aScreenSizes[SCREENSIZE_NUMBER] =
{
	-1, // 0
	    256, // 1
	    324, // 2
	    432, // 3
	    512, // 4
	    576, // 5
	    768, // 6
	    1024, // 7
	    1280, // 8
	    1536, // 9
	    1792, // 10
	    2048, // 11
    };

void CMainFrame::OnSetScreenSize(UINT id)
{
	switch (id)
	{
		default:
		case ID_VIEW_SCREENSIZE_CUSTOM:
			m_nScrSizeNum = SCREENSIZE_CUSTOM;
			m_nScreen_X = m_nScreen_CustomX ? m_nScreen_CustomX : m_aScreenSizes[4];
			break;

		case ID_VIEW_SCREENSIZE_256X192: // x0.5
			m_nScrSizeNum = SCREENSIZE_256X192;
			break;

		case ID_VIEW_SCREENSIZE_324X243:
			m_nScrSizeNum = SCREENSIZE_324X243;
			break;

		case ID_VIEW_SCREENSIZE_432X324:
			m_nScrSizeNum = SCREENSIZE_432X324;
			break;

		case ID_VIEW_SCREENSIZE_512X384: // x1
			m_nScrSizeNum = SCREENSIZE_512X384;
			break;

		case ID_VIEW_SCREENSIZE_576X432:
			m_nScrSizeNum = SCREENSIZE_576X432;
			break;

		case ID_VIEW_SCREENSIZE_768X576: // x1.5
			m_nScrSizeNum = SCREENSIZE_768X576;
			break;

		case ID_VIEW_SCREENSIZE_1024X768: // x2
			m_nScrSizeNum = SCREENSIZE_1024X768;
			break;

		case ID_VIEW_SCREENSIZE_1280X960: // x2.5
			m_nScrSizeNum = SCREENSIZE_1280X960;
			break;

		case ID_VIEW_SCREENSIZE_1536X1152: // x3
			m_nScrSizeNum = SCREENSIZE_1536X1152;
			break;

		case ID_VIEW_SCREENSIZE_1792X1344: // x3.5
			m_nScrSizeNum = SCREENSIZE_1792X1344;
			break;

		case ID_VIEW_SCREENSIZE_2048X1536: // x4
			m_nScrSizeNum = SCREENSIZE_2048X1536;
			break;
	}

	if (m_nScrSizeNum)
	{
		m_nScreen_X = m_aScreenSizes[m_nScrSizeNum];
	}

	InitScrViewPort(m_nScreen_X);
}


void CMainFrame::InitScrViewPort(int w)
{
	int h = m_pScreen->SetScreenViewport(w);
	CRect rectView; // размеры области CBKView
	CRect rectMain; // размеры главного окна
	GetActiveView()->GetClientRect(&rectView);
	GetWindowRect(rectMain);
	int offsetX = rectView.Width() - w; // дельта X;
	int offsetY = rectView.Height() - h; // дельта Y;
	int newsizeX = rectMain.Width() - offsetX;
	int newsizeY = rectMain.Height() - offsetY;
	SetWindowPos(nullptr, rectMain.left, rectMain.top, newsizeX, newsizeY, SWP_SHOWWINDOW | SWP_NOZORDER);
}



LRESULT CMainFrame::OnScreenSizeChanged(WPARAM wParam, LPARAM lParam)
{
	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	int nPixelH = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);
	m_nScreen_X = ::MulDiv(int(wParam), DEFAULT_DPIX, nPixelW);

	for (int i = SCREENSIZE_256X192; i < SCREENSIZE_NUMBER; ++i)
	{
		if (m_nScreen_X == m_aScreenSizes[i])
		{
			m_nScrSizeNum = static_cast<ScreenSizeNumber>(i);
			goto l_scrszchgend;
		}
	}

	m_nScreen_CustomX = m_nScreen_X;
	m_nScrSizeNum = SCREENSIZE_CUSTOM;
l_scrszchgend:
	CString str;
	auto y = static_cast<int>(m_nScreen_X / m_pScreen->getAspectRatio());
	str.Format(_T("Scr: %d x %d"), m_nScreen_X, y);
	m_wndStatusBar.SetPaneText(static_cast<int>(STATUS_FIELD::SCR_REZ), str.GetString());

	return S_OK;
}

void CMainFrame::OnUpdateSetScreenSize(CCmdUI *pCmdUI)
{
	switch (m_nScrSizeNum)
	{
		default:
			m_nScrSizeNum = SCREENSIZE_CUSTOM; // тут break не нужен!
			[[fallthrough]];
		case SCREENSIZE_CUSTOM:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_CUSTOM);
			break;

		case SCREENSIZE_256X192:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_256X192);
			break;

		case SCREENSIZE_324X243:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_324X243);
			break;

		case SCREENSIZE_432X324:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_432X324);
			break;

		case SCREENSIZE_512X384:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_512X384);
			break;

		case SCREENSIZE_576X432:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_576X432);
			break;

		case SCREENSIZE_768X576:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_768X576);
			break;

		case SCREENSIZE_1024X768:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_1024X768);
			break;

		case SCREENSIZE_1280X960:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_1280X960);
			break;

		case SCREENSIZE_1536X1152:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_1536X1152);
			break;

		case SCREENSIZE_1792X1344:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_1792X1344);
			break;

		case SCREENSIZE_2048X1536:
			pCmdUI->SetRadio(pCmdUI->m_nID == ID_VIEW_SCREENSIZE_2048X1536);
			break;
	}
}

CString CMainFrame::MakeUniqueName()
{
#pragma warning(disable:4996)
	// сделаем уникальное имя файла, чтоб не заморачиваться - возьмём тек. время
	constexpr auto nBufSize = 80;
	TCHAR       buf[nBufSize];
	struct tm   tstruct {};
	time_t      now = time(nullptr);
	tstruct = *localtime(&now);
	wcsftime(buf, nBufSize, _T("%Y%m%d-%H%M%S"), &tstruct);
	return {buf};
}

void CMainFrame::OnOptionsLogAy8910()
{
	if (m_aySnd.isLogEnabled())
	{
		// выключим лог
		m_aySnd.log_done();
	}
	else
	{
		// включим лог
		m_aySnd.log_start(MakeUniqueName());
	}
}


void CMainFrame::OnUpdateOptionsLogAy8910(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_aySnd.isLogEnabled());
}


void CMainFrame::OnVideoCaptureStart()
{
	CString str = MakeUniqueName();
	m_pScreen->SetCaptureStatus(true, str);
	m_pSound->SetCaptureStatus(true, str);
}

void CMainFrame::OnUpdateVideoCaptureStart(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFoundFFMPEG && !m_pScreen->IsCapture());
}

void CMainFrame::OnVideoCaptureStop()
{
	CString str = _T("");
	m_pScreen->SetCaptureStatus(false, str);
	m_pSound->SetCaptureStatus(false, str);
}

void CMainFrame::OnUpdateVideoCaptureStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFoundFFMPEG && m_pScreen->IsCapture());
}


void CMainFrame::OnCmdToggleV100()
{
	m_pBoard->Irq2Interrupt();
}

void CMainFrame::OnCmdToggleV270()
{
	m_pBoard->Irq3Interrupt();
}

void CMainFrame::RunDoc(const fs::path &doc)
{
	fs::path strTarget = g_Config.GetConfCurrPath() / fs::path(_T("Docs")) / doc;
	ShellExecute(nullptr, _T("open"), strTarget.c_str(),
				 nullptr, nullptr, SW_SHOWNORMAL);
}

void CMainFrame::OnHelpDoc()
{
	RunDoc(_T("Emulator Documentation.html"));
}


void CMainFrame::OnHelpDMBK0010()
{
	RunDoc(_T("00001-01.32.03.html"));
}


void CMainFrame::OnHelpDMBK0011()
{
	RunDoc(_T("00008-01.32.01.html"));
}


void CMainFrame::OnHelpDMBK0011M()
{
	RunDoc(_T("00015-01.32.01.html"));
}


void CMainFrame::OnHelpProgBK()
{
	RunDoc(_T("programming_BK10.html"));
}


void CMainFrame::OnHelpZaltsman()
{
	RunDoc(_T("Zaltsman.html"));
}


void CMainFrame::OnHelpPCBKKbdLayout()
{
	RunDoc(_T("kbdLayout_BKPC.html"));
}


void CMainFrame::OnQuickStartGuide()
{
	RunDoc(_T("QuickGuide.html"));
}


void CMainFrame::OnSetFocus(CWnd *pOldWnd)
{
	// сохраним состояние капслока  вне программы
	g_Config.m_bSysCapsStatus = !!(GetKeyState(VK_CAPITAL) & 1);

	CFrameWndEx::OnSetFocus(pOldWnd);
}

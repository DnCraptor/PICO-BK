
#include "pch.h"
#include "resource.h"
#include "Config.h"
#include "BKMessageBox.h"
#include "vkeycodes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#pragma warning(disable:4996)

const BK_MODEL_PARAMETERS g_mstrConfigBKModelParameters[static_cast<int>(CONF_BKMODEL::NUMBERS)] =
{
	{ _T("BK-0010-01"),          IDS_CONFNAME_BK_0010_01,          MSF_CONF::BK1001,       BK_DEV_MPI::NONE    },
	{ _T("BK-0010-01_MSTD"),     IDS_CONFNAME_BK_0010_01_MSTD,     MSF_CONF::BK1001_MSTD,  BK_DEV_MPI::NONE    },
	{ _T("BK-0010-01_EXT32RAM"), IDS_CONFNAME_BK_0010_01_EXT32RAM, MSF_CONF::BK1001_EXT32, BK_DEV_MPI::NONE    },
	{ _T("BK-0010-01_FDD"),      IDS_CONFNAME_BK_0010_01_FDD,      MSF_CONF::BK1001_FDD,   BK_DEV_MPI::STD_FDD },
	{ _T("BK-0010-01_A16M"),     IDS_CONFNAME_BK_0010_01_A16M,     MSF_CONF::BK1001_FDD,   BK_DEV_MPI::A16M    },
	{ _T("BK-0010-01_SMK512"),   IDS_CONFNAME_BK_0010_01_SMK512,   MSF_CONF::BK1001_FDD,   BK_DEV_MPI::SMK512  },
	{ _T("BK-0010-01_SAMARA"),   IDS_CONFNAME_BK_0010_01_SAMARA,   MSF_CONF::BK1001_FDD,   BK_DEV_MPI::SAMARA  },
	{ _T("BK-0011"),             IDS_CONFNAME_BK_0011,             MSF_CONF::BK11,         BK_DEV_MPI::NONE    },
	{ _T("BK-0011_FDD"),         IDS_CONFNAME_BK_0011_FDD,         MSF_CONF::BK11_FDD,     BK_DEV_MPI::STD_FDD },
	{ _T("BK-0011_A16M"),        IDS_CONFNAME_BK_0011_A16M,        MSF_CONF::BK11_FDD,     BK_DEV_MPI::A16M    },
	{ _T("BK-0011_SMK512"),      IDS_CONFNAME_BK_0011_SMK512,      MSF_CONF::BK11_FDD,     BK_DEV_MPI::SMK512  },
	{ _T("BK-0011_SAMARA"),      IDS_CONFNAME_BK_0011_SAMARA,      MSF_CONF::BK11_FDD,     BK_DEV_MPI::SAMARA  },
	{ _T("BK-0011M"),            IDS_CONFNAME_BK_0011M,            MSF_CONF::BK11M,        BK_DEV_MPI::NONE    },
	{ _T("BK-0011M_FDD"),        IDS_CONFNAME_BK_0011M_FDD,        MSF_CONF::BK11M_FDD,    BK_DEV_MPI::STD_FDD },
	{ _T("BK-0011M_A16M"),       IDS_CONFNAME_BK_0011M_A16M,       MSF_CONF::BK11M_FDD,    BK_DEV_MPI::A16M    },
	{ _T("BK-0011M_SMK512"),     IDS_CONFNAME_BK_0011M_SMK512,     MSF_CONF::BK11M_FDD,    BK_DEV_MPI::SMK512  },
	{ _T("BK-0011M_SAMARA"),     IDS_CONFNAME_BK_0011M_SAMARA,     MSF_CONF::BK11M_FDD,    BK_DEV_MPI::SAMARA  },
	{ _T("BK-0010"),             IDS_CONFNAME_BK_0010,             MSF_CONF::BK10,         BK_DEV_MPI::NONE    }
};

const int g_EnabledSSR[SSR_LIST_SIZE] =
{
	DEFAULT_SOUND_SAMPLE_RATE,
	48000,
	96000,
	192000
};

const SoundChipModelList g_EnabledSoundChipModels[SCHM_LIST_SIZE] =
{
	{ MODEL_AY_3_8910, IDS_CONFAY_AY38910 },
	{ MODEL_YM2149,    IDS_CONFAY_YM2149F }
};

const SoundChipFreqlList g_SoundChipFrequences[SCHFRQ_LIST_SIZE] =
{
	{ 1774400,                          IDS_SNDCHPFRQ_1774400 },
	{ 1500000,                          IDS_SNDCHPFRQ_1500000 },
	{ DEFAULT_EMU_SOUNDCHIP_FREQUENCY,  IDS_SNDCHPFRQ_1714286 },
	{ 1789772,                          IDS_SNDCHPFRQ_1789772 }
};

const MultiSoundChipWorkMode g_MultiSoundChipWorkModes[MSCHWM_LIST_SIZE] =
{
	{ MODE_GID,         IDS_MSCHWM_GID },
	{ MODE_GRYPHON,     IDS_MSCHWM_GRYPHON },
	{ MODE_TURBOSOUND,  IDS_MSCHWM_TURBOSOUND }
};


const CString g_strEmptyUnit = _T("<empty>");
const CString g_mstrDrives[static_cast<int>(FDD_DRIVE::NUM_FDD)] =
{
	_T("A:"), _T("B:"), _T("C:"), _T("D:")
};
const int g_mnDrivesIndx[static_cast<int>(FDD_DRIVE::NUM_FDD)] =
{
	IDS_INI_DRIVEA, IDS_INI_DRIVEB, IDS_INI_DRIVEC, IDS_INI_DRIVED
};

static const UINT g_arStrDumpAddrID[NUMBER_VIEWS_MEM_DUMP] =
{
	IDS_INI_ADDR_DUMP_0, IDS_INI_ADDR_DUMP_1, IDS_INI_ADDR_DUMP_2, IDS_INI_ADDR_DUMP_3
};

static const UINT g_arStrDumpListID[NUMBER_VIEWS_MEM_DUMP] =
{
	IDS_INI_DMP_LIST_POS_0, IDS_INI_DMP_LIST_POS_1, IDS_INI_DMP_LIST_POS_2, IDS_INI_DMP_LIST_POS_3
};

const CString g_arStrSMKPgNums[16] =
{
	_T("0000"), _T("2000"), _T("0004"), _T("2004"),
	_T("0010"), _T("2010"), _T("0014"), _T("2014"),
	_T("0001"), _T("2001"), _T("0005"), _T("2005"),
	_T("0011"), _T("2011"), _T("0015"), _T("2015")
};

CConfig g_Config; // глобально доступный конфиг.


CConfig::CConfig()
	: m_BKFDDModel(BK_DEV_MPI::NONE)
	, m_nSoundSampleRate(DEFAULT_SOUND_SAMPLE_RATE)
	, m_nCPUTempFreq(-1)    // начальное значение, если -1, то неприменимо
	, m_clrText(0)
	, m_bSysCapsStatus(false)
{
#ifdef TARGET_WINXP
	/*
	 Получение версии виндовса.
	 Выход: TRUE - версию получить удалось,
	        FALSE - версию получить не удалось по каким-то причинам

	Если dwMajor < 6 то это winxp или хуже того, вин 2000 какой-нибудь
	в зависимости от этого надо выбирать разные рендеры осциллографа и экрана.
	 */
	BOOL bRet = FALSE;
	// Пытаемся вызвать GetVersionEx используя структуру OSVERSIONINFOEX.
	// В случае ошибки пытаемся проделать тоже самое со структурой OSVERSIONINFO.
	ZeroMemory(&m_osvi, sizeof(OSVERSIONINFOEX));
	m_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (!(bRet = GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&m_osvi))))
	{
		// Если OSVERSIONINFOEX не работает, то используем OSVERSIONINFO.
		m_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		bRet = GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&m_osvi));
	}

#endif
	initVolPan();
}

CConfig::~CConfig()
{
	iniFile.Clear();
}

// TODO: right place?
extern "C" {
#include "ff.h"
}

bool fs :: exists(const fs::path& p) {
	FIL fil;
    FRESULT result = f_open(&fil, p.c_str(), FA_READ);
    if (result != FR_OK) {
        return false;
	}
	f_close(&fil);
    return true;
}

// инициализация конфига и всяких переменных, типа пути к проге
bool CConfig::InitConfig(const CString &strIniName)
{
	GetConfCurrPath(); // инициализация переменной m_strCurrentPath
	m_strIniFileName = fs::path(m_strCurrentPath.cstr() + _T("/") + strIniName);
	iniFile.SetIniFileName(m_strIniFileName);
	bool bNewConfig = false;

	if (!fs::exists(m_strIniFileName))
	{
		// если файла нету - создадим конфиг по умолчанию
		DefaultConfig();
		bNewConfig = true;
	}

	LoadConfig();
	return bNewConfig;
}

void CConfig::UnInitConfig()
{
	SaveConfig();
	iniFile.Clear();
}

const fs::path &CConfig::GetConfCurrPath()
{
	if (m_strCurrentPath.empty())
	{
		CString pModuleName;
	///	BOOL res = ::GetModuleFileName(AfxGetInstanceHandle(), pModuleName.GetBufferSetLength(_MAX_PATH), _MAX_PATH);
		pModuleName.ReleaseBuffer();

	///	if (res)
		{
	///		m_strCurrentPath = fs::path(pModuleName.GetString()).parent_path();
		}
	///	else
		{
			m_strCurrentPath = "\\BK"; /// fs::current_path();
		}
	}

	return m_strCurrentPath;
}

// статические переменные и массивы переменных
// Rom модули
fs::path CConfig::m_strBK10_017_Monitor,
CConfig::m_strBK10_018_Focal,
CConfig::m_strBK10_084_Focal,
CConfig::m_strBK10_019_MSTD,
CConfig::m_strBK10_106_basic1,
CConfig::m_strBK10_107_basic2,
CConfig::m_strBK10_108_basic3,
CConfig::m_strBK11_201_bos,
CConfig::m_strBK11_202_ext,
CConfig::m_strBK11_203_mstd,
CConfig::m_strBK11_198_basic1,
CConfig::m_strBK11_199_basic2,
CConfig::m_strBK11_200_basic3,
CConfig::m_strBK11m_324_bos,
CConfig::m_strBK11m_325_ext,
CConfig::m_strBK11m_330_mstd,
CConfig::m_strBK11m_327_basic1,
CConfig::m_strBK11m_328_basic2,
CConfig::m_strBK11m_329_basic3,
CConfig::m_strFDD_Std,
CConfig::m_strFDD_Std253,
CConfig::m_strFDD_A16M,
CConfig::m_strFDD_SMK512,
CConfig::m_strFDD_Samara,
CConfig::m_strBK11Pg121,
CConfig::m_strBK11Pg122,
CConfig::m_strBK11Pg131,
CConfig::m_strBK11Pg132,
CConfig::m_strBK10MSTD;
// Директории
fs::path CConfig::m_strBinPath,
CConfig::m_strToolsPath,
CConfig::m_strMemPath,
CConfig::m_strSavesPath,
CConfig::m_strTapePath,
CConfig::m_strScriptsPath,
CConfig::m_strROMPath,
CConfig::m_strIMGPath,
CConfig::m_strScreenShotsPath;


confStringParam CConfig::m_romModules[] =
{
	{ IDS_INI_BK10_RE2_017_MONITOR, &m_strBK10_017_Monitor, _T("bk10_017_mon.rom")        },
	{ IDS_INI_BK10_RE2_018_FOCAL,   &m_strBK10_018_Focal,   _T("bk10_018_focal.rom")      },
	{ IDS_INI_BK10_RE2_084_FOCAL,   &m_strBK10_084_Focal,   _T("bk10_084_focal.rom")      },
	{ IDS_INI_BK10_RE2_019_MSTD,    &m_strBK10_019_MSTD,    _T("bk10_019_mstd.rom")       },
	{ IDS_INI_BK10_RE2_106_BASIC1,  &m_strBK10_106_basic1,  _T("bk10_106_basic1.rom")     },
	{ IDS_INI_BK10_RE2_107_BASIC2,  &m_strBK10_107_basic2,  _T("bk10_107_basic2.rom")     },
	{ IDS_INI_BK10_RE2_108_BASIC3,  &m_strBK10_108_basic3,  _T("bk10_108_basic3.rom")     },
	{ IDS_INI_BK11_RE2_201_BOS,     &m_strBK11_201_bos,     _T("bk11_201_bos.rom")        },
	{ IDS_INI_BK11_RE2_202_EXT,     &m_strBK11_202_ext,     _T("bk11_202_ext.rom")        },
	{ IDS_INI_BK11_RE2_203_MSTD,    &m_strBK11_203_mstd,    _T("bk11_203_mstd.rom")       },
	{ IDS_INI_BK11_RE2_198_BASIC1,  &m_strBK11_198_basic1,  _T("bk11_198_basic1.rom")     },
	{ IDS_INI_BK11_RE2_199_BASIC2,  &m_strBK11_199_basic2,  _T("bk11_199_basic2.rom")     },
	{ IDS_INI_BK11_RE2_200_BASIC3,  &m_strBK11_200_basic3,  _T("bk11_200_basic3.rom")     },
	{ IDS_INI_BK11M_RE2_324_BOS,    &m_strBK11m_324_bos,    _T("bk11m_324_bos.rom")       },
	{ IDS_INI_BK11M_RE2_325_EXT,    &m_strBK11m_325_ext,    _T("bk11m_325_ext.rom")       },
	{ IDS_INI_BK11M_RE2_330_MSTD,   &m_strBK11m_330_mstd,   _T("bk11m_330_mstd.rom")      },
	{ IDS_INI_BK11M_RE2_327_BASIC1, &m_strBK11m_327_basic1, _T("bk11m_327_basic1.rom")    },
	{ IDS_INI_BK11M_RE2_328_BASIC2, &m_strBK11m_328_basic2, _T("bk11m_328_basic2.rom")    },
	{ IDS_INI_BK11M_RE2_329_BASIC3, &m_strBK11m_329_basic3, _T("bk11m_329_basic3.rom")    },
	{ IDS_INI_FDR,                  &m_strFDD_Std,          _T("DISK_326.rom")            },
	{ IDS_INI_FDR253,               &m_strFDD_Std253,       _T("DISK_253.rom")            },
	{ IDS_INI_FDR_A16M,             &m_strFDD_A16M,         _T("DISK_A16M_v2.41.rom")     },
	{ IDS_INI_FDR_SMK512,           &m_strFDD_SMK512,       _T("DISK_SMK512_v2.05.rom")   },
	{ IDS_INI_FDR_SAMARA,           &m_strFDD_Samara,       _T("DISK_SAMARA_HDD+FIS.rom") },
	{ IDS_INI_BK11_RE_OPT_PG12_1,   &m_strBK11Pg121,        g_strEmptyUnit.GetString()    },
	{ IDS_INI_BK11_RE_OPT_PG12_2,   &m_strBK11Pg122,        g_strEmptyUnit.GetString()    },
	{ IDS_INI_BK11_RE_OPT_PG13_1,   &m_strBK11Pg131,        g_strEmptyUnit.GetString()    },
	{ IDS_INI_BK11_RE_OPT_PG13_2,   &m_strBK11Pg132,        g_strEmptyUnit.GetString()    },
	{ IDS_INI_BK10_OPT_MSTD,        &m_strBK10MSTD,         g_strEmptyUnit.GetString()    },
	{ 0,                            nullptr,                _T("")                        }
};

confStringParam CConfig::m_Directories[] =
{
	{ IDS_INI_PROGRAM_DIRECTORY,    &m_strBinPath,          _T("Bin")         },
	{ IDS_INI_TOOLS_DIRECTORY,      &m_strToolsPath,        _T("Tools")       },
	{ IDS_INI_MEM_DIRECTORY,        &m_strMemPath,          _T("Memory")      },
	{ IDS_INI_SAVES_DIRECTORY,      &m_strSavesPath,        _T("UserSaves")   },
	{ IDS_INI_TAPES_DIRECTORY,      &m_strTapePath,         _T("Tapes")       },
	{ IDS_INI_SCRIPTS_DIRECTORY,    &m_strScriptsPath,      _T("Scripts")     },
	{ IDS_INI_ROM_DIRECTORY,        &m_strROMPath,          _T("ROM")         },
	{ IDS_INI_IMG_DIRECTORY,        &m_strIMGPath,          _T("Img")         },
	{ IDS_INI_SSHOT_DIRECTORY,      &m_strScreenShotsPath,  _T("Screenshots") },
	{ 0,                            nullptr,                _T("") }
};

void CConfig::_intLoadConfig(bool bLoadMain)
{
	int id, i = 0;

	// Инициализация директорий

	while ((id = m_Directories[i].nID) != 0)
	{
		// реализуем возможность задания произвольного пути
		fs::path strDefPath = iniFile.GetValueString(IDS_INI_SECTIONNAME_DIRECTORIES, id, m_Directories[i].defValue.c_str()).GetString();

		//если есть имя диска или "\\" в начале - то это абсолютный путь
	///	if (strDefPath.has_root_name())
		{
			*m_Directories[i].pstrValue = strDefPath;
		}
	///	else
		{
			// иначе - это относительный путь от домашней директории
	///		*m_Directories[i].pstrValue = GetConfCurrPath() / strDefPath;
		}

		i++;
	}

	// Инициализация параметров
	if (bLoadMain)
	{
		//      Основные параметры
		m_strBKBoardType = iniFile.GetValueString(IDS_INI_SECTIONNAME_MAIN, IDS_INI_BKMODEL, g_mstrConfigBKModelParameters[static_cast<int>(CONF_BKMODEL::BK_0010_01)].strBKModelConfigName);
		m_nScreenshotNumber = iniFile.GetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SSHOT_NUM, 1);
		m_nDateInsteadOfScreenshotNumber = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SSHOT_DATENUM, false);
	///	m_nScreenRenderType = static_cast<CONF_SCREEN_RENDER>(iniFile.GetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SCRRENDER_TYPE, static_cast<int>((IsWindowsVistaOrGreater()) ? CONF_SCREEN_RENDER::D2D : CONF_SCREEN_RENDER::VFW)));
	///	m_nOscRenderType = static_cast<CONF_OSCILLOSCOPE_RENDER>(iniFile.GetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_OSCRENDER_TYPE, static_cast<int>((IsWindowsVistaOrGreater()) ? CONF_OSCILLOSCOPE_RENDER::D2D : CONF_OSCILLOSCOPE_RENDER::OPENGL)));
		m_nSoundSampleRate = iniFile.GetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUND_SAMPLE_RATE, DEFAULT_SOUND_SAMPLE_RATE);
		m_nSoundChipFrequency = iniFile.GetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUNDCHIPFREQ, DEFAULT_EMU_SOUNDCHIP_FREQUENCY);
		m_nSoundChipModel[AY1] = iniFile.GetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUNDCHIPMODEL, MODEL_YM2149);
		m_nSoundChipModel[AY2] = iniFile.GetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUNDCHIPMODEL2, MODEL_YM2149);
		m_n2AYWorkMode = iniFile.GetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_2AYWORKMODE, MODE_GRYPHON);
		m_bUseLongBinFormat = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_LONGBIN, false);
		m_bOrigScreenshotSize = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ORIG_SCRNSHOT_SIZE, false);
		m_bBigButtons = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_BIGBUTTONS, false);
		m_bExclusiveOpenImages = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EXCLUSIVEOPENIMAGES, true);
		m_strFFMPEGLine = iniFile.GetValueString(IDS_INI_SECTIONNAME_MAIN, IDS_INI_FFMPEGCMDLINE, DEFAULT_FFMPEG_CMDLINE);
		m_nScreenW = iniFile.GetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SCRWIDTH, 1024);
		m_dAspectRatio = iniFile.GetValueFloat(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SCRASPECT, 4.0 / 3.0);
		m_bNativeRusLatSwitch = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_NATIVERUSLATSWITCH, true);
		m_bMMG = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEMMG, false);
		m_bEIS = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEEIS, false);
		m_bFIS = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEFIS, false);
		m_bFPU = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEFPU, false);
		m_bEmulateCBug = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_CBUG, true);
		m_bEmulate177702 = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_177702, true);
		m_bVM1G = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_VM1G, false);
		m_bEmulateEIS = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_EIS, false);
		m_bEmulateFIS = iniFile.GetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_FIS, false);
		CheckRenders();
		CheckSSR();
		CheckSndChipFreq();
		CheckSndChipModel();
	}

	// теперь подготовим индивидуальное имя секции
	CString strCustomize = m_strBKBoardType;
	// Инициализация Rom модулей, читаем их лишь для того, чтобы проверить наличие
	// и если их нет, то создаём
	i = 0;

	while ((id = m_romModules[i].nID) != 0)
	{
		*m_romModules[i].pstrValue = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_ROMMODULES, id, m_romModules[i].defValue.c_str()).GetString();
		i++;
	}

	//      Вариативные параметры
	m_nCPURunAddr       = Global::OctStringToWord(iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_CPU_RUN_ADDR, _T("000000")));  // если 0, то берётся значение по умолчанию для своей конфигурации
	m_nCPUFrequency     = iniFile.GetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_CPU_FREQUENCY, 0);  // если 0, то берётся значение по умолчанию для своей конфигурации
	m_nRegistersDumpInterval = iniFile.GetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_REGSDUMP_INTERVAL, 0);  // если 0, то выключено
	CString strSoundVal = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_SOUNDVOLUME, _T("30%"));// Громкость
	m_nSoundVolume      = 0xffff * _tstoi(strSoundVal) / 100;

	for (int i = 0; i < NUMBER_VIEWS_MEM_DUMP; ++i)
	{
		m_arDumper[i].nAddr = Global::OctStringToWord(iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, g_arStrDumpAddrID[i], _T("000000")));
		CString str = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, g_arStrDumpListID[i], _T("0 : 0"));
		str.Trim();
		int colon = str.Find(_T(":"), 0); // поищем начало разделителя

	///	if (colon >= 0)
		{
	///		CString strPagePos = str.Left(colon).Trim();
	///		CString strAddrPos = str.Right(str.GetLength() - colon - 1).Trim();
	///		m_arDumper[i].nPageListPos = Global::ToInt(strPagePos);
	///		m_arDumper[i].nAddrListPos = Global::ToInt(strAddrPos);
		}
	///	else
		{
			m_arDumper[i].nPageListPos = 0;
			m_arDumper[i].nAddrListPos = 0;
		}
	}

	m_nDisasmAddr       = Global::OctStringToWord(iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_ADDR_DISASM, _T("001000")));
	LoadPalettes(strCustomize);
	LoadJoyParams(strCustomize);
	LoadAYVolPanParams(strCustomize);
	// Инициализация опций
	m_bSavesDefault     = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SAVES_DEFAULT, false);
	m_bSpeaker          = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SPEAKER, true);
	m_bSpeakerFilter    = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SPEAKER_FILTER, true);
	m_bSpeakerDCOffset  = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SPEAKER_DCOFFSET, false);
	m_bCovox            = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX, false);
	m_bCovoxFilter      = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX_FILTER, true);
	m_bCovoxDCOffset    = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX_DCOFFSET, true);
	m_bStereoCovox      = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX_STEREO, false);
	m_bMenestrel        = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MENESTREL, false);
	m_bMenestrelFilter  = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MENESTREL_FILTER, true);
	m_bMenestrelDCOffset = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MENESTREL_DCOFFSET, false);
	m_bAY8910           = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AY8910, true);
	m_b2ndAY8910        = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_2NDAY8910, false);
	m_bAY8910Filter     = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AY8910_FILTER, true);
	m_bAY8910DCOffset   = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AY8910_DCOFFSET, false);
	m_bBKKeyboard       = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_BKKEYBOARD, true);
	m_bJoystick         = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_JOYSTICK, true);
	m_bICLBlock         = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_ICLBLOCK, false);
	m_bMouseMars        = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MOUSEM, false);
	m_bSmoothing        = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SMOOTHING, false); // параметры экрана
	m_bColorMode        = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COLOR_MODE, true);
	m_bAdaptBWMode      = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_BLACK_WHITE, false);
	m_bLuminoforeEmulMode = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_LUMINOFOREMODE, false);
	m_bFullscreenMode   = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_FULL_SCREEN, false);
	m_bPauseCPUAfterStart = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_PAUSE_CPU, false);
	m_bAskForBreak      = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_ASK_FOR_BREAK, false);
	m_bEmulateLoadTape  = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_EMUL_LOAD_TAPE, true);
	m_bEmulateSaveTape  = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_EMUL_SAVE_TAPE, true);
	m_bTapeAutoBeginDetection = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AUTO_BEG_TAPE, true);
	m_bTapeAutoEndDetection = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AUTO_END_TAPE, true);
	m_bShowPerformance  = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SHOW_PERFORMANCE_STRING, true);
	m_bEmulateFDDIO     = iniFile.GetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_EMULATE_FDDIO, true);
	m_nVKBDType         = iniFile.GetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_VKBD_TYPE, 0);
	// Инициализация приводов
	fs::path str = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVEA, g_strEmptyUnit).GetString();
	m_strFDDrives[0]    = CheckDriveImgName(str);
	str = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVEB, g_strEmptyUnit).GetString();
	m_strFDDrives[1]    = CheckDriveImgName(str);
	str = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVEC, g_strEmptyUnit).GetString();
	m_strFDDrives[2]    = CheckDriveImgName(str);
	str = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVED, g_strEmptyUnit).GetString();
	m_strFDDrives[3]    = CheckDriveImgName(str);
	str = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_HDD0, g_strEmptyUnit).GetString();
	m_strHDDrives[0]    = CheckDriveImgName(str);
	str = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_HDD1, g_strEmptyUnit).GetString();
	m_strHDDrives[1]    = CheckDriveImgName(str);

	if (m_bAY8910) // у сопра приоритет перед ковоксом
	{
		m_bCovox = false;
		m_bMenestrel = false;
	}
	else if (m_bCovox)
	{
		m_bAY8910 = false;
		m_bMenestrel = false;
	}
	else if (m_bMenestrel)
	{
		m_bAY8910 = false;
		m_bCovox = false;
	}

	if (m_bJoystick) // если включён джойстик
	{
		m_bICLBlock = false; // то блок нагрузок принудительно выключен, несмотря ни на что.
		m_bMouseMars = false; // то мышь выключена.
	}
	else if (m_bMouseMars)
	{
		m_bJoystick = false;
		m_bICLBlock = false;
	}
	else if (m_bICLBlock)
	{
		m_bJoystick = false;
		m_bMouseMars = false;
	}
}

void CConfig::CheckRenders()
{ /***
	// проверим на корректность подсунутых нам значений из ini
	if (m_nScreenRenderType < CONF_SCREEN_RENDER::OPENGL || m_nScreenRenderType > CONF_SCREEN_RENDER::D3D)
	{
		m_nScreenRenderType = CONF_SCREEN_RENDER::OPENGL;
	}

	// проверим на корректность подсунутых нам значений из ini
	if (m_nOscRenderType < CONF_OSCILLOSCOPE_RENDER::OPENGL || m_nOscRenderType > CONF_OSCILLOSCOPE_RENDER::GDI)
	{
		m_nOscRenderType = CONF_OSCILLOSCOPE_RENDER::OPENGL;
	}

	if (!IsWindowsVistaOrGreater())
	{
		// если windows xp, то проверим на предмет поддерживаемых рендеров
		if (m_nScreenRenderType == CONF_SCREEN_RENDER::D2D) // D2D не поддерживается
		{
			m_nScreenRenderType = CONF_SCREEN_RENDER::VFW; // заменяется на DIB
		}

		// если windows xp, то проверим на предмет поддерживаемых рендеров
		if (m_nOscRenderType == CONF_OSCILLOSCOPE_RENDER::D2D) // D2D не поддерживается
		{
			m_nOscRenderType = CONF_OSCILLOSCOPE_RENDER::OPENGL; // заменяется на OGL
		}
	} **/
}

void CConfig::CheckSSR()
{
	bool bOk = false;

	for (const auto &i : g_EnabledSSR)
	{
		if (m_nSoundSampleRate == i)
		{
			bOk = true;
			break;
		}
	}

	if (!bOk)
	{
		m_nSoundSampleRate = DEFAULT_SOUND_SAMPLE_RATE;
	}
}

void CConfig::CheckSndChipModel()
{
	bool bOk = false;

	for (const auto &i : g_EnabledSoundChipModels)
	{
		if (m_nSoundChipModel[AY1] == i.nModelNum)
		{
			bOk = true;
			break;
		}
	}

	if (!bOk)
	{
		m_nSoundChipModel[AY1] = MODEL_YM2149;
	}

	// и повторим для второго чипа
	bOk = false;

	for (const auto &i : g_EnabledSoundChipModels)
	{
		if (m_nSoundChipModel[AY2] == i.nModelNum)
		{
			bOk = true;
			break;
		}
	}

	if (!bOk)
	{
		m_nSoundChipModel[AY2] = MODEL_YM2149;
	}
}

void CConfig::CheckSndChipFreq()
{
	// частота должна быть в пределах 1..2МГц
	if (m_nSoundChipFrequency < 1000000 || m_nSoundChipFrequency > 2000000)
	{
		// если это не так - применим частоту по умолчанию.
		m_nSoundChipFrequency = DEFAULT_EMU_SOUNDCHIP_FREQUENCY;
	}
}

/*
В конфиге есть значения только для чтения, которые не будут изменяться
и есть текущие опции, которые как раз будут изменяться.
*/
void CConfig::LoadConfig(bool bLoadMain)
{
	_intLoadConfig(bLoadMain);
}

void CConfig::LoadConfig_FromMemory(uint8_t *pBuff, UINT nSize)
{
	iniFile.ReadIniFromMemory(pBuff, nSize);
	_intLoadConfig(true);
}

// сохранять в память надо только main и кастомные секции
ULONGLONG CConfig::SaveConfig_ToMemory(uint8_t *pBuff, UINT nSize)
{
	SaveConfig();
	CIni customIni; // новый ини файл.
	CString strSectionName, strCustomSection, strCustomSuffix = _T(".") + m_strBKBoardType; /***
	// в память над сохранить:
	// 1. секцию Main
	strSectionName = CString(MAKEINTRESOURCE(IDS_INI_SECTIONNAME_MAIN));
	customIni.CopySection(&iniFile, strSectionName);

	// 2. секцию RomModules.Custom если есть
	strCustomSection = CString(MAKEINTRESOURCE(IDS_INI_SECTIONNAME_ROMMODULES)) + strCustomSuffix;
	customIni.CopySection(&iniFile, strCustomSection);

	// 3. секцию Parameters
	strSectionName = CString(MAKEINTRESOURCE(IDS_INI_SECTIONNAME_PARAMETERS));
	customIni.CopySection(&iniFile, strSectionName);
	// 3.1. секцию Parameters.Custom если есть
	strCustomSection = strSectionName + strCustomSuffix;
	customIni.CopySection(&iniFile, strCustomSection);

	// 4. секцию Options
	strSectionName = CString(MAKEINTRESOURCE(IDS_INI_SECTIONNAME_OPTIONS));
	customIni.CopySection(&iniFile, strSectionName);
	// 4.1. секцию Options.Custom если есть
	strCustomSection = strSectionName + strCustomSuffix;
	customIni.CopySection(&iniFile, strCustomSection);

	// 5. секцию Palettes.Custom если есть
	strCustomSection = CString(MAKEINTRESOURCE(IDS_INI_SECTIONNAME_PALETTES)) + strCustomSuffix;
	customIni.CopySection(&iniFile, strCustomSection);

	// 6. секцию Joystick Parameters.Custom если есть
	strCustomSection = CString(MAKEINTRESOURCE(IDS_INI_SECTIONNAME_JOYSTICK)) + strCustomSuffix;
	customIni.CopySection(&iniFile, strCustomSection);

	// 7. секцию Drives.Custom если есть
	strCustomSection = CString(MAKEINTRESOURCE(IDS_INI_SECTIONNAME_DRIVES)) + strCustomSuffix;
	customIni.CopySection(&iniFile, strCustomSection); **/
	return customIni.FlushIniToMemory(pBuff, nSize);
}


void CConfig::SaveConfig()
{
	// теперь подготовим индивидуальное имя секции
	CString strCustomize = m_strBKBoardType;
	// Сохранение Rom модулей - только по чтению, их не модифицируем
	int id, i = 0;

	while ((id = m_romModules[i].nID) != 0)
	{
		iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_ROMMODULES, id, m_romModules[i++].pstrValue->c_str());
	}

	// Сохранение директорий - только по чтению, их не модифицируем
	// Сохранение параметров
	//      Основные параметры
	iniFile.SetValueString(IDS_INI_SECTIONNAME_MAIN, IDS_INI_BKMODEL, m_strBKBoardType);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SSHOT_NUM, m_nScreenshotNumber);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SSHOT_DATENUM, m_nDateInsteadOfScreenshotNumber);
///	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SCRRENDER_TYPE, static_cast<int>(m_nScreenRenderType));
///	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_OSCRENDER_TYPE, static_cast<int>(m_nOscRenderType));
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUND_SAMPLE_RATE, m_nSoundSampleRate);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUNDCHIPFREQ, m_nSoundChipFrequency);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUNDCHIPMODEL, m_nSoundChipModel[AY1]);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUNDCHIPMODEL2, m_nSoundChipModel[AY2]);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_2AYWORKMODE, m_n2AYWorkMode);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_LONGBIN, m_bUseLongBinFormat);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ORIG_SCRNSHOT_SIZE, m_bOrigScreenshotSize);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_BIGBUTTONS, m_bBigButtons);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EXCLUSIVEOPENIMAGES, m_bExclusiveOpenImages);
	iniFile.SetValueString(IDS_INI_SECTIONNAME_MAIN, IDS_INI_FFMPEGCMDLINE, m_strFFMPEGLine);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SCRWIDTH, m_nScreenW);
	iniFile.SetValueFloat(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SCRASPECT, m_dAspectRatio);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_NATIVERUSLATSWITCH, m_bNativeRusLatSwitch);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEMMG, !!m_bMMG);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEEIS, !!m_bEIS);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEFIS, !!m_bFIS);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEFPU, !!m_bFPU);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_CBUG,    m_bEmulateCBug);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_177702,  m_bEmulate177702);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_VM1G,    m_bVM1G);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_EIS,     m_bEmulateEIS);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_FIS,     m_bEmulateFIS);
	//      Вариативные параметры
	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_CPU_RUN_ADDR, Global::WordToOctString(m_nCPURunAddr));
	iniFile.SetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_CPU_FREQUENCY, m_nCPUFrequency);
	iniFile.SetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_REGSDUMP_INTERVAL, m_nRegistersDumpInterval);
	// Сохраняем Громкость
	CString strSoundVal;
	auto v = static_cast<int>(static_cast<double>(m_nSoundVolume) * 100.0 / static_cast<double>(0xffff) + 0.01);
	strSoundVal.Format(_T("%d%%"), v);
	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_SOUNDVOLUME, strSoundVal);

	for (int i = 0; i < NUMBER_VIEWS_MEM_DUMP; ++i)
	{
		iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, g_arStrDumpAddrID[i], Global::WordToOctString(m_arDumper[i].nAddr), true);
		CString str = Global::IntToString(m_arDumper[i].nPageListPos) + _T(" : ") + Global::IntToString(m_arDumper[i].nAddrListPos);
		iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, g_arStrDumpListID[i], str, true);
	}

	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_ADDR_DISASM, Global::WordToOctString(m_nDisasmAddr));
	SavePalettes(strCustomize);
	SaveJoyParams(strCustomize);
	SaveAYVolPanParams(strCustomize);
	// Сохранение опций
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SAVES_DEFAULT, m_bSavesDefault);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SPEAKER, m_bSpeaker);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SPEAKER_FILTER, m_bSpeakerFilter);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SPEAKER_DCOFFSET, m_bSpeakerDCOffset);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX, m_bCovox);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX_FILTER, m_bCovoxFilter);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX_DCOFFSET, m_bCovoxDCOffset);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX_STEREO, m_bStereoCovox);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MENESTREL, m_bMenestrel);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MENESTREL_FILTER, m_bMenestrelFilter);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MENESTREL_DCOFFSET, m_bMenestrelDCOffset);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AY8910, m_bAY8910);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_2NDAY8910, m_b2ndAY8910);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AY8910_FILTER, m_bAY8910Filter);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AY8910_DCOFFSET, m_bAY8910DCOffset);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_BKKEYBOARD, m_bBKKeyboard);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_JOYSTICK, m_bJoystick);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_ICLBLOCK, m_bICLBlock);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MOUSEM, m_bMouseMars);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SMOOTHING, m_bSmoothing);    // параметры экрана
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COLOR_MODE, m_bColorMode);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_BLACK_WHITE, m_bAdaptBWMode);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_LUMINOFOREMODE, m_bLuminoforeEmulMode);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_FULL_SCREEN, m_bFullscreenMode);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_PAUSE_CPU, m_bPauseCPUAfterStart);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_ASK_FOR_BREAK, m_bAskForBreak);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_EMUL_LOAD_TAPE, m_bEmulateLoadTape);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_EMUL_SAVE_TAPE, m_bEmulateSaveTape);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AUTO_BEG_TAPE, m_bTapeAutoBeginDetection);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AUTO_END_TAPE, m_bTapeAutoEndDetection);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SHOW_PERFORMANCE_STRING, m_bShowPerformance);
	iniFile.SetValueBoolEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_EMULATE_FDDIO, m_bEmulateFDDIO);
	iniFile.SetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_VKBD_TYPE, m_nVKBDType);
	// Сохранение приводов, принудительно в кастомную секцию
	SetDriveImgName(FDD_DRIVE::A, m_strFDDrives[0]); //сперва обработаем на всякий случай имена
	SetDriveImgName(FDD_DRIVE::B, m_strFDDrives[1]);
	SetDriveImgName(FDD_DRIVE::C, m_strFDDrives[2]);
	SetDriveImgName(FDD_DRIVE::D, m_strFDDrives[3]);
	SetDriveImgName(HDD_MODE::MASTER, m_strHDDrives[0]);
	SetDriveImgName(HDD_MODE::SLAVE, m_strHDDrives[1]);
	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVEA, m_strFDDrives[0].c_str(), true);
	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVEB, m_strFDDrives[1].c_str(), true);
	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVEC, m_strFDDrives[2].c_str(), true);
	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVED, m_strFDDrives[3].c_str(), true);
	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_HDD0,   m_strHDDrives[0].c_str(), true);
	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_DRIVES, IDS_INI_HDD1,   m_strHDDrives[1].c_str(), true);
	iniFile.FlushIni();
}

void CConfig::DefaultConfig()
{
	// идея следующая.
	// Создадим конфиг со значениями по умолчанию, и сохраним его на диске.
	// Создание Rom модулей
	int id, i = 0;

	while ((id = m_romModules[i].nID) != 0)
	{
		iniFile.SetValueString(IDS_INI_SECTIONNAME_ROMMODULES, id, m_romModules[i].defValue.c_str());
		i++;
	}

	// Создание директорий
	i = 0;

	while ((id = m_Directories[i].nID) != 0)
	{
		iniFile.SetValueString(IDS_INI_SECTIONNAME_DIRECTORIES, id, m_Directories[i].defValue.c_str());
		i++;
	}

	// Создание параметров
	//      Основные параметры
	iniFile.SetValueString(IDS_INI_SECTIONNAME_MAIN, IDS_INI_BKMODEL, g_mstrConfigBKModelParameters[static_cast<int>(CONF_BKMODEL::BK_0010_01)].strBKModelConfigName);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SSHOT_NUM, 1);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SSHOT_DATENUM, false);
///	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SCRRENDER_TYPE, static_cast<int>((IsWindowsVistaOrGreater()) ? CONF_SCREEN_RENDER::D2D : CONF_SCREEN_RENDER::VFW));
///	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_OSCRENDER_TYPE, static_cast<int>((IsWindowsVistaOrGreater()) ? CONF_OSCILLOSCOPE_RENDER::D2D : CONF_OSCILLOSCOPE_RENDER::OPENGL));
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUND_SAMPLE_RATE, DEFAULT_SOUND_SAMPLE_RATE);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUNDCHIPFREQ, DEFAULT_EMU_SOUNDCHIP_FREQUENCY);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUNDCHIPMODEL, MODEL_YM2149);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SOUNDCHIPMODEL2, MODEL_YM2149);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_2AYWORKMODE, MODE_GRYPHON);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_LONGBIN, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ORIG_SCRNSHOT_SIZE, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_BIGBUTTONS, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EXCLUSIVEOPENIMAGES, true);
	iniFile.SetValueString(IDS_INI_SECTIONNAME_MAIN, IDS_INI_FFMPEGCMDLINE, DEFAULT_FFMPEG_CMDLINE);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SCRWIDTH, 1024);
	iniFile.SetValueFloat(IDS_INI_SECTIONNAME_MAIN, IDS_INI_SCRASPECT, 4.0 / 3.0);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_NATIVERUSLATSWITCH, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEMMG, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEEIS, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEFIS, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_ENABLEFPU, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_CBUG, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_177702, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_VM1G, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_EIS, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_MAIN, IDS_INI_EMULATE_FIS, false);
	//      Вариативные параметры
	iniFile.SetValueString(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_CPU_RUN_ADDR, _T("000000"));  // если 0, то берётся значение по умолчанию для своей конфигурации
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_CPU_FREQUENCY, 0);  // если 0, то берётся значение по умолчанию для своей конфигурации
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_REGSDUMP_INTERVAL, 10);
	iniFile.SetValueString(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_SOUNDVOLUME, _T("30%")); // Громкость

	for (int n = 0; n < NUMBER_VIEWS_MEM_DUMP; ++n)
	{
		iniFile.SetValueString(IDS_INI_SECTIONNAME_PARAMETERS, g_arStrDumpAddrID[n], _T("000000"));
		iniFile.SetValueString(IDS_INI_SECTIONNAME_PARAMETERS, g_arStrDumpListID[n], _T("0 : 0"));
	}

	iniFile.SetValueString(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_ADDR_DISASM, _T("001000"));
	// Создание опций
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SAVES_DEFAULT, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SPEAKER, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SPEAKER_FILTER, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SPEAKER_DCOFFSET, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX_FILTER, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX_DCOFFSET, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COVOX_STEREO, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MENESTREL, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MENESTREL_FILTER, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MENESTREL_DCOFFSET, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AY8910, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_2NDAY8910, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AY8910_FILTER, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AY8910_DCOFFSET, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_BKKEYBOARD, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_JOYSTICK, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_ICLBLOCK, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_MOUSEM, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SMOOTHING, false);    // параметры экрана
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_COLOR_MODE, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_BLACK_WHITE, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_LUMINOFOREMODE, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_FULL_SCREEN, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_PAUSE_CPU, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_ASK_FOR_BREAK, false);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_EMUL_LOAD_TAPE, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_EMUL_SAVE_TAPE, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AUTO_BEG_TAPE, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_AUTO_END_TAPE, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_SHOW_PERFORMANCE_STRING, true);
	iniFile.SetValueBool(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_EMULATE_FDDIO, true);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_OPTIONS, IDS_INI_VKBD_TYPE, 0);
	MakeDefaultPalettes();
	MakeDefaultJoyParam();
	MakeDefaultAYVolPanParam();
	// Создание приводов
	iniFile.SetValueString(IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVEA, g_strEmptyUnit);
	iniFile.SetValueString(IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVEB, g_strEmptyUnit);
	iniFile.SetValueString(IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVEC, g_strEmptyUnit);
	iniFile.SetValueString(IDS_INI_SECTIONNAME_DRIVES, IDS_INI_DRIVED, g_strEmptyUnit);
	iniFile.SetValueString(IDS_INI_SECTIONNAME_DRIVES, IDS_INI_HDD0, g_strEmptyUnit);
	iniFile.SetValueString(IDS_INI_SECTIONNAME_DRIVES, IDS_INI_HDD1, g_strEmptyUnit);
	iniFile.FlushIni();
}

bool CConfig::VerifyRoms()
{
	int p = 0;
	fs::path *pStr = nullptr;
	bool bRes = true;
/**
	while ((pStr = m_romModules[p].pstrValue) != nullptr)   // берём очередной параметр
	{
		if (!pStr->empty()) // если значение параметра есть
		{
			if (!Global::isEmptyUnit(*pStr)) // и это не заглушка
			{
				fs::path strPath = m_strROMPath / *pStr; // попробуем открыть этот файл

				if (!fs::exists(strPath))
				{
					CString strError = CString(MAKEINTRESOURCE(IDS_ERRMSG_NOTROM));
					g_BKMsgBox.Show(strError + _T(" \'") + CString(strPath.c_str()) + _T("\'"), MB_OK | MB_ICONSTOP);
					bRes = false;
				}
			}
		}
		else
		{
			// если значения параметра нет, то тоже плохо
			CString strError = CString(MAKEINTRESOURCE(IDS_ERRMSG_ROM_NOT_DEFINED));
			CString str = CString(MAKEINTRESOURCE(m_romModules[p].nID));
			g_BKMsgBox.Show(strError + _T(" \'") + str + _T("\'"), MB_OK | MB_ICONSTOP);
			bRes = false;
		}

		p++;
	}
**/
	return bRes;
}

void CConfig::SetBKBoardType(const CString strBKBoardType)
{
	for (auto t = static_cast<int>(CONF_BKMODEL::BK_0010_01); t < static_cast<int>(CONF_BKMODEL::NUMBERS); ++t)
	{
		if (g_mstrConfigBKModelParameters[t].strBKModelConfigName == strBKBoardType)
		{
			SetBKModel(static_cast<CONF_BKMODEL>(t));
			return;
		}
	}

	SetBKModel(CONF_BKMODEL::BK_0010_01);
}

// возвращает номер текущей конфигурации.
// выход: если найдено, то номер, если не найдено - то 0 (значение по умолчанию)
// выдавать m_BKConfigModelNumber нельзя, т.к. это значение как раз формируется из
// результата этой функции. Вот такая-вот загогулина
CONF_BKMODEL CConfig::GetBKModel()
{
	for (auto t = static_cast<int>(CONF_BKMODEL::BK_0010_01); t < static_cast<int>(CONF_BKMODEL::NUMBERS); ++t)
	{
		if (g_mstrConfigBKModelParameters[t].strBKModelConfigName == m_strBKBoardType)
		{
			SetBKModel(static_cast<CONF_BKMODEL>(t));
			return static_cast<CONF_BKMODEL>(t);
		}
	}

	return CONF_BKMODEL::BK_0010_01;
}

// получение человекопонятного имени текущего конфига
const CString CConfig::GetBKConfName()
{
///	UINT nID = g_mstrConfigBKModelParameters[static_cast<int>(m_BKConfigModelNumber)].nIDBKModelName;
///	return CString(MAKEINTRESOURCE(nID));
return CString("bk_emu_gid.conf");
}

void CConfig::SetBKModel(const CONF_BKMODEL n)
{
	m_BKConfigModelNumber = n;
	BK_MODEL_PARAMETERS param = g_mstrConfigBKModelParameters[static_cast<int>(n)];
	m_strBKBoardType = param.strBKModelConfigName;
	m_BKBoardModel = param.nBKBoardModel;
	m_BKFDDModel = param.nMPIDeviceModel;
}


const CString CConfig::GetRomModuleName(int nIniKey)
{
	return iniFile.GetValueStringEx(m_strBKBoardType, IDS_INI_SECTIONNAME_ROMMODULES, nIniKey, g_strEmptyUnit);
}

bool CConfig::isBK10()
{
	switch (m_BKConfigModelNumber)
	{
		case CONF_BKMODEL::BK_0010_01:
		case CONF_BKMODEL::BK_0010_01_MSTD:
		case CONF_BKMODEL::BK_0010_01_EXT32RAM:
		case CONF_BKMODEL::BK_0010_01_FDD:
		case CONF_BKMODEL::BK_0010_01_A16M:
		case CONF_BKMODEL::BK_0010_01_SMK512:
		case CONF_BKMODEL::BK_0010_01_SAMARA:
			return true;
	}

	return false;
}

bool CConfig::isBK11()
{
	switch (m_BKConfigModelNumber)
	{
		case CONF_BKMODEL::BK_0011:
		case CONF_BKMODEL::BK_0011_FDD:
		case CONF_BKMODEL::BK_0011_A16M:
		case CONF_BKMODEL::BK_0011_SMK512:
		case CONF_BKMODEL::BK_0011_SAMARA:
			return true;
	}

	return false;
}

bool CConfig::isBK11M()
{
	switch (m_BKConfigModelNumber)
	{
		case CONF_BKMODEL::BK_0011M:
		case CONF_BKMODEL::BK_0011M_FDD:
		case CONF_BKMODEL::BK_0011M_A16M:
		case CONF_BKMODEL::BK_0011M_SMK512:
		case CONF_BKMODEL::BK_0011M_SAMARA:
			return true;
	}

	return false;
}

int CConfig::GetDriveNum(const FDD_DRIVE eDrive)
{
	switch (eDrive)
	{
		case FDD_DRIVE::A:
			return 0;

		case FDD_DRIVE::B:
			return 1;

		case FDD_DRIVE::C:
			return 2;

		case FDD_DRIVE::D:
			return 3;
	}

	return 0; // всегда возвращаем хоть что-то истинное
}

int CConfig::GetDriveNum(const HDD_MODE eDrive)
{
	switch (eDrive)
	{
		case HDD_MODE::MASTER:
			return 0;

		case HDD_MODE::SLAVE:
			return 1;
	}

	return 0; // всегда возвращаем хоть что-то истинное
}

fs::path CConfig::CheckDriveImgName(fs::path &str)
{
	if (!Global::isEmptyUnit(str))
	{
		fs::path strName = GetDriveImgName_Full(str);

		if (fs::exists(strName))
		{
			return strName;
		}
	}

	return g_strEmptyUnit.GetString();
}

fs::path CConfig::GetDriveImgName(const HDD_MODE eDrive)
{
	int nDrive = GetDriveNum(eDrive);
	return GetDriveImgName_Full(m_strHDDrives[nDrive]);
}

fs::path CConfig::GetDriveImgName(const FDD_DRIVE eDrive)
{
	int nDrive = GetDriveNum(eDrive);
	return GetDriveImgName_Full(m_strFDDrives[nDrive]);
}

fs::path CConfig::GetShortDriveImgName(const FDD_DRIVE eDrive)
{
	int nDrive = GetDriveNum(eDrive);
	return GetDriveImgName_Short(m_strFDDrives[nDrive]);
}

fs::path CConfig::GetShortDriveImgName(const HDD_MODE eDrive)
{
	int nDrive = GetDriveNum(eDrive);
	return GetDriveImgName_Short(m_strHDDrives[nDrive]);
}

fs::path CConfig::GetShortDriveImgName(const fs::path &strPathName)
{
	return GetDriveImgName_Short(strPathName);
}

fs::path CConfig::GetDriveImgName_Full(const fs::path &str)
{/***
	if (!Global::isEmptyUnit(str))
	{
		// прочитаем путь заданного привода
		if (!str.has_parent_path()) // если путь в имени образа отсутствует
		{
			// то возвращаем имя с полным путём к домашней папке
			return m_strIMGPath / str;
		}

		// если путь есть, а имени диска нету, то задан относительный путь
		if (!str.has_root_name())
		{
			// то возвращаем имя с полным путём к домашней папке
			return m_strIMGPath / str;
		}
	}
***/
	// иначе, возвращаем имя как есть, предполагаем, что там путь верный.
	return str;
}

fs::path CConfig::GetDriveImgName_Short(const fs::path &str)
{ /***
	if (!Global::isEmptyUnit(str))
	{
		if (str.has_parent_path())
		{
			// теперь разберёмся с относительными путями.
			CString ip = m_strIMGPath.c_str();
			CString str1 = str.c_str();

			// надо из пути удалить базовый путь
			if (str1.Find(ip, 0) == 0) // если путь нашёлся, и он как и положено - в начале
			{
				// +1 потому что у путей нету замыкающего слеша
				return fs::path(str1.Mid(ip.GetLength() + 1).GetString());
			}
		}
	}
***/
	// иначе, возвращаем имя как есть, предполагаем, что там путь верный.
	return str;
}

void CConfig::SetDriveImgName(const HDD_MODE eDrive, const fs::path &strPathName)
{
	int nDrive = GetDriveNum(eDrive);
	m_strHDDrives[nDrive] = GetDriveImgName_Short(strPathName);
}

void CConfig::SetDriveImgName(const FDD_DRIVE eDrive, const fs::path &strPathName)
{
	int nDrive = GetDriveNum(eDrive);
	m_strFDDrives[nDrive] = GetDriveImgName_Short(strPathName);
}

// работа с палитрами

const UINT arColIni[16] =
{
	IDS_INI_PALCOL00,
	IDS_INI_PALCOL01,
	IDS_INI_PALCOL02,
	IDS_INI_PALCOL03,
	IDS_INI_PALCOL04,
	IDS_INI_PALCOL05,
	IDS_INI_PALCOL06,
	IDS_INI_PALCOL07,
	IDS_INI_PALCOL08,
	IDS_INI_PALCOL09,
	IDS_INI_PALCOL10,
	IDS_INI_PALCOL11,
	IDS_INI_PALCOL12,
	IDS_INI_PALCOL13,
	IDS_INI_PALCOL14,
	IDS_INI_PALCOL15
};

// создание палитр по умолчанию
void CConfig::MakeDefaultPalettes()
{
	// создадим стандартную обычную ЧБ палитру
	CString strPal = ::ColorToStr(g_pMonochromePalette_std[0][0])
	                 + _T(",") + ::ColorToStr(g_pMonochromePalette_std[0][1]);
	iniFile.SetValueString(IDS_INI_SECTIONNAME_PALETTES, IDS_INI_PALBW, strPal);
	// создадим стандартную адаптивную ЧБ палитру
	strPal = ::PaletteToStr(&g_pAdaptMonochromePalette_std[0][0]);
	iniFile.SetValueString(IDS_INI_SECTIONNAME_PALETTES, IDS_INI_PALADAPTBW, strPal);

	// создадим цветные палитры
	for (int i = 0; i < 16; ++i)
	{
		strPal = ::PaletteToStr(&g_pColorPalettes_std[i][0]);
		iniFile.SetValueString(IDS_INI_SECTIONNAME_PALETTES, arColIni[i], strPal);
	}
}

// сохранение текущих палитр
void CConfig::SavePalettes(CString &strCustomize)
{
	// сохраним стандартную обычную ЧБ палитру
	CString strPal = ::ColorToStr(g_pMonochromePalette[0][0])
	                 + _T(",") + ::ColorToStr(g_pMonochromePalette[0][1]);
	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PALETTES, IDS_INI_PALBW, strPal);
	// сохраним стандартную адаптивную ЧБ палитру
	strPal = ::PaletteToStr(&g_pAdaptMonochromePalette[0][0]);
	iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PALETTES, IDS_INI_PALADAPTBW, strPal);

	// сохраним цветные палитры
	for (int i = 0; i < 16; ++i)
	{
		strPal = ::PaletteToStr(&g_pColorPalettes[i][0]);
		iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PALETTES, arColIni[i], strPal);
	}
}

// загрузка текущих палитр
void CConfig::LoadPalettes(CString &strCustomize)
{
	// создадим стандартную обычную ЧБ палитру
	CString strPal = ::ColorToStr(g_pMonochromePalette_std[0][0])
	                 + _T(",") + ::ColorToStr(g_pMonochromePalette_std[0][1]);
	// загрузим текущую обычную ЧБ палитру
	CString strPalVal = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PALETTES, IDS_INI_PALBW, strPal);
	::StrToPalette(strPalVal, &g_pMonochromePalette[0][0]);
	g_pMonochromePalette[0][2] = g_pMonochromePalette[1][0] = g_pMonochromePalette[1][1] = g_pMonochromePalette[0][0];
	g_pMonochromePalette[0][3] = g_pMonochromePalette[1][2] = g_pMonochromePalette[1][3] = g_pMonochromePalette[0][1];
	// создадим стандартную адаптивную ЧБ палитру
	strPal = ::PaletteToStr(&g_pAdaptMonochromePalette_std[0][0]);
	// загрузим текущую адаптивную ЧБ палитру
	strPalVal = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PALETTES, IDS_INI_PALADAPTBW, strPal);
	::StrToPalette(strPalVal, &g_pAdaptMonochromePalette[0][0]);
	g_pAdaptMonochromePalette[1][0] = g_pAdaptMonochromePalette[0][0];
	g_pAdaptMonochromePalette[1][1] = g_pAdaptMonochromePalette[0][1];
	g_pAdaptMonochromePalette[1][2] = g_pAdaptMonochromePalette[0][2];
	g_pAdaptMonochromePalette[1][3] = g_pAdaptMonochromePalette[0][3];

	// создадим цветные палитры
	for (int i = 0; i < 16; ++i)
	{
		strPal = ::PaletteToStr(&g_pColorPalettes_std[i][0]);
		strPalVal = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_PALETTES, arColIni[i], strPal);
		::StrToPalette(strPalVal, &g_pColorPalettes[i][0]);
	}
}
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
const JoyElem_t CConfig::m_arJoystick_std[BKJOY_PARAMLEN] =
{
	{_T("VK_UP"),    VK_UP,    0001}, //BKJOY_UP = 0;
	{_T("VK_RIGHT"), VK_RIGHT, 0002}, //BKJOY_RIGHT = 1;
	{_T("VK_DOWN"),  VK_DOWN,  0004}, //BKJOY_DOWN = 2;
	{_T("VK_LEFT"),  VK_LEFT,  0010}, //BKJOY_LEFT = 3;
	{_T("VK_HOME"),  VK_HOME,  0040}, //BKJOY_FIRE = 4;
	{_T("VK_PRIOR"), VK_PRIOR, 0100}, //BKJOY_ALTFIRE = 5;
	{_T("VK_END"),   VK_END,   0020}, //BKJOY_A = 6;
	{_T("VK_NEXT"),  VK_NEXT,  0200}, //BKJOY_B = 7;
};

static const UINT arJoyparamIni[BKJOY_PARAMLEN] =
{
	IDS_INI_BKJOY_UP,
	IDS_INI_BKJOY_RIGHT,
	IDS_INI_BKJOY_DOWN,
	IDS_INI_BKJOY_LEFT,
	IDS_INI_BKJOY_FIRE,
	IDS_INI_BKJOY_ALTFIRE,
	IDS_INI_BKJOY_A,
	IDS_INI_BKJOY_B
};
void CConfig::MakeDefaultJoyParam()
{
	for (int i = 0; i < BKJOY_PARAMLEN; ++i)
	{
		m_arJoystick[i] = m_arJoystick_std[i];
		CString str = m_arJoystick[i].strVKeyName + _T(" : ") + Global::WordToOctString(m_arJoystick[i].nMask);
		iniFile.SetValueString(IDS_INI_SECTIONNAME_JOYSTICK, arJoyparamIni[i], str);
	}
}

// если массив параметров джойстика сделать вне класса, выделенная под него память успевает освободиться
// перед тем, как выполнится эта функция
void CConfig::SaveJoyParams(CString &strCustomize)
{
	for (int i = 0; i < BKJOY_PARAMLEN; ++i)
	{
		CString str = m_arJoystick[i].strVKeyName + _T(" : ") + Global::WordToOctString(m_arJoystick[i].nMask);
		iniFile.SetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_JOYSTICK, arJoyparamIni[i], str);
	}
}

void CConfig::LoadJoyParams(CString &strCustomize)
{
	for (int i = 0; i < BKJOY_PARAMLEN; ++i)
	{
		CString strDef = m_arJoystick_std[i].strVKeyName + _T(" : ") + Global::WordToOctString(m_arJoystick_std[i].nMask);
		CString strJoy = iniFile.GetValueStringEx(strCustomize, IDS_INI_SECTIONNAME_JOYSTICK, arJoyparamIni[i], strDef);
		// теперь парсим строку
		bool bOk = false;
		strJoy.Trim();
		int colon = strJoy.Find(_T(":"), 0); // поищем начало разделителя
/***
		if (colon >= 0)
		{
			CString strName = strJoy.Left(colon).Trim(); // выделим имя клавиши
			CString strMask = strJoy.Right(strJoy.GetLength() - colon - 1).Trim(); // выделим маску
			UINT nVKey = ::getKeyValue(strName);

			if (nVKey != -1)
			{
				m_arJoystick[i].strVKeyName = strName;
				m_arJoystick[i].nVKey = nVKey;
				m_arJoystick[i].nMask = Global::OctStringToWord(strMask);
				bOk = true;
			}
		}
**/
		if (!bOk)
		{
			// не нашлось - зададим по умолчанию
			m_arJoystick[i] = m_arJoystick_std[i];
		}
	}
}


void CConfig::SaveAYVolPanParams(CString &strCustomize)
{
	iniFile.SetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1PANAL, m_AYParam[AY1].nPL[CHAN_A]);
	iniFile.SetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1PANBL, m_AYParam[AY1].nPL[CHAN_B]);
	iniFile.SetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1PANCL, m_AYParam[AY1].nPL[CHAN_C]);
	iniFile.SetValueFloatEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1VOLA, m_AYParam[AY1].V[CHAN_A]);
	iniFile.SetValueFloatEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1VOLB, m_AYParam[AY1].V[CHAN_B]);
	iniFile.SetValueFloatEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1VOLC, m_AYParam[AY1].V[CHAN_C]);
	iniFile.SetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2PANAL, m_AYParam[AY2].nPL[CHAN_A]);
	iniFile.SetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2PANBL, m_AYParam[AY2].nPL[CHAN_B]);
	iniFile.SetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2PANCL, m_AYParam[AY2].nPL[CHAN_C]);
	iniFile.SetValueFloatEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2VOLA, m_AYParam[AY2].V[CHAN_A]);
	iniFile.SetValueFloatEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2VOLB, m_AYParam[AY2].V[CHAN_B]);
	iniFile.SetValueFloatEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2VOLC, m_AYParam[AY2].V[CHAN_C]);
}

void CConfig::CheckParV(CString &strCustomize, int Sect, double &v)
{
	v = iniFile.GetValueFloatEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, Sect, AY_VOL_BASE);

	if (0 > v || v > AY_VOL_BASE)
	{
		v = AY_VOL_BASE;
	}
}

void CConfig::CheckParP(CString &strCustomize, int Sect, int def, int &v)
{
	v = iniFile.GetValueIntEx(strCustomize, IDS_INI_SECTIONNAME_PARAMETERS, Sect, def);

	if (0 > v || v > AY_PAN_BASE)
	{
		v = def;
	}
}

void CConfig::LoadAYVolPanParams(CString &strCustomize)
{
	CheckParP(strCustomize, IDS_INI_AY1PANAL, AY_LEFT_PAN_DEFAULT,   m_AYParam[AY1].nPL[CHAN_A]);
	CheckParP(strCustomize, IDS_INI_AY1PANBL, AY_CENTER_PAN_DEFAULT, m_AYParam[AY1].nPL[CHAN_B]);
	CheckParP(strCustomize, IDS_INI_AY1PANCL, AY_RIGHT_PAN_DEFAULT,  m_AYParam[AY1].nPL[CHAN_C]);
	CheckParV(strCustomize, IDS_INI_AY1VOLA, m_AYParam[AY1].V[CHAN_A]);
	CheckParV(strCustomize, IDS_INI_AY1VOLB, m_AYParam[AY1].V[CHAN_B]);
	CheckParV(strCustomize, IDS_INI_AY1VOLC, m_AYParam[AY1].V[CHAN_C]);
	CheckParP(strCustomize, IDS_INI_AY2PANAL, AY_LEFT_PAN_DEFAULT,   m_AYParam[AY2].nPL[CHAN_A]);
	CheckParP(strCustomize, IDS_INI_AY2PANBL, AY_CENTER_PAN_DEFAULT, m_AYParam[AY2].nPL[CHAN_B]);
	CheckParP(strCustomize, IDS_INI_AY2PANCL, AY_RIGHT_PAN_DEFAULT,  m_AYParam[AY2].nPL[CHAN_C]);
	CheckParV(strCustomize, IDS_INI_AY2VOLA, m_AYParam[AY2].V[CHAN_A]);
	CheckParV(strCustomize, IDS_INI_AY2VOLB, m_AYParam[AY2].V[CHAN_B]);
	CheckParV(strCustomize, IDS_INI_AY2VOLC, m_AYParam[AY2].V[CHAN_C]);
}

void CConfig::MakeDefaultAYVolPanParam()
{
	initVolPan();
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1PANAL, m_AYParam[AY1].nPL[CHAN_A]);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1PANBL, m_AYParam[AY1].nPL[CHAN_B]);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1PANCL, m_AYParam[AY1].nPL[CHAN_C]);
	iniFile.SetValueFloat(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1VOLA, m_AYParam[AY1].V[CHAN_A]);
	iniFile.SetValueFloat(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1VOLB, m_AYParam[AY1].V[CHAN_B]);
	iniFile.SetValueFloat(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY1VOLC, m_AYParam[AY1].V[CHAN_C]);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2PANAL, m_AYParam[AY2].nPL[CHAN_A]);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2PANBL, m_AYParam[AY2].nPL[CHAN_B]);
	iniFile.SetValueInt(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2PANCL, m_AYParam[AY2].nPL[CHAN_C]);
	iniFile.SetValueFloat(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2VOLA, m_AYParam[AY2].V[CHAN_A]);
	iniFile.SetValueFloat(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2VOLB, m_AYParam[AY2].V[CHAN_B]);
	iniFile.SetValueFloat(IDS_INI_SECTIONNAME_PARAMETERS, IDS_INI_AY2VOLC, m_AYParam[AY2].V[CHAN_C]);
}

// выдача параметров заданного AY.
// если задать недопустимый индекс, то выдаёт значения по умолчанию. Чем можно пользоваться.
CConfig::AYVolPan_s CConfig::getVolPan(int n) const
{
	if (0 <= n && n < AY_NUMS)
	{
		return m_AYParam[n];
	}

	return { AY_LEFT_PAN_DEFAULT, AY_CENTER_PAN_DEFAULT, AY_RIGHT_PAN_DEFAULT,
	         AY_PAN_BASE - AY_LEFT_PAN_DEFAULT, AY_PAN_BASE - AY_CENTER_PAN_DEFAULT, AY_PAN_BASE - AY_RIGHT_PAN_DEFAULT,
	         AY_VOL_BASE, AY_VOL_BASE, AY_VOL_BASE };
}

void CConfig::setVolPan(int n, AYVolPan_s &s)
{
	if (0 <= n && n < AY_NUMS)
	{
		m_AYParam[n] = s;
	}
}

void CConfig::initVolPan()
{
	m_AYParam[AY1] = getVolPan(-1); // инициализируем значениями по умолчанию.
	m_AYParam[AY2] = getVolPan(-1);
}



/* это правильная таблица, но на бк всё по особому, даже кои8 там нестандартный
// таблица соответствия верхней половины аскии кодов с 128 по 255, включая псевдографику
static const TCHAR koi8tbl_RFC1489[128] = {
0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524, 0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590,
0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248, 0x2264, 0x2265, 0xA0,   0x2321, 0xB0,   0xB2,   0xB7,   0xF7,
0x2550, 0x2551, 0x2552, 0x451,  0x2553, 0x2554, 0x2555, 0x2556, 0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E,
0x255F, 0x2560, 0x2561, 0x401,  0x2562, 0x2563, 0x2564, 0x2565, 0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0xA9,
_T('ю'), _T('а'), _T('б'), _T('ц'), _T('д'), _T('е'), _T('ф'), _T('г'), _T('х'), _T('и'), _T('й'), _T('к'), _T('л'), _T('м'), _T('н'), _T('о'),
_T('п'), _T('я'), _T('р'), _T('с'), _T('т'), _T('у'), _T('ж'), _T('в'), _T('ь'), _T('ы'), _T('з'), _T('ш'), _T('э'), _T('щ'), _T('ч'), _T('ъ'),
_T('Ю'), _T('А'), _T('Б'), _T('Ц'), _T('Д'), _T('Е'), _T('Ф'), _T('Г'), _T('Х'), _T('И'), _T('Й'), _T('К'), _T('Л'), _T('М'), _T('Н'), _T('О'),
_T('П'), _T('Я'), _T('Р'), _T('С'), _T('Т'), _T('У'), _T('Ж'), _T('В'), _T('Ь'), _T('Ы'), _T('З'), _T('Ш'), _T('Э'), _T('Щ'), _T('Ч'), _T('Ъ')
};
*/
// вот бкшная таблица
// таблица соответствия верхней половины аскии кодов с 128 по 255, включая псевдографику
const TCHAR koi8tbl[128] =
{
	// {200..237} этих символов на бк нету.
	_T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '),
	_T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '),
	_T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '),
	_T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '), _T(' '),
	// {240..277}
	0xb6,    0x2534,  0x2665,  0x2510,  0x2561,  0x251c,  0x2514,  0x2550,
	0x2564,  0x2660,  0x250c,  0x252c,  0x2568,  0x2193,  0x253c,  0x2551,
	0x2524,  0x2190,  0x256c,  0x2191,  0x2663,  0x2500,  0x256b,  0x2502,
	0x2666,  0x2518,  0x256a,  0x2565,  0x2567,  0x255e,  0x2192,  0x2593,
	// {300..337}
	_T('ю'), _T('а'), _T('б'), _T('ц'), _T('д'), _T('е'), _T('ф'), _T('г'),
	_T('х'), _T('и'), _T('й'), _T('к'), _T('л'), _T('м'), _T('н'), _T('о'),
	_T('п'), _T('я'), _T('р'), _T('с'), _T('т'), _T('у'), _T('ж'), _T('в'),
	_T('ь'), _T('ы'), _T('з'), _T('ш'), _T('э'), _T('щ'), _T('ч'), _T('ъ'),
	// {340..377}
	_T('Ю'), _T('А'), _T('Б'), _T('Ц'), _T('Д'), _T('Е'), _T('Ф'), _T('Г'),
	_T('Х'), _T('И'), _T('Й'), _T('К'), _T('Л'), _T('М'), _T('Н'), _T('О'),
	_T('П'), _T('Я'), _T('Р'), _T('С'), _T('Т'), _T('У'), _T('Ж'), _T('В'),
	_T('Ь'), _T('Ы'), _T('З'), _T('Ш'), _T('Э'), _T('Щ'), _T('Ч'), _T('Ъ')
};

CString Global::WordToOctString(uint16_t word)
{
	CString str;
	WordToOctString(word, str);
	return str;
}


CString Global::ByteToOctString(uint8_t byte)
{
	CString str;
	ByteToOctString(byte, str);
	return str;
}

void Global::WordToOctString(uint16_t word, CString &str)
{
	LPTSTR pstr = str.GetBufferSetLength(7);

	for (int i = 5; i >= 0; --i)
	{
		pstr[i] = TCHAR(060 + (word & 7));
		word >>= 3;
	}

	str.ReleaseBuffer(6);
}

void Global::ByteToOctString(uint8_t byte, CString &str)
{
	LPTSTR pstr = str.GetBufferSetLength(4);

	for (int i = 2; i >= 0; --i)
	{
		pstr[i] = TCHAR(060 + (byte & 7));
		byte >>= 3;
	}

	str.ReleaseBuffer(3);
}

#define _tcstol(x, y, z) strtol(x.GetString(), y, z)

uint16_t Global::OctStringToWord(const CString &str)
{
	int word = _tcstol(str, nullptr, 8);
	return LOWORD(word);
}

#define _itot_s(x, y, z, r) itoa(x, y, z)

CString Global::IntToString(int iInt, int radix) // radix?
{
	CString str;
	_itot_s(iInt, str.GetBufferSetLength(256), 256, radix);
	str.ReleaseBuffer();
	return str;
}

CString Global::IntToFileLengthString(int iInt)
{
	CString str = IntToString(iInt, 10); // текст в виде строки
	int nPos = str.GetLength() - 3; // указатель на позицию разделителя разрядов

	while (nPos > 0) // пока есть куда вставлять
	{
	///	str.Insert(nPos, _T(" ")); // вставляем разделитель
		nPos -= 3; // и сдвигаемся левее
	}

	return str;
}

// то же что и для дир + меняем страшные символы '\' и '/' на нестрашную '.'
void Global::SetSafeName(CString &str)
{
	SetSafeDir(str);
	str.Replace(_T('/'), _T('.'));
	str.Replace(_T('\\'), _T('.'));
}
// меняем страшные символы '>','<',':','|','?','*' на нестрашную '_'
// и все коды меньше пробела - на '_'
void Global::SetSafeDir(CString &str)
{
	str.Replace(_T('>'), _T('_'));
	str.Replace(_T('<'), _T('_'));
	str.Replace(_T(':'), _T('_'));
	str.Replace(_T('|'), _T('_'));
	str.Replace(_T('?'), _T('_'));
	str.Replace(_T('*'), _T('_'));
	int len = str.GetLength();

	// а теперь заменим все символы с кодом меньше 32
	for (int i = 0; i < len; ++i)
	{
		if (str[i] < 32)
		{
			str.SetAt(i, _T('_'));
		}
	}
}

// Вход: msTime время в миллисекундах
CString Global::MsTimeToTimeString(int msTime)
{
	CString strMin = IntToString(msTime / 60000, 10);
	CString strSec;
	strSec.Format(_T("%02i"), msTime / 1000 % 60);
	return strMin + _T(":") + strSec;
}


CString Global::BKToUNICODE(uint8_t *pBuff, int size)
{
	CString strRet;
	LPTSTR pstr = strRet.GetBufferSetLength(size + 1);
	int len = 0;

	while (len < size)
	{
		TCHAR tch = BKToWIDEChar(pBuff[len]);
		pstr[len++] = tch;

		if (tch == 0)
		{
			break;
		}
	}

	strRet.ReleaseBuffer(len - 1);
	return strRet;
}

TCHAR Global::BKToWIDEChar(uint8_t b)
{
	if (b == 0)
	{
		return 0;
	}

	if (b < 32)
	{
		return _T(' ');
	}

	if (b == 127)
	{
		return TCHAR(0x25a0);
	}

	if (b >= 128)
	{
		return koi8tbl[b - 128];
	}

	return TCHAR(b);
}

// эта функция не выдаёт 0, заменяет его пробелом
TCHAR Global::BKToSafeWIDEChar(uint8_t b)
{
	if (b < ' ')
	{
		return _T(' ');
	}

	if (b == 127)
	{
		return TCHAR(0x25a0);
	}

	if (b >= 128)
	{
		return TCHAR(koi8tbl[b - 128]);
	}

	return TCHAR(b);
}

/*
преобразование юникодной строки в бкшный кои8.
вход:
ustr - преобразуемая строка
pBuff - буфер, куда выводится результат
bufSize - размер буфера
bFillBuf - флаг. если строка короче размера буфера, буфер до конца забивается пробелами. (конца строки - 0 нету.)
*/
void Global::UNICODEtoBK(CString &ustr, uint8_t *pBuff, int bufSize, bool bFillBuf)
{
	const int len = ustr.GetLength();
	int bn = 0;

	for (int n = 0; n < len; ++n)
	{
		pBuff[bn++] = WIDEtoBKChar(ustr.GetAt(n));// берём очередной символ

		if (bn >= bufSize) // если буфер БКшной строки короче юникодной строки
		{
			break; // прерываем цикл
		}
	}

	if (bFillBuf) // если нужно и можно
	{
		while (bn < bufSize) // забьём конец БКшного буфера пробелами
		{
			pBuff[bn++] = 32;
		}
	}
}

uint8_t Global::WIDEtoBKChar(TCHAR ch)
{
	if (ch == _T('\t'))
	{
		return 9;
	}

	if (ch == _T('\n'))
	{
		return 10;
	}

	if (ch < 32)    // если символ меньше пробела,
	{
		return 32;  // то будет пробел
	}

	if ((32 <= ch) && (ch < 127)) // если буквы-цифры- знаки препинания
	{
		return LOBYTE(ch); // то буквы-цифры- знаки препинания
	}

	if (ch == 0x401)    // Если Ё
	{
		return 0345;    // то Е
	}

	if (ch == 0x451)    // если ё
	{
		return 0305;    // то е
	}

	if (ch == 0x25a0)   // если такое
	{
		return 127; // то это. это единственное исключение в нижней половине аски кодов
	}

	// если всякие другие символы
	// то ищем в таблице нужный нам символ, а его номер - будет кодом кои8
	for (uint8_t i = 0; i < 128; ++i)
	{
		if (koi8tbl[i] == ch)
		{
			return (i + 0200);
		}
	}

	return 32; // если такого символа нету в таблице - будет пробел
}


int Global::ToInt(const CString &str)
{
	return std::stoi(str.GetString(), nullptr);
}

long Global::ToLong(const CString &str)
{
	return std::stol(str.GetString(), nullptr);
}

long long Global::ToLongLong(const CString &str)
{
	return std::stoll(str.GetString(), nullptr);
}

unsigned long Global::ToULong(const CString &str)
{
	return std::stoul(str.GetString(), nullptr);
}

unsigned long long Global::ToULongLong(const CString &str)
{
	return std::stoull(str.GetString(), nullptr);
}



#ifdef _DEBUG
// функция для вывода сообщений об ошибках при отладке, когда совсем ничего не понятно, почему не работает
void Global::GetLastErrorOut(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code
	LPVOID lpMsgBuf = nullptr;
	DWORD dw = GetLastError();
	FormatMessage(
	    FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM |
	    FORMAT_MESSAGE_IGNORE_INSERTS,
	    nullptr,
	    dw,
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	    (LPTSTR)&lpMsgBuf,
	    0, nullptr);
	// Display the error message and exit the process
	CString strErr;
	strErr.Format(_T("%s failed with error %u: %s"), lpszFunction, dw, (LPCTSTR)lpMsgBuf);
	MessageBox(nullptr, strErr.GetString(), _T("Error"), MB_OK);
	LocalFree(lpMsgBuf);
}
#endif


bool Global::CheckFFMPEG()
{
	fs::path strName = g_Config.GetConfCurrPath() / _T("ffmpeg.exe");
	return !!fs::exists(strName);
}

/*
* Сохранить массив в формате bin
* Вход: buf - указатель на массив данных
* addr - адрес загрузки
* len - длина файла, == размеру массива buf
* strName - имя файла
*/
bool Global::SaveBinFile(uint8_t *buf, uint16_t addr, uint16_t len, const fs::path &strName)
{
	if (buf && !strName.empty())
	{
		CFile file;

		if (file.Open(strName.c_str(), CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
		{
			// запишем заголовок
			file.Write(&addr, sizeof(uint16_t));
			file.Write(&len, sizeof(uint16_t));

			if (g_Config.m_bUseLongBinFormat)
			{
				uint8_t bkName[16];
				CString str = strName.cstr();
				UNICODEtoBK(str, bkName, 16, true);
				file.Write(bkName, 16); // имя файла
			}

			file.Write(buf, len);

			if (g_Config.m_bUseLongBinFormat)
			{
				int crc = 0;

				for (int i = 0; i < len; ++i)
				{
					crc += buf[i];

					if (crc & 0xFFFF0000)
					{
						// если случился перенос в 17 разряд (т.е. бит С для word)
						crc &= 0x0000FFFF; // его обнулим
						crc++; // но прибавим к сумме
					}
				}

				file.Write(&crc, sizeof(uint16_t));
			}

			file.Close();
			return true;
		}

		g_BKMsgBox.Show(_T("Ошибка создания файла."), MB_OK);
	}

	return false;
}

static FSIZE_t min (FSIZE_t a, FSIZE_t b) {
    return a > b ? b : a;
}
/*
* Загрузить массив в формате bin
* Вход: buf - указатель на массив данных
* addr - адрес загрузки
* len - длина файла, == размеру массива buf
* strName - имя загружаемого файла
* bStrict == true - строго бин файл, false - любой файл
*/
bool Global::LoadBinFile(uint8_t* &buf, uint16_t &addr, uint16_t &len, const fs::path &strName, bool bStrict)
{
	CFile file;
	bool bRet = false;
	CString strExt(".bin");
	fs::path strWorkName = strName;

	if (!fs::exists(strWorkName)) //если такого файла нет
	{
	///	if (!strName.has_extension()) // если расширения нет
		{
	///		strWorkName.replace_extension(strExt.GetString()); // добавим расширение
		}
	///	else // если расширение есть
		{
	///		strWorkName.replace_extension(); // удалим расширение
		}

	///	if (!fs::exists(strWorkName)) //если и такого файла нет
		{
			return false; // выходим с ошибкой
		}
	}

	if (file.Open(strWorkName.c_str(), CFile::modeRead | CFile::typeBinary))
	{
		uint16_t nAddr;
		uint16_t nLen;

		// читаем первое слово - это адрес
		if (file.Read(&nAddr, sizeof(uint16_t)) == sizeof(uint16_t))
		{
			// читаем второе слово - это длина
			if (file.Read(&nLen, sizeof(uint16_t)) == sizeof(uint16_t))
			{
				ULONGLONG filelen = file.GetLength();
				bool bIsBin = true;

				if (filelen >= 65536) // если файл >=64k то у него заголовок bin быть не может, длина в 16 бит не влазит
				{
					bIsBin = false;
				}
				else
				{
					// если файл < 64k, то проверяем заголовок bin
					if ((nLen == filelen - 4) // если простой бин
					        || (nLen == filelen - 6))// или усложнённый
					{
						file.Seek(4, CFile::SeekPosition::begin);
					}
					else if (nLen != filelen - 22) // или сложный
					{
						file.Seek(20, CFile::SeekPosition::begin);
					}
					else
					{
						bIsBin = false; // заголовка не нашли
					}
				}

				if (!bIsBin)
				{
					file.Seek(0, CFile::SeekPosition::begin);
					//будем загружать как простой файл.
					nAddr = 0;
					nLen = (min(0160000ull, filelen)) & 0xffff; //ограничим размер, 1600000 - это максимум в режиме ALL CMK-512, режимы ОЗУ11 и Hlt11, где доступно 170000 и 177600 байтов ОЗУ учитывать не будем
				}

				if (bIsBin || !bStrict)
				{
					buf = new uint8_t[nLen]; //// TODO: delete

					if (buf)
					{
						file.Read(buf, nLen);
						addr = nAddr;
						len = nLen;
						bRet = true;
					}
				}
			}
		}

		file.Close();
	}

	return bRet;
}

// выход: true - строка равна g_strEmptyUnit
//      false - не равна
bool Global::isEmptyUnit(const CString &s)
{
	return !s.CollateNoCase(g_strEmptyUnit);
}

bool Global::isEmptyUnit(const fs::path &s)
{
	return isEmptyUnit(CString(s.c_str()));
}
/**
void Global::SetMonospaceFont(HWND hwnd, CFont *pFont)
{
	LOGFONT lf{};
	::GetObject((HGDIOBJ)::SendMessage(hwnd, WM_GETFONT, 0, 0), sizeof(LOGFONT), &lf);
	_tcscpy_s(lf.lfFaceName, LF_FACESIZE, _T("Courier New\0"));

	if (!pFont->CreateFontIndirect(&lf))
	{
		_tcscpy_s(lf.lfFaceName, LF_FACESIZE, _T("Lucida Console\0"));
		pFont->CreateFontIndirect(&lf);
	}
}
**/
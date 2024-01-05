#pragma once

#include "Ini.h"
#include "MSF.h"

#include "SafeReleaseDefines.h"
#include "ScreenColors_Shared.h"
#include "BKSound_Defines.h"

#include "HDIStuff.h"

// Timers
constexpr auto BKTIMER_UI_REFRESH = 1;
constexpr auto BKTIMER_UI_TIME    = 2;
constexpr auto BKTIMER_SCREEN_FPS = 3;
constexpr auto BKTIMER_MOUSE      = 4;
constexpr auto BKTIMER_TAPECTRL   = 5;

// тактовая частота
constexpr auto CPU_SPEED_BK11 = 4000000;
constexpr auto CPU_SPEED_BK10 = 3000000;

// адрес с которого начинаются порты, если к МПИ ничего не подключено
constexpr auto BK_PURE_PORTSIO_AREA = 0177600;
// адрес с которого начинаются порты, если к МПИ что-нибудь подключено
constexpr auto BK_PORTSIO_AREA = 0177000;

enum class STATUS_FIELD : int
{
	SYSTEM = 0,
	KBD_XLAT,
	KBD_CAPS,
	KBD_AR2,
	KBD_SU,
	SCR_REZ,
	CPU_FRQ,
	INFO,
	KBD_SHIFT // фейковая панель.
};

// определения для звука
enum : int { MODEL_AY_3_8910 = 0, MODEL_YM2149 };
enum : int { MODE_GID = 0, MODE_GRYPHON, MODE_TURBOSOUND };
// constexpr auto DEFAULT_EMU_SOUNDCHIP_FREQUENCY = 1774400; // zx spectrum
constexpr auto DEFAULT_EMU_SOUNDCHIP_FREQUENCY = 1714286; // BK: (12Mhz / 7)
// constexpr auto DEFAULT_EMU_SOUNDCHIP_FREQUENCY = 1789772; // правильная частота, такая, как должна быть 1/2 от поднесущей NTSC (3 579 545,5).

enum class CONF_SCREEN_RENDER : int
{
	NONE = -1,
	OPENGL = 0,
	D2D,
	VFW,
	D3D
};
enum class CONF_OSCILLOSCOPE_RENDER : int
{
	NONE = -1,
	OPENGL = 0,
	D2D,
	GDI
};

// определения для типа дисковода и типа бкшки
enum class BK_DEV_MPI : int
{
	NONE = -1,
	STD_FDD = 0,
	A16M,
	SMK512,
	SAMARA,

	BK0010 = 10,
	BK0011,
	BK0011M
};

enum class CONF_BKMODEL : int
{
	BK_0010_01 = 0,
	BK_0010_01_MSTD,
	BK_0010_01_EXT32RAM,
	BK_0010_01_FDD,
	BK_0010_01_A16M,
	BK_0010_01_SMK512,
	BK_0010_01_SAMARA,
	BK_0011,
	BK_0011_FDD,
	BK_0011_A16M,
	BK_0011_SMK512,
	BK_0011_SAMARA,
	BK_0011M,
	BK_0011M_FDD,
	BK_0011M_A16M,
	BK_0011M_SMK512,
	BK_0011M_SAMARA,
	BK_0010,
	NUMBERS
};

struct BK_MODEL_PARAMETERS
{
	CString strBKModelConfigName;   // имя конфигурации
	UINT nIDBKModelName;            // человекопонятное имя конфигурации
	MSF_CONF nBKBoardModel;         // тип БКшки
	BK_DEV_MPI nMPIDeviceModel;     // тип доп. блока
};

// тут надо бы придумать более человеческий способ
// а то и енум CONFIG_BKMODEL_NUMBER и массив должны совпадать.
extern const BK_MODEL_PARAMETERS g_mstrConfigBKModelParameters[static_cast<int>(CONF_BKMODEL::NUMBERS)];


// структура строковых параметров
struct confStringParam
{
	int nID; // ид ресурса
	fs::path *pstrValue; // указатель на переменную
	fs::path  defValue; // значение по умолчанию
};


constexpr auto SSR_LIST_SIZE = 4;
// список допустимых частот, чтобы что попало вручную не вводили.
extern const int g_EnabledSSR[SSR_LIST_SIZE];

struct SoundChipModelList
{
	int nModelNum;
	UINT nIDstrModel;
};
constexpr auto SCHM_LIST_SIZE = 2;
// список допустимых моделей звукового процессора, чтобы что попало вручную не вводили.
extern const SoundChipModelList g_EnabledSoundChipModels[SCHM_LIST_SIZE];

struct SoundChipFreqlList
{
	int nFreq;
	UINT nIDstrFreq;
};
constexpr auto SCHFRQ_LIST_SIZE = 4;
// список предустановленных частот звукового процессора, чтобы вручную не вводить.
extern const SoundChipFreqlList g_SoundChipFrequences[SCHFRQ_LIST_SIZE];

struct MultiSoundChipWorkMode
{
	int nMode;
	UINT nIDstrMode;
};
constexpr auto MSCHWM_LIST_SIZE = 3;
extern const MultiSoundChipWorkMode g_MultiSoundChipWorkModes[MSCHWM_LIST_SIZE];

extern const CString g_arStrSMKPgNums[16];

extern const TCHAR koi8tbl[128];

// Значения коррекций таймингов инструкций. Это величина, которая вычитается из базового
// тайминга при каждой операции ввода/вывода
// т.к. тайминги рассчитаны при работе в Дин.ОЗУ, то они считаются базовыми и для них коррекция - 0
// для работы в статическом ОЗУ, типа советского, значение от балды взято 1, т.к. я всё равно не знаю,
// какое оно должно быть
// для ПЗУ и быстрого ОЗУ СМК - значение взято от балды 2 (будем считать, что оно 0-тактовое)
constexpr auto RAM_TIMING_CORR_VALUE_D = 0; // для Динамического ОЗУ
constexpr auto RAM_TIMING_CORR_VALUE_S = 2; // для Статического ОЗУ
constexpr auto RAM_TIMING_CORR_VALUE_SMK = 4; // для ОЗУ СМК
constexpr auto ROM_TIMING_CORR_VALUE_SMK = 4; // для ПЗУ СМК
constexpr auto ROM_TIMING_CORR_VALUE = 4; // для ПЗУ 1801
constexpr auto REG_TIMING_CORR_VALUE = 4; // для регистров

#pragma pack(push)
#pragma pack(1)
// это дело сохраняется в файле состояния, поэтому, для определённости
// типы переменных должны быть строго определёнными
struct BKMEMBank_t
{
	BOOL bReadable; // флаг, что память доступна для чтения
	BOOL bWritable; // флаг, что память доступна для записи
	uint32_t nBank; // номер банка памяти 4kb
	uint32_t nPage; // страница памяти БК11 == nBank >> 2 (Этот параметр можно удалить, когда будут сделаны какие-нибудь серьёзные
	// изменения в msf структуре)
	uint32_t nOffset; // смещение в массиве == nBank << 12
	uint32_t nTimingCorrection; // значение корректировки тайминга при обращении к памяти, которая не управляется ВП1-037 (ПЗУ или ОЗУ СМК)
	BKMEMBank_t(): bReadable(FALSE), bWritable(FALSE), nBank(0), nPage(0), nOffset(0), nTimingCorrection(0) {};
};

struct ConfBKModel_t
{
	uint32_t nAltProMemBank; // код подключения страницы памяти контроллера
	/* для простоты пзу бейсика будем включать только в режиме 020. в остальных режимах не имеет смысла, хотя на реальной железке технически возможно */
	uint16_t nExtCodes; // доп коды контроллера, типа 10 - подкл. бейсика и 4 - блокировка регистров 177130 и 177132 по чтению
	/*Потому что на БК11 в страницы 10. и 11. можно опционально подключать свои ПЗУ
	*Формат такой:
	*бит 0 - наличие ПЗУ в странице 8 по адресам 100000
	*бит 1 - наличие ПЗУ в странице 8 по адресам 120000
	*бит 2 - наличие ПЗУ в странице 9 по адресам 100000
	*бит 3 - наличие ПЗУ в странице 9 по адресам 120000
	*и т.д.
	**/
	uint16_t nROMPresent; // битовая маска присутствия ПЗУ БК на своих местах.
	uint32_t nAltProMode; // код режима контроллера
	ConfBKModel_t() : nAltProMemBank(0), nExtCodes(0), nROMPresent(0), nAltProMode(0) {};
};
#pragma pack(pop)

enum class HDD_MODE : int { MASTER = 0, SLAVE, NUM_HDD };
enum class FDD_DRIVE : int
{
	NONE = -1, A = 0, B, C, D, NUM_FDD
};

extern const CString g_strEmptyUnit; // идентификатор. означает, что к данному приводу/в данный слот/прочее ничего не подключено.
extern const CString g_mstrDrives[static_cast<int>(FDD_DRIVE::NUM_FDD)];
extern const int g_mnDrivesIndx[static_cast<int>(FDD_DRIVE::NUM_FDD)];
extern const UINT arColIni[16];

#define DEFAULT_FFMPEG_CMDLINE _T("ffmpeg.exe -y -f rawvideo -vcodec rawvideo -s %dx%d -pix_fmt bgra -framerate 48.828 -i - -c:v libx264 -crf 18 -preset slow -vf scale=1024:768")

// параметры джойстика
struct JoyElem_t
{
	CString strVKeyName;    // имя виртуальной клавиши
	UINT nVKey;             // код виртуальной клавиши
	uint16_t nMask;         // битовая маска в порт.
};
// индексы в массиве параметров джойстика
constexpr auto BKJOY_UP = 0;
constexpr auto BKJOY_RIGHT = 1;
constexpr auto BKJOY_DOWN = 2;
constexpr auto BKJOY_LEFT = 3;
constexpr auto BKJOY_FIRE = 4;
constexpr auto BKJOY_ALTFIRE = 5;
constexpr auto BKJOY_A = 6;
constexpr auto BKJOY_B = 7;
constexpr auto BKJOY_PARAMLEN = 8;

// параметры AY
constexpr int AY_PAN_BASE = 100;
constexpr double AY_VOL_BASE = 1.0;

constexpr int AY_LEFT_PAN_DEFAULT = 95;
constexpr int AY_RIGHT_PAN_DEFAULT = AY_PAN_BASE - AY_LEFT_PAN_DEFAULT;
constexpr int AY_CENTER_PAN_DEFAULT = 50;

enum AYCHAN
{
	CHAN_A = 0,
	CHAN_B,
	CHAN_C,
	AY_CHANS
};

enum AYNUM
{
	AY1 = 0,
	AY2,
	AY_NUMS
};

// количество окошек дампов памяти. Если бы не нужно было делать IDы окошек
// то можно было бы наращивать количество окон просто увеличением этого числа.
constexpr auto NUMBER_VIEWS_MEM_DUMP = 4;

struct WindowParam_t
{
	CString     strName;        // отображаемое имя окна для дампера
	int         nDmpWndAddr;    // рекомендуемый адрес отображения окна, -1 - не имеет значения, можно переопределять
	int         nDmpWndLen;     // размер окна для дампера
	uint8_t    *pDmpEAPtr;      // указатель на начало окна в массиве m_pMemory
};
using VWinParam = std::vector<WindowParam_t>;
struct DumperParam_t
{
	uint16_t nAddr;
	int     nPageListPos;
	int     nAddrListPos;
};


class CConfig
{
	public:
		// Rom модули
		static fs::path m_strBK10_017_Monitor,
		       m_strBK10_018_Focal,
		       m_strBK10_084_Focal,
		       m_strBK10_019_MSTD,
		       m_strBK10_106_basic1,
		       m_strBK10_107_basic2,
		       m_strBK10_108_basic3,
		       m_strBK11_201_bos,
		       m_strBK11_202_ext,
		       m_strBK11_203_mstd,
		       m_strBK11_198_basic1,
		       m_strBK11_199_basic2,
		       m_strBK11_200_basic3,
		       m_strBK11m_324_bos,
		       m_strBK11m_325_ext,
		       m_strBK11m_330_mstd,
		       m_strBK11m_327_basic1,
		       m_strBK11m_328_basic2,
		       m_strBK11m_329_basic3,
		       m_strFDD_Std,
		       m_strFDD_Std253,
		       m_strFDD_A16M,
		       m_strFDD_SMK512,
		       m_strFDD_Samara,
		       m_strBK11Pg121,
		       m_strBK11Pg122,
		       m_strBK11Pg131,
		       m_strBK11Pg132,
		       m_strBK10MSTD;
		// Директории
		static fs::path m_strBinPath,
		       m_strToolsPath,
		       m_strMemPath,
		       m_strSavesPath,
		       m_strTapePath,
		       m_strScriptsPath,
		       m_strROMPath,
		       m_strIMGPath,
		       m_strScreenShotsPath;

		// Параметры
		uint16_t m_nCPURunAddr;         // переопределяемый адрес старта компьютера
		int     m_nCPUFrequency,        // текущая частота работы процессора
		        m_nCPUTempFreq,         // переменная для хранения текущей частоты при установке максимизации скорости
		        m_nRegistersDumpInterval, // интервал обновления данных в окне регистров
		        m_nScreenshotNumber,    // текущий номер скриншота
		        m_nSoundVolume,         // текущая громкость
		        m_nSoundSampleRate,     // текущая частота дискретизации всей звуковой подсистемы эмулятора
		        m_nSoundChipFrequency,  // текущая частота работы муз. сопроцессора
		        m_nSoundChipModel[AY_NUMS], // текущий тип модели муз. сопроцессора
		        m_n2AYWorkMode,         // режим работы двух AY
		        m_nScreenW;             // размеры экрана: ширина
		double  m_dAspectRatio;         //                 пропорции
		bool    m_bUseLongBinFormat,    // использовать длинный формат BIN при сохранении .bin файлов
		        m_bOrigScreenshotSize,  // сохранять скриншоты в своём оригинальном размере
		        m_bBigButtons,          // большие иконки Панели инструментов
		        m_bExclusiveOpenImages, // открывать образы монопольно/расшаренно
		        m_bNativeRusLatSwitch,  // способ эмуляции переключения РУС/ЛАТ
		        m_nDateInsteadOfScreenshotNumber;   // Дата и Время, вместо номера скриншота

		DumperParam_t m_arDumper[NUMBER_VIEWS_MEM_DUMP]; // адрес начала дампа в окне дампа памяти
		uint16_t m_nDisasmAddr;         // адрес начала дизассемблирования в окне дизассемблера

		CONF_SCREEN_RENDER m_nScreenRenderType;    // текущий тип рендера экрана
		CONF_OSCILLOSCOPE_RENDER m_nOscRenderType; // текущий тип рендера для осциллографа
		CString m_strFFMPEGLine;        // строка параметров командной строки для FFMPEG
		// Опции
		bool    m_bSavesDefault,        // исп. директорию для записи по умолчанию
		        m_bSpeaker,             // включить спикер
		        m_bSpeakerFilter,       // включит фильтр спикера
		        m_bSpeakerDCOffset,     // включить выравнивание смещения постоянного тока
		        m_bCovox,               // включить ковокс
		        m_bCovoxFilter,         // включить фильтр ковокса
		        m_bCovoxDCOffset,       // включить выравнивание смещения постоянного тока
		        m_bStereoCovox,         // задать стерео ковокс, иначе - моно
		        m_bMenestrel,           // включить Менестрель
		        m_bMenestrelFilter,     // включить фильтр Менестреля
		        m_bMenestrelDCOffset,   // включить выравнивание смещения постоянного тока
		        m_bAY8910,              // включить AY-сопр
		        m_b2ndAY8910,           // включить второй AY-сопр
		        m_bAY8910Filter,        // включить фильтр AY-сопра
		        m_bAY8910DCOffset,      // включить выравнивание смещения постоянного тока
		        m_bBKKeyboard,          // эмуляция БКшного поведения клавиатуры, иначе - как на ПК
		        m_bJoystick,            // включить эмуляцию джойстика
		        m_bICLBlock,            // включить эмуляцию блока нагрузок
		        m_bMouseMars,           // включить эмуляцию мыши "Марсианка"

		        m_bSmoothing,           // включить сглаживание экрана
		        m_bColorMode,           // включить цветной режим, иначе - чёрно-белый
		        m_bAdaptBWMode,         // включить адаптивный чёрно-белый, иначе - обычный чёрно-белый
		        m_bLuminoforeEmulMode,  // включить эмуляцию затухания люминофора
		        m_bFullscreenMode,      // включить полноэкранный режим

		        m_bPauseCPUAfterStart,  // не запускать конфигурацию автоматически
		        m_bAskForBreak,         /* при чтении/записи из несуществующего адреса не прерываться.
                                    (полезно для отладки и не полезно для работы эмулятора)
                                    */
		        m_bEmulateLoadTape,     // включить эмуляцию чтения с магнитофона
		        m_bEmulateSaveTape,     // включить эмуляцию записи на магнитофон
		        m_bTapeAutoBeginDetection, // включить автоопределение начала файла, записываемого на магнитофон, только для стандартной последовательности.
		        m_bTapeAutoEndDetection,   // включить автоопределение конца файла, записываемого на магнитофон, только для стандартной последовательности.

		        m_bShowPerformance,     // показывать информацию в строке состояния
		        m_bEmulateFDDIO,        // включить эмуляцию дискового обмена, иначе - полная эмуляция работы дисковода
		        m_bEmulateCBug,         // эмулировать баг бита С
		        m_bEmulate177702,       // эмулировать поведение регистра 177702
		        m_bVM1G,                // эмулировать процессора 1801ВМ1Г
		        m_bEmulateEIS,          // эмулировать набор инструкций EIS
		        m_bEmulateFIS;          // эмулировать набор инструкций FIS


		BOOL    m_bMMG,                 // инструкции диспетчера памяти !Тип BOOL нужен потому, что они напрямую участвуют в DDX!!!
		        m_bEIS,                 // EIS
		        m_bFIS,                 // FIS
		        m_bFPU;                 // FPU

		int     m_nVKBDType;            // вид клавиатуры, отображаемой в окне виртуальной клавиатуры

		COLORREF m_clrText;             // цвет текста, в разных стилях
		CString m_strPaletteFileName;   // имя файла палитр.

		// Приводы
		fs::path m_strFDDrives[static_cast<int>(FDD_DRIVE::NUM_FDD)];
		fs::path m_strHDDrives[static_cast<int>(HDD_MODE::NUM_HDD)];

		// Массивы параметров джойстика
		static const JoyElem_t m_arJoystick_std[BKJOY_PARAMLEN];
		JoyElem_t m_arJoystick[BKJOY_PARAMLEN];

		// параметры громкости и панорамирования AY

		// структура для приёма/передачи параметров
		struct AYVolPan_s
		{
			int nPL[AY_CHANS];      // значения панорамирования, число 0..100 включительно левый канал
			int nPR[AY_CHANS];      // значения панорамирования, число 0..100 включительно правый канал
			double V[AY_CHANS]; // значения громкости
		};

		AYVolPan_s  m_AYParam[AY_NUMS];

		AYVolPan_s      getVolPan(int n) const;
		void            setVolPan(int n, AYVolPan_s &s);
		void            initVolPan();

// остальные параметры, которые желательно должны быть доступны глобально.
#ifdef TARGET_WINXP
		OSVERSIONINFOEX m_osvi; // структура с версией винды
#endif
		CONF_BKMODEL    m_BKConfigModelNumber; // номер текущей конфигурации
		MSF_CONF        m_BKBoardModel;     // тип эмулируемой модели БК (для конструктора конфигураций)
		BK_DEV_MPI      m_BKFDDModel;       // переменная-посредник, сюда помещается номер модели дисковода перед созданием
		// конфигурации и используется во время создания конфигурации, в остальное время - не нужно.
		bool			m_bSysCapsStatus;	// состояние кнопки Капслок при запуске эмулятора, вне эмулятора.
	protected:
		static confStringParam m_romModules[];
		static confStringParam m_Directories[];

		CIni            iniFile; // наш распарсенный ини файл
		CString         m_strBKBoardType;
		fs::path        m_strCurrentPath; // путь к проге
		fs::path        m_strIniFileName; // полное имя ини файла, с путём

		void            DefaultConfig();
		void            _intLoadConfig(bool bLoadMain);
		void            MakeDefaultPalettes();
		void            SavePalettes(CString &strCustomize);
		void            LoadPalettes(CString &strCustomize);
		void            MakeDefaultJoyParam();
		void            SaveJoyParams(CString &strCustomize);
		void            LoadJoyParams(CString &strCustomize);
		void            SaveAYVolPanParams(CString &strCustomize);
		void            LoadAYVolPanParams(CString &strCustomize);
		void            CheckParV(CString &strCustomize, int Sect, double &v);
		void            CheckParP(CString &strCustomize, int Sect, int def, int &v);
		void            MakeDefaultAYVolPanParam();

		fs::path        GetDriveImgName_Full(const fs::path &str);
		fs::path        GetDriveImgName_Short(const fs::path &str);

	public:
		CConfig();
		virtual ~CConfig();
		bool            InitConfig(const CString &strIniName);
		void            UnInitConfig();

#ifdef TARGET_WINXP
		inline BOOL     IsWindowsVistaOrGreater()
		{
			return m_osvi.dwMajorVersion >= 6;
		}
#endif

		inline CIni    *GetIniObj()
		{
			return &iniFile;
		}

		int             GetDriveNum(const FDD_DRIVE eDrive);
		int             GetDriveNum(const HDD_MODE eDrive);
		fs::path        CheckDriveImgName(fs::path &str);
		fs::path        GetDriveImgName(const FDD_DRIVE eDrive);
		fs::path        GetDriveImgName(const HDD_MODE eDrive);
		fs::path        GetShortDriveImgName(const FDD_DRIVE eDrive);
		fs::path        GetShortDriveImgName(const HDD_MODE eDrive);
		fs::path        GetShortDriveImgName(const fs::path &strPathName);
		void            SetDriveImgName(const FDD_DRIVE eDrive, const fs::path &strPathName);
		void            SetDriveImgName(const HDD_MODE eDrive, const fs::path &strPathName);
		const fs::path  &GetConfCurrPath();
		void            LoadConfig(bool bLoadMain = true);
		void            LoadConfig_FromMemory(uint8_t *pBuff = nullptr, UINT nSize = 0);
		void            SaveConfig();
		ULONGLONG       SaveConfig_ToMemory(uint8_t *pBuff = nullptr, UINT nSize = 0);
		bool            VerifyRoms();
		void            SetBKBoardType(const CString strBKBoardType);
		void            SetBKModel(const CONF_BKMODEL n);
		CONF_BKMODEL    GetBKModel();
		const CString   GetBKConfName();
		const CString   GetRomModuleName(int nIniKey);

		bool            isBK10();
		bool            isBK11();
		bool            isBK11M();

		void            CheckRenders();
		void            CheckSSR();
		void            CheckSndChipModel();
		void            CheckSndChipFreq();
};

extern CConfig g_Config;

namespace Global
{
	CString     WordToOctString(uint16_t word);
	CString     ByteToOctString(uint8_t byte);
	void        WordToOctString(uint16_t word, CString &str);
	void        ByteToOctString(uint8_t byte, CString &str);
	uint16_t    OctStringToWord(const CString &str);
	CString     IntToString(int iInt, int radix = 10);
	CString     IntToFileLengthString(int iInt);
	CString     MsTimeToTimeString(int msTime);
	void        SetSafeName(CString &str);
	void        SetSafeDir(CString &str);

	CString     BKToUNICODE(uint8_t *pBuff, int size);
	TCHAR       BKToWIDEChar(uint8_t b);
	TCHAR       BKToSafeWIDEChar(uint8_t b);
	void        UNICODEtoBK(CString &ustr, uint8_t *pBuff, int bufSize, bool bFillBuf);
	uint8_t     WIDEtoBKChar(TCHAR ch);

	int         ToInt(const CString &str);
	long        ToLong(const CString &str);
	long long   ToLongLong(const CString &str);
	unsigned long ToULong(const CString &str);
	unsigned long long ToULongLong(const CString &str);

#ifdef _DEBUG
	void        GetLastErrorOut(LPTSTR lpszFunction);
#endif

	bool        CheckFFMPEG();

	bool        SaveBinFile(uint8_t *buf, uint16_t addr, uint16_t len, const fs::path &strName);
	bool        LoadBinFile(std::unique_ptr<uint8_t[]> &buf, uint16_t &addr, uint16_t &len, const fs::path &strName, bool bStrict);

	bool        isEmptyUnit(const CString &s);
	bool        isEmptyUnit(const fs::path &s);

	inline int GetDigit(uint16_t word, int pos)
	{
		return (pos ? (word >> ((pos << 1) + pos)) : word) & 7;
	}

	static CString getCompileDate(const LPCTSTR &p_format = _T("%Y-%m-%d"))
	{
		COleDateTime tCompileDate;
		tCompileDate.ParseDateTime(_T(__DATE__), LOCALE_NOUSEROVERRIDE, 1033);
		return tCompileDate.Format(p_format).GetString();
	}

	static CString getCompileTime(const LPCTSTR &p_format = _T("%H-%M-%S"))
	{
		COleDateTime tCompileDate;
		tCompileDate.ParseDateTime(_T(__TIME__), LOCALE_NOUSEROVERRIDE, 1033);
		return tCompileDate.Format(p_format).GetString();
	}

	void    SetMonospaceFont(HWND hwnd, CFont *pFont);
}

// #define WM_DBG_BREAKPOINT             (WM_USER + 100)
#define WM_DBG_CURRENT_ADDRESS_CHANGE   (WM_USER + 101)
#define WM_DBG_TOP_ADDRESS_UPDATE       (WM_USER + 102)
#define WM_DBG_TOP_ADDRESS_SET          (WM_USER + 103)
//
#define WM_START_PLATFORM               (WM_USER + 108)
#define WM_SETCPUFREQ                   (WM_USER + 110)
#define WM_RECEIVE_CMD_STRING           (WM_USER + 111)
#define WM_RESET_KBD_MANAGER            (WM_USER + 112)
#define WM_CPU_DEBUGBREAK               (WM_USER + 113)
#define WM_MEMDUMP_NEED_UPDATE          (WM_USER + 114)
// Карта памяти
#define WM_MEMMAP_CLOSE                 (WM_USER + 115)
// Droptarget
#define WM_DROP_TARGET                  (WM_USER + 116)
// TapeManagerDlg
#define WM_BUFFER_READY                 (WM_USER + 117)
#define WM_INFO_READY                   (WM_USER + 118)
// Виртуальная клавиатура
#define WM_VKBD_DOWN                    (WM_USER + 119)
#define WM_VKBD_UP                      (WM_USER + 120)
#define WM_VKBD_DN_CALLBACK             (WM_USER + 121)
#define WM_VKBD_UP_CALLBACK             (WM_USER + 122)
#define WM_OUTKBDSTATUS                 (WM_USER + 123)
// Экран
#define WM_SCREENSIZE_CHANGED           (WM_USER + 124)
#define WM_OSC_DRAW                     (WM_USER + 125)
#define WM_OSC_SETBUFFER                (WM_USER + 126)
#define WM_OSC_FILLBUFFER               (WM_USER + 127)
#define WM_SCR_DRAW                     (WM_USER + 128)
#define WM_SCR_DEBUGDRAW                (WM_USER + 129)
#define WM_SETT_SENDTOTAB               (WM_USER + 130)


// Interrups
constexpr uint16_t NO_INTERRUPT  = 0000;
constexpr uint16_t INTERRUPT_4   = 0004;
constexpr uint16_t INTERRUPT_10  = 0010;
constexpr uint16_t INTERRUPT_14  = 0014;
constexpr uint16_t INTERRUPT_20  = 0020;
constexpr uint16_t INTERRUPT_24  = 0024;
constexpr uint16_t INTERRUPT_30  = 0030;
constexpr uint16_t INTERRUPT_34  = 0034;
constexpr uint16_t INTERRUPT_60  = 0060;
constexpr uint16_t INTERRUPT_100 = 0100;
constexpr uint16_t INTERRUPT_270 = 0270;
constexpr uint16_t INTERRUPT_274 = 0274;

// Commands

// No fields
constexpr auto PI_HALT       = 0000000;
constexpr auto PI_WAIT       = 0000001;
constexpr auto PI_RTI        = 0000002;
constexpr auto PI_BPT        = 0000003;
constexpr auto PI_IOT        = 0000004;
constexpr auto PI_RESET      = 0000005;
constexpr auto PI_RTT        = 0000006;
constexpr auto PI_MFPT       = 0000007;
constexpr auto PI_START10    = 0000010;
constexpr auto PI_START11    = 0000011;
constexpr auto PI_START12    = 0000012;
constexpr auto PI_START13    = 0000013;
constexpr auto PI_STEP14     = 0000014;
constexpr auto PI_STEP15     = 0000015;
constexpr auto PI_STEP16     = 0000016;
constexpr auto PI_STEP17     = 0000017;
constexpr auto PI_NOP        = 0000240;
constexpr auto PI_CLC        = 0000241;
constexpr auto PI_CLV        = 0000242;
constexpr auto PI_CLVC       = 0000243;
constexpr auto PI_CLZ        = 0000244;
constexpr auto PI_CLZC       = 0000245;
constexpr auto PI_CLZV       = 0000246;
constexpr auto PI_CLZVC      = 0000247;
constexpr auto PI_CLN        = 0000250;
constexpr auto PI_CLNC       = 0000251;
constexpr auto PI_CLNV       = 0000252;
constexpr auto PI_CLNVC      = 0000253;
constexpr auto PI_CLNZ       = 0000254;
constexpr auto PI_CLNZC      = 0000255;
constexpr auto PI_CLNZV      = 0000256;
constexpr auto PI_CCC        = 0000257;
constexpr auto PI_NOP260     = 0000260;
constexpr auto PI_SEC        = 0000261;
constexpr auto PI_SEV        = 0000262;
constexpr auto PI_SEVC       = 0000263;
constexpr auto PI_SEZ        = 0000264;
constexpr auto PI_SEZC       = 0000265;
constexpr auto PI_SEZV       = 0000266;
constexpr auto PI_SEZVC      = 0000267;
constexpr auto PI_SEN        = 0000270;
constexpr auto PI_SENC       = 0000271;
constexpr auto PI_SENV       = 0000272;
constexpr auto PI_SENVC      = 0000273;
constexpr auto PI_SENZ       = 0000274;
constexpr auto PI_SENZC      = 0000275;
constexpr auto PI_SENZV      = 0000276;
constexpr auto PI_SCC        = 0000277;

// One field
constexpr auto PI_RTS        = 0000200;
constexpr auto PI_FADD       = 0075000;
constexpr auto PI_FSUB       = 0075010;
constexpr auto PI_FMUL       = 0075020;
constexpr auto PI_FDIV       = 0075030;

// Two fields
constexpr auto PI_JMP        = 0000100;
constexpr auto PI_SWAB       = 0000300;
constexpr auto PI_CLR        = 0005000;
constexpr auto PI_COM        = 0005100;
constexpr auto PI_INC        = 0005200;
constexpr auto PI_DEC        = 0005300;
constexpr auto PI_NEG        = 0005400;
constexpr auto PI_ADC        = 0005500;
constexpr auto PI_SBC        = 0005600;
constexpr auto PI_TST        = 0005700;
constexpr auto PI_ROR        = 0006000;
constexpr auto PI_ROL        = 0006100;
constexpr auto PI_ASR        = 0006200;
constexpr auto PI_ASL        = 0006300;
constexpr auto PI_MARK       = 0006400;
constexpr auto PI_MFPI       = 0006500;
constexpr auto PI_MTPI       = 0006600;
constexpr auto PI_SXT        = 0006700;
constexpr auto PI_MTPS       = 0106400;
constexpr auto PI_MFPD       = 0106500;
constexpr auto PI_MTPD       = 0106600;
constexpr auto PI_MFPS       = 0106700;
// Branches & interrupts
constexpr auto PI_BR         = 0000400;
constexpr auto PI_BNE        = 0001000;
constexpr auto PI_BEQ        = 0001400;
constexpr auto PI_BGE        = 0002000;
constexpr auto PI_BLT        = 0002400;
constexpr auto PI_BGT        = 0003000;
constexpr auto PI_BLE        = 0003400;
constexpr auto PI_BPL        = 0100000;
constexpr auto PI_BMI        = 0100400;
constexpr auto PI_BHI        = 0101000;
constexpr auto PI_BLOS       = 0101400;
constexpr auto PI_BVC        = 0102000;
constexpr auto PI_BVS        = 0102400;
constexpr auto PI_BHIS       = 0103000;
constexpr auto PI_BLO        = 0103400;

constexpr auto PI_EMT        = 0104000;
constexpr auto PI_TRAP       = 0104400;

// Three fields
constexpr auto PI_JSR        = 0004000;
constexpr auto PI_XOR        = 0074000;
constexpr auto PI_SOB        = 0077000;
constexpr auto PI_MUL        = 0070000;
constexpr auto PI_DIV        = 0071000;
constexpr auto PI_ASH        = 0072000;
constexpr auto PI_ASHC       = 0073000;

// Four fields
constexpr auto PI_MOV        = 0010000;
constexpr auto PI_CMP        = 0020000;
constexpr auto PI_BIT        = 0030000;
constexpr auto PI_BIC        = 0040000;
constexpr auto PI_BIS        = 0050000;
constexpr auto PI_ADD        = 0060000;
constexpr auto PI_SUB        = 0160000;

// BK Key codes
constexpr auto BKKEY_SHAG    = 0000000;
constexpr auto BKKEY_POVT    = 0000001;
constexpr auto BKKEY_INDSU   = 0000002;
constexpr auto BKKEY_KT      = 0000003;
constexpr auto BKKEY_BLOKRED = 0000004;
constexpr auto BKKEY_GRAF    = 0000005;
constexpr auto BKKEY_ZAP     = 0000006;
constexpr auto BKKEY_STIR    = 0000007;
constexpr auto BKKEY_L_ARROW = 0000010;
constexpr auto BKKEY_TAB     = 0000011;
constexpr auto BKKEY_ENTER   = 0000012;
constexpr auto BKKEY_RGTDEL  = 0000013;
constexpr auto BKKEY_SBR     = 0000014;
constexpr auto BKKEY_USTTAB  = 0000015;
constexpr auto BKKEY_RUS     = 0000016;
constexpr auto BKKEY_LAT     = 0000017;
constexpr auto BKKEY_SBRTAB  = 0000020;
constexpr auto BKKEY_VS      = 0000023;
constexpr auto BKKEY_GT      = 0000024;
constexpr auto BKKEY_SDVIG   = 0000026;
constexpr auto BKKEY_RAZDVIG = 0000027;
constexpr auto BKKEY_ZAB     = 0000030;
constexpr auto BKKEY_R_ARROW = 0000031;
constexpr auto BKKEY_U_ARROW = 0000032;
constexpr auto BKKEY_D_ARROW = 0000033;
constexpr auto BKKEY_PROBEL  = 0000040;

// флаги сохранения настроек
constexpr DWORD NO_CHANGES = 0;
constexpr DWORD CHANGES_NOREBOOT = 1;
constexpr DWORD CHANGES_NEEDREBOOT = 0x100;

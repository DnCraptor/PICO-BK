/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */


// FDDController.h: interface for the CFDDController class.
//


#pragma once

#include <stdint.h>
typedef unsigned int    UINT;     /* int must be 16-bit or 32-bit */
typedef int64_t         LONGLONG;

#define LOBYTE(w)           ((char)(((uint16_t)(w)) & 0xff))
#define HIBYTE(w)           ((char)((((uint16_t)(w)) >> 8) & 0xff))

//#include "Device.h"
//#include "HDD.h"
//#include "Config.h"
//#include "FloppyDrive.h"
#include <mutex>

constexpr auto A16M_ROM_10 = 16;
constexpr auto A16M_A0_10 = 18;
constexpr auto A16M_A2_10 = 20;

constexpr auto A16M_ROM_11M = 50;
constexpr auto A16M_A0_11M = 52;
constexpr auto A16M_A2_11M = 54;

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

/////////////////////////////////////////////////////////////////////////////
// CFloppy

constexpr auto FLOPPY_CMD_SELECT_DRIVE_A    =  01   ;   // выбор накопителя 0
constexpr auto FLOPPY_CMD_SELECT_DRIVE_B    =  02   ;   // выбор накопителя 1
constexpr auto FLOPPY_CMD_SELECT_DRIVE_C    =  04   ;   // выбор накопителя 2
constexpr auto FLOPPY_CMD_SELECT_DRIVE_D    =  010  ;   // выбор накопителя 3
constexpr auto FLOPPY_CMD_ENGINESTART       =  020  ;   // включение электродвигателя
constexpr auto FLOPPY_CMD_SIDEUP            =  040  ;   // выбор стороны, 0 - верхняя
constexpr auto FLOPPY_CMD_DIR               =  0100 ;   // направление шага
constexpr auto FLOPPY_CMD_STEP              =  0200 ;   // шаг
constexpr auto FLOPPY_CMD_READDATA          =  0400 ;   // признак "начало чтения"
constexpr auto FLOPPY_CMD_WRITEMARKER       =  01000;   // признак "запись маркера"
constexpr auto FLOPPY_CMD_PRECORRECTION     =  02000;   // включение схемы прекоррекции

// dir == 0 to center (towards trk0)
// dir == 1 from center (towards trk80)

constexpr auto FLOPPY_STATUS_TRACK0         = 01     ;  // признак "дорожка 0"
constexpr auto FLOPPY_STATUS_RDY            = 02     ;  // признак готовности накопителя
constexpr auto FLOPPY_STATUS_WRITEPROTECT   = 04     ;  // запрет записи
constexpr auto FLOPPY_STATUS_MOREDATA       = 0200   ;  // признак, что данные готовы или требование новых данных
constexpr auto FLOPPY_STATUS_CHECKSUMOK     = 040000 ;  // для чтения - признак записи ЦКС
constexpr auto FLOPPY_STATUS_INDEXMARK      = 0100000;  // датчик индексного отверстия

constexpr auto ALTPRO_A16M_START_MODE = 0160;
constexpr auto ALTPRO_A16M_STD10_MODE = 060;
constexpr auto ALTPRO_A16M_OZU10_MODE = 0120;
constexpr auto ALTPRO_A16M_BASIC_MODE = 020;
constexpr auto ALTPRO_A16M_STD11_MODE = 0140;
constexpr auto ALTPRO_A16M_OZU11_MODE = 040;
constexpr auto ALTPRO_A16M_OZUZZ_MODE = 0100;
constexpr auto ALTPRO_A16M_HLT11_MODE = 0;

constexpr auto ALTPRO_SMK_SYS_MODE = 0160;
constexpr auto ALTPRO_SMK_STD10_MODE = 060;
constexpr auto ALTPRO_SMK_OZU10_MODE = 0120;
constexpr auto ALTPRO_SMK_ALL_MODE = 020;
constexpr auto ALTPRO_SMK_STD11_MODE = 0140;
constexpr auto ALTPRO_SMK_OZU11_MODE = 040;
constexpr auto ALTPRO_SMK_HLT10_MODE = 0100;
constexpr auto ALTPRO_SMK_HLT11_MODE = 0;

constexpr auto ALTPRO_BEGIN_STROB_TRIGGER = 6;
constexpr auto ALTPRO_MODE_MASK = 0160;
constexpr auto ALTPRO_CODE_MASK = 02015;

extern "C" {
#include "CPU.h"
#include "CPU_i.h"
#include "CPU_ef.h"
extern TDevice_Data Device_Data;
}

enum class REGISTER : int {
			R0 = 0,
			R1,
			R2,
			R3,
			R4,
			R5,
			SP6,
			PC7,
			PSW8
};

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
enum class PSW_BIT : int {
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

class CMotherBoard {
	public:
        inline uint16_t GetRON(REGISTER r) {
			if (r == REGISTER::PSW8) return Device_Data.CPU_State.psw;
			return Device_Data.CPU_State.r [(int)r];
		}
		inline uint16_t GetWordIndirect(uint16_t addr) {
			return CPU_ReadMemW(addr);
		}
		inline uint16_t GetWord(uint16_t addr) {
			// TODO: diff ^ ?
			return CPU_ReadMemW(addr);
		}
		inline void SetPSWBit(PSW_BIT pos, bool val) {
			if (val)
                Device_Data.CPU_State.psw |= (1 << (int)pos);
			else
			    Device_Data.CPU_State.psw &= ~(1 << (int)pos);
		}
		inline void SetWordIndirect(const uint16_t addr, uint16_t value) {
            CPU_WriteW(addr, value); // TODO: err?
		}
		inline void SetWord(const uint16_t addr, uint16_t value) {
			// TODO: diff ^ ?
            CPU_WriteW(addr, value); // TODO: err?
		}
};

// флаги операций для функций обащения к системным регистрам
#define GSSR_NONE 0
#define GSSR_BYTEOP 1
#define GSSR_INTERNAL 2

enum class FDD_DRIVE : int
{
	NONE = -1, A = 0, B, C, D, NUM_FDD
};

class CDevice ///: public CObject
{
	protected:
		uint64_t           m_tickCount; // Device ticks
		CDevice            *m_pParent;

	public:
		CDevice(): m_pParent(nullptr), m_tickCount(0) {}
		virtual ~CDevice() = default;

		void AttachParent(CDevice *pParent)
		{
			m_pParent = pParent;
		};
		// Method for count device ticks
		void                Reset() {
            m_tickCount = 0;
	        OnReset();
        }
		virtual void        NextTick() { m_tickCount++; }

		// Virtual method called after reset command
		virtual void        OnReset() = 0;

		// Methods for Set/Get byte/word
		virtual void        GetByte(const uint16_t addr, uint8_t *pValue) = 0;
		virtual void        GetWord(const uint16_t addr, uint16_t *pValue) = 0;
		virtual void        SetByte(const uint16_t addr, uint8_t value) = 0;
		virtual void        SetWord(const uint16_t addr, uint16_t value) = 0;
};

constexpr auto FLOPPY_SIDES = 2;            // две стороны у дискеты
constexpr auto FLOPPY_TRACKS = 80;          // количество дорожек на стороне дискеты
constexpr auto FLOPPY_SECTOR_TYPE = 2;      // размер сектора в байтах 512
constexpr auto FLOPPY_TRACK_LEN = 10;       // количество секторов на дорожке
constexpr auto FLOPPY_TRKLEN_DD = 6250;
constexpr auto m_nRawTrackSize = FLOPPY_TRKLEN_DD; // размер сырой дорожки в байтах = m_nSectorsPerTrk * 650
constexpr auto m_nRawMarkerSize = m_nRawTrackSize / 2; // размер таблицы позиций маркера = m_nRawTrackSize / 2
constexpr auto FLOPPY_TRKLEN_HD = 17700;
constexpr auto FLOPPY_INDEXLENGTH = 32;

extern "C" {
	#include "ff.h"
}

class CFloppyDrive
{
		FIL         m_File;
		const char* m_strFileName;              // имя файла образа, который примонтирован в привод, чтобы повторно не перепримонтировывать
		bool        m_bReadOnly;                // Флаг защиты от записи
		bool        m_bTrackChanged;            // true = m_data было изменено - надо сохранить его в файл
		int         m_nDataTrack;               // Номер трека данных в массиве data
		int         m_nDataSide;                // Сторона диска данных в массиве data
		size_t      m_nDataPtr;                 // Смещение данных внутри data - позиция заголовка
		uint8_t     m_pData[m_nRawTrackSize];   // Raw track image for the current track
		uint8_t     m_pMarker[m_nRawMarkerSize];// Позиции маркеров

		int         m_nTracks;                  // количество дорожек у дискеты
		int         m_nSides;                   // количество сторон у дискеты
		int         m_nSectorsPerTrk;           // количество секторов на дорожке
		int         m_nSectorSize;              // номер размера сектора 0=128,1=256,2=512,3=1024
		size_t      m_nTrackSize;               // размер логической дорожки в байтах = m_nSectorsPerTrk * m_nSectorSizes[m_nSectorSize]

		bool        SetArrays();
		void        EncodeTrackData(uint8_t *pSrc);
		bool        DecodeTrackData(uint8_t *pDest) const;

		uint16_t    getCrc(const uint8_t *ptr, size_t len) const;

	public:
		CFloppyDrive() : m_bReadOnly(false)
                       , m_bTrackChanged(false)
                       , m_nDataTrack(-1)
                       , m_nDataSide(-1)
                       , m_nDataPtr(0)
                       , m_pData({0})
                       , m_pMarker({0})
	                   // геометрия дискеты
                       , m_nTracks(FLOPPY_TRACKS + 3)          // количество дорожек
                       , m_nSides(FLOPPY_SIDES)                // количество сторон
                       , m_nSectorsPerTrk(FLOPPY_TRACK_LEN)    // количество секторов на дорожке
                       , m_nSectorSize(FLOPPY_SECTOR_TYPE)     // тип размера сектора
	    {
            SetArrays();
        }
		~CFloppyDrive() {}

		static const int m_nSectorSizes[4];     // массив размеров секторов
		void        Reset();

		inline bool isReadOnly() const
		{
			return m_bReadOnly;
		}

		inline bool isAttached() const
		{
			return true;// TODO: return (m_File.m_hFile != CFile::hFileNull);
		}

		bool        Attach(const char* strFile, bool bExclusive);
		void        Detach();
		void        SetGeometry(int nTracks, int nSides, int nSectPerTrk, int nSectSz);
		void        PrepareTrack(int nTrack, int nSide);
		void        FlushChanges();  // Если текущая дорожка была изменена, сохраним её

		// функции для работы с эмуляцией дисковых операций

		LONGLONG    FDSeek(LONGLONG pos, UINT from);    // позиционирование в файле образа
		UINT        FDRead(void *lpBuf, UINT nCount);   // чтение данных из образа
		bool        FDWrite(void *lpBuf, UINT nCount);  // запись данных в образ

		// функции для работы с эмуляцией вращения

		void        MovePtr();          // сдвиг указателя на следующее слово
		bool        isIndex() const     // проверка, находится ли указатель в зоне индексного отверстия
		{
			return (m_nDataPtr < FLOPPY_INDEXLENGTH);
		}
		void        WrData(uint16_t w); // запись данных на текущую позицию указателя
		uint16_t    RdData() const;     // чтение данных по текущей позиции указателя
		void        setMarker(bool m)   // задание позиции маркера
		{
			m_pMarker[m_nDataPtr / 2] = m;
		}
		bool        getMarker() const   // получение позиции маркера
		{
			return m_pMarker[m_nDataPtr / 2];
		}
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

#pragma pack(push)
#pragma pack(1)
// это дело сохраняется в файле состояния, поэтому, для определённости
// типы переменных должны быть строго определёнными
struct BKMEMBank_t
{
	bool bReadable; // флаг, что память доступна для чтения
	bool bWritable; // флаг, что память доступна для записи
	uint32_t nBank; // номер банка памяти 4kb
	uint32_t nPage; // страница памяти БК11 == nBank >> 2 (Этот параметр можно удалить, когда будут сделаны какие-нибудь серьёзные
	// изменения в msf структуре)
	uint32_t nOffset; // смещение в массиве == nBank << 12
	uint32_t nTimingCorrection; // значение корректировки тайминга при обращении к памяти, которая не управляется ВП1-037 (ПЗУ или ОЗУ СМК)
	BKMEMBank_t(): bReadable(false), bWritable(false), nBank(0), nPage(0), nOffset(0), nTimingCorrection(0) {};
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

enum class HDD_REGISTER : int
{
	H1F0 = 0,
	H1F1,
	H1F2,
	H1F3,
	H1F4,
	H1F5,
	H1F6,
	H1F7,
	H3F6,
	H3F7
};

class CFDDController : public CDevice {
		enum {
			FDD_NOERROR = 0,
			FDD_DISK_PROTECTED,         // 1
			FDD_SECT_HEADER_ERROR,      // 2
			FDD_0_TRACK_ERROR,          // 3
			FDD_POSITIONING_ERROR,      // 4
			FDD_SECTOR_NOT_FOUND,       // 5
			FDD_NO_DISK,                // 6
			FDD_STOP,                   // 7
			FDD_ADDR_MARKER_NOT_FOUND,  // 010
			FDD_DATA_MARKER_NOT_FOUND,  // 011
			FDD_BAD_FORMAT              // 012
		};
#pragma pack(push)
#pragma pack(1)
		struct TABLE_EMFDD {
			uint16_t    CSRW;               // 00 копия по записи регистра состояния КНГМД
			uint16_t    CURTRK;             // 02 адрес текущей дорожки (адрес одного из последующих байтов)
			uint8_t     TRKTAB[4];          // 04 таблица текущих дорожек
			uint16_t    TDOWN;              // 10 задержка опускания головки
			uint16_t    TSTEP;              // 12 задержка перехода с дорожки на дорожку
			uint8_t     TRKCOR;             // 14 дорожка начала предкомпенсации
			uint8_t     BRETRY;             // 15 число попыток повтора при ошибке
			uint8_t     FLAGS;              // 16 рабочая ячейка драйвера
			uint8_t     FILLB;              // 17 код заполнения при форматировании
			uint16_t    FLGPTR;             // 20 указатель на байт признаков (один из следующих байтов)
			uint8_t     FLGTAB[4];          // 22 байты признаков
			uint16_t    ADDR;               // 26 адрес начала массива данных в ОЗУ (обязательно чётный)
			int16_t     WCNT;               // 30 количество слов для пересылки
			uint8_t     SIDE;               // 32 номер стороны диска
			uint8_t     TRK;                // 33 номер дорожки
			uint8_t     UNIT;               // 34 номер привода
			uint8_t     SECTOR;             // 35 номер сектора
			uint16_t    WRTVAR;             // 36 значение, записываемое при форматировании
			uint16_t    MARKER;             // 40 буфер маркера при записи
			uint16_t    FREE;               // 42 длина пустого остатка сектора
			uint16_t    INTIME;             // 44 счётчик длительности индекса
			uint16_t    BUF4;               // 46 буфер для сохранения вектора 4
			uint16_t    BUFSP;              // 50 буфер для сохранения SP
			uint16_t    BUFPSW;             // 52 буфер для сохранения PSW
			uint8_t     CRETRY;             // 54 счётчик повторов при ошибке
			uint8_t     TURNS;              // 55 число оборотов диска при поиске сектора
			uint8_t     SECRET;             // 56 число повторных попыток поиска сектора
			uint8_t     ERRNUM;             // 57 буфер для номера ошибки
			uint16_t    MAXSEC;             // 60 число секторов на дорожке
			uint16_t    HOLTIN;             // 62 время задержки после индекса до первого сектора
			uint16_t    SECLEN;             // 64 длина сектора в словах (следить за значением при записи, иначе портится разметка диска)
		};
#pragma pack(pop)
	protected:
		bool            m_bFloppyIsInited;  // костыль от повторной инициализации
		CFloppyDrive*   m_pFloppy[4/*NUM_FDD*/];
		FDD_DRIVE       m_drive;       // Номер привода: от 0 до 3; -1 если не выбран
		CFloppyDrive   *m_pDrive;      // Текущий привод; nullptr если не выбран
		int             m_track;       // Номер дорожки: от 0 до 79 (в реальных дисководах возможно до 81, в особых случаях даже до 83)
		int             m_side;        // Сторона диска: 0 или 1
		uint16_t        m_status;      // См. определения FLOPPY_STATUS_XXX (флаги, принимаемые от контроллера дисковода)
		uint16_t        m_flags;       // См. определения FLOPPY_CMD_XXX (флаги, передаваемые контроллеру дисковода)
		uint16_t        m_datareg;     // Регистр данных режима чтения
		uint16_t        m_writereg;    // Регистр данных режима записи
		bool            m_writeflag;   // В регистре данных режима записи есть данные
		bool            m_writemarker; // Запись маркера в m_marker
		uint16_t        m_shiftreg;    // Регистр сдвига режима записи
		bool            m_shiftflag;   // В регистре сдвига режима записи есть данные
		bool            m_shiftmarker; // Запись маркера в m_marker
		bool            m_bWriteMode;  // true = режим записи, false = режим чтения
		bool            m_bSearchSync; // Подрежим чтения: true = поиск синхро последовательности, false = просто чтение
		bool            m_bSearchSyncTrigger; // триггер для отлова перехода из 1 в 0 бита FLOPPY_CMD_READDATA
		bool            m_bCRCCalc;     // true = CRC в процессе подсчёта
		int             m_nCRCAcc;      // аккумулятор CRC
		bool            m_bCrcOK;       // CRC при чтении совпала
		int             m_nA1Cnt;       // анализ маркера, кол-во идущих подряд A1
		int             m_nCrcCnt;      // счётчик байтов данных, для которых делается подсчёт, если <= 0, то перестать считать
		int             m_nCrcSecSz;    // тип размера сектора, берётся из заголовка сектора
		bool            m_bHdrMk;       // флаг, что найден маркер заголовка сектора, это как раз чтобы взять тип размера сектора
		bool            m_bCrcMarker;   // флаг, что идёт анализ маркера
		void            ini_crc_16_rd();
		void            add_crc_16(uint8_t val);
		void            add_crc_16_rd(uint8_t val);
		BK_DEV_MPI      m_FDDModel;
		bool            m_bA16M_Trigger;
		uint16_t        m_nAltproMode; // код режима работы контроллера Альтпро
		uint16_t        m_nAltproMemBank; // номер банка памяти контроллера Альтпро
		//CATA_IDE        m_ATA_IDE;
		//mutex      m_mutPeriodicBusy;
	public:
		CFDDController();
		virtual ~CFDDController() override;
		// Virtual method called after reset command
		virtual void        OnReset() override;
		void                InitVariables();
		// Methods for Set/Get byte/word
		virtual void        GetByte(const uint16_t addr, uint8_t *pValue) override;
		virtual void        GetWord(const uint16_t addr, uint16_t *pValue) override;
		virtual void        SetByte(const uint16_t addr, uint8_t value) override;
		virtual void        SetWord(const uint16_t addr, uint16_t value) override;
		bool                AttachImage(FDD_DRIVE eDrive, const char* strFileName);
		void                DetachImage(FDD_DRIVE eDrive);
		void                AttachDrives();
		void                DetachDrives();
		void                EmulateFDD(CMotherBoard *pBoard);
		bool                IsAttached(FDD_DRIVE eDrive);
		bool                IsReadOnly(FDD_DRIVE eDrive);
		bool                GetDriveState(FDD_DRIVE eDrive);
		void                init_A16M_10(ConfBKModel_t *mmodl, const int v);
		void                init_A16M_11M(ConfBKModel_t *mmodl, const int v);
		bool                Change_AltPro_Mode(ConfBKModel_t *mmodl, const uint16_t w);
		void                A16M_MemManager_10(BKMEMBank_t *mmap, ConfBKModel_t *mmodl);
		void                A16M_MemManager_11M(BKMEMBank_t *mmap, ConfBKModel_t *mmodl);
		void                SMK512_MemManager_10(BKMEMBank_t *mmap, ConfBKModel_t *mmodl);
		void                SMK512_MemManager_11M(BKMEMBank_t *mmap, ConfBKModel_t *mmodl);
		void                SetFDDType(BK_DEV_MPI model);
		BK_DEV_MPI          GetFDDType();
		bool                WriteHDDRegisters(uint16_t num, uint16_t data);     // запись в регистры HDD
		bool                ReadHDDRegisters(uint16_t num, uint16_t &data);     // чтение из регистров HDD
		bool                ReadHDDRegistersInternal(uint16_t num, uint16_t &data);     // отладочное чтение из регистров HDD
		uint16_t            ReadDebugHDDRegisters(int nDrive, HDD_REGISTER num, bool bReadMode);   // отладочное чтение из регистров HDD
		bool                IsEngineOn();
		uint16_t            GetData();                  // Чтение порта 177132 - данные
		uint16_t            GetState();                 // Чтение порта 177130 - состояние устройства
		void                WriteData(uint16_t Data);   // Запись в порт 177132 - данные
		void                SetCommand(uint16_t cmd);   // Запись в порт 177130 - команды
		uint16_t            GetDataDebug();             // Получить значение порта 177132 - данные для отладки
		uint16_t            GetStateDebug();            // Получить значение порта 177130 - состояние устройства для отладки
		uint16_t            GetWriteDataDebug();        // Получить значение порта 177132 - переданные данные для отладки
		uint16_t            GetCmdDebug();              // Получить значение порта 177130 - переданные команды для отладки
		void                Periodic(); // Вращение диска; вызывается каждые 64 мкс - 15625 раз в секунду. Вызывается из основного цикла
	private:
		inline void         A16M_SetMemBank(BKMEMBank_t *mmap, int nBnk, int BnkNum);
		inline void         A16M_SetMemSegment(BKMEMBank_t *mmap, int nBnk, int BnkNum);
		inline void         A16M_SetRomBank(BKMEMBank_t *mmap, int nBnk, int BnkNum);
};

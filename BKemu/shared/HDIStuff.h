#pragma once

#include <string>
#include <list>

#pragma pack(push)
#pragma pack(1)
struct SYS_SECTOR
{
	uint16_t    main_config;            // 0
	uint16_t    cylinders;              // 1
	uint16_t    reserved2;              // 2
	uint16_t    heads;                  // 3
	uint16_t    raw_bytes_per_track;    // 4
	uint16_t    raw_bytes_per_sector;   // 5
	uint16_t    sectors;                // 6
	uint16_t    reserved7[3];           // 7,8,9
	uint8_t     serial_number[20];      // 10
	uint16_t    buffer_type;            // 20
	uint16_t    buffer_size_in_sectors; // 21
	uint16_t    ecc_bytes_num;          // 22
	uint8_t     fw_version[8];          // 23
	uint8_t     model_name[40];         // 27
	uint16_t    word47;                 // 47
	uint16_t    word48;                 // 48
	uint16_t    capabilities1;          // 49
	uint16_t    capabilities2;          // 50
	uint16_t    reserved51[6];          // 51
	uint32_t    capacity_in_sectors;    // 57,58
	uint16_t    reserved59;             // 59
	uint32_t    total_used_sectors;     // 60,61
	uint16_t    reserved62[194];        // 62
};
#pragma pack(pop)

constexpr auto SECTOR_SIZEB = 512;
constexpr auto SECTOR_SIZEW = 256;

// это сектор. чтобы удобнее было делать одновременно байтовый и словный доступ
using USector = union _usector
{
	uint8_t  b[SECTOR_SIZEB];
	uint16_t w[SECTOR_SIZEW];
};

// структура параметров образа
enum class IMGIOSTATUS : int
{
	IO_ERROR = -1,
	IO_OK = 0
};
struct IMGFormat
{
	bool        bHDI;           // признак образа в формате HDI
	bool        bSamara;        // флаг формата самара
	int         nLogDiskNum;    // количество разделов
	uint16_t    C;              // геометрия: количество дорожек
	uint16_t    H;              // геометрия: количество головок
	uint16_t    S;              // геометрия: количество секторов на дорожке
	uint16_t    nCyl;           // текущий номер дорожки при формировании образа
	uint16_t    nHead;          // текущий номер головки при формировании образа
	uint16_t    nSector;        // текущий номер сектора при формировании образа
	IMGIOSTATUS nIOStatus;

	IMGFormat()
	{
		memset(this, 0, sizeof(IMGFormat));
	}
	IMGFormat(const IMGFormat *src)
	{
		memcpy(this, src, sizeof(IMGFormat));
	}
	IMGFormat &operator = (const IMGFormat &src)
	{
		memcpy(this, &src, sizeof(IMGFormat));
		return *this;
	}
};

// флаги признаки, совпадают с принятыми в Самарском формате
enum ALD_ATTR : uint32_t
{
	NONE = 0,
	BAD = 1,    // битые сектора
	WP  = 2,    // защ от записи
	RP  = 4,    // защ от чтения
	FWP = 8,    // полн.защ от записи
	FRP = 16    // полн.защ от чтения
};


// описание логического диска
struct AbstractLD
{
	// координаты без учёта HDI - заголовка
	uint16_t Cb;        // начало раздела: номер дорожки
	uint16_t Hb;        // начало раздела: номер головки
	uint16_t Sb;        // начало раздела: номер сектора (для АльтПро всегда 1)
	uint32_t lba;       // начало раздела: номер логического блока
	uint16_t Ce;        // конец раздела: номер дорожки
	uint16_t He;        // конец раздела: номер головки
	uint16_t Se;        // конец раздела: номер сектора
	uint32_t len;       // размер раздела в блоках
	uint32_t attr;      // атрибуты
	uint8_t  num;       // номер лог. диска (0 - А, 2 - С ...)
	// специфические для самары параметры:
	uint16_t sam_ldr_addr; // адрес загрузки загрузчика лог. диска
	uint16_t sam_bpb_addr; // адрес блока параметров для загрузчика
	uint16_t sam_pages; // состояние регистра страниц

	AbstractLD()
	{
		memset(this, 0, sizeof(AbstractLD));
	}
	AbstractLD(const AbstractLD *src)
	{
		memcpy(this, src, sizeof(AbstractLD));
	}
	AbstractLD &operator = (const AbstractLD &src)
	{
		memcpy(this, &src, sizeof(AbstractLD));
		return *this;
	}
};


// константы для доступа к данным таблицы разделов АльтПро

// номер блока, где находится таблица разделов АльтПро
constexpr auto AHDD_PT_SEC = 7;
constexpr auto AHDD_PART_W = 251;
constexpr auto AHDD_LOGD_W = 252;
constexpr auto AHDD_SEC_W  = 253;
constexpr auto AHDD_HEAD_W = 254;
constexpr auto AHDD_CYL_W  = 255;

constexpr auto AHDD_PART_B = 0766;
constexpr auto AHDD_LOGD_B = 0770;
constexpr auto AHDD_UNI_B  = 0771;
constexpr auto AHDD_SEC_B  = 0772;
constexpr auto AHDD_HEAD_B = 0774;
constexpr auto AHDD_DRV_B  = 0775;
constexpr auto AHDD_CYL_B  = 0776;

// константы для доступа к данным таблицы разделов Самара

// номер блока, где находится таблица разделов Самара
constexpr auto SHDD_PT_SEC = 1;

constexpr auto SHDD_BOOT_W     = 0; // # устр. для загрузки по умолч. (0 - А, 2 - С ...)
constexpr auto SHDD_CYLVOL_W   = 1; // объём цилиндра (общее количество секторов на дорожке) == H * S
constexpr auto SHDD_HEAD_SEC_W = 2; // количество секторов на дорожке & номер последней головки (H - 1)
constexpr auto SHDD_PART_W     = 3; // таблица разделов

constexpr auto SHDD_BOOT_B     = 0; // # устр. для загрузки по умолч. (0 - А, 2 - С ...)
constexpr auto SHDD_CYLVOL_B   = 2; // объём цилиндра (общее количество секторов на дорожке) == H * S
constexpr auto SHDD_SEC_B      = 4; // количество секторов на дорожке
constexpr auto SHDD_HEAD_B     = 5; // номер последней головки (H - 1)
constexpr auto SHDD_PART_B     = 6; // таблица разделов

// константы для доступа к данным начального блока раздела Самара
constexpr auto SHDD_NLD_W      = 0; // номер лог. диска
constexpr auto SHDD_LD_LEN_W   = 1; // размер лог. диска в блоках
constexpr auto SHDD_LD_FLAGS_W = 2; // флаги - признаки
constexpr auto SHDD_ADR_BOOT_W = 3; // адрес загрузки загрузчика лог. диска
constexpr auto SHHD_ADR_PAR_W  = 4; // адрес блока параметров для загрузчика
constexpr auto SHDD_PAGE_W     = 5; // состояние регистра страниц



namespace HDIStuff
{
	void    CopyString2Sys(uint8_t *dst, const std::wstring &str, const size_t max_len);
	void    CreateHDISector(IMGFormat *imgf, SYS_SECTOR *bufSector, const std::wstring &sn, const std::wstring &mn);
	std::wstring GenerateSerialNumber();
	void    InverseSector(USector &sector) noexcept;
	bool    CheckCS(const USector *sector) noexcept;

	bool    CheckAltPro(FILE *f, IMGFormat *imgf);
	bool    CheckSamara(FILE *f, IMGFormat *imgf);
	bool    CheckFormat(FILE *f, IMGFormat *imgf);

	bool    getPartitionList(FILE *f, std::list<AbstractLD> *pLD);
}

#include "mkdos.h"
#include "stdint.h"
extern "C" {
	#include "ff.h"
}

#pragma pack(push)
#pragma pack(1)
struct MKDosFileRecord {
	uint8_t     status;     // статус файла, или номер директории для записи-директории
	uint8_t     dir_num;    // номер директории, которой принадлежит запись
	uint8_t     name[14];   // имя файла/каталога, если начинается с байта 0177 - это каталог.
	uint16_t    start_block;// начальный блок
	uint16_t    len_blk;    // длина в блоках
	uint16_t    address;    // стартовый адрес
	uint16_t    length;     // длина
	MKDosFileRecord() {
		memset(this, 0, sizeof(MKDosFileRecord));
	}
	MKDosFileRecord &operator = (const MKDosFileRecord &src) {
		memcpy(this, &src, sizeof(MKDosFileRecord));
		return *this;
	}
	MKDosFileRecord &operator = (const MKDosFileRecord *src) {
		memcpy(this, src, sizeof(MKDosFileRecord));
		return *this;
	}
};
#pragma pack(pop)

constexpr auto FMT_MKDOS_CAT_RECORD_NUMBER = 030;
constexpr auto FMT_MKDOS_TOTAL_FILES_USED_BLOCKS = 032;
constexpr auto FMT_MKDOS_DISK_SIZE = 0466;
constexpr auto FMT_MKDOS_FIRST_FILE_BLOCK = 0470;
constexpr auto FMT_MKDOS_CAT_BEGIN = 0500;
constexpr auto MKDOS_CAT_RECORD_SIZE = 173;

// хотя размер каталога в мкдосе почти 9 секторов, чуть-чуть до конца сектора не добивает.
static uint8_t m_pCatBuffer[20 * BLOCK_SIZE] = { 0 };
static MKDosFileRecord *m_pDiskCat = 0;
static unsigned int    m_nCatSize;         // размер буфера каталога, выровнено по секторам, включая служебную область
static unsigned int    m_nMKCatSize;       // размер каталога в записях
static unsigned int    m_nMKLastCatRecord; // индекс последней записи в каталоге, чтобы определять конец каталога
static bool            m_bMakeAdd;         // флаг, реализованы ли функции добавления
static bool            m_bMakeDel;         // флаг, реализованы ли функции удаления
static bool            m_bMakeRename;      // флаг, реализованы ли функции переименования
static bool            m_bChangeAddr;      // флаг, разрешено ли менять адрес записей
static bool            m_bFileROMode;      // режим открытия образа, только для чтения == true. или для чтения/записи == false

void mkdos_review(const PARSE_RESULT_C& parse_result) {
    m_pDiskCat = reinterpret_cast<MKDosFileRecord *>(m_pCatBuffer + FMT_MKDOS_CAT_BEGIN); // каталог диска
	m_nMKCatSize = MKDOS_CAT_RECORD_SIZE;
	m_nMKLastCatRecord = m_nMKCatSize - 1;
	m_bMakeAdd = true;
	m_bMakeDel = true;
	m_bMakeRename = true;
	m_bChangeAddr = true;

	//m_sDiskCat.bHasDir = true;
	//m_sDiskCat.nMaxDirNum = 0177;

	FIL fil;
	if (f_open(&fil, parse_result.strName, FA_READ | FA_WRITE) == FR_OK) {
		m_bFileROMode = false;
	} else if (f_open(&fil, parse_result.strName, FA_READ) != FR_OK) {
        return;
	} else {
		m_bFileROMode = true;
	}
	// TODO:
    f_close(&fil);
}

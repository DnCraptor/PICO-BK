/**
 * Partly migration from https://gid.pdp-11.ru/bkemuc.html?custom=3.13.2401.10604
*/
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

struct DiskCatalog {
	//std::vector<BKDirDataItem> vecFC; // текущий каталог
	//std::vector<int>vecDir;     // вектор номеров вложенных каталогов.
	int         nCurrDirNum;    // номер текущего отображаемого каталога.
	bool        bHasDir;        // флаг, имеет ли файловая система директории
	bool        bTrueDir;       // флаг, что файловая система имеет настояшие директории, как в MS-DOS например
	uint8_t     nMaxDirNum;     // максимальный номер директории для заданной ОС
	uint8_t     arDirNums[256]; // список номеров директорий, найденных в каталоге. Нужно, чтобы создавать новые директории.
	// переменные для отображения информации о ФС диска
	int         nTotalRecs;     // всего записей в каталоге
	int         nTotalBlocks;   // общее количество блоков/кластеров (зависит от ФС)
	int         nFreeRecs;      // количество свободных записей в каталоге
	int         nFreeBlocks;    // количество свободных блоков/кластеров (зависит от ФС) на основе чего высчитывается кол-во свободных байтов
	int         nDataBegin;     // блок, с которого начинаются данные на диске (зависит от ФС)
	DiskCatalog() : nCurrDirNum(0), bHasDir(false), bTrueDir(false), nMaxDirNum(255), arDirNums{}, nTotalRecs(0), nTotalBlocks(0), nFreeRecs(0), nFreeBlocks(0), nDataBegin(0)
	{
		memset(&arDirNums[0], 0, 256);
	}
	void init()	{
		//vecFC.clear();
		//vecDir.clear();
		nCurrDirNum = 0;
		bHasDir = false;
		bTrueDir = false;
		nMaxDirNum = 255;
		memset(&arDirNums[0], 0, 256);
		nTotalRecs = 0;
		nTotalBlocks = 0;
		nFreeRecs = 0;
		nFreeBlocks = 0;
		nDataBegin = 0;
	}
};
static DiskCatalog m_sDiskCat;


constexpr auto SPECIFIC_DATA_BUFFER_LENGTH = 64; // размер буфера в байтах, где хранится оригинальная запись как есть.

class BKDirDataItem {
	public:
		enum class RECORD_TYPE : int        // тип записи каталога
		{
			UP = 0,                         // обозначение выхода из каталога
			DIRECTORY,                      // обозначение директории
			LINK,                           // обозначение ссылки (на другой диск)
			LOGDSK,                         // обозначение логического диска
			FILE                            // обозначение файла
		};
		char            strName[16];        // имя/название
		unsigned int    nAttr;              // атрибуты, ну там директория, защищённый, скрытый, удалённый, плохой
		RECORD_TYPE     nRecType;           // тип записи: 0="..", 1=директория, 2=ссылка, 3=логический диск, 4=простой файл, вводится специально для сортировки
		// поэтому частично дублирует атрибуты
		int             nDirBelong;         // номер каталога, к которому принадлежит файл для файла и директории
		int             nDirNum;            // номер директории для директории, 0 - для файла
		unsigned int    nAddress;           // адрес файла, для директории 0, если для директории не 0, то это ссылка на другой диск.
		unsigned int    nSize;              // размер файла
		int             nBlkSize;           // размер в блоках, или кластерах (для андос)
		unsigned int    nStartBlock;        // начальный сектор/блок/кластер
		bool            bSelected;          // вводим флаг, что запись в списке выделена.
	//	time_t          timeCreation;       // для ФС умеющих хранить дату создания файла
		// здесь будем хранить оригинальную запись о файле.
		uint32_t        nSpecificDataLength; // размер записи о файле. на БК они гарантированно меньше SPECIFIC_DATA_BUFFER_LENGTH байтов
		uint8_t         pSpecificData[SPECIFIC_DATA_BUFFER_LENGTH]; // буфер данных
		BKDirDataItem()	{
			clear();
		}
		~BKDirDataItem() = default;
		void clear() {
			memset(this->strName, 0, 16);
			this->nAttr = 0;
			this->nStartBlock = this->nBlkSize = this->nSize = this->nAddress = this->nDirNum = this->nDirBelong = 0;
			this->nRecType = RECORD_TYPE::UP;
			this->bSelected = false;
			this->nSpecificDataLength = 0;
		//	this->timeCreation = 0;
			memset(this->pSpecificData, 0, SPECIFIC_DATA_BUFFER_LENGTH);
		}
		BKDirDataItem(const BKDirDataItem *src)	{
			strncpy(this->strName, src->strName, sizeof this->strName);
			this->nAttr = src->nAttr;
			this->nRecType = src->nRecType;
			this->nDirBelong = src->nDirBelong;
			this->nDirNum = src->nDirNum;
			this->nAddress = src->nAddress;
			this->nSize = src->nSize;
			this->nBlkSize = src->nBlkSize;
			this->nStartBlock = src->nStartBlock;
			this->bSelected = src->bSelected;
			//this->timeCreation = src->timeCreation;
			this->nSpecificDataLength = src->nSpecificDataLength;
			memcpy(this->pSpecificData, src->pSpecificData, SPECIFIC_DATA_BUFFER_LENGTH);
		}
		BKDirDataItem &operator = (const BKDirDataItem &src) {
			strncpy(this->strName, src.strName, sizeof this->strName);
			this->nAttr = src.nAttr;
			this->nRecType = src.nRecType;
			this->nDirBelong = src.nDirBelong;
			this->nDirNum = src.nDirNum;
			this->nAddress = src.nAddress;
			this->nSize = src.nSize;
			this->nBlkSize = src.nBlkSize;
			this->nStartBlock = src.nStartBlock;
			this->bSelected = src.bSelected;
		//	this->timeCreation = src.timeCreation;
			this->nSpecificDataLength = src.nSpecificDataLength;
			memcpy(this->pSpecificData, src.pSpecificData, SPECIFIC_DATA_BUFFER_LENGTH);
			return *this;
		}
};

// на входе указатель на абстрактную запись.
// в ней заполнена копия реальной записи, по ней формируем абстрактную
inline static void ConvertRealToAbstractRecord(BKDirDataItem *pFR) {
	auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать
	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		if (pRec->status == 0377) {
			pFR->nAttr |= FR_ATTR::DELETED;
		} else if (pRec->status == 0200) {
			pFR->nAttr |= FR_ATTR::BAD;
			if (pRec->name[0] == 0)	{
				pRec->name[0] = 'B';
				pRec->name[1] = 'A';
				pRec->name[2] = 'D';
			}
		}
		if (pRec->name[0] == 0177) {
			// если директория
			pFR->nAttr |= FR_ATTR::DIRECTORY;
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::DIRECTORY;
			strncpy(pFR->strName, (const char*)pRec->name + 1, 13);
			pFR->nDirBelong = pRec->dir_num;
			pFR->nDirNum = pRec->status;
			pFR->nBlkSize = 0;
			// в МКДОС 3.17 вслед за АНДОС 3.30 появились ссылки на диски.
			// опознаются точно так же по ненулевому значению (имя буквы привода) в поле адреса у директории
			if (pRec->address && pRec->address < 0200) {
				pFR->nAttr |= FR_ATTR::LINK;
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::LINK;
			}
		} else {
			// если файл
		/*	std::string name = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 14, m_pKoi8tbl));
			std::string ext;
			size_t l = name.length();
			size_t t = name.rfind('.');
			if (t != std::string::npos) // если расширение есть
			{
				ext = name.substr(t, l);
				name = strUtil::trim(name.substr(0, t));
			}
			if (!ext.empty()) {
				name += ext;
			} */
			strncpy(pFR->strName, (const char*)pRec->name, 14);
			//pFR->strName = name;
			pFR->nDirBelong = pRec->dir_num;
			pFR->nDirNum = 0;
			pFR->nBlkSize = pRec->len_blk;
			if (pFR->nAttr & FR_ATTR::DELETED) {
				pFR->nDirBelong = 0; // а то удалённые файлы вообще не видны. А всё потому, что у удалённых dir_num == 255 тоже.
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
			}
			if (pRec->status == 2) {
				// лог. диск это просто ещё каталог и файлы.
				pFR->nAttr |= FR_ATTR::LOGDISK;
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::LOGDSK;
			} else {
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
			}
			if (pRec->status == 1) {
				pFR->nAttr |= FR_ATTR::PROTECTED;
			}
		}
		pFR->nAddress = pRec->address;
		if (pFR->nAttr & FR_ATTR::LOGDISK) { // если у нас логический диск, то размер надо считать по блокам.
			pFR->nSize = pRec->len_blk * BLOCK_SIZE;
		} else {
			/*  правильный расчёт длины файла.
			т.к. в curr_record.length хранится остаток длины по модулю 0200000, нам из длины в блоках надо узнать
			сколько частей по 0200000 т.е. сколько в блоках частей по 128 блоков.(128 блоков == 65536.== 0200000)

			hw = (curr_record.len_blk >>7 ) << 16; // это старшее слово двойного слова
			hw = (curr_record.len_blk / 128) * 65536 = curr_record.len_blk * m_nSectorSize
			*/
			uint32_t hw = (pRec->len_blk << 9) & 0xffff0000; // это старшее слово двойного слова
			pFR->nSize = hw + pRec->length; // это ст.слово + мл.слово
		}
		pFR->nStartBlock = pRec->start_block;
	}
}

inline static bool AppendDirNum(uint8_t nNum) {
	bool bRet = false;
	if (m_sDiskCat.arDirNums[nNum] == 0) {
		m_sDiskCat.arDirNums[nNum] = 1;
		bRet = true;
	}
	// иначе - такой номер уже есть
	return bRet;
}

void mkdos_review(const PARSE_RESULT_C& parse_result, int curr_dir_num) {
    m_pDiskCat = reinterpret_cast<MKDosFileRecord *>(m_pCatBuffer + FMT_MKDOS_CAT_BEGIN); // каталог диска
	m_nMKCatSize = MKDOS_CAT_RECORD_SIZE;
	m_nMKLastCatRecord = m_nMKCatSize - 1;
	m_bMakeAdd = true;
	m_bMakeDel = true;
	m_bMakeRename = true;
	m_bChangeAddr = true;
    m_sDiskCat.init();
	m_sDiskCat.nCurrDirNum = curr_dir_num;
	m_sDiskCat.bHasDir = true;
	m_sDiskCat.nMaxDirNum = 0177;
	FIL* fil = new FIL();
	if (f_open(fil, parse_result.strName, FA_READ | FA_WRITE) == FR_OK) {
		m_bFileROMode = false;
	} else if (f_open(fil, parse_result.strName, FA_READ) != FR_OK) {
		delete fil;
        return;
	} else {
		m_bFileROMode = true;
	}
	UINT br;
	if (f_read(fil, m_pCatBuffer, sizeof m_pCatBuffer, &br) != FR_OK) {
		f_close(fil);
		delete fil;
		return;
	}
	f_close(fil);
    delete fil;
	int files_count = 0;
	int files_total = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_CAT_RECORD_NUMBER])); // читаем общее кол-во файлов. (НЕ записей!)
	m_sDiskCat.nDataBegin = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_FIRST_FILE_BLOCK])); // блок начала данных
	m_sDiskCat.nTotalRecs = m_nMKCatSize; // это у нас объём каталога, из него надо будет вычесть общее количество записей
	int used_size = 0;
	BKDirDataItem* AFR = new BKDirDataItem(); // экземпляр абстрактной записи
	auto pRec = reinterpret_cast<MKDosFileRecord *>(AFR->pSpecificData); // а в ней копия оригинальной записи
	for (unsigned int i = 0; i < m_nMKCatSize; ++i) // цикл по всему каталогу
	{
		if (files_count >= files_total) // файлы кончились, выходим
		{
			m_nMKLastCatRecord = i;
			break;
		}
		// преобразуем запись и поместим в массив
		AFR->clear();
		AFR->nSpecificDataLength = sizeof(MKDosFileRecord);
		*pRec = m_pDiskCat[i]; // копируем текущую запись как есть
		ConvertRealToAbstractRecord(AFR);
		if ((m_pDiskCat[i].status == 0377) || (m_pDiskCat[i].status == 0200)) {
			// удалённые не считаются,
			// плохие наверное тоже не считаются, но проверить не на чём
		} else {
			files_count++;
		}
		if (!(AFR->nAttr & FR_ATTR::DELETED)) {
			if (m_pDiskCat[i].name[0] == 0177) {
				// если директория
				if (!AppendDirNum(m_pDiskCat[i].status)) {
					// встретили дублирование номеров директорий
		//			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_DUPLICATE;
				}
			} else {
				// если файл
				used_size += m_pDiskCat[i].len_blk;
			}
		}
		// выбираем только те записи, которые к нужной директории принадлежат.
		if (AFR->nDirBelong == m_sDiskCat.nCurrDirNum) {
	//		m_sDiskCat.vecFC.push_back(AFR);
	        m_add_file_ext(AFR->strName, AFR->nRecType == BKDirDataItem::RECORD_TYPE::DIRECTORY, AFR->nDirNum);
		}
	}
	delete AFR;
#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - files_total;
	m_sDiskCat.nTotalBlocks = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_DISK_SIZE])) - *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_FIRST_FILE_BLOCK]));
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size;
	return;
}

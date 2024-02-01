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
static IMAGE_ERROR     m_nLastErrorNumber;

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

static FIL fil;

inline static bool read_cat(const PARSE_RESULT_C& parse_result) {
	if (f_open(&fil, parse_result.strName, FA_READ | FA_WRITE) == FR_OK) {
		m_bFileROMode = false;
	} else if (f_open(&fil, parse_result.strName, FA_READ) != FR_OK) {
        return false;
	} else {
		m_bFileROMode = true;
	}
	UINT br;
	if (f_read(&fil, m_pCatBuffer, sizeof m_pCatBuffer, &br) != FR_OK) {
		return false;
	}
	return true;
}

inline static bool mkdos_init(const PARSE_RESULT_C& parse_result, int curr_dir_num) {
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
    if (!read_cat(parse_result)) {
		f_close(&fil);
		return false;
	}
	m_sDiskCat.nDataBegin = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_FIRST_FILE_BLOCK])); // блок начала данных
	m_sDiskCat.nTotalRecs = m_nMKCatSize; // это у нас объём каталога, из него надо будет вычесть общее количество записей
	f_close(&fil);
	return true;
}

void mkdos_review(const PARSE_RESULT_C& parse_result, int curr_dir_num) {
    if (!mkdos_init(parse_result, curr_dir_num)) return;
	int files_count = 0;
	int files_total = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_CAT_RECORD_NUMBER])); // читаем общее кол-во файлов. (НЕ записей!)
	int used_size = 0;
	BKDirDataItem AFR; // экземпляр абстрактной записи
	auto pRec = reinterpret_cast<MKDosFileRecord *>(AFR.pSpecificData); // а в ней копия оригинальной записи
	for (unsigned int i = 0; i < m_nMKCatSize; ++i) // цикл по всему каталогу
	{
		if (files_count >= files_total) // файлы кончились, выходим
		{
			m_nMKLastCatRecord = i;
			break;
		}
		// преобразуем запись и поместим в массив
		AFR.clear();
		AFR.nSpecificDataLength = sizeof(MKDosFileRecord);
		*pRec = m_pDiskCat[i]; // копируем текущую запись как есть
		ConvertRealToAbstractRecord(&AFR);
		if ((m_pDiskCat[i].status == 0377) || (m_pDiskCat[i].status == 0200)) {
			// удалённые не считаются,
			// плохие наверное тоже не считаются, но проверить не на чём
		} else {
			files_count++;
		}
		if (!(AFR.nAttr & FR_ATTR::DELETED)) {
			if (m_pDiskCat[i].name[0] == 0177) {
				// если директория
				if (!AppendDirNum(m_pDiskCat[i].status)) {
					// встретили дублирование номеров директорий
					m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_DUPLICATE;
				}
			} else {
				// если файл
				used_size += m_pDiskCat[i].len_blk;
			}
		}
		// выбираем только те записи, которые к нужной директории принадлежат.
		if (AFR.nDirBelong == m_sDiskCat.nCurrDirNum) {
	//		m_sDiskCat.vecFC.push_back(AFR);
	        m_add_file_ext(AFR.strName, AFR.nRecType == BKDirDataItem::RECORD_TYPE::DIRECTORY, AFR.nDirNum);
		}
	}
#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - files_total;
	m_sDiskCat.nTotalBlocks = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_DISK_SIZE])) - *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_FIRST_FILE_BLOCK]));
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size;
	return;
}

// оптимизация каталога - объединение смежных дырок
inline static bool OptimizeCatalog() {
	int nUsedBlocs = 0;
	unsigned int p = 0;
	while (p <= m_nMKLastCatRecord) {
		if (m_pDiskCat[p].status == 0377) // если нашли дырку
		{
			unsigned int n = p + 1; // индекс следующей записи
			if (n > m_nMKLastCatRecord) // если p - последняя запись
			{
				// Тут надо обработать выход
				m_pDiskCat[p].status = m_pDiskCat[p].dir_num = 0;
				memset(m_pDiskCat[p].name, 0, 14);
				m_pDiskCat[p].address = m_pDiskCat[p].length = 0;
				m_pDiskCat[p].start_block = m_pDiskCat[p - 1].start_block + m_pDiskCat[p - 1].len_blk;
				m_pDiskCat[p].len_blk = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_DISK_SIZE])) - nUsedBlocs;
				break;
			}
			if (m_pDiskCat[n].status == 0377) // и за дыркой снова дырка
			{
				m_pDiskCat[p].len_blk += m_pDiskCat[n].len_blk; // первую - укрупним
				// а вторую удалим.
				while (n < m_nMKLastCatRecord) // сдвигаем каталог
				{
					m_pDiskCat[n] = m_pDiskCat[n + 1];
					n++;
				}
				memset(&m_pDiskCat[m_nMKLastCatRecord--], 0, sizeof(MKDosFileRecord));
				continue; // и всё сначала
			}
		}
		nUsedBlocs += m_pDiskCat[p].len_blk;
		p++;
	}
	return true;
}

inline static bool WriteCurrentDir() {
	OptimizeCatalog();
	if (f_lseek(&fil, 0) != FR_OK) return false;
	UINT bw;
	return f_write(&fil, m_pCatBuffer, m_nCatSize, &bw) == FR_OK;
}

inline static bool SeekToBlock(size_t lba) {
	lba *= BLOCK_SIZE;
    return f_lseek(&fil, lba) == FR_OK;
}

inline static bool ReadData(uint8_t *pBuff, size_t sz) {
	UINT br;
    return f_read(&fil, pBuff, sz, &br) == FR_OK;
}

inline static bool WriteData(uint8_t *pBuff, size_t sz) {
	UINT br;
    return f_write(&fil, pBuff, sz, &br) == FR_OK;
}

// сквизирование диска
bool Squeeze() {
	bool bRet = true;
	int nUsedBlocs = 0;
	unsigned int p = 0;
	while (p <= m_nMKLastCatRecord) {
		if (m_pDiskCat[p].status == 0377) // если нашли дырку
		{
			unsigned int n = p + 1; // индекс следующей записи
			if (n >= m_nMKLastCatRecord) // если p - последняя запись
			{
				// Тут надо обработать выход
				m_pDiskCat[p].status = m_pDiskCat[p].dir_num = 0;
				memset(m_pDiskCat[p].name, 0, 14);
				m_pDiskCat[p].address = m_pDiskCat[p].length = 0;
				m_pDiskCat[p].start_block = m_pDiskCat[p - 1].start_block + m_pDiskCat[p - 1].len_blk;
				m_pDiskCat[p].len_blk = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_DISK_SIZE])) - nUsedBlocs;
				m_nMKLastCatRecord--;
				break;
			}
			if (m_pDiskCat[n].status == 0377) // и за дыркой снова дырка
			{
				m_pDiskCat[p].len_blk += m_pDiskCat[n].len_blk; // первую - укрупним
				// а вторую удалим.
				while (n < m_nMKLastCatRecord) // сдвигаем каталог
				{
					m_pDiskCat[n] = m_pDiskCat[n + 1];
					n++;
				}
				memset(&m_pDiskCat[m_nMKLastCatRecord--], 0, sizeof(MKDosFileRecord));
				continue; // и всё сначала
			}
			size_t nBufSize = size_t(m_pDiskCat[n].len_blk) * BLOCK_SIZE;
			auto pBuf = new uint8_t[nBufSize];
			if (pBuf) {
				if (SeekToBlock(m_pDiskCat[n].start_block)) {
					if (!ReadData(pBuf, nBufSize)) {
						m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
						bRet = false;
						delete pBuf;
						break;
					}
					if (SeekToBlock(m_pDiskCat[p].start_block)) {
						if (!WriteData(pBuf, nBufSize)) {
							m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
							bRet = false;
							delete pBuf;
							break;
						}
						// теперь надо записи местами поменять.
						MKDosFileRecord t = m_pDiskCat[n]; // обменяем записи целиком
						m_pDiskCat[n] = m_pDiskCat[p];
						m_pDiskCat[p] = t;
                        uint16_t tt =  m_pDiskCat[n].start_block; // начальные блоки вернём как было.
						m_pDiskCat[n].start_block = m_pDiskCat[p].start_block;
						m_pDiskCat[p].start_block = tt;
						m_pDiskCat[n].start_block = m_pDiskCat[p].start_block + m_pDiskCat[p].len_blk;
					} else {
						bRet = false;
					}
				} else {
					bRet = false;
				}
				delete pBuf;
			} else {
				m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
				bRet = false;
				break;
			}
		}
		nUsedBlocs += m_pDiskCat[p].len_blk;
		p++;
	}
	WriteCurrentDir(); // сохраним
	return bRet;
}

inline static size_t ext(const char* strMKName, size_t sz, char* strMKExt) { // включая точку
    size_t i = sz - 1;
    while(i > 0 && strMKName[i--] != '.');
	if (i > 0) {
	    strncpy(strMKExt, strMKName + i, 5);
		return strnlen(strMKExt, 4);
	} else {
		strMKExt[0] = 0;
		return 0;
	}
}

// размер проги выровняем по границе блока, сперцифичного для заданной ОС
inline static int EvenSizeByBlock_l(int length) {
	return length ? (((length - 1) | (BLOCK_SIZE - 1)) + 1) : 0;
}

// размер проги в размерах блока, сперцифичного для заданной ОС
inline static int ByteSizeToBlockSize_l(int length) {
	return EvenSizeByBlock_l(length) / BLOCK_SIZE;
}

inline static void ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) {
	auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly) {
		if (!bRenameOnly) {
			pFR->nSpecificDataLength = sizeof(MKDosFileRecord);
			memset(pRec, 0, sizeof(MKDosFileRecord));
		}
		// надо сформировать мкдосную запись из абстрактной
		size_t sz = strnlen(pFR->strName, 16);
		char strMKExt[5];
		size_t strMKExt_len = ext(pFR->strName, sz, strMKExt);
		size_t nNameLen = (14 - strMKExt_len); // допустимая длина имени
		char strMKName[15];
		size_t cpz = nNameLen < sz ? nNameLen : sz;
		memcpy(strMKName, pFR->strName, cpz);
        if (strMKExt_len) memcpy(strMKName + cpz, strMKExt, strMKExt_len); // прицепляем расширение
		strMKName[cpz + strMKExt_len] = 0;
		if (pFR->nAttr & FR_ATTR::DIRECTORY) {
		//	std::wstring strDir = strUtil::CropStr(pFR->strName, 13); /// ???
		//	imgUtil::UNICODEtoBK(strDir, pRec->name + 1, 13, true);
			if (!bRenameOnly) {
				pRec->name[0] = 0177; // признак каталога
				pRec->status = pFR->nDirNum;
				pRec->dir_num = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
			}
		} else {
		//	imgUtil::UNICODEtoBK(strMKName, pRec->name, 14, true);
			pRec->address = pFR->nAddress; // возможно, если мы сохраняем бин файл, адрес будет браться оттуда
			if (!bRenameOnly) {
				if (pFR->nAttr & FR_ATTR::PROTECTED) {
					pRec->status = 1;
				} else {
					pRec->status = 0;
				}
				pRec->dir_num = pFR->nDirBelong;
				pRec->len_blk = ByteSizeToBlockSize_l(pFR->nSize); // размер в блоках
				pRec->length = pFR->nSize % 0200000; // остаток от длины по модулю 65535
			}
		}
	}
}

IMAGE_ERROR MkDosErrorNumber() {
	return m_nLastErrorNumber;
}

		/* поиск заданной записи в каталоге.
		если bFull==false делается поиск по имени
		если bFull==true делается поиск по имени и другим параметрам
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
inline static int FindRecord2(MKDosFileRecord *pRec, bool bFull) {
	int nIndex = -1;
	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i) {
		if ((m_pDiskCat[i].status == 0377) || (m_pDiskCat[i].status == 0200)) {
			// если удалённый файл - дырка, то игнорируем
			continue;
		}
		if (m_pDiskCat[i].dir_num == m_sDiskCat.nCurrDirNum) // проверяем только в текущей директории
		{
			if (memcmp(pRec->name, m_pDiskCat[i].name, 14) == 0)  // проверим имя
			{
				if (bFull) {
					if (pRec->name[0] == 0177) // если директория
					{
						if (m_pDiskCat[i].status == pRec->status) // то проверяем номер директории
						{
							nIndex = static_cast<int>(i);
							break;
						}
					}
					else // если файл - то проверяем параметры файла
					{
						if (m_pDiskCat[i].dir_num == pRec->dir_num) {
							nIndex = static_cast<int>(i);
							break;
						}
					}
				} else {
					nIndex = static_cast<int>(i);
					break;
				}
			}
		}
	}
	return nIndex;
}

inline static uint8_t AssignNewDirNum() {
	for (uint8_t i = 1; i < m_sDiskCat.nMaxDirNum; ++i) {
		if (m_sDiskCat.arDirNums[i] == 0) {
			m_sDiskCat.arDirNums[i] = 1;
			return i;
		}
	}
	return 0;
}

inline static bool MkDosCreateDir(BKDirDataItem *pFR) {
	bool bRet = false;
	if (m_bFileROMode) {
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // то записать в него мы ничего не сможем.
	}
	if (m_sDiskCat.nFreeRecs <= 0) {
		m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
		return false;
	}
	if (pFR->nAttr & FR_ATTR::DIRECTORY) {
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
	///	pFR->nDirBelong = pRec->dir_num = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
		// проверим, вдруг такая директория уже есть
		int nInd = FindRecord2(pRec, false); // мы тут не знаем номер директории. мы можем только по имени проверить.
		if (nInd >= 0) {
			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_EXIST;
			pFR->nDirNum = m_pDiskCat[nInd].status; // и заодно узнаем номер директории
		} else {
			unsigned int nIndex = 0;
			// найдём свободное место в каталоге.
			bool bHole = false;
			bool bFound = false;
			for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i) {
				if ((m_pDiskCat[i].status == 0377) && (m_pDiskCat[i].name[0] == 0177)) {
					// если нашли дырку в которой было имя директории
					bHole = true;
					nIndex = i;
					bFound = true;
					break;
				}
			}
			if (!bFound) {
				nIndex = m_nMKLastCatRecord;
				if (nIndex < m_nMKCatSize) // если в конце каталога вообще есть место
				{
					bFound = true; // то нашли, иначе - нет
				}
			}
			if (bFound) {
				pFR->nDirNum = pRec->status = AssignNewDirNum(); // назначаем номер директории.
				if (pFR->nDirNum == 0) {
					m_nLastErrorNumber = IMAGE_ERROR::FS_DIRNUM_FULL;
				}
				// если ошибок не произошло, сохраним результаты
				if (bHole) {
					// сохраняем нашу запись вместо удалённой директории
					m_pDiskCat[nIndex] = pRec;
				} else {
					// если нашли свободную область в конце каталога
					int nHoleSize = m_pDiskCat[nIndex].len_blk;
					int nStartBlock = m_pDiskCat[nIndex].start_block;
					// сохраняем нашу запись
					m_pDiskCat[nIndex] = pRec;
					// сохраняем признак конца каталога
					MKDosFileRecord hole;
					hole.start_block = nStartBlock + pRec->len_blk;
					hole.len_blk = nHoleSize - pRec->len_blk;
					// и запись с инфой о свободной области
					m_pDiskCat[nIndex + 1] = hole;
					m_nMKLastCatRecord++;
				}
				// сохраняем каталог
				*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_CAT_RECORD_NUMBER])) += 1; // поправим параметры - количество файлов
				bRet = WriteCurrentDir();
				m_sDiskCat.nFreeRecs--;
			} else {
				m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
			}
		}
	} else {
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
	}
	return bRet;
}

void mkdos_mkdir(const PARSE_RESULT_C& parse_result, int curr_dir_num, char* name) {
    if (!mkdos_init(parse_result, curr_dir_num)) {
		f_close(&fil);
		return;
	}
	BKDirDataItem itm = { 0 };
	strncpy(itm.strName, name, 16);
	itm.nRecType = BKDirDataItem::RECORD_TYPE::DIRECTORY;
	itm.nDirBelong = curr_dir_num;
	MkDosCreateDir(&itm);
	f_close(&fil);
}


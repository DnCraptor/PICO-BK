#include "pch.h"
#include "BKFloppyImage_ANDos.h"
#include "StringUtil.h"

#include <ctime>

#pragma warning(disable:4996)

// атрибуты файла
constexpr auto FAT_ENTRY_ATTR_READONLY = 0x01;
constexpr auto FAT_ENTRY_ATTR_HIDDEN = 0x02;
constexpr auto FAT_ENTRY_ATTR_SYSTEM = 0x04;
constexpr auto FAT_ENTRY_ATTR_VOLUME_ID = 0x08;
constexpr auto FAT_ENTRY_ATTR_DIRECTORY = 0x10;
constexpr auto FAT_ENTRY_ATTR_ARCHIVE = 0x20;

CBKFloppyImage_ANDos::CBKFloppyImage_ANDos(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_nClusterSectors(0)
	, m_nClusterSize(0)
	, m_nBootSectors(0)
	, m_nRootFilesNum(0)
	, m_nRootSize(0)
	, m_nFatSectors(0)
	, m_nFatSize(0)
	, m_nRootSectorOffset(0)
	, m_nDataSectorOffset(0)
	, m_pFatTbl(nullptr)
	, m_pCatTbl(nullptr)
	, m_pDiskCat(nullptr)
{
	m_bMakeAdd    = true;
	m_bMakeDel    = true;
	m_bMakeRename = true;
	m_bChangeAddr = true;
}


CBKFloppyImage_ANDos::~CBKFloppyImage_ANDos()
{
}

int CBKFloppyImage_ANDos::GetNextFat(int fat)
{
	/*
	0  1  2  3  4  5  6  7  8  9
	xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4
	0                   1
	fat*3/2
	0 1 2 3 4 5 6 7  8
	0 1 3 4 6 7 9 10 12
	*/
	int num = fat * 3 / 2;
	int val = *(reinterpret_cast<uint16_t *>(&m_pFatTbl[num]));

	if (fat & 1)
	{
		// если номер нечётный, взять старшие 12 бит
		val >>= 4;
	}

	// если номер чётный, взять мл 12 бит
	return (val & 0xfff);
}

// ищем свободную ячейку от текущей (которая в качестве параметра)
int CBKFloppyImage_ANDos::FindFreeFat(int fat)
{
	uint16_t val;
	unsigned int num = fat * 3 / 2;

	while (num < (m_nFatSize - 1))
	{
		val = *(reinterpret_cast<uint16_t *>(&m_pFatTbl[num]));

		if (fat & 1)
		{
			// если номер нечётный, взять старшие 12 бит
			val >>= 4;
		}

		val &= 0xfff;

		if (val == 0)
		{
			// нашли пустую ячейку
			return fat;
		}

		fat++;
		num = fat * 3 / 2;
	}

	return -1;
}
// устанавливаем значение ячейки фат, возвращаем прошлое значение
uint16_t CBKFloppyImage_ANDos::SetFat(int fat, uint16_t val)
{
	int num = fat * 3 / 2;
	auto pval = reinterpret_cast<uint16_t *>(&m_pFatTbl[num]);
	uint16_t cval = *pval;
	val &= 0xfff;

	if (fat & 1)
	{
		*pval = (cval & 0x000f) | (val << 4);
		cval >>= 4;
	}
	else
	{
		*pval = (cval & 0xf000) | val;
	}

	return (cval & 0xfff);
}

int CBKFloppyImage_ANDos::FindRecord(AndosFileRecord *pRec)
{
	int nIndex = -1;

	for (unsigned int i = 0; i < m_nRootFilesNum; ++i) // цикл по всему каталогу
	{
		if (m_pDiskCat[i].filename[0] == 0)
		{
			break; // конец каталога
		}

		if (m_pDiskCat[i].parent_dir_num == m_sDiskCat.nCurrDirNum) // ищем только в текущей директории
		{
			if (memcmp(&m_pDiskCat[i], pRec, sizeof(AndosFileRecord)) == 0)
			{
				nIndex = static_cast<int>(i);
				break;
			}
		}
	}

	return nIndex;
}

int CBKFloppyImage_ANDos::FindRecord2(AndosFileRecord *pRec, bool bFull)
{
	int nIndex = -1;

	for (unsigned int i = 0; i < m_nRootFilesNum; ++i)
	{
		if (m_pDiskCat[i].filename[0] == 0)
		{
			break;    // конец каталога
		}

		if (m_pDiskCat[i].filename[0] == 0345)
		{
			continue;
		}

		if (m_pDiskCat[i].parent_dir_num == m_sDiskCat.nCurrDirNum) // ищем только в текущей директории
		{
			if (memcmp(pRec->filename, m_pDiskCat[i].filename, 11) == 0)
			{
				if (bFull) // если полная проверка
				{
					if (m_pDiskCat[i].dir_num)
					{
						// если директория
						// кроме имени проверим и остальные поля
						if ((m_pDiskCat[i].dir_num == pRec->dir_num)
						        && (m_pDiskCat[i].parent_dir_num == pRec->parent_dir_num)
						   )
						{
							nIndex = static_cast<int>(i);
							break;
						}
					}
					else
					{
						// если файл
						// кроме имени проверим и остальные поля
						if (m_pDiskCat[i].parent_dir_num == pRec->parent_dir_num)
						{
							nIndex = static_cast<int>(i);
							break;
						}
					}
				}
				else
				{
					nIndex = static_cast<int>(i);
					break;
				}
			}
		}
	}

	return nIndex;
}


void CBKFloppyImage_ANDos::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly)
{
	auto pRec = reinterpret_cast<AndosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = sizeof(AndosFileRecord);
			memset(pRec, 0, sizeof(AndosFileRecord));
		}

		if (pFR->nAttr & FR_ATTR::DIRECTORY)
		{
			std::string name = strUtil::CropStr(pFR->strName, 11);
			imgUtil::UNICODEtoBK(name, pRec->filename, 11, true); // берём первые 8 символов имени.
		}
		else
		{
			// надо сформировать андосную запись из абстрактной
			std::string name = strUtil::CropStr(pFR->strName.stem().string(), 8);
			imgUtil::UNICODEtoBK(name, pRec->file.name, 8, true); // берём первые 8 символов имени.
			std::string ext = pFR->strName.extension().string();

			if (!ext.empty())
			{
				ext = strUtil::CropStr(ext.substr(1), 3); // без точки
			}

			imgUtil::UNICODEtoBK(ext, pRec->file.ext, 3, true);
		}

		pRec->address = pFR->nAddress; // возможно, если мы сохраняем бин файл, адрес будет браться оттуда

		if (!bRenameOnly)
		{
			// теперь скопируем некоторые атрибуты
			if (pFR->nAttr & FR_ATTR::READONLY)
			{
				pRec->attr |= FAT_ENTRY_ATTR_READONLY;
			}

			if (pFR->nAttr & FR_ATTR::HIDDEN)
			{
				pRec->attr |= FAT_ENTRY_ATTR_HIDDEN;
			}

			if (pFR->nAttr & FR_ATTR::PROTECTED)
			{
				pRec->attr |= FAT_ENTRY_ATTR_SYSTEM;
			}

			if (pFR->nAttr & FR_ATTR::VOLUMEID)
			{
				pRec->attr |= FAT_ENTRY_ATTR_VOLUME_ID;
			}

			if (pFR->nAttr & FR_ATTR::DIRECTORY)
			{
				pRec->attr |= FAT_ENTRY_ATTR_DIRECTORY;
			}

			if (pFR->nAttr & FR_ATTR::ARCHIVE)
			{
				pRec->attr |= FAT_ENTRY_ATTR_ARCHIVE;
			}

			pRec->dir_num = pFR->nDirNum;
			pRec->parent_dir_num = pFR->nDirBelong;
			/*  формат даты. биты 0-4 - день месяца 1-31;
			биты 5-8 – месяц года, допускаются значения 1-12;
			биты 9-15 – год, считая от 1980 г. («эпоха MS-DOS»), возможны значения от 0 до 127 включительно, т.е. 1980-2107 гг.
			*/
			tm ctm{};
			gmtime_s(&ctm, &pFR->timeCreation);
			pRec->date = (ctm.tm_mday & 037) | (((ctm.tm_mon & 017) + 1) << 5) | (((ctm.tm_year + 1900) > 1980 ? ctm.tm_year + 1900 - 1980 : 0) << 9);
			pRec->first_cluster = pFR->nStartBlock;
			pRec->length = pFR->nSize;
		}
	}
}
// на входе указатель на абстрактную запись.
// в ней заполнена копия реальной записи, по ней формируем абстрактную
void CBKFloppyImage_ANDos::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<AndosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		if (pRec->dir_num)
		{
			std::string name = strUtil::trim(imgUtil::BKToUNICODE(pRec->filename, 11, m_pKoi8tbl));
			// если каталог
			pFR->nAttr |= FR_ATTR::DIRECTORY;
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::DIRECTORY;
			pFR->strName = name;
			pFR->nDirNum = pRec->dir_num;
			pFR->nDirBelong = pRec->parent_dir_num;
			pFR->nBlkSize = 0;

			// в АНДОС 3.30 появились ссылки на диски.
			// опознаются по ненулевому значению (имя буквы привода) в поле адреса у директории
			if (pRec->address && pRec->address < 0200)
			{
				pFR->nAttr |= FR_ATTR::LINK;
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::LINK;
			}
		}
		else
		{
			std::string name = strUtil::trim(imgUtil::BKToUNICODE(pRec->file.name, 8, m_pKoi8tbl));
			std::string ext = strUtil::trim(imgUtil::BKToUNICODE(pRec->file.ext, 3, m_pKoi8tbl));
			// если файл
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
			pFR->strName = name;

			if (!ext.empty())
			{
				pFR->strName += "." + ext;
			}

			pFR->nDirNum = 0;
			pFR->nDirBelong = pRec->parent_dir_num;
			// посчитаем занятое пространство в кластерах.
			pFR->nBlkSize = (pRec->length) ? ((((pRec->length - 1) | (m_nClusterSize - 1)) + 1) / m_nClusterSize) : 0;
		}

		if (pRec->filename[0] == 0345)
		{
			pFR->nAttr |= FR_ATTR::DELETED;
			std::string t = pFR->strName.string();
			t[0] = 'x';
			pFR->strName = fs::path(t);
		}

		// теперь скопируем некоторые атрибуты
		if (pRec->attr & FAT_ENTRY_ATTR_READONLY)
		{
			pFR->nAttr |= FR_ATTR::READONLY;
		}

		if (pRec->attr & FAT_ENTRY_ATTR_HIDDEN)
		{
			pFR->nAttr |= FR_ATTR::HIDDEN;
		}

		if (pRec->attr & FAT_ENTRY_ATTR_SYSTEM)
		{
			pFR->nAttr |= FR_ATTR::PROTECTED;
		}

		if (pRec->attr & FAT_ENTRY_ATTR_VOLUME_ID)
		{
			pFR->nAttr |= FR_ATTR::VOLUMEID;
		}

		if (pRec->attr & FAT_ENTRY_ATTR_DIRECTORY)
		{
			pFR->nAttr |= FR_ATTR::DIRECTORY; // ?? надо ли? вдруг в андосе с этим вольно обращались
		}

		if (pRec->attr & FAT_ENTRY_ATTR_ARCHIVE)
		{
			pFR->nAttr |= FR_ATTR::ARCHIVE;
		}

		pFR->nAddress = pRec->address;
		pFR->nSize = pRec->length;
		pFR->nStartBlock = pRec->first_cluster;
		// обратная операция для времени
		/*  формат даты. биты 0-4 - день месяца 1-31;
		биты 5-8 – месяц года, допускаются значения 1-12;
		биты 9-15 – год, считая от 1980 г. («эпоха MS-DOS»), возможны значения от 0 до 127 включительно, т.е. 1980-2107 гг.
		*/
		tm ctm{};
		memset(&ctm, 0, sizeof(tm));
		ctm.tm_mday = pRec->date & 0x1f;
		ctm.tm_mon = ((pRec->date >> 5) & 0x0f) - 1;
		ctm.tm_year = ((pRec->date >> 9) & 0x7f) + 1980 - 1900;

		if (ctm.tm_year < 0)
		{
			ctm.tm_year = 0;
		}

		pFR->timeCreation = mktime(&ctm);
	}
}

void CBKFloppyImage_ANDos::OnReopenFloppyImage()
{
	m_sDiskCat.bHasDir = true;
	m_sDiskCat.nMaxDirNum = 255;
}

/*
*выход: true - успешно
*      false - ошибка
*/
bool CBKFloppyImage_ANDos::SeekToCluster(int nCluster)
{
	bool bRet = false;

	if ((1 < nCluster) && (nCluster < 07760))
	{
		long nOffs = m_sParseImgResult.nBaseOffset + m_nDataSectorOffset + (nCluster - 2) * m_nClusterSize;
		m_nSeekOffset = nOffs / BLOCK_SIZE;
		bRet = true;
	}

	return bRet;
}

const std::string CBKFloppyImage_ANDos::GetSpecificData(BKDirDataItem *fr) const
{
	auto pRec = reinterpret_cast<AndosFileRecord *>(fr->pSpecificData);
	int dd = pRec->date;
	int nDay = dd & 0x1f;
	int nMon = (dd >> 5) & 0xf;
	int nYear = ((dd >> 9) & 0x7f) + 1980;
	return imgUtil::string_format("%02d.%02d.%04d\0", nDay, nMon, nYear);
}

const std::string CBKFloppyImage_ANDos::GetImageInfo() const
{
	std::string strf = "Free in dir: %d record%s from %d. Free: %d cluster%s / %d byte%s frim %d / %d";

	if (!strf.empty())
	{
		auto freeblocks = static_cast<unsigned int>(m_sDiskCat.nFreeBlocks);
		auto totalblocks = static_cast<unsigned int>(m_sDiskCat.nTotalBlocks);
		auto freerecs = static_cast<unsigned int>(m_sDiskCat.nFreeRecs);
		unsigned int bytes = freeblocks * m_nClusterSize;
		unsigned int totalbytes = totalblocks * m_nClusterSize;
		return imgUtil::string_format(strf, freerecs, imgUtil::tblStrRec[imgUtil::GetWordEndIdx(freerecs)].c_str(),
		                              static_cast<unsigned int>(m_sDiskCat.nTotalRecs),
		                              freeblocks, imgUtil::tblStrBlk[imgUtil::GetWordEndIdx(freeblocks)].c_str(),
		                              bytes, imgUtil::tblStrBlk[imgUtil::GetWordEndIdx(bytes)].c_str(),
		                              totalblocks, totalbytes);
	}

	return strf;
}

const size_t CBKFloppyImage_ANDos::GetImageFreeSpace() const
{
	return static_cast<size_t>(m_sDiskCat.nFreeBlocks) * m_nClusterSize;
}

bool CBKFloppyImage_ANDos::ReadCurrentDir()
{
	if (!CBKFloppyImage_Prototype::ReadCurrentDir())
	{
		return false;
	}

	// читаем нулевой сектор
	if (!SeektoBlkReadData(0, m_nSector, sizeof(m_nSector)))
	{
		return false;
	}

	// прочитаем данные BPB
	m_nBlockSize = *(reinterpret_cast<uint16_t *>(&m_nSector[013]));
	m_nClusterSectors = m_nSector[015];
	m_nClusterSize = m_nClusterSectors * m_nBlockSize;
	m_nBootSectors = *(reinterpret_cast<uint16_t *>(&m_nSector[016]));
	m_nRootFilesNum = *(reinterpret_cast<uint16_t *>(&m_nSector[021]));
	m_nRootSize = m_nRootFilesNum * 32;
	m_nFatSectors = *(reinterpret_cast<uint16_t *>(&m_nSector[026]));
	m_nFatSize = m_nFatSectors * m_nBlockSize;
	m_nRootSectorOffset = (m_nBootSectors + m_nFatSectors * 2) * m_nBlockSize;
	m_nDataSectorOffset = m_nRootSectorOffset + m_nRootSize;

	// единственное место, где размеры массивов заранее неизвестны, и их надо выделять тут
	// и на всякий случай сделана проверка, вдруг уже было выделено раньше
	if (!m_pFatTbl)
	{
		m_pFatTbl = std::make_unique<uint8_t[]>(m_nFatSize); // место под ФАТ
	}

	if (!m_pCatTbl)
	{
		m_pCatTbl = std::make_unique<uint8_t[]>(m_nRootSize); // место под каталог

		if (m_pCatTbl)
		{
			m_pDiskCat = reinterpret_cast<AndosFileRecord *>(m_pCatTbl.get());// каталог диска
		}
	}

	if (!m_pFatTbl || !m_pCatTbl)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

	// перемещаемся к началу фат и прочтём фат
	if (!SeektoBlkReadData(m_nBootSectors, m_pFatTbl.get(), m_nFatSize))
	{
		return false;
	}

	// Перемещаемся к началу каталога и читаем каталог
	if (!SeektoBlkReadData(m_nBootSectors + static_cast<size_t>(m_nFatSectors) * 2, m_pCatTbl.get(), m_nRootSize))
	{
		return false;
	}

	// теперь наш каталог представим в виде записей
	BKDirDataItem curr_fr; // экземпляр абстрактной записи
	auto pRec = reinterpret_cast<AndosFileRecord *>(curr_fr.pSpecificData); // а в ней копия оригинальной записи
	// каталог занимает m_nRootFilesNum/16 секторов, длина записи 32. байта, макс кол-во записей m_nRootFilesNum
	int free_recs = m_nRootFilesNum;
	int used_size = 0;

	for (unsigned int rec_count = 0; rec_count < m_nRootFilesNum; ++rec_count)
	{
		if (m_pDiskCat[rec_count].filename[0] == 0)
		{
			break; // теоретически это вообще конец каталога
		}

		curr_fr.clear();
		curr_fr.nSpecificDataLength = sizeof(AndosFileRecord);
		*pRec = m_pDiskCat[rec_count]; // копируем текущую запись как есть
		ConvertRealToAbstractRecord(&curr_fr);

		if (m_pDiskCat[rec_count].filename[0] != 0345)
		{
			free_recs--;
		}

		if (!(curr_fr.nAttr & FR_ATTR::DELETED))
		{
			if (m_pDiskCat[rec_count].dir_num)
			{
				// если каталог
				if (!AppendDirNum(m_pDiskCat[rec_count].dir_num))
				{
					// встретили дублирование номеров директорий
					m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_DUPLICATE;
				}
			}
			else
			{
				// если файл
				used_size += curr_fr.nBlkSize; // если не удалённый, посчитаем используемый размер
			}
		}

		// выбираем только те записи, которые к нужной директории принадлежат.
		if (curr_fr.nDirBelong == m_sDiskCat.nCurrDirNum)
		{
			m_sDiskCat.vecFC.push_back(curr_fr);
		}
	}

#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif
	m_sDiskCat.nTotalRecs = m_nRootFilesNum;
	m_sDiskCat.nFreeRecs = free_recs; // сколько свободных записей в каталоге
	m_sDiskCat.nTotalBlocks = (*(reinterpret_cast<uint16_t *>(&m_nSector[023])) - m_nDataSectorOffset / m_nBlockSize) / m_nClusterSectors;
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size;
	return true;
}

bool CBKFloppyImage_ANDos::WriteCurrentDir()
{
	if (!CBKFloppyImage_Prototype::WriteCurrentDir())
	{
		return false;
	}

	if (!m_pFatTbl || !m_pCatTbl)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif

	if (SeektoBlkWriteData(m_nBootSectors, m_pFatTbl.get(), m_nFatSize))// перейдём к началу фат, сохраним первую копию фат
	{
		if (WriteData(m_pFatTbl.get(), m_nFatSize))// сохраним вторую копию фат
		{
			return WriteData(m_pCatTbl.get(), m_nRootSize); // сохраним каталог
		}
	}

	return false;
}

bool CBKFloppyImage_ANDos::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	// если плохой или удалённый или директория - ничего не делаем
	if (pFR->nAttr & (FR_ATTR::BAD | FR_ATTR::DIRECTORY))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return bRet;
	}

	ConvertAbstractToRealRecord(pFR);
	int nLen = pFR->nSize;
	int currFat = pFR->nStartBlock;
	uint8_t *bufp = pBuffer;

	while ((nLen > 0) && ((1 < currFat) && (currFat < 07760)))
	{
		bRet = SeekToCluster(currFat);

		if (!bRet)
		{
			break;
		}

		// читаем всегда покластерно
		bRet = ReadData(&m_mBlock, m_nClusterSize); // !!!если m_nClusterSize будет больше COPY_BLOCK_SIZE, то будет облом

		if (!bRet)
		{
			break;
		}

		// но если в конце кластер неполон, нужно взять только нужную часть
		int nReaded = (nLen >= static_cast<int>(m_nClusterSize)) ? static_cast<int>(m_nClusterSize) : nLen;
		memcpy(bufp, m_mBlock, nReaded);
		bufp += nReaded;
		nLen -= nReaded;
		currFat = GetNextFat(currFat);
	}

	return bRet;
}

bool CBKFloppyImage_ANDos::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
{
	bNeedSqueeze = false;
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // то записать в него мы ничего не сможем.
	}

	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false;
	}

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<AndosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
	pFR->nDirBelong = pRec->parent_dir_num = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога

	if (m_sDiskCat.nFreeRecs <= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
		return false;
	}

	if (m_sDiskCat.nFreeBlocks * m_nClusterSize < (int)((pFR->nSize) ? (((pFR->nSize - 1) | (m_nClusterSize - 1)) + 1) : 0))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_FULL;
		return bRet;
	}

	// поищем, вдруг такое имя файла уже существует.
	int nIndex = FindRecord2(pRec, true);

	if (nIndex >= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_EXIST;
		*pRec = m_pDiskCat[nIndex];
		ConvertRealToAbstractRecord(pFR); // и сделаем найденную запись актуальной.
		return false;
	}

	// найдём свободное место в каталоге.
	bool bFound = false;
	unsigned int CatOffs = 0; // смещение в массиве каталога, до первой найденной свободной записи.

	for (CatOffs = 0; CatOffs < m_nRootFilesNum; ++CatOffs)
	{
		if ((m_pDiskCat[CatOffs].filename[0] == 0345) || (m_pDiskCat[CatOffs].filename[0] == 0))
		{
			bFound = true;
			break;
		}
	}

	if (bFound)
	{
		// можно сохранять
		bool bNoErr = true;
		int currFat = 1;
		int nPrevFat = 0;
		int nLen = pFR->nSize; // размер файла

		if (nLen == 0)
		{
			nLen++; // запись файла нулевой длины не допускается, портится фат, поэтому скорректируем.
		}

		while ((nLen > 0) && bNoErr)
		{
			currFat = FindFreeFat(currFat + 1); // если искать от текущего, то мы дважды будем считать

			// один и тот же кластер, т.к. он заполнится только после того как будет найден следующий
			if (currFat == -1)
			{
				bNoErr = false;
				break;
			}

			if (nPrevFat == 0)
			{
				pFR->nStartBlock = pRec->first_cluster = currFat;
			}
			else
			{
				SetFat(nPrevFat, currFat);
			}

			nPrevFat = currFat;
			bNoErr = SeekToCluster(currFat);
			memset(m_mBlock, 0, m_nClusterSize); // сделано чтобы хвост последнего кластера не заполнялся мусором от предыдущего, если размер данных меньше размера кластера
			int nReaded = (nLen >= static_cast<int>(m_nClusterSize)) ? static_cast<int>(m_nClusterSize) : nLen; // прочесть можем и меньше чем кластер
			memcpy(m_mBlock, pBuffer, nReaded);
			pBuffer += nReaded;
			WriteData(m_mBlock, m_nClusterSize); // пишем по любому покластерно.
			nLen -= m_nClusterSize;
		}

		if (bNoErr)
		{
			SetFat(nPrevFat, 07777);
			// если ошибок не произошло, сохраним результаты
			m_pDiskCat[CatOffs] = pRec;
			bRet = WriteCurrentDir();
			m_sDiskCat.nFreeRecs--;
			m_sDiskCat.nFreeBlocks -= ((int)((pFR->nSize) ? (((pFR->nSize - 1) | (m_nClusterSize - 1)) + 1) : 0)) / m_nClusterSize;
		}
		else
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_STRUCT_ERR;
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
	}

	return bRet;
}

bool CBKFloppyImage_ANDos::DeleteFile(BKDirDataItem *pFR, bool bForce)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // то записать в него мы ничего не сможем.
	}

	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false;
	}

	if (pFR->nAttr & FR_ATTR::DELETED)
	{
		return true; // уже удалённое не удаляем повторно
	}

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<AndosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
	int nIndex = FindRecord2(pRec);

	if (nIndex >= 0)
	{
		if ((m_pDiskCat[nIndex].attr & (FAT_ENTRY_ATTR_READONLY | FAT_ENTRY_ATTR_HIDDEN | FAT_ENTRY_ATTR_SYSTEM)) && !bForce)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_PROTECTED;
		}
		else
		{
			uint16_t vfat = m_pDiskCat[nIndex].first_cluster; // удаляем цепочку ФАТ

			do
			{
				vfat = SetFat(vfat, 0);
			}
			while (vfat < 07760);

			m_pDiskCat[nIndex].filename[0] = 0345; // пометим запись как удалённую
			bRet = WriteCurrentDir();
			m_sDiskCat.nFreeRecs++;
			m_sDiskCat.nFreeBlocks += (m_pDiskCat[nIndex].length ? (((m_pDiskCat[nIndex].length - 1) | (m_nClusterSize - 1)) + 1) : 0) / m_nClusterSize;
		}
	}

	return bRet;
}


bool CBKFloppyImage_ANDos::CreateDir(BKDirDataItem *pFR)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // то записать в него мы ничего не сможем.
	}

	if (m_sDiskCat.nFreeRecs <= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
		return false;
	}

	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<AndosFileRecord *>(pFR->pSpecificData); // Вот эта запись каталога
		pFR->nDirBelong = pRec->parent_dir_num = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
		// проверим, вдруг такая директория уже есть
		int nIndex = FindRecord2(pRec, false);

		if (nIndex >= 0)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_EXIST;
			pFR->nDirNum = m_pDiskCat[nIndex].dir_num;  // и заодно узнаем номер директории
		}
		else
		{
			// найдём свободное место в каталоге.
			bool bFound = false;
			unsigned int CatOffs = 0; // смещение в массиве каталога, до первой найденной свободной записи.

			for (CatOffs = 0; CatOffs < m_nRootFilesNum; ++CatOffs)
			{
				if ((m_pDiskCat[CatOffs].filename[0] == 0345) || (m_pDiskCat[CatOffs].filename[0] == 0))
				{
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				pFR->nDirNum = pRec->dir_num = AssignNewDirNum();

				if (pFR->nDirNum == 0)
				{
					m_nLastErrorNumber = IMAGE_ERROR::FS_DIRNUM_FULL;
					ASSERT(false);
				}

				m_pDiskCat[CatOffs] = pRec;
				bRet = WriteCurrentDir();
				m_sDiskCat.nFreeRecs--;
			}
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
	}

	return bRet;
}

bool CBKFloppyImage_ANDos::VerifyDir(BKDirDataItem *pFR)
{
	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<AndosFileRecord *>(pFR->pSpecificData); // Вот эта запись каталога
		// проверим, вдруг такая директория уже есть
		int nIndex = FindRecord2(pRec, false);

		if (nIndex >= 0)
		{
			return true;
		}
	}

	return false;
}

bool CBKFloppyImage_ANDos::DeleteDir(BKDirDataItem *pFR)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // то записать в него мы ничего не сможем.
	}

	if (pFR->nAttr & FR_ATTR::DELETED)
	{
		return true; // уже удалённое не удаляем повторно
	}

	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		// узнать есть ли файлы в директории.
		bool bExist = false;
#ifdef _DEBUG
		DebugOutCatalog(m_pDiskCat);
#endif

		for (unsigned int i = 0; i < m_nRootFilesNum; ++i)
		{
			if (m_pDiskCat[i].filename[0] == 0)
			{
				break; // теоретически это вообще конец каталога
			}

			if (m_pDiskCat[i].filename[0] == 0345)
			{
				// если удалённый файл - дырка, то игнорируем
				continue;
			}

			if (m_pDiskCat[i].parent_dir_num == pFR->nDirNum)
			{
				bExist = true; // нашли файл принадлежащий этой директории
				break;
			}
		}

		// если есть - не удалять.
		if (bExist)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_NOT_EMPTY;
		}
		else
		{
			// удалить можно пустую директорию
			auto pRec = reinterpret_cast<AndosFileRecord *>(pFR->pSpecificData);  // вот эту запись надо удалить
			int nIndex = FindRecord(pRec);

			if (nIndex >= 0) // если нашли
			{
				m_pDiskCat[nIndex].filename[0] = 0345; // пометим как удалённую
				bRet = WriteCurrentDir(); // сохраним директорию
				m_sDiskCat.nFreeRecs++;
			}
			else
			{
				m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
			}
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
	}

	return bRet;
}

bool CBKFloppyImage_ANDos::GetNextFileName(BKDirDataItem *pFR)
{
	pFR->clear();
	auto pRec = reinterpret_cast<AndosFileRecord *>(pFR->pSpecificData); // оригинальная запись
#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif

	// и выводим всё что найдём. но по одному.
	while (m_nCatPos < m_nRootFilesNum) // цикл по всему каталогу
	{
		if (m_pDiskCat[m_nCatPos].filename[0] == 0)
		{
			break; // теоретически это вообще конец каталога
		}

		if (m_pDiskCat[m_nCatPos].parent_dir_num == m_sDiskCat.nCurrDirNum)
		{
			*pRec = m_pDiskCat[m_nCatPos];
			pFR->nSpecificDataLength = sizeof(AndosFileRecord);
			ConvertRealToAbstractRecord(pFR);
			m_nCatPos++;
			return true;
		}

		m_nCatPos++;
	}

	if (!m_vecPC.empty())
	{
		m_nCatPos = m_vecPC.back();
		m_vecPC.pop_back();
	}

	return false;
}


bool CBKFloppyImage_ANDos::RenameRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<AndosFileRecord *>(pFR->pSpecificData); // оригинальная запись
	int nIndex = FindRecord(pRec); // сперва найдём её

	if (nIndex >= 0)
	{
		ConvertAbstractToRealRecord(pFR, true); // теперь скорректируем имя реальной записи
		m_pDiskCat[nIndex] = *pRec; // теперь скорректируем в каталоге
		return WriteCurrentDir(); // сохраним каталог
	}

	// что-то не так
	return false;
}

#ifdef _DEBUG
// отладочный вывод каталога
#pragma warning(disable:4996)
void CBKFloppyImage_ANDos::DebugOutCatalog(AndosFileRecord *pRec)
{
	auto strModuleName = std::vector<char_t>(_MAX_PATH);
	::GetModuleFileName(AfxGetInstanceHandle(), strModuleName.data(), _MAX_PATH);
	fs::path strName = fs::path(strModuleName.data()).parent_path() / "dirlog.txt";
	FILE *log = _wfopen(strName.c_str(), "wt");
	fprintf(log, "N#  Dir. Par.Name           date        attr   clust.  address length\n");
	uint8_t name[12] {};
	name[11] = 0;

	for (unsigned int i = 0; i < m_nRootFilesNum; ++i)
	{
		if (pRec[i].filename[0] == 0)
		{
			break; // теоретически это вообще конец каталога
		}

		memcpy(name, pRec[i].filename, 11);

		if (name[0] == 0345)
		{
			name[0] = 'x';
		}

		fprintf(log, "%03d %03d  %03d %-11s    ", i, pRec[i].dir_num, pRec[i].parent_dir_num, name);
		int dd = pRec[i].date;
		int nDay = dd & 0x1f;
		int nMon = (dd >> 5) & 0xf;
		int nYear = ((dd >> 9) & 0x7f) + 1980;
		fprintf(log, "%02d.%02d.%04d ", nDay, nMon, nYear);
		fprintf(log, " %06o %06o  %06o  %d\n",
		        pRec[i].attr, pRec[i].first_cluster, pRec[i].address, pRec[i].length);
	}

	fclose(log);
}
#endif
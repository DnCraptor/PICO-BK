#include "pch.h"
#include "BKFloppyImage_MSDOS.h"
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

CBKFloppyImage_MSDOS::CBKFloppyImage_MSDOS(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_nClusterSectors(0)
	, m_nClusterSize(0)
	, m_nBootSectors(0)
	, m_nRootFilesNum(0)
	, m_nRootSize(0)
	, m_nFatSectors(0)
	, m_nFatSize(0)
	, m_pFatTbl(nullptr)
	, m_pCatBuffer(nullptr)
	, m_pFindCatBuffer(nullptr)
	, m_pDiskCat(nullptr)
{
}


CBKFloppyImage_MSDOS::~CBKFloppyImage_MSDOS()
{
}

int CBKFloppyImage_MSDOS::GetNextFat(int fat)
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
int CBKFloppyImage_MSDOS::FindFreeFat(int fat)
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
uint16_t CBKFloppyImage_MSDOS::SetFat(int fat, uint16_t val)
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

int CBKFloppyImage_MSDOS::FindRecord(MSDOSFileRecord *pRec)
{
	int nIndex = -1;
	MSDOSFileRecord *pFindRec = GetFirstRecord();

	if (pFindRec)
	{
		do
		{
			if (pFindRec->filename[0] == 0)
			{
				break; // конец каталога
			}

			if (memcmp(pFindRec, pRec, sizeof(MSDOSFileRecord)) == 0)
			{
				nIndex = static_cast<int>(m_nFindRecNum);
				break;
			}

			pFindRec = GetNextRecord();
		}
		while (pFindRec);
	}

	return nIndex;
}

int CBKFloppyImage_MSDOS::FindRecord2(MSDOSFileRecord *pRec, bool bFull)
{
	int nIndex = -1;
	MSDOSFileRecord *pFindRec = GetFirstRecord();

	if (pFindRec)
	{
		do
		{
			if (pFindRec->filename[0] == 0)
			{
				break; // конец каталога
			}

			if (pFindRec->filename[0] != 0345)
			{
				if (memcmp(pFindRec->filename, pRec->filename, 11) == 0)
				{
					if (bFull) // если полная проверка
					{
						if (pFindRec->attr & FAT_ENTRY_ATTR_DIRECTORY)
						{
							// если директория
							nIndex = static_cast<int>(m_nFindRecNum);
							break;
						}

						// если файл
						if (pFindRec->length == pRec->length)
						{
							nIndex = static_cast<int>(m_nFindRecNum);
							break;
						}
					}
					else
					{
						nIndex = static_cast<int>(m_nFindRecNum);
						break;
					}
				}
			}

			pFindRec = GetNextRecord();
		}
		while (pFindRec);
	}

	return nIndex;
}

MSDOSFileRecord *CBKFloppyImage_MSDOS::GetFirstRecord()
{
	if (m_sDiskCat.nCurrDirNum == 0)
	{
		// читаем корневой каталог
		m_nFindCatCluster = 0;
		m_nFindCatRootBlock = 0;

		// Перемещаемся к началу каталога
		if (!SeekToBlock(m_nBootSectors + static_cast<size_t>(m_nFatSectors) * 2 + m_nFindCatRootBlock))
		{
			return nullptr;
		}

		// читаем очередной сектор корневого каталога
		if (!ReadData(m_pFindCatBuffer.get(), m_nBlockSize))
		{
			return nullptr;
		}
	}
	else
	{
		// нужно читать кластеры пока не получим маркер конца
		m_nFindCatCluster = m_sDiskCat.nCurrDirNum;

		if (!SeekToCluster(m_nFindCatCluster))
		{
			return nullptr;
		}

		// читаем кластер
		if (!ReadData(m_pFindCatBuffer.get(), m_nClusterSize))
		{
			return nullptr;
		}
	}

	m_nFindRecNum = 0;
	m_nFindRecNum++;
	return reinterpret_cast<MSDOSFileRecord *>(m_pFindCatBuffer.get()); // каталог
}

MSDOSFileRecord *CBKFloppyImage_MSDOS::GetNextRecord()
{
	if (m_sDiskCat.nCurrDirNum == 0)
	{
		if (m_nFindRecNum >= m_nBlockSize / 32)
		{
			if (++m_nFindCatRootBlock < m_nRootSize / m_nBlockSize)
			{
				// читаем очередной сектор корневого каталога
				if (!SeekToBlock(m_nBootSectors + static_cast<size_t>(m_nFatSectors) * 2 + m_nFindCatRootBlock))
				{
					return nullptr;
				}

				if (!ReadData(m_pFindCatBuffer.get(), m_nBlockSize))
				{
					return nullptr;
				}

				m_nFindRecNum = 0;
			}
			else
			{
				return nullptr; // конец каталога
			}
		}
	}
	else
	{
		if (m_nFindRecNum >= m_nClusterSize / 32)
		{
			m_nFindCatCluster = GetNextFat(m_nFindCatCluster);

			if ((1 < m_nFindCatCluster) && (m_nFindCatCluster < 07760))
			{
				if (!SeekToCluster(m_nFindCatCluster))
				{
					return nullptr;
				}

				// читаем кластер
				if (!ReadData(m_pFindCatBuffer.get(), m_nClusterSize))
				{
					return nullptr;
				}

				m_nFindRecNum = 0;
			}
			else
			{
				return nullptr; // конец каталога
			}
		}
	}

	return reinterpret_cast<MSDOSFileRecord *>(m_pFindCatBuffer.get()) + (m_nFindRecNum++);
}

void CBKFloppyImage_MSDOS::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly)
{
	auto pRec = reinterpret_cast<MSDOSFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = sizeof(MSDOSFileRecord);
			memset(pRec, 0, sizeof(MSDOSFileRecord));
		}

		// надо сформировать msдосную запись из абстрактной
		char szName[_MAX_PATH];
		char szExt[_MAX_EXT];
		_tsplitpath_s(pFR->strName.c_str(), nullptr, 0, nullptr, 0, szName, _MAX_PATH, szExt, _MAX_EXT);
		char chstr[9];
		memcpy(chstr, szName, 8);
	////	WideCharToMultiByte(CP_OEMCP, 0, szName, -1, chstr, 8, nullptr, nullptr);
		chstr[8] = 0;
		memset(pRec->file.name, ' ', 8);
		memset(pRec->file.ext, ' ', 3);
		strcpy(reinterpret_cast<char *>(pRec->file.name), chstr);

		if (strlen(szExt))
		{
			memcpy(chstr, szExt, 3);
		////	WideCharToMultiByte(CP_OEMCP, 0, szExt + 1, -1, chstr, 3, nullptr, nullptr);
			chstr[3] = 0;
			strcpy(reinterpret_cast<char *>(pRec->file.ext), chstr);
		}

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

			/*  формат даты. биты 0-4 - день месяца 1-31;
			биты 5-8 – месяц года, допускаются значения 1-12;
			биты 9-15 – год, считая от 1980 г. («эпоха MS-DOS»), возможны значения от 0 до 127 включительно, т.е. 1980-2107 гг.
			*/
			tm ctm;
			gmtime_s(&ctm, &pFR->timeCreation);
			pRec->wrt_date = (ctm.tm_mday & 037) | ((ctm.tm_mon & 017) << 5) | (((ctm.tm_year + 1900) > 1980 ? ctm.tm_year + 1900 - 1980 : 0) << 9);
			pRec->wrt_time = ((ctm.tm_hour & 0x01f) << 11) | ((ctm.tm_min & 0x03f) << 5) | ((ctm.tm_sec / 2) & 0x1f);
			pRec->first_cluster_lo = pFR->nStartBlock & 0xffff;
			pRec->first_cluster_hi = (pFR->nStartBlock >> 16) & 0xffff;
			pRec->length = pFR->nSize;
		}
	}
}
// на входе указатель на абстрактную запись.
// в ней заполнена копия реальной записи, по ней формируем абстрактную
void CBKFloppyImage_MSDOS::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<MSDOSFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		char wch[9];
		MultiByteToWideChar(CP_OEMCP, 0, reinterpret_cast<LPCSTR>(pRec->file.name), 8, wch, 8);
		wch[8] = 0;
		std::string name = strUtil::trim(std::string(wch));
		MultiByteToWideChar(CP_OEMCP, 0, reinterpret_cast<LPCSTR>(pRec->file.ext), 3, wch, 8);
		wch[3] = 0;
		std::string ext = strUtil::trim(std::string(wch));

		if (pRec->attr & FAT_ENTRY_ATTR_DIRECTORY)
		{
			// если каталог
			pFR->nAttr |= FR_ATTR::DIRECTORY;
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::DIRECTORY;
			pFR->strName = name + ext;
			pFR->nDirNum = pRec->first_cluster_lo + (pRec->first_cluster_hi << 16); // номер начального кластера
			pFR->nDirBelong = 0;
			pFR->nBlkSize = 0;
		}
		else
		{
			// если файл
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
			pFR->strName = name;

			if (!ext.empty())
			{
				pFR->strName += "." + ext;
			}

			pFR->nDirNum = 0;
			pFR->nDirBelong = 0;
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

		if (pRec->attr & FAT_ENTRY_ATTR_ARCHIVE)
		{
			pFR->nAttr |= FR_ATTR::ARCHIVE;
		}

		pFR->nAddress = 0;
		pFR->nSize = pRec->length;
		pFR->nStartBlock = pRec->first_cluster_lo + (pRec->first_cluster_hi << 16);
		// обратная операция для времени
		/*  формат даты. биты 0-4 - день месяца 1-31;
		биты 5-8 – месяц года, допускаются значения 1-12;
		биты 9-15 – год, считая от 1980 г. («эпоха MS-DOS»), возможны значения от 0 до 127 включительно, т.е. 1980-2107 гг.
		*/
		tm ctm;
		memset(&ctm, 0, sizeof(tm));
		ctm.tm_mday = pRec->wrt_date & 0x1f;
		ctm.tm_mon = (pRec->wrt_date >> 5) & 0x0f;
		ctm.tm_year = ((pRec->wrt_date >> 9) & 0x7f) + 1980 - 1900;

		if (ctm.tm_year < 0)
		{
			ctm.tm_year = 0;
		}

		ctm.tm_sec = (pRec->wrt_time & 0x1f) * 2;
		ctm.tm_min = (pRec->wrt_time >> 5) & 0x03f;
		ctm.tm_hour = (pRec->wrt_time >> 11) & 0x01f;
		pFR->timeCreation = mktime(&ctm);
	}
}

void CBKFloppyImage_MSDOS::OnReopenFloppyImage()
{
	m_sDiskCat.bHasDir = true;
	m_sDiskCat.bTrueDir = true;
}

/*
*выход: true - успешно
*      false - ошибка
*/
bool CBKFloppyImage_MSDOS::SeekToCluster(int nCluster)
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

// TODO: выводит неверные данные
const std::string CBKFloppyImage_MSDOS::GetImageInfo() const
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


bool CBKFloppyImage_MSDOS::GetStartFileName(BKDirDataItem *pFR)
{
	if (ReadCurrentDir())
	{
		return CBKFloppyImage_Prototype::GetStartFileName(pFR);
	}

	return false;
}


const size_t CBKFloppyImage_MSDOS::GetImageFreeSpace() const
{
	return size_t(m_sDiskCat.nFreeBlocks) * m_nClusterSize;
}

bool CBKFloppyImage_MSDOS::ReadCurrentDir()
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

	if (!m_pCatBuffer)
	{
		m_pCatBuffer = std::make_unique<uint8_t[]>(m_nClusterSize); // буфер каталога

		if (m_pCatBuffer)
		{
			m_pDiskCat = reinterpret_cast<MSDOSFileRecord *>(m_pCatBuffer.get()); // каталог
		}
	}

	if (!m_pFindCatBuffer)
	{
		m_pFindCatBuffer = std::make_unique<uint8_t[]>(m_nClusterSize); // буфер каталога для поиска
	}

	if (!m_pFatTbl || !m_pCatBuffer || !m_pFindCatBuffer)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

	// перемещаемся к началу фат и прочтём фат
	if (!SeektoBlkReadData(m_nBootSectors, m_pFatTbl.get(), m_nFatSize))
	{
		return false;
	}

	bool bRet = true;
	BKDirDataItem curr_fr; // экземпляр абстрактной записи
	auto pRec = reinterpret_cast<MSDOSFileRecord *>(curr_fr.pSpecificData); // а в ней копия оригинальной записи
	int free_recs = m_nRootFilesNum;
	int total_recs = m_nRootFilesNum;
	int used_size = 0;

	if (m_sDiskCat.nCurrDirNum == 0)
	{
		// читаем корневой каталог
		m_nCatCluster = 0;
		m_nCatRootBlock = 0;

		// Перемещаемся к началу каталога
		if (!SeekToBlock(m_nBootSectors + static_cast<size_t>(m_nFatSectors) * 2))
		{
			return false;
		}

		// каталог занимает m_nRootSize / m_nBlockSize секторов, длина записи 32. байта, макс кол-во записей m_nRootFilesNum
		int nRootSizeBlocks = m_nRootSize / m_nBlockSize; // размер корневого каталога в секторах

		for (int n = 0; n < nRootSizeBlocks; ++n)
		{
			// читаем очередной сектор корневого каталога
			bRet = ReadData(m_pCatBuffer.get(), m_nBlockSize);

			if (!bRet)
			{
				break;
			}

			m_nCatRootBlock = n;
			bool bEoc = false;

			// теперь обработаем все записи в нём
			for (unsigned int i = 0; i < m_nBlockSize / 32; ++i)
			{
				if (m_pDiskCat[i].filename[0] == 0)
				{
					bEoc = true;
					break; // конец каталога
				}

				curr_fr.clear();
				curr_fr.nSpecificDataLength = sizeof(MSDOSFileRecord);
				*pRec = m_pDiskCat[i]; // копируем текущую запись как есть
				ConvertRealToAbstractRecord(&curr_fr);

				if (m_pDiskCat[i].filename[0] != 0345)
				{
					free_recs--;
				}

				if (!(curr_fr.nAttr & FR_ATTR::DELETED))
				{
					if (m_pDiskCat[i].attr & FAT_ENTRY_ATTR_DIRECTORY)
					{
						// если каталог
					}
					else
					{
						// если файл
						used_size += curr_fr.nBlkSize; // если не удалённый, посчитаем используемый размер
					}
				}

				// для выхода из директории добавим тип записи
				if (curr_fr.strName == "..")
				{
					curr_fr.nRecType = BKDirDataItem::RECORD_TYPE::UP;
				}

				// будем игнорировать запись '.'
				if (curr_fr.strName != ".")
				{
					m_sDiskCat.vecFC.push_back(curr_fr);
				}
			}

			if (bEoc)
			{
				break;
			}
		}
	}
	else
	{
		// читаем подкаталог
		free_recs = 0;
		total_recs = 0;
		// нужно читать кластеры пока не получим маркер конца
		int currFat = m_sDiskCat.nCurrDirNum;

		while ((1 < currFat) && (currFat < 07760))
		{
			bRet = SeekToCluster(currFat);

			if (!bRet)
			{
				break;
			}

			// читаем кластер
			bRet = ReadData(m_pCatBuffer.get(), m_nClusterSize);

			if (!bRet)
			{
				break;
			}

			m_nCatCluster = currFat;
			bool bEoc = false;

			// теперь обработаем все записи в нём
			for (unsigned int i = 0; i < m_nClusterSize / 32; ++i)
			{
				if (m_pDiskCat[i].filename[0] == 0)
				{
					bEoc = true;
					break; // конец каталога
				}

				curr_fr.clear();
				curr_fr.nSpecificDataLength = sizeof(MSDOSFileRecord);
				*pRec = m_pDiskCat[i]; // копируем текущую запись как есть
				ConvertRealToAbstractRecord(&curr_fr);
				total_recs++;

				if (m_pDiskCat[i].filename[0] == 0345)
				{
					free_recs++;
				}

				if (!(curr_fr.nAttr & FR_ATTR::DELETED))
				{
					if (m_pDiskCat[i].attr & FAT_ENTRY_ATTR_DIRECTORY)
					{
						// если каталог
					}
					else
					{
						// если файл
						used_size += curr_fr.nBlkSize; // если не удалённый, посчитаем используемый размер
					}
				}

				// для выхода из директории добавим тип записи
				if (curr_fr.strName == "..")
				{
					curr_fr.nRecType = BKDirDataItem::RECORD_TYPE::UP;
				}

				// будем игнорировать запись '.'
				if (curr_fr.strName != ".")
				{
					m_sDiskCat.vecFC.push_back(curr_fr);
				}
			}

			if (bEoc)
			{
				break;
			}

			currFat = GetNextFat(currFat);
		}
	}

	m_sDiskCat.nTotalRecs = total_recs;
	m_sDiskCat.nFreeRecs = free_recs; // сколько свободных записей в каталоге
	m_sDiskCat.nTotalBlocks = (*(reinterpret_cast<uint16_t *>(&m_nSector[023])) - m_nDataSectorOffset / m_nBlockSize) / m_nClusterSectors;
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size; // !!!эта штука врёт
	return bRet;
}

// TODO: ещё не реализовано
bool CBKFloppyImage_MSDOS::WriteCurrentDir()
{
	if (!CBKFloppyImage_Prototype::WriteCurrentDir())
	{
		return false;
	}

	return false;
}

bool CBKFloppyImage_MSDOS::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
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

// TODO: ещё не реализовано
bool CBKFloppyImage_MSDOS::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
{
	bNeedSqueeze = false;
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;
	return bRet;
}

// TODO: ещё не реализовано
bool CBKFloppyImage_MSDOS::DeleteFile(BKDirDataItem *pFR, bool bForce)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;
	return bRet;
}

// TODO: ещё не реализовано
bool CBKFloppyImage_MSDOS::CreateDir(BKDirDataItem *pFR)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;
	return bRet;
}

bool CBKFloppyImage_MSDOS::VerifyDir(BKDirDataItem *pFR)
{
	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<MSDOSFileRecord *>(pFR->pSpecificData); // Вот эта запись каталога
		// проверим, вдруг такая директория уже есть
		int nIndex = FindRecord2(pRec, false);

		if (nIndex >= 0)
		{
			return true;
		}
	}

	return false;
}

// TODO: ещё не реализовано
bool CBKFloppyImage_MSDOS::DeleteDir(BKDirDataItem *pFR)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;
	return bRet;
}

// TODO: ещё не реализовано
bool CBKFloppyImage_MSDOS::RenameRecord(BKDirDataItem *pFR)
{
	return false;
}


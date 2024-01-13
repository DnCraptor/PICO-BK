#include "pch.h"
#include "BKFloppyImage_MKDos.h"
#include "StringUtil.h"

CBKFloppyImage_MKDos::CBKFloppyImage_MKDos(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_pDiskCat(nullptr)
{
	m_nCatSize = 20 * m_nBlockSize; // всю нулевую дорожку будем считать каталогом. // *(uint16_t*)&m_mSector[FMT_MKDOS_FIRST_FILE_BLOCK] * m_nSectorSize;
	// хотя размер каталога в мкдосе почти 9 секторов, чуть-чуть до конца сектора не добивает.
	m_pCatBuffer = std::make_unique<uint8_t[]>(m_nCatSize); // необязательно выделять массив во время чтения каталога, размер-то его известен

	if (m_pCatBuffer)
	{
		m_pDiskCat = reinterpret_cast<MKDosFileRecord *>(m_pCatBuffer.get() + FMT_MKDOS_CAT_BEGIN); // каталог диска
	}

	m_nMKCatSize = MKDOS_CAT_RECORD_SIZE;
	m_nMKLastCatRecord = m_nMKCatSize - 1;
	m_bMakeAdd = true;
	m_bMakeDel = true;
	m_bMakeRename = true;
	m_bChangeAddr = true;
}


CBKFloppyImage_MKDos::~CBKFloppyImage_MKDos()
{
}

int CBKFloppyImage_MKDos::FindRecord(MKDosFileRecord *pRec)
{
	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i) // цикл по всему каталогу
	{
		if (m_pDiskCat[i].dir_num == m_sDiskCat.nCurrDirNum) // ищем только в текущей директории
		{
			if (memcmp(&m_pDiskCat[i], pRec, sizeof(MKDosFileRecord)) == 0)
			{
				return static_cast<int>(i);
			}
		}
	}

	return -1;
}

int CBKFloppyImage_MKDos::FindRecord2(MKDosFileRecord *pRec, bool bFull)
{
	int nIndex = -1;

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
	{
		if ((m_pDiskCat[i].status == 0377) || (m_pDiskCat[i].status == 0200))
		{
			// если удалённый файл - дырка, то игнорируем
			continue;
		}

		if (m_pDiskCat[i].dir_num == m_sDiskCat.nCurrDirNum) // проверяем только в текущей директории
		{
			if (memcmp(pRec->name, m_pDiskCat[i].name, 14) == 0)  // проверим имя
			{
				if (bFull)
				{
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
						if (m_pDiskCat[i].dir_num == pRec->dir_num)
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

void CBKFloppyImage_MKDos::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly)
{
	auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить

	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = sizeof(MKDosFileRecord);
			memset(pRec, 0, sizeof(MKDosFileRecord));
		}

		// надо сформировать мкдосную запись из абстрактной
		std::string strMKName = pFR->strName.stem().string();
		std::string strMKExt = strUtil::CropStr(pFR->strName.extension().string(), 4); // включая точку
		size_t nNameLen = (14 - strMKExt.length()); // допустимая длина имени

		if (strMKName.length() > nNameLen) // если имя длиньше
		{
			strMKName = strUtil::CropStr(strMKName, nNameLen); // обрезаем
		}

		strMKName += strMKExt; // прицепляем расширение

		if (pFR->nAttr & FR_ATTR::DIRECTORY)
		{
			std::string strDir = strUtil::CropStr(pFR->strName, 13);
			imgUtil::UNICODEtoBK(strDir, pRec->name + 1, 13, true);

			if (!bRenameOnly)
			{
				pRec->name[0] = 0177; // признак каталога
				pRec->status = pFR->nDirNum;
				pRec->dir_num = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
			}
		}
		else
		{
			imgUtil::UNICODEtoBK(strMKName, pRec->name, 14, true);
			pRec->address = pFR->nAddress; // возможно, если мы сохраняем бин файл, адрес будет браться оттуда

			if (!bRenameOnly)
			{
				if (pFR->nAttr & FR_ATTR::PROTECTED)
				{
					pRec->status = 1;
				}
				else
				{
					pRec->status = 0;
				}

				pRec->dir_num = pFR->nDirBelong;
				pRec->len_blk = ByteSizeToBlockSize_l(pFR->nSize); // размер в блоках
				pRec->length = pFR->nSize % 0200000; // остаток от длины по модулю 65535
			}
		}
	}
}

// на входе указатель на абстрактную запись.
// в ней заполнена копия реальной записи, по ней формируем абстрактную
void CBKFloppyImage_MKDos::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		if (pRec->status == 0377)
		{
			pFR->nAttr |= FR_ATTR::DELETED;
		}
		else if (pRec->status == 0200)
		{
			pFR->nAttr |= FR_ATTR::BAD;

			if (pRec->name[0] == 0)
			{
				pRec->name[0] = 'B';
				pRec->name[1] = 'A';
				pRec->name[2] = 'D';
			}
		}

		if (pRec->name[0] == 0177)
		{
			// если директория
			pFR->nAttr |= FR_ATTR::DIRECTORY;
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::DIRECTORY;
			pFR->strName = strUtil::trim(imgUtil::BKToUNICODE(pRec->name + 1, 13, m_pKoi8tbl));
			pFR->nDirBelong = pRec->dir_num;
			pFR->nDirNum = pRec->status;
			pFR->nBlkSize = 0;

			// в МКДОС 3.17 вслед за АНДОС 3.30 появились ссылки на диски.
			// опознаются точно так же по ненулевому значению (имя буквы привода) в поле адреса у директории
			if (pRec->address && pRec->address < 0200)
			{
				pFR->nAttr |= FR_ATTR::LINK;
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::LINK;
			}
		}
		else
		{
			// если файл
			std::string name = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 14, m_pKoi8tbl));
			std::string ext;
			size_t l = name.length();
			size_t t = name.rfind('.');

			if (t != std::string::npos) // если расширение есть
			{
				ext = name.substr(t, l);
				name = strUtil::trim(name.substr(0, t));
			}

			if (!ext.empty())
			{
				name += ext;
			}

			pFR->strName = name;
			pFR->nDirBelong = pRec->dir_num;
			pFR->nDirNum = 0;
			pFR->nBlkSize = pRec->len_blk;

			if (pFR->nAttr & FR_ATTR::DELETED)
			{
				pFR->nDirBelong = 0; // а то удалённые файлы вообще не видны. А всё потому, что у удалённых dir_num == 255 тоже.
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
			}

			if (pRec->status == 2)
			{
				// лог. диск это просто ещё каталог и файлы.
				pFR->nAttr |= FR_ATTR::LOGDISK;
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::LOGDSK;
			}
			else
			{
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
			}

			if (pRec->status == 1)
			{
				pFR->nAttr |= FR_ATTR::PROTECTED;
			}
		}

		pFR->nAddress = pRec->address;

		if (pFR->nAttr & FR_ATTR::LOGDISK) // если у нас логический диск, то размер надо считать по блокам.
		{
			pFR->nSize = pRec->len_blk * m_nBlockSize;
		}
		else
		{
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


void CBKFloppyImage_MKDos::OnReopenFloppyImage()
{
	m_sDiskCat.bHasDir = true;
	m_sDiskCat.nMaxDirNum = 0177;
}

bool CBKFloppyImage_MKDos::ReadCurrentDir()
{
	if (!CBKFloppyImage_Prototype::ReadCurrentDir())
	{
		return false;
	}

	// если памяти под массив не выделилось - то ошибка, дальше работать невозможно
	if (!m_pCatBuffer)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

	memset(m_pCatBuffer.get(), 0, m_nCatSize);
	BKDirDataItem AFR; // экземпляр абстрактной записи
	auto pRec = reinterpret_cast<MKDosFileRecord *>(AFR.pSpecificData); // а в ней копия оригинальной записи

	if (!SeektoBlkReadData(0, m_pCatBuffer.get(), m_nCatSize)) // читаем каталог
	{
		return false;
	}

	int files_count = 0;
	int files_total = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_CAT_RECORD_NUMBER])); // читаем общее кол-во файлов. (НЕ записей!)
	m_sDiskCat.nDataBegin = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_FIRST_FILE_BLOCK])); // блок начала данных
	m_sDiskCat.nTotalRecs = m_nMKCatSize; // это у нас объём каталога, из него надо будет вычесть общее количество записей
	int used_size = 0;

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

		if ((m_pDiskCat[i].status == 0377) || (m_pDiskCat[i].status == 0200))
		{
			// удалённые не считаются,
			// плохие наверное тоже не считаются, но проверить не на чём
		}
		else
		{
			files_count++;
		}

		if (!(AFR.nAttr & FR_ATTR::DELETED))
		{
			if (m_pDiskCat[i].name[0] == 0177)
			{
				// если директория
				if (!AppendDirNum(m_pDiskCat[i].status))
				{
					// встретили дублирование номеров директорий
					m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_DUPLICATE;
				}
			}
			else
			{
				// если файл
				used_size += m_pDiskCat[i].len_blk;
			}
		}

		// выбираем только те записи, которые к нужной директории принадлежат.
		if (AFR.nDirBelong == m_sDiskCat.nCurrDirNum)
		{
			m_sDiskCat.vecFC.push_back(AFR);
		}
	}

#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - files_total;
	m_sDiskCat.nTotalBlocks = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_DISK_SIZE])) - *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_FIRST_FILE_BLOCK]));
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size;
	return true;
}

bool CBKFloppyImage_MKDos::WriteCurrentDir()
{
	// если памяти под массив не выделилось - то ошибка, дальше работать невозможно
	if (m_pCatBuffer == nullptr)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

	OptimizeCatalog();

	if (!CBKFloppyImage_Prototype::WriteCurrentDir())
	{
		return false;
	}

#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif

	if (!SeektoBlkWriteData(0, m_pCatBuffer.get(), m_nCatSize)) // сохраняем каталог как есть
	{
		return false;
	}

	return true;
}

bool CBKFloppyImage_MKDos::ChangeDir(BKDirDataItem *pFR)
{
	if (CBKFloppyImage_Prototype::ChangeDir(pFR))
	{
		return true;
	}

	if ((pFR->nAttr & FR_ATTR::LOGDISK) && (pFR->nRecType == BKDirDataItem::RECORD_TYPE::LOGDSK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
		return true;
	}

	m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
	return false;
}

bool CBKFloppyImage_MKDos::VerifyDir(BKDirDataItem *pFR)
{
	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
		// проверим, вдруг такая директория уже есть
		int nIndex = FindRecord2(pRec, false); // мы тут не знаем номер директории. мы можем только по имени проверить.

		if (nIndex >= 0)
		{
			return true;
		}
	}

	return false;
}

bool CBKFloppyImage_MKDos::CreateDir(BKDirDataItem *pFR)
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
		auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
		pFR->nDirBelong = pRec->dir_num = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
		// проверим, вдруг такая директория уже есть
		int nInd = FindRecord2(pRec, false); // мы тут не знаем номер директории. мы можем только по имени проверить.

		if (nInd >= 0)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_EXIST;
			pFR->nDirNum = m_pDiskCat[nInd].status; // и заодно узнаем номер директории
		}
		else
		{
			unsigned int nIndex = 0;
			// найдём свободное место в каталоге.
			bool bHole = false;
			bool bFound = false;

			for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
			{
				if ((m_pDiskCat[i].status == 0377) && (m_pDiskCat[i].name[0] == 0177))
				{
					// если нашли дырку в которой было имя директории
					bHole = true;
					nIndex = i;
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				nIndex = m_nMKLastCatRecord;

				if (nIndex < m_nMKCatSize) // если в конце каталога вообще есть место
				{
					bFound = true; // то нашли, иначе - нет
				}
			}

			if (bFound)
			{
				pFR->nDirNum = pRec->status = AssignNewDirNum(); // назначаем номер директории.

				if (pFR->nDirNum == 0)
				{
					m_nLastErrorNumber = IMAGE_ERROR::FS_DIRNUM_FULL;
				}

				// если ошибок не произошло, сохраним результаты
				if (bHole)
				{
					// сохраняем нашу запись вместо удалённой директории
					m_pDiskCat[nIndex] = pRec;
				}
				else
				{
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
			}
			else
			{
				m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
			}
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
	}

	return bRet;
}

bool CBKFloppyImage_MKDos::DeleteDir(BKDirDataItem *pFR)
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

		for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
		{
			if (m_pDiskCat[i].status == 0377)
			{
				// если удалённый файл - дырка, то игнорируем
				continue;
			}

			if (m_pDiskCat[i].dir_num == pFR->nDirNum)
			{
				bExist = true; // нашли файл, принадлежащий этой директории
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
			// удалить можно пустую директорию.
			auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
			int nIndex = FindRecord(pRec);

			if (nIndex >= 0) // если нашли
			{
				m_pDiskCat[nIndex].status = 0377; // пометим как удалённую
				m_pDiskCat[nIndex].dir_num = 0377;
				*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_CAT_RECORD_NUMBER])) -= 1; // поправим параметры - количество файлов
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

bool CBKFloppyImage_MKDos::GetNextFileName(BKDirDataItem *pFR)
{
	pFR->clear();
	auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // оригинальная запись
// #ifdef _DEBUG
//  DebugOutCatalog(m_pDiskCat);
// #endif

	// и выводим всё что найдём. но по одному.
	while (m_nCatPos < m_nMKLastCatRecord) // цикл по всему каталогу
	{
		if (m_pDiskCat[m_nCatPos].dir_num == m_sDiskCat.nCurrDirNum) // ищем только в текущей директории
		{
			*pRec = m_pDiskCat[m_nCatPos];
			pFR->nSpecificDataLength = sizeof(MKDosFileRecord);
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

bool CBKFloppyImage_MKDos::RenameRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // оригинальная запись
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


bool CBKFloppyImage_MKDos::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;

	if (pFR->nAttr & (FR_ATTR::DIRECTORY | FR_ATTR::LINK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false; // если это не файл - выход с ошибкой
	}

	ConvertAbstractToRealRecord(pFR);
	size_t len = pFR->nSize;
	size_t sector = pFR->nStartBlock;

	if (SeekToBlock(sector)) // перемещаемся к началу файла
	{
		// поскольку файл располагается единым куском, прочитаем его целиком
		if (len > 0)
		{
			if (!ReadData(pBuffer, len))
			{
				m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
				bRet = false;
			}
		}
	}
	else
	{
		bRet = false;
	}

	return bRet;
}

bool CBKFloppyImage_MKDos::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
{
	// если попадаем сюда с такой ошибкой - то мы в цикле пытаемся просквизировать диск
	if (bNeedSqueeze)
	{
		if (!Squeeze()) // если сквизирование неудачное
		{
			bNeedSqueeze = false;
			return false; // то выходим с ошибкой
		}
	}

	bNeedSqueeze = false;
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return false; // то записать в него мы ничего не сможем.
	}

	if (pFR->nAttr & (FR_ATTR::DIRECTORY | FR_ATTR::LINK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false; // если это не файл - выход с ошибкой
	}

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
	pFR->nDirBelong = pRec->dir_num = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога

	if (m_sDiskCat.nFreeBlocks < pRec->len_blk)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_FULL;
		return false;
	}

	if (m_sDiskCat.nFreeRecs <= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
		return false;
	}

	// поищем, вдруг такое имя файла уже существует.
	int nInd = FindRecord2(pRec, true);

	if (nInd >= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_EXIST;
		*pRec = m_pDiskCat[nInd];
		ConvertRealToAbstractRecord(pFR); // и сделаем найденную запись актуальной.
		return false;
	}

	bool bRet = false;
	unsigned int nIndex = 0;
	// найдём свободное место в каталоге.
	bool bFound = false;
	bool bHole = false; // что нашли, дырку или место в конце, т.к. разные способы обработки каталога

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
	{
		if ((m_pDiskCat[i].status == 0377) && (m_pDiskCat[i].len_blk >= pRec->len_blk))
		{
			// если нашли дырку подходящего размера
			bHole = true;
			nIndex = i;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		nIndex = m_nMKLastCatRecord;

		if (nIndex < m_nMKCatSize) // если в конце каталога вообще есть место
		{
			bFound = true; // то нашли, иначе - нет
		}
	}

	if (bFound)
	{
		// можно сохранять
		uint16_t nFirstFreeBlock = pRec->start_block = m_pDiskCat[nIndex].start_block;
		uint16_t nHoleSize = m_pDiskCat[nIndex].len_blk;

		if (bHole)
		{
			// если нашли дырку в каталоге, надо раздвинуть каталог, или не раздвигать, если размер дырки совпадает
			if (nHoleSize > pRec->len_blk)
			{
				// раздвинем каталог
				unsigned int i = m_nMKLastCatRecord + 1; // найдём конец каталога

				if (i >= m_nMKCatSize) // если каталог раздвинуть нельзя
				{
					m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
					goto l_squeeze;
				}
				else
				{
					// раздвигаем
					while (i > nIndex)
					{
						m_pDiskCat[i] = m_pDiskCat[i - 1];
						i--;
					}

					// сохраняем нашу запись
					m_pDiskCat[nIndex] = pRec;
					MKDosFileRecord hole;
					hole.status = 0377;
					hole.dir_num = 0377;
					hole.start_block = pRec->start_block + pRec->len_blk;
					hole.len_blk = nHoleSize - pRec->len_blk;
					imgUtil::UNICODEtoBK("<HOLE>", hole.name, 14, true);
					hole.length = (hole.len_blk * m_nBlockSize) % 0200000;
					// и запись с инфой о дырке
					m_pDiskCat[nIndex + 1] = hole;
					m_nMKLastCatRecord++;
				}
			}
			else
			{
				// ничего раздвигать не надо, просто
				// сохраняем нашу запись
				m_pDiskCat[nIndex] = pRec;
			}
		}
		else
		{
			// если нашли свободную область в конце каталога
			// сохраняем нашу запись
			m_pDiskCat[nIndex] = pRec;
			// сохраняем признак конца каталога
			MKDosFileRecord hole;
			hole.start_block = pRec->start_block + pRec->len_blk;
			hole.len_blk = nHoleSize - pRec->len_blk;
			// и запись с инфой о свободной области
			m_pDiskCat[nIndex + 1] = hole;
			m_nMKLastCatRecord++;
		}

		if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
		{
			// перемещаемся к месту
			if (SeekToBlock(nFirstFreeBlock))
			{
				int nReaded;
				int nLen = pFR->nSize; // размер файла

				if (nLen == 0)
				{
					nLen++; // запись файла нулевой длины не допускается, поэтому скорректируем.
				}

				while (nLen > 0)
				{
					memset(m_mBlock, 0, COPY_BLOCK_SIZE);
					nReaded = (nLen >= COPY_BLOCK_SIZE) ? COPY_BLOCK_SIZE : nLen;
					memcpy(m_mBlock, pBuffer, nReaded);
					pBuffer += nReaded;

					// пишем с выравниванием по секторам.
					if (nReaded > 0)
					{
						if (!WriteData(m_mBlock, EvenSizeByBlock_l(nReaded)))
						{
							return false;
						}
					}

					nLen -= nReaded;
				}

				if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
				{
					// если ошибок не произошло, сохраним результаты
					*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_CAT_RECORD_NUMBER])) += 1;
					*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_TOTAL_FILES_USED_BLOCKS])) += pRec->len_blk;
					// наконец сохраняем каталог
					bRet = WriteCurrentDir();
					m_sDiskCat.nFreeRecs--;
					m_sDiskCat.nFreeBlocks -= pRec->len_blk;
				}
			}
		}
	}
	else
	{
		// если нет дырки, но суммарное свободное место на диске есть - делаем сквизирование и пишем в конец.
		m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_NEED_SQEEZE;
l_squeeze:
		bNeedSqueeze = true;
	}

	return bRet;
}


bool CBKFloppyImage_MKDos::DeleteFile(BKDirDataItem *pFR, bool bForce)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // то записать в него мы ничего не сможем.
	}

	if (pFR->nAttr & (FR_ATTR::DIRECTORY | FR_ATTR::LINK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false; // если это не файл - выход с ошибкой
	}

	if (pFR->nAttr & FR_ATTR::DELETED)
	{
		return true; // уже удалённое не удаляем повторно
	}

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<MKDosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
	// найдём её в каталоге
	int nIndex = FindRecord(pRec);

	if (nIndex >= 0) // если нашли
	{
		if ((m_pDiskCat[nIndex].status == 2) && !bForce)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_PROTECTED;
		}
		else
		{
			m_pDiskCat[nIndex].status = 0377; // пометим как удалённую
			m_pDiskCat[nIndex].dir_num = 0377;
			*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_CAT_RECORD_NUMBER])) -= 1; // поправим параметры - количество файлов
			*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MKDOS_TOTAL_FILES_USED_BLOCKS])) -= m_pDiskCat[nIndex].len_blk; // кол-во занятых блоков
			bRet = WriteCurrentDir(); // сохраним директорию
			m_sDiskCat.nFreeRecs++;
			m_sDiskCat.nFreeBlocks += m_pDiskCat[nIndex].len_blk;
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
	}

	return bRet;
}

// сквизирование диска
bool CBKFloppyImage_MKDos::Squeeze()
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;
	int nUsedBlocs = 0;
	unsigned int p = 0;

	while (p <= m_nMKLastCatRecord)
	{
#ifdef _DEBUG
		DebugOutCatalog(m_pDiskCat);
#endif

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

			size_t nBufSize = size_t(m_pDiskCat[n].len_blk) * m_nBlockSize;
			auto pBuf = std::vector<uint8_t>(nBufSize);

			if (pBuf.data())
			{
				if (SeekToBlock(m_pDiskCat[n].start_block))
				{
					if (!ReadData(pBuf.data(), nBufSize))
					{
						m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
						bRet = false;
						break;
					}

					if (SeekToBlock(m_pDiskCat[p].start_block))
					{
						if (!WriteData(pBuf.data(), nBufSize))
						{
							m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
							bRet = false;
							break;
						}

						// теперь надо записи местами поменять.
						std::swap(m_pDiskCat[p], m_pDiskCat[n]); // обменяем записи целиком
						std::swap(m_pDiskCat[p].start_block, m_pDiskCat[n].start_block); // начальные блоки вернём как было.
						m_pDiskCat[n].start_block = m_pDiskCat[p].start_block + m_pDiskCat[p].len_blk;
					}
					else
					{
						bRet = false;
					}
				}
				else
				{
					bRet = false;
				}
			}
			else
			{
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

// оптимизация каталога - объединение смежных дырок
bool CBKFloppyImage_MKDos::OptimizeCatalog()
{
	int nUsedBlocs = 0;
	unsigned int p = 0;

	while (p <= m_nMKLastCatRecord)
	{
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

#ifdef _DEBUG
// отладочный вывод каталога
#pragma warning(disable:4996)
void CBKFloppyImage_MKDos::DebugOutCatalog(MKDosFileRecord *pRec)
{
	auto strModuleName = std::vector<char_t>(_MAX_PATH);
	::GetModuleFileName(AfxGetInstanceHandle(), strModuleName.data(), _MAX_PATH);
	fs::path strName = fs::path(strModuleName.data()).parent_path() / "dirlog.txt";
	FILE *log = _wfopen(strName.c_str(), "wt");
	fprintf(log, "N#  Stat.Dir.Name           start  lenblk  address length\n");
	uint8_t name[15];

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
	{
		memcpy(name, pRec[i].name, 14);
		name[14] = 0;
		fprintf(log, "%03d %03d  %03d %-14s %06o %06o  %06o  %06o\n", i, pRec[i].status, pRec[i].dir_num, name, pRec[i].start_block, pRec[i].len_blk, pRec[i].address, pRec[i].length);
	}

	fclose(log);
}
#endif

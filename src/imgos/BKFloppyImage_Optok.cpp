#include "pch.h"
#include "BKFloppyImage_OPtok.h"
#include "StringUtil.h"

#pragma warning(disable:4996)

CBKFloppyImage_Optok::CBKFloppyImage_Optok(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_nRecsNum(0)
	, m_nFreeBlock(0)
	, m_pDiskCat(nullptr)
{
	m_pFoppyImgFile.SetGeometry(0xff, 0xff, 9); // задаём 9 секторов на дорожке
	m_bMakeAdd    = true;
	m_bMakeDel    = true;
	m_bMakeRename = true;
	m_bChangeAddr = true;
	m_pCatBuffer = std::make_unique<uint8_t[]>(OPTOK_CAT_SIZE);

	if (m_pCatBuffer)
	{
		m_pDiskCat = reinterpret_cast<OptokFileRecord *>(m_pCatBuffer.get()); // каталог диска
	}
}


CBKFloppyImage_Optok::~CBKFloppyImage_Optok()
{
}


int CBKFloppyImage_Optok::FindRecord(OptokFileRecord *pPec)
{
	for (int i = 0; i < m_nRecsNum; ++i) // цикл по всему каталогу
	{
		if (m_pDiskCat[i].status) // ищем только в действительных записях
		{
			if (memcmp(&m_pDiskCat[i], pPec, OPTOK_REC_SIZE) == 0)
			{
				return i;
			}
		}
	}

	return -1;
}

int CBKFloppyImage_Optok::FindRecord2(OptokFileRecord *pPec, bool bFull /*= true*/)
{
	for (int i = 1; i < m_nRecsNum; ++i)
	{
		if (m_pDiskCat[i].status) // ищем только в действительных записях
		{
			// сравниваем имя
			if (memcmp(m_pDiskCat[i].name, pPec->name, 16) == 0)
			{
				// если полное сравнение
				if (bFull)
				{
					// сравним ещё и другие параметры
					if (m_pDiskCat[i].address == pPec->address
					        && m_pDiskCat[i].length == pPec->length
					        && m_pDiskCat[i].crc == pPec->crc)
					{
						return i;
					}
				}
				else
				{
					return i;
				}
			}
		}
	}

	return -1;
}

void CBKFloppyImage_Optok::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly /*= false*/)
{
	auto pRec = reinterpret_cast<OptokFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить

// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = OPTOK_REC_SIZE;
			memset(pRec, 0, OPTOK_REC_SIZE);
		}

		std::string strName = strUtil::CropStr(pFR->strName, 16);
		imgUtil::UNICODEtoBK(strName, pRec->name, 16, true);
		//if (!bRenameOnly)
		{

			pRec->address = pFR->nAddress;
			pRec->length = pFR->nSize;
			pRec->status = (pFR->nAttr & FR_ATTR::DELETED) ? 0 : 0xffff;
			pRec->masks = (pFR->nAttr & FR_ATTR::PROTECTED) ? 1 : 0;
			pRec->masks |= (pFR->nAttr & FR_ATTR::BAD) ? 2 : 0;

			pRec->start_block = pFR->nStartBlock;
			pRec->crc = (pFR->timeCreation & 0xffff);
			uint16_t t = ((pFR->timeCreation >> 16) & 0xffff);
			if (t)
			{
				pRec->WAAR = t;
				pRec->masks |= 4;
			}
		}
	}
}

void CBKFloppyImage_Optok::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<OptokFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;

		if (pRec->status == 0)
		{
			pFR->nAttr |= FR_ATTR::DELETED;
			pFR->strName = "<DELETED>";
		}
		else
		{
			pFR->strName = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 16, m_pKoi8tbl));
		}

		if (pRec->masks & 1)
		{
			pFR->nAttr |= FR_ATTR::PROTECTED;
		}
		if (pRec->masks & 2)
		{
			pFR->nAttr |= FR_ATTR::BAD;
		}


		pFR->nDirBelong = 0;
		pFR->nDirNum = 0;
		pFR->nAddress = pRec->address;
		pFR->nSize = pRec->length;
		pFR->nBlkSize = ByteSizeToBlockSize_l(pFR->nSize);
		pFR->nStartBlock = pRec->start_block;
		pFR->timeCreation = pRec->crc; // тут будем хранить КС
		if (pRec->masks & 4)
		{
			pFR->timeCreation |= static_cast<time_t>(pRec->WAAR) << 16;
		}
	}
}

bool CBKFloppyImage_Optok::ReadCurrentDir()
{
	// если памяти под массив не выделилось - то ошибка, дальше работать невозможно
	if (!m_pCatBuffer)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

	if (!CBKFloppyImage_Prototype::ReadCurrentDir())
	{
		return false;
	}

	memset(m_pCatBuffer.get(), 0, OPTOK_CAT_SIZE);

	if (!SeektoBlkReadData(2, m_pCatBuffer.get(), OPTOK_CAT_SIZE)) // читаем каталог
	{
		return false;
	}

	m_nRecsNum = 0;
	m_nFreeBlock = *(reinterpret_cast<uint16_t *>(& m_pCatBuffer[FMT_OPTOK_FIRST_FILE_BLOCK]));
	BKDirDataItem AFR; // экземпляр абстрактной записи
	auto pOptokRec = reinterpret_cast<OptokFileRecord *>(AFR.pSpecificData); // а в ней копия оригинальной записи
	int used_size = 0;

	for (int i = 1; i < OPTOK_CAT_SIZE / OPTOK_REC_SIZE; ++i) // самую первую запись игнорируем, это служебная область
	{
		m_nRecsNum++;   // будем считать количество записей а не файлов на диске.
		// проверка на конец каталога - будем считать что вся запись полностью из нулей
		{
			auto pRec = reinterpret_cast<uint8_t *>(&m_pDiskCat[i]); // обратно в байты
			int s = 0;

			for (int i = 0; i < OPTOK_REC_SIZE; ++i)
			{
				s += pRec[i];
			}

			if (s == 0)
			{
				break;
			}
		}
		// преобразуем запись и поместим в массив
		AFR.clear();
		AFR.nSpecificDataLength = OPTOK_REC_SIZE;
		*pOptokRec = m_pDiskCat[i]; // копируем текущую запись как есть
		ConvertRealToAbstractRecord(&AFR);
		used_size += AFR.nBlkSize; // удалённые файлы тоже считаются.
		m_nFreeBlock += AFR.nBlkSize; // с какого места начинается свободная область
		m_sDiskCat.vecFC.push_back(AFR);
	}

	m_sDiskCat.nTotalRecs = OPTOK_CAT_SIZE / OPTOK_REC_SIZE;
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - m_nRecsNum;
	int imgSize = max(720, (int)m_sParseImgResult.nImageSize / m_nBlockSize); // неизвестно как определяется размер диска в блоках,
	m_sDiskCat.nTotalBlocks = imgSize - 9; // будем высчитывать это по размеру образа.
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size;
#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif
	//Squeeze();
	return true;
}

bool CBKFloppyImage_Optok::WriteCurrentDir()
{
#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif

	// если памяти под массив не выделилось - то ошибка, дальше работать невозможно
	if (!m_pCatBuffer)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

	if (!CBKFloppyImage_Prototype::WriteCurrentDir())
	{
		return false;
	}

	if (!SeektoBlkWriteData(2, m_pCatBuffer.get(), OPTOK_CAT_SIZE)) // сохраняем каталог как есть
	{
		return false;
	}

	return true;
}

bool CBKFloppyImage_Optok::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;
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

bool CBKFloppyImage_Optok::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
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

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<OptokFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
	pFR->nDirBelong = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
	bool bRet = false;

	if (m_sDiskCat.nFreeBlocks < ByteSizeToBlockSize_l(pRec->length))
	{
		// предложим сделать уплотнение диска
		m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_FULL;
		bNeedSqueeze = true;
		return false;
	}

	if (m_sDiskCat.nFreeRecs <= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
		return false;
	}

	// поищем, вдруг такое имя файла уже существует.
	int nIndex = FindRecord2(pRec, true);

	if (nIndex >= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_EXIST; // если файл существует - выходим с ошибкой
		*pRec = m_pDiskCat[nIndex];
		ConvertRealToAbstractRecord(pFR); // и сделаем найденную запись актуальной.
		return false;
	}

	// просто добавляем файл в конец. т.к. ОС не допускает перемешанных файлов.
	// они должны следовать строго последовательно.
	// перемещаемся к месту
	if (SeekToBlock(m_nFreeBlock))
	{
		int nReaded;
		int nLen = pFR->nSize; // размер файла

		if (nLen == 0)
		{
			nLen++; // запись файла нулевой длины не допускается, поэтому скорректируем.
		}

		int crc = 0; // посчитаем КС

		while (nLen > 0)
		{
			memset(m_mBlock, 0, COPY_BLOCK_SIZE);
			nReaded = (nLen >= COPY_BLOCK_SIZE) ? COPY_BLOCK_SIZE : nLen;
			memcpy(m_mBlock, pBuffer, nReaded);
			pBuffer += nReaded;

			for (int i = 0; i < nReaded; ++i)
			{
				crc += (int)(char)m_mBlock[i];
			}

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
			int nBlkSize = ByteSizeToBlockSize_l(pRec->length);
			pRec->crc = crc & 0xffff;
			pRec->start_block = m_nFreeBlock;
			m_pDiskCat[m_nRecsNum] = pRec;
			*(reinterpret_cast<uint16_t *>(m_pCatBuffer.get() + FMT_OPTOK_DISK_SIZE)) -= nBlkSize;
			// наконец сохраняем каталог
			bRet = WriteCurrentDir();
			m_sDiskCat.nFreeRecs--;
			m_sDiskCat.nFreeBlocks -= nBlkSize;
		}
	}

	return bRet;
}

bool CBKFloppyImage_Optok::DeleteFile(BKDirDataItem *pFR, bool bForce /*= false*/)
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

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<OptokFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
	// найдём её в каталоге
	int nIndex = FindRecord(pRec);

	if (nIndex >= 0) // если нашли
	{
		m_pDiskCat[nIndex].status = 0; // пометим как удалённую
		memset(m_pDiskCat[nIndex].name, 0, 16);
		bRet = WriteCurrentDir(); // сохраним директорию
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
	}

	return bRet;
}

bool CBKFloppyImage_Optok::RenameRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<OptokFileRecord *>(pFR->pSpecificData); // оригинальная запись
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

bool CBKFloppyImage_Optok::Squeeze()
{
	// нужно пройтись по каталогу и удалить все удалённые файлы.
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	int nBlocks = 0;    // количество блоков, которое будет освобождено
	int nRec = 0; // номер первой найденной удалённой записи

	for (int i = 1; i < m_nRecsNum; ++i) // индекс текущей записи.
	{
		if (!m_pDiskCat[i].status) // ищем, это удалёная запись?
		{
			nRec = i;   // да
			break;
		}
	}

	if (nRec) // если что-то нашлось
	{
		// теперь поищем сколько подряд удалённых записей есть
		int nCur = nRec;

		for (;;)
		{
			while (nCur < m_nRecsNum && !m_pDiskCat[nCur].status);

			{
				// нашли удалённый файл.
				int nBlkSize = ByteSizeToBlockSize_l(m_pDiskCat[nCur].length);
				nBlocks += nBlkSize; // прибавим к освобождаемым блокам
				nCur++;
			}

			// вот теперь у нас:
			// nRec указывает на пустую запись
			// nCur указывает на непустую запись или на конец каталога
			// и нам надо переместить запись о файле с позиции nCur на позицию nRec
			// и переместить файл

			// если получилось так, что до самого конца каталога у нас удалённые записи
			if (nCur == m_nRecsNum)
			{
				// то просто надо обнулить все записи от nRec, до конца.
				auto p = reinterpret_cast<uint8_t *>(&m_pDiskCat[nRec]);
				auto pe = reinterpret_cast<uint8_t *>(&m_pDiskCat[m_nRecsNum]);
				memset(p, 0, pe - p);
				break; //for
			}

			// а иначе - будем сдвигать файлы на диске.
			int nBlk = m_pDiskCat[nRec].start_block; // номер блока, с которого надо размещать файлы
			auto nBufSize = size_t(EvenSizeByBlock_l(m_pDiskCat[nCur].length));

			if (nBufSize)
			{
				auto pBuf = std::vector<uint8_t>(nBufSize);

				if (pBuf.data())
				{
					if (SeekToBlock(m_pDiskCat[nCur].start_block))
					{
						if (ReadData(pBuf.data(), nBufSize))
						{
							if (SeekToBlock(m_pDiskCat[nRec].start_block))
							{
								if (WriteData(pBuf.data(), nBufSize))
								{
									m_pDiskCat[nRec] = m_pDiskCat[nCur];
									m_pDiskCat[nRec].start_block = nBlk;
									nBlk += ByteSizeToBlockSize_l(m_pDiskCat[nRec].length);
									nRec++;
								}
								else
								{
									m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
								}
							}
							else
							{
								m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_SEEK;
							}
						}
						else
						{
							m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
						}
					}
					else
					{
						m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_SEEK;
					}

					if (m_nLastErrorNumber != IMAGE_ERROR::OK_NOERRORS)
					{
						return false;
					}
				}
			}

			nCur++;
		}

		*(reinterpret_cast<uint16_t *>(m_pCatBuffer.get() + FMT_OPTOK_DISK_SIZE)) += nBlocks;
#ifdef _DEBUG
		DebugOutCatalog(m_pDiskCat);
#endif
		return WriteCurrentDir(); // сохраним каталог
	}

	return true;
}

#ifdef _DEBUG
// отладочный вывод каталога
#pragma warning(disable:4996)
void CBKFloppyImage_Optok::DebugOutCatalog(OptokFileRecord *pRec)
{
	auto strModuleName = std::vector<char_t>(_MAX_PATH);
	::GetModuleFileName(AfxGetInstanceHandle(), strModuleName.data(), _MAX_PATH);
	fs::path strName = fs::path(strModuleName.data()).parent_path() / "dirlog.txt";
	FILE *log = _wfopen(strName.c_str(), "wt");
	fprintf(log, "N#  Stat.FileName         masks  WAAR   start  address length CRC\n");
	uint8_t name[17];

	for (int i = 0; i < m_nRecsNum; ++i)
	{
		memcpy(name, pRec[i].name, 16);
		name[16] = 0;
		fprintf(log, "%03d %c    %-16s %06o %06o %06o %06o  %06o %06o\n", i, pRec[i].status ? _T(' ') : _T('D'), name, pRec[i].masks, pRec[i].WAAR, pRec[i].start_block, pRec[i].address, pRec[i].length, pRec[i].crc);
	}

	fclose(log);
}
#endif